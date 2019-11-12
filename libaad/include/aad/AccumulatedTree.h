#pragma once

#include <aad/BinaryTree.h>

#include <tuple>
#include <vector>
#include <map>
#include <boost/container/flat_map.hpp>
#include <boost/container/map.hpp>
#include <boost/unordered_map.hpp>

#include <xutils/Log.h>
#include <xutils/NotImplementedException.h>

using libxutils::NotImplementedException;
using std::endl;

namespace libaad {

class AccumulatedTree {
public:
    using NodePtrType = Node*;
    using BinaryTreeType = BinaryTree<Node>;
    using BinaryTreePtrType = BinaryTreeType*;
    using AccumulatedTreeType = AccumulatedTree;
    using AccumulatedTreePtrType = AccumulatedTreeType*;

protected:
    std::unique_ptr<BinaryTreeType> tree;
    int maxDepth;   // the maximum depth of the AT (and minimum too actually, since ATs are fixed depth)

public:
    AccumulatedTree(int maxDepth)
        : tree(new BinaryTreeType()), maxDepth(maxDepth)
    {}

    /**
     * Adds a node to the accumulated tree for every prefix of the hash
     */
    AccumulatedTree(int maxDepth, const BitString& hash)
        : AccumulatedTree(maxDepth)
    {
        appendPath(hash);
    }
    
    /**
     * Merges the two ATs into this AT.
     */
    AccumulatedTree(std::unique_ptr<AccumulatedTreeType> left, std::unique_ptr<AccumulatedTreeType> right)
        : tree(std::move(left->tree)), maxDepth(left->maxDepth)
    {
        assertNull(left->tree);
        assertEqual(left->maxDepth, right->maxDepth);

        mergeBinaryTrees(right->tree.get());
    }

    ~AccumulatedTree() {
        //logdbg << "Destroying AT" << endl;
    }

public:
    int getSize() {
        return tree->getRoot()->getSize();
    }

    /**
     * Appends a path of nodes, as specified by the label of the bottom-most node in the path.
     * For example, if path = [ 0 1 1 ], appends a root \varepsilon, its left child 0,
     * 0's right child 01 and 01's right child 011.
     */
    void appendPath(const BitString& lastNode) {
        assertNotNull(tree);
        if(tree->getRoot() == nullptr)
            tree->setRoot(NodeFactory::makeNode());

        auto parent = tree->getRoot();
        assertNotNull(parent);

        for(size_t i = 0; i < lastNode.size(); i++) {
            bool bit = lastNode[i];
            if(bit) {
                // When calling appendPath twice, some nodes might already be inserted
                if(parent->right == nullptr) {
                    parent->right.reset(NodeFactory::makeNode());
                    parent->right->parent = parent;
                }
                parent = parent->right.get();
            } else {
                // When calling appendPath twice, some nodes might already be inserted
                if(parent->left == nullptr) {
                    parent->left.reset(NodeFactory::makeNode());
                    parent->left->parent = parent;
                }
                parent = parent->left.get();
            }
        }
    }

    /**
     * Returns all prefixes appended to this AT. Need them to generate characteristic
     * polynomial for AT and later commit to it.
     */
    std::vector<BitString> getPrefixes(int sizeHint = 512) const {
        std::vector<BitString> prefixes;
        prefixes.reserve(static_cast<size_t>(sizeHint));
        BitString label = BitString::empty();
        assertNotNull(tree->getRoot());
        getPrefixesHelper(tree->getRoot(), label, prefixes);
        return prefixes;
    }

    void getPrefixesHelper(NodePtrType root, const BitString& label, std::vector<BitString>& prefixes) const {
        if(root != nullptr) {
            prefixes.push_back(label);

            BitString left = label, right = label;
            left << 0;
            right << 1;
            
            getPrefixesHelper(root->left.get(), left, prefixes);
            getPrefixesHelper(root->right.get(), right, prefixes);
        }
    }
 
    /**
     * Given the hash of a key, returns true if the key is in the tree and false
     * otherwise. If the key is not in, returns the first prefix of the hash of 
     * the key that's not in the tree.
     *
     * NOTE: Need this when proving non-membership of a key!
     */
    std::tuple<bool, NodePtrType, BitString> containsKey(const BitString& hashOfKey) const {
        assertNotNull(tree->getRoot());
        bool found;
        NodePtrType nodePtr;
        BitString prefix;
        std::tie(found, nodePtr, prefix) = tree->getRoot()->findNode(hashOfKey);
        return std::make_tuple(found, nodePtr, prefix);
    }

    /**
     * Returns the set of frontier nodes (i.e., prefixes) corresponding to this AT.
     */
    void getFullFrontier(std::vector<BitString>& frontier) const {
        std::vector<NodePtrType> lowerRoots;    // dummy, won't be filled with anything
        frontier.clear();
        getFrontierHelper(tree->getRoot(), BitString::empty(), frontier, lowerRoots, maxDepth, false);
    }

    /**
     * Returns the set of frontier nodes in the upper half of the AT tree and pointers
     * to the roots of the lower AT subtrees, which are later used to obtain
     * lower frontier nodes.
     */
    void getUpperFrontier(std::vector<BitString>& frontier) {
        std::vector<NodePtrType> dummy;
        frontier.clear();
        getFrontierHelper(tree->getRoot(), BitString::empty(), frontier, dummy, maxDepth / 2, false);
    }

    void getUpperFrontier(std::vector<BitString>& frontier, std::vector<NodePtrType>& lowerRoots) {
        assertTrue(maxDepth % 2 == 0);
        frontier.clear();
        lowerRoots.clear();
        getFrontierHelper(tree->getRoot(), BitString::empty(), frontier, lowerRoots, maxDepth / 2, true); 
    }

    /**
     * Given a key, returns the lower frontier nodes in this AT associated with all values of that key.
     * (Lower frontier nodes are used to prove complete membership of all values of a key.)
     *
     * NOTE: Need this when proving completeness of membership for a specific key's values in this AT.
     */
    void getLowerFrontier(std::vector<BitString>& frontier, const BitString& hashOfKey) {
        bool found;
        NodePtrType nodePtr;
        std::tie(found, nodePtr, std::ignore) = tree->getRoot()->findNode(hashOfKey);
        assertTrue(found);
        
        getLowerFrontier(frontier, hashOfKey, nodePtr);
    }

    /**
     * Returns the lower frontier associated with all the values of a given key.
     */
    void getLowerFrontier(std::vector<BitString>& frontier, BitString nodeLabel, NodePtrType lowerRoot) const { 
        std::vector<NodePtrType> lowerRoots;    // dummy, won't be filled with anything
        getFrontierHelper(lowerRoot, nodeLabel, frontier, lowerRoots, maxDepth - static_cast<int>(nodeLabel.size()), false);
    }

public:
    /**
     * We use this to merge ATs and to merge proof trees in the AT forest (and maybe the frontier too).
     * WARNING: This restiches part of tree 'b' into tree 'a', returning the modified 'a'. The caller
     * will then free 'a'.
     */
    void mergeBinaryTrees(BinaryTreePtrType src)
    {
        auto rootA = tree->getRoot();
        auto rootB = src->getRoot();
        assertNotNull(rootA);
        assertNotNull(rootB);

        mergeTreesHelper(rootA, rootB);
    }

protected:
    static void getFrontierHelper(NodePtrType node, const BitString& nodeLabel, 
        std::vector<BitString>& frontier, std::vector<NodePtrType>& lowerRoots, int levelsLeft, bool includeLowerRoots) 
    {
        // labels of left and right children of 'node'
        BitString leftLabel(nodeLabel), rightLabel(nodeLabel);
        leftLabel << 0;
        rightLabel << 1;

        /**
         * Recurse down AT tree as long as we didn't reach max depth.
         * Invariant: node is in AT but not all children might be, so add them 
         * to frontier if they are not.
         */
        if(node->left != nullptr) {
            getFrontierHelper(node->left.get(), leftLabel, frontier, lowerRoots, levelsLeft - 1, includeLowerRoots);
        } else {
            // Do not add nodes below max depth!
            if(levelsLeft > 0) {
                frontier.push_back(leftLabel);
            }
        }

        if(node->right != nullptr) {
            getFrontierHelper(node->right.get(), rightLabel, frontier, lowerRoots, levelsLeft - 1, includeLowerRoots);
        } else {
            // Do not add nodes below max depth!
            if(levelsLeft > 0) {
                frontier.push_back(rightLabel);
            }
        }

        // If we reached the bottom of the tree, add this node as a 'lower root'
        // so we can pass it to getLowerFrontier()
        if(includeLowerRoots && levelsLeft == 0) {
            lowerRoots.push_back(node);
        }
    }

    /**
     * We use this to merge two ATs together.
     */
    static void mergeTreesHelper(NodePtrType dest, NodePtrType src) {
        assertTrue(dest != nullptr && src != nullptr);

        for(bool childIdx : { true, false }) {
            NodePtrType destChild = dest->getChild(childIdx);
            NodePtrType srcChild = src->getChild(childIdx);

            // When the destination node has no child where the source node does, we
            // "restitch" the source node's child as the destination node's.
            if(destChild == nullptr && srcChild != nullptr) {
                srcChild->parent = nullptr;    // cut off the parent
                dest->setChild(srcChild, childIdx);
                src->disownChild(childIdx); // make sure the source child won't be double freed now!
            }

            if(destChild != nullptr && srcChild != nullptr) {
                mergeTreesHelper(destChild, srcChild);
            }
        }
    }
};

template<class LookupT, class DataType, class MergeFunc>
class IndexedForest : public BinaryForest<DataType, MergeFunc> {
public:
    using DataPtrType = DataType*;

    using NodeType = DataNode<DataType>;
    using NodePtrType = NodeType*;

    using BinaryTreeType = BinaryTree<NodeType>;
    using BinaryTreePtrType = BinaryTreeType*;
    using BinaryForestType = BinaryForest<DataType, MergeFunc>;

protected:
    /**
     * Maps each element to a vector of the leaves in the forest containing it (via an element-derived lookup key).
     * WARNING: The vector stores the leaves in the order they were appended!
     * (AAD::getValues() depends on this because it returns the list of values in the order they were inserted.)
     */
    boost::unordered_map<LookupT, std::vector<NodePtrType>> keyToLeaves;

public:
    IndexedForest()
        : BinaryForestType()
    {}

    virtual ~IndexedForest() {
        // delete binary trees from parent class
        //logdbg << "Destroying accumulated forest" << endl;
        for(auto tup : this->trees) {
            delete std::get<1>(tup);
        }
    }

public:
    void appendLeaf(DataPtrType data, const LookupT& lookupKey) {
        auto leafPtr = NodeFactory::makeNode(data);
        keyToLeaves[lookupKey].push_back(leafPtr);
        BinaryForestType::appendLeaf(leafPtr);
    }
    
    /**
     * Returns paths to all leaves with the specified key.
     */
    template<class MerkleNodeType>
    std::vector<BinaryTree<MerkleNodeType>*>* partialMembershipProof(
        const LookupT& key, 
        std::function<void(NodePtrType, MerkleNodeType*, bool)> copierFunc) const
    {
        std::vector<NodePtrType> leaves;
        if(keyToLeaves.count(key) > 0) {
            leaves = keyToLeaves.at(key);
        }

        return copyMerklePaths(leaves, copierFunc);
    }
    
    template<class MerkleNodeType>
    std::vector<BinaryTree<MerkleNodeType>*>* copyMerklePaths(
        const std::vector<NodePtrType>& nodes,
        std::function<void(NodePtrType, MerkleNodeType*, bool)> copierFunc) const
    {
        auto proof = new std::vector<BinaryTree<MerkleNodeType>*>();

        // Our proof maps each index i to a subtree of the ith tree in the forest.
        // This subtree contains paths to all relevant key-value pairs from the ith tree.
        // Some indices i will be unmapped because there are no keys in those trees.
        boost::unordered_map<NodePtrType, size_t> rootToProofIdx;
        size_t i = 0;
        for(auto tup : this->trees) {
            auto root = std::get<1>(tup);
            proof->push_back(nullptr);      // initially, assume the key has no values in the forest
            rootToProofIdx[root] = i++;
        }

        BinaryTree<MerkleNodeType>* proofTree = nullptr;

        // WARNING: For membership proofs, assuming the leaves were added to the vector in the order they were appended!
        // (Eh, I don't recall where this assumption is needed. In copierFunc, which assumes we're only adding nodes "to the right"?)
        for(auto node : nodes) {
            assertNotNull(node);

            // current leaf might be in the next tree in the forest, so we need to 
            // create a new binary tree for its proof path
            auto currRoot = dynamic_cast<NodePtrType>(node->getAncestorRoot());
            assertNotNull(currRoot);
            assertTrue(rootToProofIdx.count(currRoot) > 0);

            size_t idx = rootToProofIdx[currRoot];
            assertInclusiveRange(0, idx, proof->size() - 1);
            if((*proof)[idx] == nullptr) {
                proofTree = new BinaryTree<MerkleNodeType>(new MerkleNodeType());
                (*proof)[idx] = proofTree;
            } else {
                proofTree = (*proof)[idx];
            }

            node->copyPathToRoot(proofTree, copierFunc);
            //logtrace << "Merkle proof size: " << proofTree->getRoot()->getSize() << endl;
        }

        return proof;
    }
    
    const std::list<std::tuple<int, NodePtrType>>& getTrees() const {
        return BinaryForestType::trees;
    }

    int getNumTrees() const {
        return static_cast<int>(BinaryForestType::trees.size());
    }

    int getOccurrenceCount(const LookupT& key) const {
        if(keyToLeaves.count(key) > 0) {
            return keyToLeaves[key].size();
        } else {
            return 0;
        }
    }

    std::vector<NodePtrType> getLeaves(const LookupT& key) const {
        if(keyToLeaves.count(key) > 0) {
            return keyToLeaves.at(key);
        } else {
            return std::vector<NodePtrType>();
        }
    }
    
    template<class Container>
    void getLookupKeys(Container& c) const {
        for(auto& kv : keyToLeaves) {
            c.push_back(kv.first);
        }
    }
};

}
