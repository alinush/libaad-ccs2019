#pragma once

#include <aad/BinaryTree.h>
#include <aad/PublicParameters.h>
#include <aad/Hashing.h>

#include <vector>

namespace libaad {

// (AT accumulator, frontier accumulator, Merkle hash)
using Digest = std::vector<std::tuple<G1, G1, MerkleHash>>;

template<
    class MerkleDataType
>
class AADProof {
public:
    using ForestNodeType = DataNode<MerkleDataType>;
    using ForestNodePtrType = DataNode<MerkleDataType>*;

    using ForestTreeType = BinaryTree<ForestNodeType>;
    using ForestTreePtrType = ForestTreeType*;

    using ForestProofsType = std::vector<ForestTreePtrType>;
    using ForestProofsPtrType = ForestProofsType*;

protected:
    /**
     * An array of pointers to binary trees (consisting of subset paths with accumulator subset proofs and Merkle hashes.)
     * For membership proofs, there will be a binary tree for every tree in the forest where the key has one or more values.
     */
    PublicParameters * pp;

public:
    ForestProofsPtrType forestProofs;

    bool verified;

public:
    AADProof(PublicParameters *pp)
        : pp(pp), forestProofs(nullptr), verified(false)
    {}

    virtual ~AADProof() {
        if(forestProofs != nullptr) {
            for(auto binTree : *forestProofs) {
                delete binTree;
            }
            delete forestProofs;
        }
    }

public:
    bool hasPublicParameters() const { return pp != nullptr; }
    PublicParameters& params() { return *pp; }

    bool simulate() const { return !hasPublicParameters(); }

    virtual int getProofSize() const {
        return getForestProofSize();
    }

    virtual int getForestProofSize() const {
        if(this->verified)
            throw std::runtime_error("Please call this before calling verify(), which increases the proof size.");

        int size = 0;
        for(size_t i = 0; i < this->forestProofs->size(); i++) {
            auto subtree = (*this->forestProofs)[i];
            int forestPath = subtree != nullptr ? getForestProofSize(subtree->getRoot()) : 0;

            //logdbg << "Forest subtree #" << i << ": " << forestPath << endl;

            size += forestPath;
        }

        return size;
    }

    ForestNodePtrType merkleCast(Node * node) const {
        assertNotNull(node);
        auto merkleNode = dynamic_cast<DataNode<MerkleDataType>*>(node);
        assertNotNull(merkleNode);
        return merkleNode;
    }

    bool isMerkleSibling(Node* node) const {
        assertNotNull(node);
        auto data = merkleCast(node)->getData();
        assertNotNull(data);

        // if it's a sibling, assert it has a Merkle hash
        if(data->isSibling()) {
            assertTrue(!data->merkleHash.isUnset());
            return true;
        } else {
            return false;
        }
    }

    bool isMerkleLeaf(Node* node) const {
        assertNotNull(node);
        auto data = merkleCast(node)->getData();
        assertNotNull(data);

        // if this is a leaf, it should have no left and right children
        if(data->isLeaf()) {
            assertTrue(node->left == nullptr && node->right == nullptr);
            return true;
        } else {
            return false;
        }
    }

    bool prevalidateForestProof(Node* node, std::vector<ForestNodePtrType>& leafs) {
        logtrace << "Prevalidating node " << node->getLabel() << " (subtree of size " << node->getSize() << ")" << endl;
        // INVARIANT: 'node' is a node 'on path' to a leaf with a key-value pair or the leaf itself
        if(node == nullptr) {
            throw std::logic_error("Expected non-null node as input!");
        }

        if(isMerkleLeaf(node)) {
            leafs.push_back(merkleCast(node));
            return true;
        } else {
            bool leftRet = true, rightRet = true;
            auto data = merkleCast(node)->getData();
            assertNotNull(data)

            // Make sure there's no hash down the path to a leaf
            if(!data->merkleHash.isUnset()) {
                logerror << "Did not expect on path node to have Merkle hash set" << endl;
                return false;
            }
            
            // Ensure that the AT accumulator is set!
            if(data->acc == nullptr) {
                logerror << "Did not expect on path node to not have accumulator" << endl;
                return false;
            }
                
            // Ensure that the AT subset proof is set!
            if(!node->isRoot() && data->subsetProof == nullptr) {
                logerror << "Did not expect on path node to not have subset proof" << endl;
                return false;
            }

            bool leftMerkleSib = node->left != nullptr && isMerkleSibling(node->left.get());
            bool rightMerkleSib = node->right != nullptr && isMerkleSibling(node->right.get());
            if(leftMerkleSib && rightMerkleSib) {
                logerror << "Cannot have both children be sibling nodes" << endl;
                return false;
            }
            
            if(node->left != nullptr && !leftMerkleSib)
                leftRet = prevalidateForestProof(node->left.get(), leafs);
            if(node->right != nullptr && !rightMerkleSib)
                rightRet = prevalidateForestProof(node->right.get(), leafs);

            return leftRet && rightRet;
        }
    }

    void computeMerkleHashes(Node* node, bool hashLeaves) {
        logtrace << "Merkle hashing node " << node->getLabel() << " (subtree of size " << node->getSize() << ")" << endl;
        if(node == nullptr) {
            throw std::logic_error("Expected non-null node as input!");
        }

        auto data = dynamic_cast<MerkleDataType*>(this->merkleCast(node)->getData());
        assertNotNull(data);

        if(this->isMerkleLeaf(node)) {
            // when hashing append-only proof, leaves are 'old roots' and we have their hash already
            // when hashing membership proofs, leaves are key-value pairs and we need to hash them
            if(hashLeaves) {
                // compute and store hash from leaf's AT and empty left/right hashes!
                auto& at = *data->acc;
                assertTrue(data->merkleHash.isUnset());
                data->merkleHash = MerkleHash(at);
            }

            // TODO(Crypto): Should also check leaf is at the correct height
            // (Though, if it is not, I am not sure what attacks are possible.)
        } else {
            bool leftMerkleSib = node->left != nullptr && this->isMerkleSibling(node->left.get());
            bool rightMerkleSib = node->right != nullptr && this->isMerkleSibling(node->right.get());
            if(leftMerkleSib && rightMerkleSib) {
                throw std::logic_error("Cannot have both children be sibling nodes");
            }

            if(node->left != nullptr && !leftMerkleSib)
                computeMerkleHashes(node->left.get(), hashLeaves);
            if(node->right != nullptr && !rightMerkleSib)
                computeMerkleHashes(node->right.get(), hashLeaves);

            auto& at = *data->acc;
            auto& leftHash = this->merkleCast(node->left.get())->getData()->merkleHash;
            auto& rightHash = this->merkleCast(node->right.get())->getData()->merkleHash;
            assertTrue(!leftHash.isUnset() && !rightHash.isUnset());
            
            // hash current node (from AT and left/right hashes)
            data->merkleHash = MerkleHash(at, leftHash, rightHash);
        }

        // INVARIANT: 'node' stores a Merkle hash now
        assertFalse(data->merkleHash.isUnset());
    }

    bool verifySubsetProofs(Node* node) {
        logtrace << "Verifying subset proof at node " << node->getLabel() << " ..." << endl;
        if(node == nullptr) {
            throw std::logic_error("Expected non-null node as input!");
        }

        if(node->parent != nullptr) {
            // Check subset proofs against parent, if any.
            auto data = dynamic_cast<MerkleDataType*>(merkleCast(node)->getData());
            auto parentData = dynamic_cast<MerkleDataType*>(merkleCast(node->parent)->getData());
            assertNotNull(data);
            assertNotNull(parentData);

            assertNotNull(parentData->acc);
            assertNotNull(data->acc);
            assertNotNull(data->subsetProof);
            auto parentAcc = *parentData->acc;
            auto acc = *data->acc;
            auto proof = *data->subsetProof;
            if(ReducedPairing(parentAcc, G2::one()) != ReducedPairing(acc, proof) && !this->simulate()) {
                logerror << "Subset check failed along Merkle path" << endl;
                return false;
            }
        }

        bool leftRet = true, rightRet = true;
        
        bool leftMerkleSib = node->left != nullptr && isMerkleSibling(node->left.get());
        bool rightMerkleSib = node->right != nullptr && isMerkleSibling(node->right.get());

        if(leftMerkleSib && rightMerkleSib) {
            throw std::logic_error("Cannot have both children be sibling nodes");
        }

        if(node->left && !leftMerkleSib)
            leftRet = verifySubsetProofs(node->left.get());

        if(node->right && !rightMerkleSib)
            rightRet = verifySubsetProofs(node->right.get());

        assertTrue(node->left != nullptr || node->right != nullptr || isMerkleLeaf(node));

        return leftRet && rightRet;
    }

    int getForestProofSize(Node* node) const {
        int size = 0;

        if(node == nullptr) {
            return 0;
        } else if(isMerkleLeaf(node)) {
            // A leaf stores an append-only proof and a <k,v,i> pair but k and v are 
            // not part of the proof and the i is encoded in the Merkle path itself.
            return G2ElementSize;
        } else if(isMerkleSibling(node)) {
            // A sibling node only stores a Merkle hash of 32 bytes
            return MerkleHashSize;
        } else if(node->isRoot()) {
            // Root doesn't even store its AT accumulator because client has it in digest.
            // Root has no parent, so no append-only proof.
            size = 0; 
        } else { // if it's not a leaf, nor a sibling, nor a root, then it's an 'on path' node
            // 'On path' nodes only store accumulator and append-only proof, but they must be extractable.
            // (Their Merkle hash is computed from their children and sibling.)
            size = G1ElementSize*2 + G2ElementSize;
        }

        int l, r;
        l = getForestProofSize(node->left.get());
        r = getForestProofSize(node->right.get());
        
        return size + l + r;
    }
};

}
