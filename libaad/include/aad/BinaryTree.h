#pragma once

#include <aad/BitString.h>

#include <xassert/XAssert.h>
#include <xutils/Log.h>
#include <xutils/Timer.h>
#include <xutils/NotImplementedException.h>

#include <list>
#include <memory>
#include <stdexcept>
#include <unordered_map>

using libxutils::NotImplementedException;
using std::endl;

namespace libaad {

template<class NodeType>
class BinaryTree;

class Node {
public:
    using NodePtrType = Node*;

public:
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;
    Node* parent;

public:
    Node() : left(nullptr), right(nullptr), parent(nullptr) {}
    virtual ~Node() {
        //logdbg << "Destroying node" << endl;
    } 

public:
    void setChild(NodePtrType child, bool bit) {
        assertNotNull(child);

        if(bit)
            right.reset(child);
        else
            left.reset(child);
        
        assertTrue(child->parent == nullptr);
        child->parent = this;
    }
    
    bool hasChild(bool bit) {
        if(bit) {
            return right != nullptr;
        } else {
            return left != nullptr;
        }
    }

    /**
     * Surgically removes the child from its parent (the current node) which
     * means the caller now "owns" the child and is responsible for freeing it.
     */
    NodePtrType disownChild(bool bit) {
        if(bit) {
            return right.release();
        } else {
            return left.release();
        }
    }

    NodePtrType getChild(bool bit) {
        return bit ? right.get() : left.get();
    }

    bool hasTwoChildren() const {
        return right != nullptr && left != nullptr;
    }

    bool isRoot() const {
        return parent == nullptr;
    }
    
    bool isLeaf() const {
        return left == nullptr && right == nullptr;
    }

    int getSize() {
        return getSizeHelper(this);
    }

    NodePtrType getAncestorRoot() {
        auto root = this;
        while(root->parent != nullptr) {
            root = root->parent;
        }
        return root;
    }

    /**
     * Returns the label of this node as a BitString
     */
    BitString getLabel() const {
        BitString bs;
        auto curr = this;
        while(curr->isRoot() == false) {
            bs << curr->getBit();
            curr = curr->parent;
        }
        bs.reverse();
        return bs;
    }
    
    /**
     * Returns the height of the subtree rooted at this node.
     * Single-node tree has height 1.
     */
    int getHeight() const {
        return getHeight(this);
    }

    int getHeight(Node const * node) const {
        if(node) {
            return 1 + std::max(getHeight(node->left.get()), getHeight(node->right.get()));
        } else {
            return 0;
        }
    }

    /**
     * Returns true if it's a right child and false if it's a left child.
     */
    bool getBit() const {
        NodePtrType parentPtr = parent;
        if(parentPtr == nullptr) {
            throw std::logic_error("root node does not have a parent");
        }

        assertNotEqual(parentPtr->right.get(), parentPtr->left.get());
        return parentPtr->right.get() == this;
    }
    
    /**
     * Returns the sibling of this node, if any, or null otherwise.
     */
    NodePtrType getSibling() {
        NodePtrType parentPtr = parent;
        if(parentPtr == nullptr) {
            throw std::logic_error("root node does not have a sibling");
        }

        return parentPtr->getChild(! getBit());
    }

    void inorderTraverse(std::function<void(NodePtrType)> func) {
        inorderTraverseRecursive(this, func);
    }
    
    void preorderTraverse(std::function<void(NodePtrType)> func) {
        preorderTraverseRecursive(this, func);
    }

    void inorderTraverseRecursive(NodePtrType node, std::function<void(NodePtrType)> func) {
        if(node == nullptr)
            return;

        inorderTraverseRecursive(node->left.get(), func);
        func(node);
        inorderTraverseRecursive(node->right.get(), func);
    }
    
    void preorderTraverseRecursive(NodePtrType node, std::function<void(NodePtrType)> func) {
        if(node == nullptr)
            return;

        func(node);
        preorderTraverseRecursive(node->left.get(), func);
        preorderTraverseRecursive(node->right.get(), func);
    }

    /**
     * Checks if the specified node is in this subtree. If so, returns a pointer to it.
     * If not, returns a pointer to the node where the search ended and its label.
     */
    std::tuple<bool, NodePtrType, BitString> findNode(const BitString& bs) {
        auto curr = this;
        auto next = curr;
        bool found = true;
        BitString label;

        for(size_t i = 0; i < bs.size(); i++) {
            bool bit = bs[i];
            label << bit;
            
            if(bit) {
                next = curr->right.get();
            } else {
                next = curr->left.get(); 
            }
    
            if(next == nullptr) {
                found = false;
                break;
            }

            curr = next;
        }

        return std::make_tuple(found, curr, label);
    }

    /** 
     * Assumes destTree already has root node inserted.
     * The first arg of copierFunc is the source, the second is the destination
     */
    template<class SrcNodeType, class DestNodeType>
    void copyPathToRoot(BinaryTree<DestNodeType>* destTree,
        std::function<void(SrcNodeType*, DestNodeType*, bool)> copierFunc)
    {
        copyPathToRootHelper<SrcNodeType, DestNodeType>(
            getLabel(), 
            dynamic_cast<SrcNodeType*>(getAncestorRoot()), 
            destTree, 
            copierFunc
        );
    }

public:
    template<class SrcNodeType, class DestNodeType>
    static void copyPathToRootHelper(
        BitString label,
        SrcNodeType* root, 
        BinaryTree<DestNodeType>* destTree,
        std::function<void(SrcNodeType*, DestNodeType*, bool)> copierFunc)
    {
        assertNotNull(root);
        assertNotNull(destTree);
        auto curr = root;
        auto currDest = destTree->getRoot();
        assertNotNull(currDest);

        // Copy root
        copierFunc(curr, currDest, false);

        // Walk down path, and copy node and sibling node data into destTree
        BitString currLabel;
        if(label.size() == 0) {
            logtrace << "No path to walk down on. Just the root." << endl;
        }

        for(size_t i = 0; i < label.size(); i++) {
            bool bit = label[i];
            currLabel << bit;
            logtrace << "Walking down path '" << label << "', currently at " << currLabel << endl;
            auto child =   dynamic_cast<SrcNodeType*>(curr->getChild(bit));
            auto sibling = dynamic_cast<SrcNodeType*>(curr->getChild(!bit));

            // Our forest trees are always complete => nodes always have siblings
            // Our frontier trees are also always complete (because of how we built them)
            assertNotNull(child);
            assertNotNull(sibling);
           
            // Create nodes in the destination tree, if not there from a previous copyPathToRoot
            for(bool bit : { true, false }) {
                if(currDest->hasChild(bit) == false) {
                    currDest->setChild(new DestNodeType(), bit);
                }
            }

            // Get pointers to destination nodes (data might be in them from previous copyPathToRoot)
            auto childDest =    dynamic_cast<DestNodeType*>(currDest->getChild(bit));
            auto childSibDest = dynamic_cast<DestNodeType*>(currDest->getChild(!bit));

            // WARNING: Our frontier proof code assumes the 'on path' call is made first and the sibling second.

            // "On path" node data always overwrites data in destination tree, which
            // could be data from either a previous "on path" node or a sibling node 
            // (from a previous copyPathToRoot)
            copierFunc(child, childDest, false);

            // Sibling node data should NOT overwrite "on path" node data, if any
            // from a previous copyPathToRoot
            copierFunc(sibling, childSibDest, true);

            // Move down along the path (both in the source path and in the destination tree)
            curr = child;
            currDest = childDest;
        }

        assertEqual(currLabel, label);
    }

    static int getSizeHelper(NodePtrType root) {
        if(root == nullptr)
            return 0;
        return 1 + getSizeHelper(root->left.get()) + getSizeHelper(root->right.get());
    }
};

template<class T>
class DataNode : public Node {
public:
    std::unique_ptr<T> data;

public:
    DataNode()
        : data(nullptr)
    {}

    DataNode(T* data)
        : Node(), data(data)
    {}
    
    virtual ~DataNode() {
    }

public:
    bool hasData() const { return data != nullptr; }
    T* getData() { return data.get(); }
    void setData(T* d) { data.reset(d); }

};

class NodeFactory {
public:
    using NodePtrType = Node*;

public:
    static void setPointers(NodePtrType parent, NodePtrType left, NodePtrType right) {
        parent->left.reset(left);
        parent->right.reset(right);

        // assert parents are unset
        assertTrue(right == nullptr || right->parent == nullptr);
        assertTrue(left == nullptr || left->parent == nullptr);

        if(left != nullptr)
            left->parent = parent;
        if(right != nullptr)
            right->parent = parent;
    }
    
    static NodePtrType makeNode(NodePtrType left = nullptr, NodePtrType right = nullptr) {
        auto node = new Node();
        setPointers(node, left, right);
        return node;
    }

    template<class DataType>
    static DataNode<DataType>* makeNode(DataType* data, 
        NodePtrType left = nullptr, NodePtrType right = nullptr)
    {
        auto node = new DataNode<DataType>(data);
        setPointers(node, left, right);
        return node;
    }
};

/**
 * An AT will be a BinaryTree
 *  + should be able to create one given a list of BitStrings 
 *  + should be able to get an std::vector<BitString> of frontier nodes for some <k,V={}> or some <k>
 * Forest subset paths will be BinaryTree's, one for each root AT
 *  + should be able to append a leaf
 *  + should be able to walk them down for verification 
 *  + for AAD w/ low BW, we only send nodes along subset paths and infer sibling nodes 
 *  + (sometimes there won't be a sibling when two subset paths intersect)
 * Frontier proofs will be a BinaryTree, one for each root AT
 *  + should be able to walk them down for verification
 *  + should "raise up" the rightmost leaves
 *     - can append leaves, just like in forest, and then have a final round that hashes all roots up
 *
 * Seems like all trees have an append() call
 *  - for forests, appends a leaf to the right and merges
 *  - for frontier proofs, appends a leaf to the right
 *  - for ATs, appends a new path
 */
template<class NodeType>
class BinaryTree {
public:
    using BinaryTreeType = BinaryTree<NodeType>;
    using BinaryTreePtrType = BinaryTreeType;
    using NodePtrType = NodeType*;

protected:
    std::unique_ptr<NodeType> root;

public:
    BinaryTree() : root(nullptr) {}
    BinaryTree(NodePtrType root) : root(root) {}
    BinaryTree(BinaryTreeType&& moved) : root(std::move(moved.root)) {}
    virtual ~BinaryTree() {
        //logdbg << "Destroying binary tree" << endl;
    }

public:
    NodePtrType getRoot() { return root.get(); }
    void setRoot(NodePtrType r) { root.reset(r); }
};

template<class T, class MergeFunc>
class BinaryForest {
public:
    using NodeType = DataNode<T>;
    using NodePtrType = NodeType*;

protected:
    // WARNING: The nodes are NOT owned by BinaryForest
    std::list<std::tuple<int, NodePtrType>> trees;  // <size, rootNode>, biggest tree first
    int count;      // leaf count
    MergeFunc* mergeFunc;

public:
    BinaryForest() 
        : count(0)
    {}

    virtual ~BinaryForest() {
        // WARNING: This class does NOT delete 'trees' but relies on its subclasses (or users) to do so
        // e.g., IndexedForest deletes trees and Frontier deletes them as well by deleting i
        // the merged BinaryTree obtained after calling mergeAllRoots(false)
        //logdbg << "Destroying binary forest" << endl;
    }

public:
    void setMergeFunc(MergeFunc *f) { mergeFunc = f; }
    //const MergeFunc& getMergeFunc() const { return *mergeFunc; }

    /**
     * Appends a leaf to the forest and returns its index. Triggers merges in the forest
     * if the new leaf has a left sibling who doesn't have a parent. In this case,
     * this results in a new tree of height two, which could also have a sibling tree
     * of the same height. Merges continue until all trees in the forest have different heights.
     */
    int appendLeaf(NodePtrType leaf) {
        count++;

        // Push new tree of size 1
        trees.push_back(
            std::make_tuple(
                1, 
                leaf));

        maybeTriggerMerges(true);

        return count - 1;
    }
    
    /** 
     * Returns the number of leaves in the forest.
     */
    int getCount() const { return count; }

    /**
     * Take each two roots in tree and create a parent for them. Proceed recursively for parents.
     */
    NodePtrType mergeAllRoots() {
        return maybeTriggerMerges(false);
    }
    
    std::vector<NodePtrType> getRoots() {
        std::vector<NodePtrType> roots; 
        for(auto tree : trees) {
            roots.push_back(std::get<1>(tree));
        }
        return roots;
    }

    /**
     * Returns the root nodes of the forest at the specified version (i.e., number of appends)
     */
    std::vector<NodePtrType> getOldRoots(int version) const {
        if(version == 0)
            throw std::logic_error("Version 0 has no roots");

        if(version < 0)
            throw std::logic_error("Version must be positive");
       
        std::vector<NodePtrType> roots; // array of roots in old forest at version 'version'
        int left = version;             // we iterate through the left-most leaves of the subtrees in the old forest
        int leafNo = 0;                 // the current leaf, whose old root we will fetch

        while(left > 0) {
            NodePtrType leaf, root;
            std::tie(std::ignore, leaf) = getTreeAndLeaf(leafNo);

            int size = Utils::greatestPowerOfTwoBelow(left);
            assertIsPowerOfTwo(size);
            leafNo += size; // leafNo of next iteration
            left -= size;   // # of leaves left for next iteration

            root = leaf;
            int levels = Utils::log2floor(size);
            while(levels > 0) {
                root = dynamic_cast<NodePtrType>(root->parent);
                assertNotNull(root);
                levels--;
            }

            roots.push_back(root);
        }
    
        return roots;
    }
    
    /**
     * 'leafIndex' is 0-based
     */
    std::tuple<NodePtrType, NodePtrType> getTreeAndLeaf(int leafIndex) const {
        assertStrictlyLessThan(leafIndex, count);
        assertIsPositive(leafIndex);

        // Find the tree for the specified leaf
        for(auto tup : trees) {
            // The size of the current tree
            int size = std::get<0>(tup);
            assertIsPowerOfTwo(size);

            if(leafIndex < size) {
                BitString leafLabel(leafIndex, Utils::numBits(size) - 1);
                auto root = std::get<1>(tup);
                auto result = root->findNode(leafLabel);
                assertTrue(std::get<0>(result));    // unless we did something wrong, this should give us the leaf
                assertNotNull(std::get<1>(result));

                auto leaf = dynamic_cast<NodePtrType>(std::get<1>(result));
                assertNotNull(leaf);

                return std::make_tuple(root, leaf);
            }

            leafIndex -= size;
        }

        // Return null if found nothing
        return std::make_tuple(static_cast<NodePtrType>(nullptr), static_cast<NodePtrType>(nullptr));
    }

protected:
    NodePtrType maybeTriggerMerges(bool equalSizedOnly) {
        // Nothing to merge
        if(trees.size() == 0)
            return nullptr;     

        // If we have at least two trees in the forest, (maybe) merge!
        if(trees.size() >= 2) {
            int secondToLastSize = std::get<0>(*std::prev(trees.end(), 2));
            int lastSize = std::get<0>(trees.back());
            return maybeTriggerMergesHelper(secondToLastSize, lastSize, equalSizedOnly);
        } else {
            // Returns the only tree in the forest
            return std::get<1>(trees.back());
        }
    }

    NodePtrType maybeTriggerMergesHelper(int secondToLastSize, int lastSize, bool equalSizedOnly) {
        //int i = 1;
        //for(auto& sizeAndTree : trees) {
        //    logdbg << "Tree " << i++ << ": size " << std::get<0>(sizeAndTree) << endl;
        //}

        // Note: We use this to merge equal-sized trees in the forest recursively 
        // (for the AT forest) but we also use it to merge all trees into a single tree 
        // (for the frontier accumulator) and return this tree's root
        if(secondToLastSize == lastSize || equalSizedOnly == false) {
            bool isLastMerge = true;
            int newLastSize = secondToLastSize + lastSize;

            // Get last two trees
            auto lastRoot = std::get<1>(trees.back());
            trees.pop_back();
            auto secondToLastRoot = std::get<1>(trees.back());
            trees.pop_back();
        
            // Roots of trees should never be null nor equal
            assertNotNull(secondToLastRoot);
            assertNotNull(lastRoot);
            assertNotEqual(secondToLastRoot, lastRoot);

            // Pre-determine if there will be another merge after this one by looking
            // at the size of the 3rd to last tree. If it equals the size of the merged
            // tree, then another merge will follow.
            int new2ndToLastSize = 0;
            if(!trees.empty()) {
                new2ndToLastSize = std::get<0>(trees.back());
                if(new2ndToLastSize == newLastSize)
                    isLastMerge = false;
            }

            // Call the merge function
            //logdbg << "Merging second-to-last size " << secondToLastSize << " with last size " << lastSize << endl;
            T* parentData;
            //ManualTimer t;
            assertNotNull(mergeFunc);
            parentData = (*mergeFunc)(secondToLastRoot, lastRoot, isLastMerge);
            //std::chrono::milliseconds mus = std::chrono::duration_cast<std::chrono::milliseconds>(t.stop());
            //if(mus.count() > 100) {
            //    logperf << "Merging " << secondToLastSize << " with " << lastSize << " took " << mus.count() << " ms" << endl;
            //    logperf << endl;
            //}

            // Create a new tree whose root has the old tree's roots as its children
            auto mergedTree = NodeFactory::makeNode(
                    parentData, 
                    secondToLastRoot, 
                    lastRoot);

            // Add this tree to our forest, and continue recursively
            //logdbg << "Pushing new tree of size " << newLastSize << endl;
            trees.push_back(
                std::make_tuple(
                    newLastSize,
                    mergedTree));
             
            // If there's just one tree in the forest, we are done.
            if(trees.size() == 1)
                return mergedTree;
            else
                return maybeTriggerMergesHelper(new2ndToLastSize, newLastSize, equalSizedOnly);
        } else {
            // NOTE: We don't care about this execution path since we set equalSizedOnly to false
            // when calling mergeAllRoots(). So we just return the last tree in the forest.
            return std::get<1>(trees.back());
        }
    }
};

}
