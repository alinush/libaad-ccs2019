#pragma once

#include <aad/AADProof.h>

#include <xutils/NotImplementedException.h>

namespace libaad {

template<
    class MerkleDataType
>
class AppendOnlyProof : public AADProof<MerkleDataType> {
protected:
    using AADProofType = AADProof<MerkleDataType>;
    using ForestNodePtrType = typename AADProofType::ForestNodePtrType;
    
public:
    AppendOnlyProof(PublicParameters *pp)
        : AADProofType(pp)
    {
    }

public:
    bool verify(const Digest& oldDigest, const Digest& newDigest) {
        size_t oldidx = 0;
        assertEqual(this->forestProofs->size(), newDigest.size());
        std::vector<ForestNodePtrType> oldRoots;

        for(size_t i = 0; i < this->forestProofs->size(); i++) {
            logtrace << "Verifying forest tree #" << i << endl;

            auto merkleSubtree = (*this->forestProofs)[i];

            // some trees in the forest are not part of the proof because they are 'new' trees which contain no 'old roots'
            if(merkleSubtree == nullptr) {
                continue;
            }

            auto root = merkleSubtree->getRoot();
            assertNotNull(root);
            auto rootData = root->getData();
            assertNotNull(rootData);

            // If the old root is also a new root, we only check that the new root has the same digest 
            // (this only happens for the biggest tree in the forest, which is always the 1st tree)
            if(rootData->isOldRoot()) {
                assertIsZero(oldidx);
                // remove the frontier accumulator from the new digest, since the old root will not have it
                if(oldDigest[0] != newDigest[0]) {
                    logerror << "New digest has different 1st tree" << endl;
                    return false;
                } else {
                    // Mark old root as validated and move on to next subtree in proof
                    oldidx++;
                    continue;
                }
            } else {
                // Copy (new) root AT accumulator into proof
                assertNull(rootData->acc);
                rootData->acc.reset(new G1(std::get<0>(newDigest[i])));
            }

            // Pass 1: get pointers to all old roots in the proof
            oldRoots.clear();
            if(!this->prevalidateForestProof(root, oldRoots))
                return false;

            // Go through old roots in current proof subtree and update them with their AT accumulator and Merkle hash
            for(auto oldRoot : oldRoots) {
                auto data = dynamic_cast<MerkleDataType*>(oldRoot->getData());
                assertNotNull(data);
                assertNull(data->acc);
                assertNotNull(data->subsetProof);
                assertTrue(data->merkleHash.isUnset());

                data->acc.reset(new G1(std::get<0>(oldDigest[oldidx])));
                data->merkleHash = std::get<2>(oldDigest[oldidx]);
                oldidx++;   // marks this root as ready for validation
            }

            // Pass 2: recursively compute the root Merkle hash
            this->computeMerkleHashes(root, false);

            // Next, check root Merkle hash matches the one in digest
            auto& expectedHash = std::get<2>(newDigest[i]);
            if(root->getData()->merkleHash != expectedHash && !this->simulate()) {
                logerror << "Merkle root did not match" << endl;
                return false;
            }

            // Pass 3: recursively check subset proofs
            if(!this->verifySubsetProofs(root))
                return false;

            // Done checking forest paths!
        }

        return oldidx == oldDigest.size();
    }
};

}
