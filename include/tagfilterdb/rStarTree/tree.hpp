#ifndef TAGFILTERDB_R_STAR_TREE_HPP_
#define TAGFILTERDB_R_STAR_TREE_HPP_

#include "tagfilterdb/rStarTree/box.hpp"
#include "tagfilterdb/export.hpp"
#include "tagfilterdb/status.hpp"

#include <vector>
#include <cassert>

// TODO: Remove a node
// TODO: Search a range
// TODO: Search a certain value
// TODO: Search a neighbor
// TODO: Search all paths (for checking all tags)
// TODO: Display breadth-first search
// TODO: Balance the tree after insertions and deletions
// TODO: Implement node splitting logic when maximum capacity is exceeded
// TODO: Optimize insertions to minimize overlap and area enlargement
// TODO: Reinsert nodes during splits to optimize tree structure
// TODO: Optimize search performance for high-dimensional data
// TODO: Implement bulk loading of data into the R* tree
// TODO: Extend support for non-rectangular shapes (if applicable)
// TODO: Implement persistent storage for saving and loading the tree from disk
// TODO: Add concurrency control for thread-safe operations
// TODO: Visualize the R* tree structure for debugging and analysis purposes
// TODO: Implement an iterator for traversing the R* tree
// TODO: Implement a const iterator for read-only access

namespace tagfilterdb
{
#define RSTARTREE_TEMPLATE template <class DATATYPE, class RANGETYPE, int DIMS, int MAXCHILD, int MINCHILD>
#define RSTARTREE_QUAL RStarTree<DATATYPE, ELEMTYPE, NUMDIMS, ELEMTYPEREAL, TMAXNODES, TMINNODES>

    template <class DATATYPE, class RANGETYPE, int DIMS, int MAXCHILD = 8, int MINCHILD = MAXCHILD / 2>
    class TAGFILTERDB_EXPORT RStarTree
    {
        using BBox = BROUNDINGBOX_QUAL;

    private:
        // Base class for all nodes in the R*-Tree
        class Node
        {
        public:
            BBox bound;
            virtual ~Node() = default;
        };

        // Leaf node class containing the bounding box and the actual data entry
        class Leaf : public Node
        {
        public:
            DATATYPE leaf;

            Leaf(const BBox &boundingBox, const DATATYPE &data)
            {
                this->bound = boundingBox;
                this->leaf = data;
            }
        };

        class InterNode : public Node
        {
        public:
            std::vector<BoundingBox> bounds; // Bounding boxes of child nodes
            std::vector<Node *> children;    // Pointers to child nodes
            bool hasLeaves;                  // Indicates if this node contains leaves

            InterNode() : hasLeaves(false) {}

            void addChild(Node *child)
            {
                children.push_back(child);
                bounds.push_back(child->bound);
                this->bound = this->bound.unionBox(child->bound);
            }

            ~InterNode()
            {
                for (auto child : children)
                {
                    delete child;
                }
            }
        };

    public:
        // Default constructor
        RStarTree() : root_(nullptr), size_(0)
        {
            assert(1 <= min_child && min_child <= max_child / 2);
        }

        // Method to insert a bounding box and associated data into the R*-Tree
        Status insert(const BoundingBox &bound, const LeafType &data)
        {
            Leaf *newLeaf = new Leaf(bound, data);

            if (!root_)
            {
                InterNode *newRoot = new InterNode();
                newRoot->hasLeaves = true;
                newRoot->addChild(newLeaf);
                root_ = newRoot;
            }
            else
            {
                insertInternal(newLeaf, root_);
            }

            size_++;
            return Status::OK();
        }

    private:
        // Recursive insert helper function
        void insertInternal(Leaf *leaf, InterNode *node)
        {
            node->bound = node->bound.unionBox(leaf->bound);

            if (node->hasLeaves)
            {
                node->addChild(leaf);
            }
            else
            {
                InterNode *childNode = properSubtree(node, leaf->bound);
                insertInternal(leaf, childNode);
            }
            if (node->children.size() > max_child)
            {
                // splitNode
            }
        }

        // Choose the best subtree for insertion
        InterNode *properSubtree(InterNode *node, const BoundingBox &bound)
        {
            // Select the child whose bounding box requires the least enlargement
            InterNode *bestNode = nullptr;
            double minEnlargement = std::numeric_limits<double>::max();

            for (size_t i = 0; i < node->children.size(); ++i)
            {
                BoundingBox tempBound = node->bounds[i].unionBox(bound);
                double enlargement = tempBound.area() - node->bounds[i].area();

                if (enlargement < minEnlargement)
                {
                    minEnlargement = enlargement;
                    bestNode = dynamic_cast<InterNode *>(node->children[i]);
                }
            }

            return bestNode;
        }

    private:
        InterNode *root_;
        std::size_t size_;
    };
}

#endif // TAGFILTERDB_R_STAR_TREE_HPP_
