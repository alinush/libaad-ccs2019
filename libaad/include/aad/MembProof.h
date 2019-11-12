#pragma once

#include <aad/AADProof.h>
#include <aad/AccumulatedTree.h>
#include <aad/BinaryTree.h>
#include <aad/CommitUtils.h>
#include <aad/EllipticCurves.h>
#include <aad/Frontier.h>

#include <vector>

#include <xutils/Utils.h>

using libaad::G1;
using libaad::G2;
using libaad::ReducedPairing;

namespace libaad {

template<
    int SecParam,
    class CryptoHash,
    class MerkleDataType,
    class LeafMerkleDataType,
    class FrontierProofData      // the type for a frontier node in the frontier proofs
>
class MembershipProof : public AADProof<
    MerkleDataType
> {
public:
    using FrontierNodeType = DataNode<FrontierProofData>;
    using FrontierNodePtrType = FrontierNodeType*; 
    using FrontierTreeType = BinaryTree<FrontierNodeType>;
    using FrontierTreePtrType = FrontierTreeType*;
    
    using FrontierProofsType = std::vector<FrontierTreePtrType>;
    using FrontierProofsPtrType = FrontierProofsType*;

    using AADProofType = AADProof<MerkleDataType>;
    using ForestNodePtrType = typename AADProofType::ForestNodePtrType;

public:
    /**
     * An array of pointers to binary trees (consisting of subset paths with accumulator subset proofs in the siblings.)
     * There will be a binary tree for every root AT in the AAD.
     */
    FrontierProofsPtrType frontierProofs;

    /**
     * In some roots, the key will not be found so we must include a list of its missing prefixes: 
     * one missing prefix for each root AT where it's missing.
     * We map each prefix to the root of the frontier "proof tree" where it can be found!
     */
    boost::unordered_map<FrontierTreePtrType, BitString> missingPrefixes;

public:
    MembershipProof(PublicParameters *pp)
        : AADProofType(pp), frontierProofs(new FrontierProofsType())
    {
    }

    ~MembershipProof() {
        if(frontierProofs != nullptr) {
            for(auto binTree : *frontierProofs) {
                delete binTree;
            }
            delete frontierProofs;
        }
    }

public:
    template<class Group>
    Group simulateCommitment(const std::vector<Fr>& exp) {
        std::vector<Group> bases;
        for(size_t i = 0; i < exp.size(); i++) {
            bases.push_back(Group::one());
        }
        return multiExp<Group>(bases, exp);
    }

    bool fillInFrontierLeaves(FrontierNodePtrType root, std::vector<BitString>& expectedFrontier) {
        assertFalse(expectedFrontier.empty());
        size_t initialSize = expectedFrontier.size();
        fillInFrontierLeavesRecursive(root, expectedFrontier);
        if(!expectedFrontier.empty()) {
            logerror << "Did not find leaves for all frontier nodes. Started with " << initialSize 
                << " leaves but " << expectedFrontier.size() << " still remained." << endl;
        }
        return expectedFrontier.empty();
    }
    
    void fillInFrontierLeavesRecursive(FrontierNodePtrType root, std::vector<BitString>& expectedFrontier) {
        if(root != nullptr) {
            auto data = root->getData();
            assertNotNull(data);
            // the case where the proof path is just the root node should never arise because frontiers are large (512 nodes at least)
            bool isRoot = root->isRoot();
            bool isLeaf = root->isLeaf();
            assertFalse(isRoot && isLeaf);

            if(data->getType() == FrontierProofData::Type::Leaf) {
                //logdbg << "Filling in frontier leaf " << root->getLabel() << " (subtree size: " << root->getSize() << ")" <<endl;

                assertFalse(expectedFrontier.empty());
                size_t chunkSize = expectedFrontier.size() >= SecParam * 4 ? SecParam * 4 : expectedFrontier.size();
                auto beg = expectedFrontier.cbegin();
                auto end = beg + static_cast<long>(chunkSize);

                std::vector<Fr> roots;
                std::vector<Fr> leafPoly;
                hashToField(beg, end, roots);
                poly_from_roots_ntl(leafPoly, roots);

                if(leafPoly.size() > 513) {
                    throw std::runtime_error("Frontier leaf polynomial degree should be 513 or less");
                }

                FrontierNodePtrType sibling = dynamic_cast<FrontierNodePtrType>(root->getSibling());
                assertNotNull(sibling);
                auto sibData = sibling->getData();
                assertNotNull(sibData);
                // figure out what accumulator is in sibling (G1 or G2), if any, to create a different one in the leaf 
                // (e.g., might have two leaves for a key's lower frontiers so we compute G1 in one and G2 in the other)
                if(!sibData->hasG2()) {
                    //assertFalse(sibData->hasG1()); // sibling of leaf might be OnPath node with G1
                    if(this->simulate()) {
                        data->setG2(simulateCommitment<G2>(leafPoly));
                    } else {
                        data->setG2(PolyCommit::commitG2(this->params(), leafPoly, false));
                    }
                } else {
                    if(this->simulate()) {
                        data->setG1(simulateCommitment<G1>(leafPoly));
                    } else {
                        data->setG1(PolyCommit::commitG1(this->params(), leafPoly, false));
                    }
                }

                // remove first chunkSize elements from expectedFrontier
                expectedFrontier.erase(beg, end);
            } else {
                if(isLeaf) {
                    // if it's a sibling leaf node, ignore 
                    //logdbg << "Ignoring " << root->getLabel() << endl;
                } else {
                    assertNotNull(root->left);
                    assertNotNull(root->right);
                    auto left = dynamic_cast<FrontierNodePtrType>(root->left.get());
                    auto right = dynamic_cast<FrontierNodePtrType>(root->right.get());
                    // Make sure casts worked
                    assertTrue(left != nullptr); 
                    assertTrue(right != nullptr);

                    fillInFrontierLeavesRecursive(left, expectedFrontier);
                    fillInFrontierLeavesRecursive(right, expectedFrontier);
                }
            }
        } else {
            // do nothing
        }
    }

    bool isExtractableFrontier(FrontierNodePtrType node) {
        auto left = dynamic_cast<FrontierNodePtrType>(node->left.get());
        auto right = dynamic_cast<FrontierNodePtrType>(node->right.get());

        if(node->isRoot()) {
            assertNotNull(left); 
            assertNotNull(right);
            return isExtractableFrontier(left) && isExtractableFrontier(right);
        } else {
            auto data = node->getData();
            assertNotNull(data);
            if(data->getType() == FrontierProofData::Type::OnPath) {
                assertNotNull(left); 
                assertNotNull(right);
                auto leftData = left->getData();
                auto rightData = right->getData();
                assertNotNull(leftData);
                assertNotNull(rightData);

                if(data->hasG1ext()) {
                    // check node is extractable, if it has g1ext
                    if(ReducedPairing(data->getG1(), this->hasPublicParameters() ? this->params().getG2toTau() : G2::one()) != ReducedPairing(data->getG1ext(), G2::one()) && !this->simulate()) {
                        logerror << "Frontier G1 accumulator is not extractable" << endl;
                        return false;
                    }
                } else {
                    std::array<FrontierProofData*, 2> children;
                    children[0] = leftData;
                    children[1] = rightData;
                    
                    // If the parent has no extractable G1 and one of the children is neither a Leaf nor an OnPath node, then something is wrong.
                    for(auto c : children) {
                        auto t = c->getType();
                        if(t != FrontierProofData::Type::OnPath && t != FrontierProofData::Type::Leaf) {
                            return false;
                        }
                    }
                }

                // INVARIANT: either parent is extractable or it's not but both its children are OnPath/Leaf (and we verify them to be extractable recursively)
                return isExtractableFrontier(left) && isExtractableFrontier(right);
            } else {
                // leaf is extractable
                // sibling+leaf, sibling+non-leaf need not be extractable
                return true;
            }
        }
    }

    bool verifyFrontier(FrontierNodePtrType root) {
        // the case where the proof path is just the root node should never arise because frontiers are large (512 nodes at least)
        bool isRoot = root->isRoot();
        bool isLeaf = root->isLeaf();
        assertFalse(isRoot && isLeaf);

        if(isLeaf) {
            // if it's a sibling node or a leaf, we are done
            return true;
        } else {
            auto data = root->getData();
            assertNotNull(data);
            assertNotNull(root->left);
            assertNotNull(root->right);
            auto left = dynamic_cast<FrontierNodePtrType>(root->left.get());
            auto right = dynamic_cast<FrontierNodePtrType>(root->right.get());
            // Make sure casts worked
            assertNotNull(left); 
            assertNotNull(right);

            auto leftData = left->getData();
            auto rightData = right->getData();
            assertNotNull(leftData); 
            assertNotNull(rightData);

            assertTrue(leftData->hasG1() || leftData->hasG2());
            assertTrue(rightData->hasG1() || rightData->hasG2());
            //assertFalse(leftData->acc1 == nullptr && rightData->acc1 == nullptr);   // it could be true, when we have paths to two leaves
            assertFalse(leftData->hasG2() == false && rightData->hasG2() == false);
            //assertFalse(leftData->hasG1() && leftData->hasG2()); // it could be true, when we have paths to two leaves
            //assertFalse(rightData->hasG1() && rightData->hasG2()); // it could be true, when we have paths to two leaves

            //logdbg << "Verifying frontier node: " << root->getLabel() << endl;
            G1 acc1; 
            G2 acc2;
            if(leftData->hasG2()) {
                acc1 = rightData->getG1();
                acc2 = leftData->getG2();
            } else if(rightData->hasG2()) {
                acc1 = leftData->getG1();
                acc2 = rightData->getG2();
            } else {
                logerror << "Expected one of the two siblings to have a G2 accumulator" << endl;
                return false;
            }

            assertTrue(data->hasG1());
            // get current node's accumulator and children accumulators and check the subset proof
            auto gt = ReducedPairing(data->getG1(), G2::one());
            if(gt != ReducedPairing(acc1, acc2) && !this->simulate()) {
                logerror << "A frontier node's accumulator did not verify against children" << endl;
                return false;
            }

            // check G1 and G2 match in on path node, if it has them
            if(data->getType() != FrontierProofData::Type::Leaf && data->hasG1() && data->hasG2()) {
                if(gt != ReducedPairing(G1::one(), data->getG2()) && !this->simulate()) {
                    logerror << "Non leaf node " << root->getLabel() << " does not have same G1 and G2 accumulators" << endl;
                    return false;
                }
            }

            bool l = verifyFrontier(left);
            bool r = verifyFrontier(right);
            return l && r;
        }
    }

    template<class Key, class Val>
    void getLowerFrontierPrefixes(
        const Key& k,
        const std::vector<std::tuple<Val, int>>& valIds,
        std::vector<BitString>& frontier) const
    {
        BitString keyHash(hashKey(k));

        AccumulatedTree at(SecParam*4);
        for(auto& valId : valIds) {
            auto& val = std::get<0>(valId);
            auto& id = std::get<1>(valId);

            BitString path(keyHash);
            path << hashValue(val, id);

            //logdbg << "Appending " << path << " to reconstructed AT" << endl;
            at.appendPath(path);
        }

        at.getLowerFrontier(frontier, keyHash);
        assertFalse(frontier.empty());
        std::sort(frontier.begin(), frontier.end());
        //logdbg << "Lower frontier for key " << k << " with " << valIds.size() << " values: " << frontier.size() << " prefixes" << endl;
    }

    /**
     * NOTE: We specify the expected values in 'values' for debugging purposes. The values are already in the Merkle paths in the proof
     * and we verify completeness via the frontier. We could've checked that the values are what we expected outside of this function by
     * returning the values.
     */
    template<class Key, class Val>
    bool verify(const Key& k, std::list<Val> values, const Digest& digest) {
        this->verified = true;
        assertEqual(this->forestProofs->size(), frontierProofs->size());
        assertEqual(this->forestProofs->size(), digest.size());

        std::vector<std::tuple<Val, int>> valIds; // values and their indexes (for the current tree in the forest)
        valIds.reserve(values.size());
        std::vector<ForestNodePtrType> leafs;   // leafs with key-value pairs (for the current tree in the forest)
        leafs.reserve(values.size());
        std::vector<BitString> expectedFrontier;
        expectedFrontier.reserve(values.empty() ? 1 : values.size() * SecParam * 2);

        for(size_t i = 0; i < this->forestProofs->size(); i++) {
            logtrace << "Verifying forest tree #" << i << endl;
            leafs.clear();
            valIds.clear();
            expectedFrontier.clear();

            auto merkleSubtree = (*this->forestProofs)[i];
            auto frontierSubtree = (*frontierProofs)[i];

            if(merkleSubtree != nullptr) {
                auto root = merkleSubtree->getRoot();
                assertNotNull(root);
                auto rootData = root->getData();
                assertNotNull(rootData);

                // Copy root AT accumulator into proof, unless the root is a leaf in which case we compute it
                if(!root->isLeaf())
                    rootData->acc.reset(new G1(std::get<0>(digest[i])));

                // Pass 1: get a vector of pointers to all leafs with data.
                if(!this->prevalidateForestProof(root, leafs))
                    return false;

                for(auto leaf : leafs) {
                    // Store every value and its leafNo in valIds. We need these to check the frontier later.
                    auto data = dynamic_cast<LeafMerkleDataType*>(leaf->getData());
                    assertNotNull(data);
                    valIds.push_back(std::make_tuple(data->v, data->leafNo));

                    // Key in leafs should match actual key
                    if(data->k != k)
                        return false;

                    // Compute leaf's AT accumulator
                    AccumulatedTree at(SecParam*4, CryptoHash().hashKV(k, data->v, data->leafNo));
                    G1 acc;
                    assertTrue(at.getPrefixes().size() == 513);
                    std::tie(acc, std::ignore) = CommitUtils::commitAT(&at,
                        this->hasPublicParameters() ? &this->params() : nullptr, 
                        false);
                    data->acc.reset(new G1(acc));

                    // Also, mark values as verified by removing them from 'values'
                    Utils::removeFirst(values, data->v);
                }

                // Pass 2: recursively compute the root Merkle hash
                this->computeMerkleHashes(root, true);

                // Next, check root Merkle hash matches the one in digest
                auto& expectedHash = std::get<2>(digest[i]);
                if(root->getData()->merkleHash != expectedHash && !this->simulate()) {
                    logerror << "Merkle root did not match" << endl;
                    return false;
                }

                // Pass 3: recursively check subset proofs
                if(!this->verifySubsetProofs(root))
                    return false;

                // Done checking forest paths!

                // Compute lower frontier from valIds 
                assertFalse(valIds.empty());
                getLowerFrontierPrefixes(k, valIds, expectedFrontier);
            } else {
                // Key is not present in the current forest tree, so ensure \exists prefix of key in frontier
                expectedFrontier.push_back(missingPrefixes[frontierSubtree]);
            }

            // Copy root frontier accumulator into proof
            auto frontierRoot = frontierSubtree->getRoot();
            assertNotNull(frontierRoot);
            assertFalse(frontierRoot->data->hasG1());
            frontierRoot->data->setG1(std::get<1>(digest[i]));
             
            //logdbg << "Frontier size (nodes): " << frontierSubtree->getRoot()->getSize() << endl;
            if(!fillInFrontierLeaves(frontierSubtree->getRoot(), expectedFrontier)) {
                logerror << "Did not find prefixes for all missing keys/values" << endl;
                return false;
            }

            if(!isExtractableFrontier(frontierRoot)) {
                logerror << "Frontier proof is not extractable everyhere" << endl;
                return false;
            }
 
            if(!verifyFrontier(frontierRoot)) {
                logerror << "Frontier proof for completeness of values did NOT verify" << endl;
                return false;
            }
        }

        // All values should've been validated and removed
        return values.empty();
    }

    virtual int getProofSize() const {
        return this->getForestProofSize() + getFrontierProofSize();
    }

    int getFrontierProofSize() const {
        if(this->verified)
            throw std::runtime_error("Please call this before calling verify(), which increases the proof size.");

        int size = 0;
        for(size_t i = 0; i < this->forestProofs->size(); i++) {
            int frontierPath = getFrontierProofSize((*frontierProofs)[i]->getRoot());

            //logdbg << "Forest subtree #" << i << ": " << forestPath << endl;

            size += frontierPath;
        }

        return size;
    }

    int getFrontierProofSize(Node* node) const {
        if(node == nullptr) {
            return 0;
        } else {
            FrontierNodePtrType nodeFr = dynamic_cast<FrontierNodePtrType>(node);
            assertNotNull(nodeFr);
            int size = 0;
            // for the root node, client already has the (extractable) root accumulators
            if(!node->isRoot()) {
                auto data = nodeFr->getData();
                if(data->hasG1())
                    size += G1ElementSize;
                if(data->hasG1ext())
                    size += G1ElementSize;
                if(data->hasG2())
                    size += G2ElementSize;
            }

            return size + getFrontierProofSize(node->left.get()) + getFrontierProofSize(node->right.get()); 
        }
    }
    
    bool isNonMembershipProof() const {
        for(auto tree : *this->forestProofs) {
            if(tree != nullptr)
                return false;
        }
        return true;
    }
};

template<int SecParam, class CryptoHash, class FoDT, class FoLDT, class FrDT>
std::ostream& operator<<(std::ostream& out, MembershipProof<SecParam, CryptoHash, FoDT, FoLDT, FrDT>& proof) {
    std::stringstream str;
    if(!proof.isNonMembershipProof()) {
        int treeCount = 0;
        for(auto tree : *proof.forestProofs) {
            if(tree != nullptr) {
                treeCount++;
                str << tree->getRoot()->getSize() << ", ";
            }
        }
        out << treeCount << " non-empty forest trees of size: " << str.str() << endl;
    } else {
        out << "Non-membership proof, so no forest trees." << endl;
    }

    str.str(std::string());
    for(auto tree : *proof.frontierProofs) {
        testAssertNotNull(tree);
        str << tree->getRoot()->getSize() << ", ";
    }
    
    out << proof.frontierProofs->size() << " frontier proofs of size " << str.str() << endl;
    return out;
}

} // namespace libaad
