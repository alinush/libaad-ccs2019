#pragma once

#include <functional>   // std::hash
#include <boost/container/flat_map.hpp>
#include <boost/container/map.hpp>
#include <map>

#include <aad/AccumulatedTree.h>
#include <aad/MembProof.h>
#include <aad/AppendOnlyProof.h>
#include <aad/CommitUtils.h>
#include <aad/EllipticCurves.h>
#include <aad/Hashing.h>
#include <aad/PublicParameters.h>
#include <aad/PolyInterpolation.h>

#include <xutils/Utils.h>
#include <xutils/NotImplementedException.h>

using libaad::G1;
using libaad::G2;
using libaad::Fr;

namespace libaad {

class Sha256 {
public:
    BitString hashK(const std::string& k) {
        return hashString(k);
    }

    BitString hashKV(const std::string& k, const std::string& v, int idx) {
        return hashKeyValuePair(k, v, idx);
    }
};

/**
 * BitString CryptoHash(const KeyT& k, const ValT& v, int index);
 * EnableFrontier is set to false when bechmarking append-only proofs because we don't need to waste time creating frontiers in those benchmarks
 */
template<class KeyT, class ValT, bool EnableFrontier = true, int SecParam = 128, class CryptoHash = Sha256>
class AAD {
public:
    class DataType;     // data stored in internal nodes in the forest
    class LeafDataType; // data stored in leaf nodes in the forest
    class MerkleData;           // data stored in Merkle nodes in a membership or append-only proof
    class LeafMerkleData;       // data stored in leaf Merkle nodes in a membership proof
    class MergeFunc;    // function that merges two nodes' data in the forest

public:
    using DataPtrType = DataType*;  // pointer to data stored in nodes in the forest
    using ForestNodeType = DataNode<DataType>;  // nodes in the forest
    using ForestNodePtrType = ForestNodeType*;  // pointer to nodes in the forest

    using FrontierType = Frontier;  // the class used to represent a frontier
    using FrontierPtrType = FrontierType*;  // pointer to a Frontier object
    using FrontierNodeType = DataNode<typename FrontierType::DataType>; // nodes in the frontier BinaryTree

    using MembProofType = MembershipProof<SecParam, CryptoHash, MerkleData, LeafMerkleData, typename FrontierType::ProofData>;
    using MembProofPtrType = std::unique_ptr<MembProofType>;
    using AppendOnlyProofType = AppendOnlyProof<MerkleData>;
    using AppendOnlyProofPtrType = std::unique_ptr<AppendOnlyProofType>;

    using AccTreeNodePtrType = Node*;
    using AccTreeType = AccumulatedTree;
    using AccTreePtrType = AccTreeType*;
    using IndexedForestType = IndexedForest<KeyT, DataType, MergeFunc>;

public:
    class DataType {
    public:
        int size;   // the number of key-value pairs under this subtree (for debugging)

        // The Merkle hash
        MerkleHash merkleHash;
        // AT accumulator over all prefixes (G1)
        G1 acc, eAcc;
        // Append-only proof (none for roots) (G2)
        G2 subsetProof;
        // Disjointness/GCD proof here consisting of Bezout coefficients (only for roots) (both in G2)
        std::unique_ptr<G2> x, y;  // e(acc, x) e(frontierAcc, y) = e(g,g)
        // AT polynomial here (only for roots)
        std::vector<Fr> accPoly;

        // The accumulated tree (AT) (only for roots)
        std::unique_ptr<AccTreeType> at;
        // NOTE: The frontier accumulator and proofs are stored in this Frontier object.
        // Also, the frontier polynomial is not needed once frontier proofs are computed.
        std::unique_ptr<FrontierType> frontier;

    public:
        // Called when copying node data during a membership proof
        DataType(int size)
            : size(size),
              acc(G1::one()), eAcc(G1::one()), subsetProof(G2::one()),
              x(nullptr), y(nullptr),
              at(nullptr), frontier(nullptr)
        {
        }

        // We added a computeFrontier flag because, when merging repeatedly, we waste time computing the frontier
        // just to discard it in the next merge.
        DataType(PublicParameters * pp, AccTreePtrType at, int size, bool computeFrontier)
            : DataType(size)
        {
            bool simulate = pp == nullptr;
            this->at.reset(at);

            if(!simulate) {
                assertNotNull(pp);
                std::tie(acc, eAcc) = CommitUtils::commitAT(at, accPoly, pp, true);

                assertEqual(ReducedPairing(acc, pp->getG2toTau()), ReducedPairing(eAcc, G2::one()));
            } else {
                acc = G1::one();
                eAcc = G1::one(); // normally this should be g^{tau}, but we have no public params when simulate=true
            }

            if(computeFrontier && EnableFrontier) {
                ManualTimer t;
                std::chrono::microseconds::rep micros = 0;

                // Create an empty frontier
                frontier.reset(new FrontierType(pp));

                // Get upper frontier nodes and pointers to 'upper frontier leafs' or 'lower frontier roots'
                std::vector<BitString> upperFrontierNodes;
                std::vector<AccTreeNodePtrType> lowerRoots;
                t.restart();
                at->getUpperFrontier(upperFrontierNodes, lowerRoots);
                micros += t.stop().count();

                // First, get lower frontier nodes for each value in the AT
                std::vector<BitString> frontierNodes;
                for(auto lowRoot : lowerRoots) {
                    t.restart();
                    frontierNodes.clear();  // clear upper frontier nodes or previous iteration's lower frontier nodes

                    // NOTE(Alin): Getting the label after already having the node pointer is a bit inefficient
                    // because we already walked down the tree to obtain the node pointer and could've gotten the label too.
                    BitString keyHash = lowRoot->getLabel();
                    
                    at->getLowerFrontier(frontierNodes, keyHash, lowRoot);
                    std::sort(frontierNodes.begin(), frontierNodes.end());
                    micros += t.stop().count();

                    size_t chunkSize = SecParam * 4;
                    size_t numChunks = frontierNodes.size() / chunkSize;
                    auto it = frontierNodes.cbegin();
                    for(size_t i = 0; i < numChunks; i++) {
                        frontier->addMissingValuesPrefixes(keyHash, it, it + static_cast<long>(chunkSize));
                        it += static_cast<long>(chunkSize);
                    }
                    size_t leftOver = frontierNodes.size() % chunkSize;
                    if(leftOver > 0) {
                        frontier->addMissingValuesPrefixes(keyHash, it, it + static_cast<long>(leftOver));
                    }
                }
                
                // Second, add prefixes for the missing keys (upper frontier)
                for(auto& prefix : upperFrontierNodes) {
                    frontier->addMissingKeyPrefix(prefix);
                }

                t.restart();
                std::vector<BitString>().swap(upperFrontierNodes);
                micros += t.stop().count();

                //logdbg << "Finalizing frontier..." << endl;
                
                frontier->finalize();
                 
                if(!simulate) {
                    printOpPerf(micros, "frontierCompute", static_cast<size_t>(frontier->getSize()));
                    assertNotNull(pp);

                    // compute EEA between AT and frontier polynomials
                    std::vector<Fr> coeffX, coeffY;
                    auto& frRootPoly = frontier->getRootPoly();
                    assertFalse(libfqfft::_is_zero(frRootPoly));
                    ManualTimer eeaCompTimer;
                    eea_ntl(accPoly, frRootPoly, coeffX, coeffY);
                    printOpPerf(eeaCompTimer.stop().count(), "computeEEA", frRootPoly.size());

                    // clear the frontier polynomial
                    std::vector<Fr>().swap(frRootPoly);

                    // commit to EEA coeffs (assuming no next MergeFunc call)
                    ManualTimer eeaCommitTimer;
                    x.reset(new G2(PolyCommit::commitG2(*pp, coeffX, false)));
                    y.reset(new G2(PolyCommit::commitG2(*pp, coeffY, false)));
                    printOpPerf(eeaCommitTimer.stop().count(), "commitEEA", coeffX.size() + coeffY.size());
            
                    assertEqual(ReducedPairing(acc, *x)*ReducedPairing(frontier->getRootAcc(), *y), ReducedPairing(G1::one(), G2::one()));
                } else {
                    x.reset(new G2(G2::one()));
                    y.reset(new G2(G2::one()));
                }
            }
        }

        virtual ~DataType() {
        }
        
    public:
        void freeAfterMerge() {
            x.reset(nullptr);
            y.reset(nullptr);
            std::vector<Fr>().swap(accPoly);   // clears memory
            assertNull(at); // was std::move'd so should be null
            frontier.reset(nullptr);
        }
    };

    class LeafDataType : public DataType {
    public:
        KeyT k;
        ValT v; 
        int leafNo;

    public:
        // Used when creating a new leaf in the forest
        LeafDataType(PublicParameters * pp, const KeyT& k, const ValT& v, int leafNo, int batchSize)
            : DataType(pp, 
                new AccTreeType(SecParam*4, CryptoHash().hashKV(k, v, leafNo)), 
                1,
                batchSize == 1 ? leafNo % 2 == 0 : false), // only computes frontier if batchSize is 1 and leaf is even numbered (e.g., 0, 2, 4, ...)
             k(k), v(v), leafNo(leafNo)
        {
            bool simulate = pp == nullptr;
            if(!simulate) {
                assertNotNull(pp);
                // NOTE: AT and frontier are computed in DataType class's constructor

                // compute leaf's Merkle hash from AT and empty left and right hashes
                this->merkleHash = MerkleHash(this->acc); 
            } else {
                this->merkleHash = MerkleHash::dummy();
            }
        }

    };
    
    // Used for Merkle proof data
    class MerkleData {
    public:
        enum class Type { Sibling, Leaf, OnPath };

    public:
        std::unique_ptr<G1> acc;
        std::unique_ptr<G2> subsetProof;
        MerkleHash merkleHash;
        Type t;

    public:
        MerkleData(Type t) : t(t) {}
        virtual ~MerkleData() {}

    public:
        bool isSibling() const { return t == Type::Sibling; }
        bool isLeaf() const { return t == Type::Leaf; }
        /**
         * We mark old roots in an append-only proof as leaves, since that's what the verification code expects.
         */
        bool isOldRoot() const { return isLeaf(); }
    };

    // Used for forest leaves in a membership proof
    class LeafMerkleData : public MerkleData {
    public:
        KeyT k;
        ValT v;
        int leafNo;

    public:
        // Used when copying a leaf during a membership proof
        LeafMerkleData(const KeyT& k, const ValT& v, int leafNo)
            : MerkleData(MerkleData::Type::Leaf), k(k), v(v), leafNo(leafNo)
        {
        }
    };

    class MergeFunc {
    protected:
        int batchSize;
        PublicParameters *pp;

    public:
        MergeFunc()
            : batchSize(1) {
        }

    public:
        void setBatchSize(int size) { batchSize = size; }
        void setPublicParameters(PublicParameters *p) { pp = p; }

        DataPtrType operator() (ForestNodePtrType leftNode, ForestNodePtrType rightNode, bool isLastMerge)
        {
            bool simulate = pp == nullptr;
            DataPtrType left = leftNode->getData();
            DataPtrType right = rightNode->getData();
            //ManualTimer t1;
            //logdbg << "Merging size " << left->size << " with size " << right->size 
            //    << " ... (isLastMerge = " << isLastMerge << ")" << endl;

            // Merges the two children ATs
            auto at = new AccTreeType(std::move(left->at), std::move(right->at));
            //std::chrono::milliseconds mus1 = std::chrono::duration_cast<std::chrono::milliseconds>(t1.stop());

            //ManualTimer t2;

            // Automatically computes frontier if this is the last merge (and its the last append in a batch)
            int parentLevel = leftNode->getHeight();    // parent will be one level higher than child (leaves are at level 1)
            bool haveFullBatch = parentLevel - 1 >= Utils::log2floor(batchSize); // e.g., when batchSize = 2 log2floor will be 1 => parent of two leaves has parentLevel = 2
            auto data = new DataType(pp, at, left->size + right->size, isLastMerge && haveFullBatch);
            //std::chrono::milliseconds mus2 = std::chrono::duration_cast<std::chrono::milliseconds>(t2.stop());

            //if(mus1.count() + mus2.count() > 100) {
            //    logperf << "Merging ATs took " << mus1.count() << " ms" << endl;
            //    if(isLastMerge)
            //        logperf << "Creating frontier (n = " << data->size << " ) took " << mus2.count() << " ms" << endl;
            //}
            
            if(!simulate) {
                assertNotNull(pp);

                // now parent AT is ready, so compute append-only proofs (store in 'left' and 'right')
                std::vector<Fr> quotient, rem;
                poly_divide_ntl(quotient, rem, data->accPoly, left->accPoly);
                assertTrue(libfqfft::_is_zero(rem));
                left->subsetProof = PolyCommit::commitG2(*pp, quotient, false);

                quotient.clear();
                rem.clear();
                poly_divide_ntl(quotient, rem, data->accPoly, right->accPoly);
                assertTrue(libfqfft::_is_zero(rem));
                right->subsetProof = PolyCommit::commitG2(*pp, quotient, false);

                // compute Merkle hash
                data->merkleHash = MerkleHash(data->acc, left->merkleHash, right->merkleHash);
            } else {
                left->subsetProof = G2::one();
                right->subsetProof = G2::one();
                data->merkleHash = MerkleHash::dummy();
            }

            assertEqual(ReducedPairing(data->acc, G2::one()), ReducedPairing(left->acc, left->subsetProof));
            assertEqual(ReducedPairing(data->acc, G2::one()), ReducedPairing(right->acc, right->subsetProof));

            // No longer need AT and frontier in the merged old roots
            left->freeAfterMerge();
            right->freeAfterMerge();

            return data;
        }
    };

protected:
    IndexedForestType forest;
    PublicParameters * params;
    bool simulate;
    int batchSize;  // only computes frontiers for trees with more leaves than 'batchSize'
    MergeFunc mergeFunc;

public:
    AAD(PublicParameters * p = nullptr)
        : params(p), simulate(p == nullptr), batchSize(1)
    {
        mergeFunc.setPublicParameters(p);
        forest.setMergeFunc(&mergeFunc);
    }

    ~AAD() {
        //logdbg << "Destroying AAD" << endl;
    }

public:
    void setBatchSize(int size) {
        assertStrictlyPositive(size);

        batchSize = size;
        mergeFunc.setBatchSize(size);
    }

    int getSize() const { return forest.getCount(); }

    KeyT getKeyByLeafNo(int leafNo) {
        (void)leafNo;
        auto result = forest.getTreeAndLeaf(leafNo);
        auto leafNode = dynamic_cast<ForestNodePtrType>(std::get<1>(result));
        assertNotNull(leafNode);
        auto data = dynamic_cast<LeafDataType*>(leafNode->data.get());
        assertNotNull(data);
        return data->k;
    }

    std::vector<std::tuple<AccTreePtrType, FrontierPtrType>> getRootATs() const {
        std::vector<std::tuple<AccTreePtrType, FrontierPtrType>> roots;
        auto trees = forest.getTrees();
        for(auto tup : trees) {
            auto rootNode = std::get<1>(tup);
            auto data = rootNode->data.get();
            roots.push_back(std::make_tuple(data->at.get(), data->frontier.get()));
        }

        return roots;
    }

    Digest getDigest(int version) const {
        Digest d;
        auto oldRoots = forest.getOldRoots(version);

        for(auto rootNode : oldRoots) {
            auto data = rootNode->data.get();
            assertNotNull(data);
            // Older roots will not have their frontier trees around anymore, so we just return G1::zero() for the frontier accumulator
            // (Unless an 'old root' is still a 'new root', in which case it does and we include its frontier accumulator.)
            G1 fracc = G1::zero();
            if(data->frontier != nullptr)
                fracc = data->frontier->getRootAcc();
            d.push_back(std::make_tuple(data->acc, fracc, data->merkleHash));
        }

        return d;
    }

    Digest getDigest() const {
        Digest d;

        // returns the trees in the forest, highest tree first!
        auto trees = forest.getTrees();
        for(auto tup : trees) {
            auto rootNode = std::get<1>(tup);
            auto data = rootNode->data.get();
            d.push_back(std::make_tuple(
                data->acc,
                EnableFrontier ? data->frontier->getRootAcc() : G1::zero(),
                data->merkleHash));
        }

        return d;
    }

    void append(const KeyT& k, const ValT& v) {
        int i = forest.getCount();

        //logdbg << endl;
        //logdbg << "Append #" << i+1 << ": (" << k << ", " << v << ") ..." << endl;

        auto leafData = new LeafDataType(params, k, v, i, batchSize);
        forest.appendLeaf(leafData, k);
        //logdbg << "Num trees: " << forest.getNumTrees() << endl;
    }

    /**
     * Returns the keys in the AAD, in no particular order.
     */
    std::vector<KeyT> getKeys() const {
        std::vector<KeyT> keys;
        forest.getLookupKeys(keys);
        return keys;
    }

    /**
     * Returns the list of values for 'key', in the order they were inserted!
     */
    std::list<ValT> getValues(const KeyT& key) const {
        std::list<ValT> vals;

        const auto& leaves = forest.getLeaves(key);

        // WARNING: Assuming the leaves were added to the vector in the order they were appended!
        for(auto leafNode : leaves) {
            assertNotNull(leafNode);
            auto leafData = dynamic_cast<LeafDataType*>(leafNode->getData());
            assertNotNull(leafData);

            vals.push_back(leafData->v);
        }
        
        return vals;
    }

    AppendOnlyProofPtrType appendOnlyProof(int prevVersion) {
        // NOTE: Append-only proof does not include EEA proofs in the new roots (we assume when the client gets the new digest it also gets EEA proofs)
        AppendOnlyProofPtrType proof(new AppendOnlyProofType(params));

        // get old roots
        auto oldRoots = forest.getOldRoots(prevVersion);
        std::set<ForestNodePtrType> oldRootSet(oldRoots.begin(), oldRoots.end());
        
        std::function<void(ForestNodePtrType, typename MembProofType::ForestNodePtrType, bool)> copierFunc = 
        [this, &oldRootSet](ForestNodePtrType srcNode, typename MembProofType::ForestNodePtrType destNode, bool isSrcSibling) {
            assertNotNull(srcNode);
            assertNotNull(destNode);

            bool isOldRoot = oldRootSet.count(srcNode) > 0;
            logtrace << "Copying source node " << srcNode->getLabel() << endl; 

            DataType * srcData = srcNode->getData();
            bool destHasData = destNode->getData() != nullptr;

            if(isSrcSibling) {
                logtrace << " * Source node is a sibling" << endl;
                // If destNode is has no proof data, then make it a sibling Merkle node and store a Merkle hash in it.
                // Otherwise, it's a previously-set sibling node or an "on path" node and needs no Merkle hash.
                if(!destHasData) {
                    logtrace << " * Dest (sibling) node has NO data" << endl;
                    auto destData = new MerkleData(MerkleData::Type::Sibling);
                    destData->merkleHash = srcData->merkleHash;
                    destNode->setData(destData);
                } else {
                    logtrace << " * Dest (sibling or 'on path') node has data, doing nothing." << endl;
                }
            } else {
                if(isOldRoot) {
                    logtrace << " * Source node is old root" << endl;
                    destNode->setData(new MerkleData(MerkleData::Type::Leaf));
                    // ...and the subset proof
                    if(!destNode->isRoot()) {
                        destNode->data->subsetProof.reset(new G2(!simulate ? srcData->subsetProof : G2::random_element()));
                    }
                } else {
                    logtrace << " * Source node is 'on path'" << endl;
                    // We could be overwriting a sibling or an "on path" node (never a leaf)
                    if(!destHasData || destNode->getData()->isSibling()) {
                        logtrace << " * (Re)allocating dest node data" << endl;
                        // If it's a sibling, we need to change its type.
                        destNode->setData(new MerkleData(MerkleData::Type::OnPath));
                    } else {
                        logtrace << " * Dest node is allocated (and not sibling)" << endl;
                    }
                        
                    // INVARIANT: At this point, 'destNode' is an "on path" node with MerkleData in it

                    auto destData = destNode->getData();
                    assertNotNull(destData);

                    // For "on path" nodes, make sure Merkle hash is unset if previously set for this node as a sibling
                    // (should be unset by replacing data above)
                    assertTrue(destData->merkleHash.isUnset());

                    // For "on path" nodes (except root), copy AT accumulator (if not already there)
                    if(!destNode->isRoot() && destData->acc == nullptr)
                        destData->acc.reset(new G1(!simulate ? srcData->acc : G1::random_element()));

                    // For "on path" nodes (except root), copy AT subset proof (if not already there)
                    if(!destNode->isRoot() && destData->subsetProof == nullptr)
                        destData->subsetProof.reset(new G2(!simulate ? srcData->subsetProof : G2::random_element()));
                }
            }
        };
        
        // Get Merkle paths from old roots to new roots
        proof->forestProofs = forest.copyMerklePaths(oldRoots, copierFunc);
        
        return proof;
    }

    /**
     * NOTE: To get the values of key k, call getValues().
     */
    MembProofPtrType completeMembershipProof(const KeyT& k) const {
        if(!EnableFrontier) {
            throw std::runtime_error("Cannot do complete membership proofs with EnableFrontier = false");
        }

        MembProofPtrType membProof(new MembProofType(params));

        /**
         * When isSrcSibling is true, it means we are copying a (source) sibling node over. 
         * If destination has no data, we create a "sibling" MerkleData for it.
         * Otherwise, destination is previously-set sibling or previously-set "on path" node, so we don't copy.
         *
         * When isSrcSibling is false, it means we are copying an "on path" node over.
         * If destination has no data or has data, but from a sibling node (i.e., "sibling" MerkleData),
         * we replace it with MerkleData.
         * So we either overwrite previously-set sibling node or overwrite "on path" node.
         */
        std::function<void(ForestNodePtrType, typename MembProofType::ForestNodePtrType, bool)> copierFunc = 
        [this](ForestNodePtrType srcNode, typename MembProofType::ForestNodePtrType destNode, bool isSrcSibling) {
            assertNotNull(srcNode);
            assertNotNull(destNode);

            bool isSrcLeaf = srcNode->isLeaf();
            logtrace << "Copying source node " << srcNode->getLabel() << endl; 

            DataType * srcData = srcNode->getData();
            bool destHasData = destNode->getData() != nullptr;

            if(isSrcSibling) {
                logtrace << " * Source node is a sibling" << endl;
                // If destNode is has no proof data, then make it a sibling Merkle node and store a Merkle hash in it.
                // Otherwise, it's a previously-set sibling node or an "on path" node and needs no Merkle hash.
                if(!destHasData) {
                    logtrace << " * Dest (sibling) node has NO data" << endl;
                    auto destData = new MerkleData(MerkleData::Type::Sibling);
                    destData->merkleHash = srcData->merkleHash;
                    destNode->setData(destData);
                } else {
                    logtrace << " * Dest (sibling or 'on path') node has data, doing nothing." << endl;
                }
            } else {
                if(isSrcLeaf) {
                    logtrace << " * Source node is leaf" << endl;
                    // We will never copy the same path (i.e., leaf) twice, but the dest leaf might have data because 
                    // it might be a sibling hash from a previous key-value pair
                    auto leafData = dynamic_cast<LeafDataType*>(srcData);
                    //logdbg << "Looking at leaf with key " << leafData->k
                    //    << ", value " << leafData->v << " and index " << leafData->leafNo << endl;
                    assertNotNull(leafData);
                    // For leafs, copy the <k,v,i> pair
                    destNode->setData(new LeafMerkleData(leafData->k, leafData->v, leafData->leafNo));
                    // ...and the subset proof
                    if(!destNode->isRoot()) {
                        destNode->data->subsetProof.reset(new G2(!simulate ? srcData->subsetProof : G2::random_element()));
                    }
                } else {
                    logtrace << " * Source node is 'on path'" << endl;
                    // We could be overwriting a sibling or an "on path" node (never a leaf)
                    if(!destHasData || destNode->getData()->isSibling()) {
                        logtrace << " * (Re)allocating dest node data" << endl;
                        // If it's a sibling, we need to change its type.
                        destNode->setData(new MerkleData(MerkleData::Type::OnPath));
                    } else {
                        logtrace << " * Dest node is allocated (and not sibling)" << endl;
                    }
                        
                    // INVARIANT: At this point, 'destNode' is an "on path" node with MerkleData in it

                    auto destData = destNode->getData();
                    assertNotNull(destData);

                    // For "on path" nodes, make sure Merkle hash is unset if previously set for this node as a sibling
                    // (should be unset by replacing data above)
                    assertTrue(destData->merkleHash.isUnset());

                    // For "on path" nodes (except root), copy AT accumulator (if not already there)
                    if(!destNode->isRoot() && destData->acc == nullptr)
                        destData->acc.reset(new G1(!simulate ? srcData->acc : G1::random_element()));

                    // For "on path" nodes (except root), copy AT subset proof (if not already there)
                    if(!destNode->isRoot() && destData->subsetProof == nullptr)
                        destData->subsetProof.reset(new G2(!simulate ? srcData->subsetProof : G2::random_element()));
                }
            }
        };

        // Get Merkle paths to all values of 'k', if any.
        membProof->forestProofs = forest.partialMembershipProof(k, copierFunc);

        for(auto tree : *membProof->forestProofs) {
            if(tree != nullptr) {
                // Remove root accumulator from tree since client has it
                auto rootData = tree->getRoot()->getData();
                (void)rootData;
                assertNull(rootData->acc);
                assertNull(rootData->subsetProof);
            }
        }

        BitString keyHash = CryptoHash().hashK(k);
        BitString missingPrefix;

        // Go through all roots and do completeness proofs (handles non-membership of 'k')
        for(auto tup : getRootATs()) {
            bool found;
            auto accuTree = std::get<0>(tup); // get AT
            auto frontier = std::get<1>(tup); // get frontier

            // Check if key is in AT, and if not, get is first missing prefix for frontier proofs later
            std::tie(found, std::ignore, missingPrefix) = accuTree->containsKey(keyHash);

            // Get frontier proofs, either for completeness of k's values or for non-membership of k
            auto frontierProof = frontier->getFrontierProof(
                found ? keyHash : missingPrefix,
                found);
            
            membProof->frontierProofs->push_back(frontierProof);

            // Map missing prefixes to their frontier proof
            if(!found)
                membProof->missingPrefixes[frontierProof] = missingPrefix;
        }

        // We won't have Merkle paths in every tree in the forest, but we store nullptr's where we don't
        assertEqual(membProof->forestProofs->size(), static_cast<size_t>(forest.getNumTrees()));
        // ...but we will have a frontier proof for every tree
        assertEqual(membProof->frontierProofs->size(), static_cast<size_t>(forest.getNumTrees()));

        return membProof;
    }

    const IndexedForestType& getIndexedForest() const {
        return forest;
    }
};

}
