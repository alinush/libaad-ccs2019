#pragma once

#include <functional>   // std::hash
#include <map>

#include <aad/Hashing.h>
#include <aad/PolyCommit.h>
#include <aad/PolyInterpolation.h>
#include <aad/PublicParameters.h>

#include <libfqfft/polynomial_arithmetic/basic_operations.hpp>

#include <xutils/NotImplementedException.h>

using namespace libaad;

using libfqfft::_polynomial_multiplication;

namespace libaad {

class Frontier {
public:
    class DataType;
    class MergeFunc;

    using DataPtrType = DataType*;
    using NodeType = DataNode<DataType>;
    using NodePtrType = NodeType*;
    using BinaryTreeType = BinaryTree<NodeType>;
    using BinaryTreePtrType = BinaryTreeType*;
    using BinaryForestType = BinaryForest<DataType, MergeFunc>;

public:
    class DataType {
    public:
        // accumulator over all prefixes in all leaves underneath this node
        G1 acc1, eAcc1; // extractable accumulator in G1
        G2 acc2;        // non-extractable accumulator in G2
        //boost::variant<std::tuple<G1, G1>, G2> acc;
        // polynomial over all leaves underneath this node
        std::vector<Fr> poly;
    public:
        DataType()
            : acc1(G1::one()), eAcc1(G1::one()), acc2(G2::one())
        {}

    public:
        void commitToPolynomial(PublicParameters* pp, bool clearPoly, bool isLeaf, bool isRoot) {
            acc1 = PolyCommit::commitG1(*pp, poly, false);
            if(!isLeaf) {
                eAcc1 = PolyCommit::commitG1(*pp, poly, true);
                if(!isRoot) {
                    acc2 = PolyCommit::commitG2(*pp, poly, false);
                    assertEqual(ReducedPairing(acc1, G2::one()), ReducedPairing(G1::one(), acc2));
                }
            
                assertEqual(ReducedPairing(acc1, pp->getG2toTau()), ReducedPairing(eAcc1, G2::one()));
            }

            // Get rid of the polynomial, unless it's the root polynomial, which we need for EEA with the AT polynomial
            if(clearPoly) {
                std::vector<Fr>().swap(poly);
                assertEqual(poly.capacity(), 0);
            }
        }
    };

    class ProofData {
    public:
        enum class Type {
            Unknown, Leaf, SiblingLeaf, SiblingNonLeaf, OnPath, Root
        };

    protected:
        std::unique_ptr<G2> acc2;
        std::unique_ptr<G1> acc1, eAcc1;
        Type type;

    public:
        ProofData()
            : type(Type::Unknown)
        {}

        ProofData(Type t)
            : type(t)
        {}

    public:
        void setType(Type t) { type = t; }
        Type getType() const { return type; }

        void resetG1() { acc1.reset(nullptr); }
        void resetG1ext() { eAcc1.reset(nullptr); }
        void resetG2() { acc2.reset(nullptr); }

        void setG1(const G1& g1) { acc1.reset(new G1(g1)); }
        void setG2(const G2& g2) { acc2.reset(new G2(g2)); }
        void setG1ext(const G1& g1e) { eAcc1.reset(new G1(g1e)); }

        bool hasG1() const { return acc1 != nullptr; }
        bool hasG2() const { return acc2 != nullptr; }
        bool hasG1ext() const { return eAcc1 != nullptr; }

        G1 getG1() const {
            if(!hasG1())
                throw std::runtime_error("Does not have G1 set");
            return *acc1;
        }

        G2 getG2() const {
            if(!hasG2())
                throw std::runtime_error("Does not have G2 set");
            return *acc2;
        }

        G1 getG1ext() const {
            if(!hasG1ext())
                throw std::runtime_error("Does not have G1 extractable set");
            return *eAcc1;
        }
    };

    class MergeFunc {
    protected:
        microseconds::rep multTime, commitTime;
        size_t multCoeffs;
        size_t commCoeffs;
        PublicParameters *pp;

    public:
        MergeFunc()
            : multTime(0), commitTime(0), multCoeffs(0), commCoeffs(0)
        {}

    public:
        void setPublicParameters(PublicParameters *p) { pp = p; }

        DataPtrType operator() (NodePtrType leftNode, NodePtrType rightNode, bool isLastMerge)
        {
            bool simulate = pp == nullptr;
            //logdbg << "Merging size " << leftNode->getSize() << " with size " << rightNode->getSize() << endl;
            DataPtrType left = leftNode->getData();
            DataPtrType right = rightNode->getData();
            (void)isLastMerge;

            auto parent = new DataType();
            if(!simulate) {
                assertNotNull(pp);
                // Reserve space for and compute the product of the children's polynomials
                assertStrictlyPositive(left->poly.size());
                assertStrictlyPositive(right->poly.size());
                //parent->poly.reserve(left->poly.size() + right->poly.size() - 1);
                ManualTimer t;
                poly_multiply(parent->poly, left->poly, right->poly);
                multTime += t.stop().count();
                multCoeffs += parent->poly.size();
            
                // Commit to left and right polynomials (note that this commits to the leaves as well)
                commCoeffs += left->poly.size() + right->poly.size();
                t.restart();
                left->commitToPolynomial(pp, true, leftNode->isLeaf(), false);
                right->commitToPolynomial(pp, true, rightNode->isLeaf(), false);
                commitTime += t.stop().count();

                // We defer committing to the parent poly until the next MergeFunc or finalize().
                // (This is because by then we knew if we should commit in G1 or in G2 for a left or right child. However,
                //  this is irrelevant now, because we don't use this scheme anymore.)
            } else {
                // accumulators already initialized to one() in constructor
                // NOTE: we could pick them randomly, but that would be too slow with libff, instead
                // we pick them randomly when generating mock proofs
            }

            return parent;
        }

        void printStatistics() const {
            printOpPerf(multTime, "frontierMult", multCoeffs);
            printOpPerf(commitTime, "frontierCommit", commCoeffs);
        }
    };

protected:
    // This 'upper tree' is the frontier tree. When deleted it deletes nodes referred to
    // by 'lowerTrees' as well!
    std::unique_ptr<BinaryTreeType> upperTree;
    BinaryForestType lowerTrees;

    /**
     * Maps a prefix of some key (that is not in the AT) to its leaf in the frontier
     * tree so we can walk up this tree and build a frontier proof for the key.
     *
     * WARNING: used std:: and boost:: unordered maps here before, but they are too slow!
     */
    std::vector<std::tuple<BitString, NodePtrType>> keyPrefixToLeaf; // we don't care about fast lookup right now

    /**
     * Maps an inserted key (and its values) to its leaves in the frontier tree. These leaves
     * store accumulators over all the "lower frontier" prefixes associated with this key's values.
     * This way, we can build O(|V|) frontier proofs for all these O(\lambda|V|) prefixes,
     * where V is the set of values of the key. Each proof will be a path to one of these leaves.
     *
     * WARNING: used std:: and boost:: unordered maps here before, but they are too slow!
     */
    std::vector<std::tuple<BitString, std::vector<NodePtrType>>> keyToAccumulatorLeaf; // we don't care about fast lookup right now

    PublicParameters *params;
    bool simulate;
    MergeFunc mergeFunc;

public:
    Frontier(PublicParameters * p = nullptr)
        : upperTree(nullptr), params(p), simulate(p == nullptr)
    {
        mergeFunc.setPublicParameters(p);
        lowerTrees.setMergeFunc(&mergeFunc);
    }

    ~Frontier() {
        //logdbg << "Destroying frontier" << endl;
    }

public:
    std::vector<Fr>& getRootPoly() const {
        auto root = upperTree->getRoot();
        assertNotNull(root);
        auto data = root->getData();
        assertNotNull(data);
        return data->poly;
    }
    G1& getRootAcc() const {
        auto root = upperTree->getRoot();
        assertNotNull(root);
        auto data = root->getData();
        assertNotNull(data);
        return data->acc1;
    }

    int getNumLeafs() const { return lowerTrees.getCount(); }

    int getSize() const {
        return upperTree->getRoot()->getSize();
    }

    NodePtrType getPrefixLeaf(const BitString& prefix) {
        return std::get<1>(*std::find_if(
            keyPrefixToLeaf.begin(),
            keyPrefixToLeaf.end(),
            [&prefix](const std::tuple<BitString, NodePtrType>& tup) {
                return std::get<0>(tup) == prefix;
        }));
    }

    std::vector<NodePtrType>& getKeyLeaves(const BitString& keyHash) {
        auto it = std::find_if(
            keyToAccumulatorLeaf.begin(),
            keyToAccumulatorLeaf.end(),
            [&keyHash](const std::tuple<BitString, std::vector<NodePtrType>>& tup) {
                return std::get<0>(tup) == keyHash;
        });
        assertTrue(it != keyToAccumulatorLeaf.end());
        return std::get<1>(*it);
    }

    /**
     * Takes a single prefix for a missing key and creates a leaf for it.
     */
    void addMissingKeyPrefix(const BitString& prefix) {
        auto data = new DataType();
        if(!simulate) {
            Fr el = hashToField(prefix);
            // Stores (x - el) as a polynomial
            data->poly.push_back(-el);
            data->poly.push_back(Fr::one());
            assertNotNull(params);
        } else {
            // do nothing
        }

        NodePtrType leafPtr = NodeFactory::makeNode(data);
        lowerTrees.appendLeaf(leafPtr);
        keyPrefixToLeaf.push_back(std::make_tuple(prefix, leafPtr));
    }

    /**
     * Takes a batch of prefixes (could be 2 or more) associated with the missing
     * values for the specified key and creates a leaf for them with a characteristic
     * polynomial.
     */
    void addMissingValuesPrefixes(const BitString& keyHash, std::vector<BitString>::const_iterator pfxbeg, std::vector<BitString>::const_iterator pfxend) {
        auto data = new DataType();
        if(!simulate) {
            assertNotNull(params);

            // build characteristic polynomial
            std::vector<Fr> hashes;
            hashToField(pfxbeg, pfxend, hashes);
            poly_from_roots_ntl(data->poly, hashes);
        } else {
            // do nothing
        }
         
        auto leafPtr = NodeFactory::makeNode(data);
        lowerTrees.appendLeaf(leafPtr);

        auto it = std::find_if(
            keyToAccumulatorLeaf.begin(),
            keyToAccumulatorLeaf.end(),
            [&keyHash](const std::tuple<BitString, std::vector<NodePtrType>>& tup) {
                return std::get<0>(tup) == keyHash;
        });

        //logdbg << "Adding prefixes for key " << keyHash << " to leaf " << leafPtr->getLabel() << endl;

        if(it == keyToAccumulatorLeaf.end()) {
            std::vector<NodePtrType> vec;
            vec.push_back(leafPtr);
            keyToAccumulatorLeaf.push_back(std::make_tuple(keyHash, vec));
        } else {
            auto& vec = std::get<1>(*it);
            vec.push_back(leafPtr);
        }
    }

    /**
     * Easiest way to build a frontier accumulator was to maintain a bunch of 2^i-sized
     * trees and "merge" them later: we create a new tree whose leaves are these old trees.
     *
     * NOTE: This "merging" is different than how we typically merge trees in BinaryForest.
     */
    void finalize() {
        if(upperTree == nullptr) {
            upperTree.reset(new BinaryTreeType(lowerTrees.mergeAllRoots()));
            //logdbg << "Number of nodes in (finalized) frontier: " << upperTree->getRoot()->getSize() << endl;

            if(!simulate) {
                assertNotNull(params);
                auto root = upperTree->getRoot();
                auto rootData = root->getData();
                // We don't clear the poly yet because we need it for computing EEA
                rootData->commitToPolynomial(params, false, false, true);

                //assertEqual(rootData->acc.which(), 0);
                assertFinalized(root->left.get(), BitString("0"));
                assertFinalized(root->right.get(), BitString("1"));

                mergeFunc.printStatistics();
            } else {
                // accumulators already initialized in constructor
            }
        } else {
            throw std::logic_error("Can only finalize frontier once!");
        }
    }

    void assertFinalized(Node* node, const BitString& bs) {
        if(node == nullptr)
            return;

        auto dataNode = dynamic_cast<NodePtrType>(node);
        assertNotNull(dataNode);
        auto data = dataNode->getData();

        (void)bs;
        //logdbg << "Checking node: " << bs << endl;
        assertEqual(data->poly.capacity(), 0);
        //if(node->getBit() == 0) {
        //    assertEqual(data->acc.which(), 0);
        //} else {
        //    assertEqual(data->acc.which(), 1);
        //}
        if(!node->isLeaf() && !simulate) {
            // In the frontier, every node has a left and right child, unless it's a leaf
            auto left = dynamic_cast<NodePtrType>(node->left.get());
            auto right = dynamic_cast<NodePtrType>(node->right.get());
            assertNotNull(left);
            assertNotNull(right);
#ifndef NDEBUG
            // assert leaves don't have accumulators in G2
            if(!left->isLeaf() && !right->isLeaf()) {
                auto leftData = left->getData();
                auto rightData = right->getData();
                assertNotNull(leftData);
                assertNotNull(rightData);
                assertEqual(ReducedPairing(data->acc1, G2::one()), ReducedPairing(leftData->acc1, rightData->acc2));
                assertEqual(ReducedPairing(data->acc1, G2::one()), ReducedPairing(rightData->acc1, leftData->acc2));
                // root doesn't have acc in G2
                if(!node->isRoot()) {
                    assertEqual(ReducedPairing(G1::one(), data->acc2), ReducedPairing(leftData->acc1, rightData->acc2));
                    assertEqual(ReducedPairing(G1::one(), data->acc2), ReducedPairing(rightData->acc1, leftData->acc2));
                }
            }
#endif
        }

        BitString leftLab(bs), rightLab(bs);
        leftLab << 0;
        rightLab << 1;
        assertFinalized(node->left.get(), leftLab);
        assertFinalized(node->right.get(), rightLab);
    }
 
    /**
     * Returns a frontier proof (single subset path) for the specified prefix
     * associated with a key which might or might not have values in the AAD.
     *
     * @param prefix      prefix of a (hashed) missing key or full hash of present key
     *
     * @param isKeyPrefix false if a prefix of a missing AAS/AAD key, true if 
     *                    the hash of a key for an existing AAD key-value pair
     */
    BinaryTree<DataNode<ProofData>>* getFrontierProof(const BitString& prefix, bool isInsertedKey) {
        std::vector<NodePtrType> leaves;
        if(isInsertedKey) {
            //logdbg << "Getting frontier proof for key with hash " << prefix << endl;
            leaves = getKeyLeaves(prefix);
        } else {
            //logdbg << "Getting frontier proof for missing key prefix " << prefix << endl;
            leaves.push_back(getPrefixLeaf(prefix));
        }
        assertFalse(leaves.empty());

        auto proof = new BinaryTree<DataNode<ProofData>>(new DataNode<ProofData>());

        // WARNING: Apparently, using an auto here results in a compile error
        std::function<void(NodePtrType, DataNode<ProofData>*, bool)> copierFunc = [this](
            NodePtrType srcNode, DataNode<ProofData>* destNode, bool isSibling)
        {
            assertNotNull(srcNode);
            assertNotNull(srcNode->getData());
            assertNotNull(destNode);
            bool isRoot = srcNode->isRoot();
            bool isLeaf = srcNode->isLeaf();
            auto srcData = srcNode->getData();

            // might have data set from previous copyPathToRoot call
            if(!destNode->hasData()) {
                destNode->setData(new ProofData());
            }
            
            auto destData = destNode->getData();
            
            // copy nothing for root node, since client has digest
            if(isRoot) {
                destData->setType(ProofData::Type::Root);
                return;
            }

            //logdbg << "Copying node " << srcNode->getLabel() << " (isLeaf = " << isLeaf << ", isSibling = " << isSibling << ")..." << endl;

            G1 g1rand = G1::random_element();
            G1 g1 = !simulate ? srcData->acc1 : g1rand;
            G1 g1ext = !simulate ? srcData->eAcc1 : g1rand;
            G2 g2 = !simulate ? srcData->acc2 : G2::random_element();
            auto type = destData->getType();
            if(isSibling) { // if it's a sibling node, then it needs an accumulator in G2, unless it's a sibling leaf, which don't have G2
                if(isLeaf) {
                    //logdbg << "Setting G1 accumulator in SiblingLeaf" << endl;
                    if(type == ProofData::Type::Unknown) {
                        destData->setType(ProofData::Type::SiblingLeaf);
                        destData->setG1(g1);
                        destData->resetG1ext();
                        destData->resetG2();
                    }
                } else {
                    if(type == ProofData::Type::Unknown) {
                        //logdbg << "Setting G1, G1ext & G2 accumulators in SiblingNonLeaf'" << endl;
                        destData->setType(ProofData::Type::SiblingNonLeaf);
                        destData->setG1(g1);
                        destData->setG1ext(g1ext);
                        destData->setG2(g2);
                    }
                }
            } else { // if it's not a sibling node but an 'on path' node, then it needs extractable accumulators
                if(isLeaf) {
                    // NOTE: the actual leaf being copied will be reconstructed by the client
                    //logdbg << "Setting no accumulators for Leaf " << srcNode->getLabel() << endl;
                    destData->setType(ProofData::Type::Leaf);
                    // leaves will be reconstructed by verifier, so need no accumulators
                    destData->resetG1();
                    destData->resetG1ext();
                    destData->resetG2();
                } else {
                    //logdbg << "Setting G1, G1ext & G2 accumulator(s) for OnPath" << endl;
                    destData->setType(ProofData::Type::OnPath);
                    // Set accumulators, but avoid reallocations
                    if(!destData->hasG1())
                        destData->setG1(g1);
                    if(!destData->hasG1ext())
                        destData->setG1ext(g1ext);
                    if(!destData->hasG2())
                        destData->setG2(g2);
                }
            }

            assertNotNull(destNode->getData());
        };
            
        //logdbg << "Frontier size: " << upperTree->getRoot()->getSize() << endl;
        for(auto leaf : leaves) {
            //logdbg << "Copying leaf " << leaf->getLabel() << "..." << endl;
            leaf->template copyPathToRoot<NodeType, DataNode<ProofData>>(proof, copierFunc);
            //logdbg << "Proof size   : " << proof->getRoot()->getSize() << endl;
        }

        // clears some extractable G1 accumulators
        proof->getRoot()->preorderTraverse([](Node * node) {
            auto dataNode = castProofNode(node);
            auto data = dataNode->getData();
            assertNotNull(data);
            assertFalse(data->getType() == ProofData::Type::Unknown);

            // Remove useless accumulators from proof
            switch(data->getType()) {
            // if node type is Root or Leaf, assert all accumulators are cleared
            case ProofData::Type::Root:
            case ProofData::Type::Leaf:
                assertFalse(data->hasG1());
                assertFalse(data->hasG1ext());
                assertFalse(data->hasG2());
                break;
            case ProofData::Type::OnPath: {
                auto sib = castProofNode(dataNode->getSibling());
                auto sibData = sib->getData();

                auto parent = castProofNode(node->parent);
                auto parentData = parent->getData();
                assertNotNull(parentData);

                switch(sibData->getType()) {
                // if the sibling is a reconstructible frontier leaf
                case ProofData::Type::Leaf:
                    assertTrue(data->hasG1());
                    assertTrue(data->hasG1ext());
                    data->resetG2();            // sibling leaf will have (reconstructed) G2
                    parentData->resetG1ext();   // leaf is extractable too, so parent need not be
                    break;
                // if the sibling is a non-reconstructible frontier leaf, then we need everything in the OnPath node 
                case ProofData::Type::SiblingLeaf:
                    assertTrue(data->hasG1());
                    assertTrue(data->hasG1ext());
                    assertTrue(data->hasG2());
                    break;
                case ProofData::Type::SiblingNonLeaf:
                    data->resetG2();    // sibling has G2
                    assertTrue(data->hasG1());
                    assertTrue(data->hasG1ext());
                    break;
                case ProofData::Type::OnPath:
                    // if sibling has G2, clear own G2 (translates to "if I'm left child, clear my G2")
                    if(sibData->hasG2()) {
                        data->resetG2();
                    } else {
                        assertTrue(data->hasG2());
                    }
                    parentData->resetG1ext();   // sibling is extractable too, so parent need not be
                    assertTrue(data->hasG1());
                    assertTrue(data->hasG1ext());
                    break;
                default:
                    throw std::logic_error("Unexpected frontier proof sibling node type");
                }
                break;
            }
            case ProofData::Type::SiblingLeaf:
                data->resetG2();        // sibling leafs only need G1
                data->resetG1ext();     // sibling leafs only need G1
                assertTrue(data->hasG1());
                break;
            case ProofData::Type::SiblingNonLeaf:
                data->resetG1();        // sibling non-leafs only need G2
                data->resetG1ext();     // sibling non-leafs only need G2
                assertTrue(data->hasG2());
                break;
            default:
                throw std::logic_error("Unexpected frontier proof node type");
            };

        });

#ifndef NDEBUG
        // make sure we properly removed extractable accumulators from certain parents
        proof->getRoot()->inorderTraverse([](Node * node) {
            auto dataNode = castProofNode(node);
            auto data = dataNode->getData();
            assertNotNull(data);

            if(!node->isRoot() && !node->isLeaf() && !data->hasG1ext()) {
                assertTrue(node->hasTwoChildren());
                
                // NOTE: Children might not have extractable G1 either because their children might both be extractable
                //auto left = castProofNode(node->left.get());
                //auto right = castProofNode(node->right.get());
                //assertTrue(left->data->hasG1ext() || left->data->getType() == ProofData::Type::Leaf);
                //assertTrue(right->data->hasG1ext() || right->data->getType() == ProofData::Type::Leaf);
            }
        });
#endif

        return proof;
    }

public:
    static DataNode<ProofData>* castProofNode(Node* node) {
        assertNotNull(node);
        auto cast = dynamic_cast<DataNode<ProofData>*>(node);
        assertNotNull(cast);
        return cast;
    }
};

} // libaad namespace
