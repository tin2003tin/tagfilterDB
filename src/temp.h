/**
 * @file spatialIndex.h
 * @brief N-dimensional RTree implementation in C++
 * 
 * This implementation is inspired by the N-dimensional RTree implementation from
 * the `nushoin/RTree` repository. The original implementation can be found at:
 * https://github.com/nushoin/RTree
 * 
 * Credit: RTree implementation by nushoin.
 * 
 * @note This code is based on the original work in the `nushoin/RTree` repository.
 * 
 * @license MIT License
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * provided to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef TAGFILTERDB_SPATIAL_INDEX_H
#define TAGFILTERDB_SPATIAL_INDEX_H

#include "broundingbox.h"
#include "arena.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <limits>
#include <stddef.h>

namespace tagfilterdb {

/**
 * @struct SpatialIndexOptions
 * @brief Configuration options for the spatial index.
 *
 * This struct provides default values and types for various parameters used
 * in the `SpatialIndex`. It includes default maximum and minimum number of
 * children per node, as well as default types for range and area calculations.
 */
struct SpatialIndexOptions {
    static const int DEFAULT_MAX_CHILD =
        4; ///< Default maximum number of children per node.
    static const int DEFAULT_MIN_CHILD = DEFAULT_MAX_CHILD / 2; ///< Default minimum number of children per node.
    using DEFAULT_RANGETYPE = double; ///< Default type for range values.
    using DEFAULT_AREATYPE = double;  ///< Default type for area calculations.
};

/**
 * @class ISIndexCallback
 * @brief Interface for callback functions used in spatial index operations.
 *
 * This class is used to define callbacks that can process the results of
 * spatial index operations such as searches. It is a template class where
 * `SIndex` is the spatial index type.
 *
 * @tparam SIndex The spatial index type using this callback.
 */
template <class SIndex> class ISIndexCallback {
  public:
    using CallBackValue =
        typename SIndex::CallBackValue; ///< Type of value processed by the
                                        ///< callback.

    /**
     * @brief Process a value returned by the spatial index.
     * @param value The value to be processed.
     * @return True if processing should continue, false otherwise.
     */
    virtual bool process(const CallBackValue &value) = 0;
};

/**
 * @class SpatialIndex
 * @brief A spatial index for querying and inserting multidimensional data.
 *
 * This class represents a spatial index structure that supports efficient
 * querying and insertion of multidimensional data. It can handle varying
 * numbers of dimensions and provides functionality for managing nodes
 * and sub-nodes.
 *
 * @tparam DataType The type of data stored in the index.
 * @tparam Dimensions The number of dimensions in the spatial index.
 * @tparam MaxChildren Maximum number of children per node.
 * @tparam MinChildren Minimum number of children per node.
 * @tparam RangeType Type used for range calculations.
 * @tparam AreaType Type used for area calculations.
 */
template <class DataType, std::size_t Dimensions,
          std::size_t MaxChildren = SpatialIndexOptions::DEFAULT_MAX_CHILD,
          std::size_t MinChildren = SpatialIndexOptions::DEFAULT_MIN_CHILD,
          class RangeType = typename SpatialIndexOptions::DEFAULT_RANGETYPE,
          class AreaType = typename SpatialIndexOptions::DEFAULT_AREATYPE>
class SpatialIndex {
    static_assert(Dimensions > 0, "Dimensions must be greater than 0");

    static_assert(std::numeric_limits<RangeType>::is_iec559,
                  "RangeType must be a floating-point type");

    static_assert(MaxChildren > MinChildren && MinChildren > 0,
                  "MaxChildren must be greater than MinChildren and "
                  "MinChildren must be greater than 0");

    using BND = BoundingBox<Dimensions, RangeType, AreaType>;

  protected:
    struct Node;
    struct SubNode;
    struct Group;
    struct GroupAssign;

    /**
     * @struct Node
     * @brief Represents a node in the spatial index tree.
     *
     * A node contains an array of sub-nodes and information about its height
     * and the current number of children. It provides functionality to check
     * if the node is a leaf node.
     */
    struct Node {
        int m_csize;  ///< Current number of children in the node.
        int m_height; ///< Height of the node in the tree.

        std::array<SubNode, MaxChildren> m_subNodes; ///< Array of sub-nodes.

        Node() = default;
        Node(int a_csize = 0, int a_height = -1)
            : m_csize(a_csize), m_height(a_height) {}

        /**
         * @brief Check if the node is a leaf.
         * @return True if the node is a leaf (height is 0), false otherwise.
         */
        bool isLeaf() const { return m_height == 0; }
    };

    /**
     * @struct SubNode
     * @brief Represents a sub-node in the spatial index.
     *
     * A sub-node contains a bounding box, an optional child node, and
     * associated data. It represents a data entry or a child node in the
     * index.
     */
    struct SubNode {
        BND m_box;               ///< Bounding box of the sub-node.
        Node *m_child = nullptr; ///< Pointer to child node, if any.
        DataType m_data;         ///< Data associated with the sub-node.

        SubNode() = default;
        SubNode(const BND &box, Node *child = nullptr,
                const DataType &data = DataType())
            : m_box(box), m_child(child), m_data(data) {}
    };

    /**
     * @struct Group
     * @brief Represents a group of nodes used during node splitting.
     *
     * A group contains a bounding box and a count of nodes. It is used to
     * manage and split nodes during insertions.
     */
    struct Group {
        BND m_box;       ///< Bounding box of the group.
        int m_count = 0; ///< Number of nodes in the group.
    };

    /**
     * @struct GroupAssign
     * @brief Assignment of nodes to groups during splitting.
     *
     * This struct is used to assign nodes to two groups during the node
     * splitting process. It includes an array for group assignments and
     * two groups for managing the split.
     */
    struct GroupAssign {
        int m_groupAssign[MaxChildren + 1]; ///< Array for group assignments.
        int m_size;                         ///< Size of the assignment array.
        Group m_groups[2];                  ///< Two groups used for splitting.

        GroupAssign() {
            for (int i = 0; i < MaxChildren + 1; ++i) {
                m_groupAssign[i] = -1;
            }
            m_size = MaxChildren + 1;
        }
    };

  public:
    using CallBackValue = SubNode; ///< Alias for the callback value type.

    /**
     * @brief Constructs a SpatialIndex with an initial root node.
     * 
     * This constructor initializes the spatial index with a root node located at (0, 0)
     * and sets the initial size of the index to 0.
     */
    SpatialIndex(Arena* arena) : m_arena(arena) {
        m_root = newNode(0, 0);
        m_size = 0; 
    }

    /**
     * @brief Destructor for SpatialIndex.
     * 
     * The destructor recursively deletes all nodes in the spatial index to free up memory.
     */
    ~SpatialIndex() {
        // RecursivelyDeleteNode(m_root);
    }

    /**
     * @brief Insert a new bounding box and associated data into the index.
     * @param a_box The bounding box to insert.
     * @param r_data The data to associate with the bounding box.
     * @return Status of the insertion operation.
     */
    bool Insert(BND a_box, const DataType &r_data) {
        SubNode subNode(a_box, nullptr, r_data);
        InsertSubNode(subNode, &m_root);
        m_size++;                 
        return true;  
    }

    /**
     * @brief Print the contents of the spatial index.
     *
     * This function prints a textual representation of the spatial index,
     * useful for debugging and visualization.
     */
    void Print() {
        BND t_b;                      
        RecursivelyPrint(m_root, &t_b);
    }

    /**
     * @brief Search for tags within a specified bounding box.
     * @param r_target The bounding box to search within.
     * @param callback The callback to process each result.
     *
     * This function performs a search within the index and uses the provided
     * callback to handle each result found within the bounding box.
     */
    void SearchTag(BND r_target, ISIndexCallback<SpatialIndex> *callback) {
         RecursivelySearchTag(r_target, m_root, callback);
    }

    /**
     * @brief Get the number of elements in the spatial index.
     * @return The number of elements.
     */
    std::size_t size() { return m_size; }

  private:
    Node *m_root;       ///< Pointer to the root node of the index.
    std::size_t m_size; ///< Number of elements in the index.

    Node **m_nodeBuffer; ///< Buffer for node pointers.

    Arena* m_arena;

  private:

    Node* newNode(size_t a_csize = 0, size_t a_height = -1) {
        char* node_memory = m_arena->AllocateAligned(sizeof(Node));
        Node* node = new (node_memory) Node(a_csize, a_height);
        return node;
    }
    /**
     * @brief Recursively delete nodes starting from a given node.
     * @param p_node The starting node for deletion.
     *
     * This function deletes the given node and all its children recursively.
     */
    void RecursivelyDeleteNode(Node *p_node) {
        if (p_node == nullptr) {
        return;
        }
        for (auto &e : p_node->m_subNodes) {
            RecursivelyDeleteNode(e.m_child); 
        }
        delete p_node;
    }

    /**
     * @brief Insert a sub-node into the index.
     * @param r_SubNode The sub-node to insert.
     * @param p_root Pointer to the root node.
     * @return True if the node was split, false otherwise.
     */
    bool InsertSubNode(const SubNode &r_SubNode, Node **p_root) {
        assert(p_root); 
        bool splited = RecursivelyInsertSubNode(r_SubNode, *p_root); 

        if (splited) {
        Node *newRoot = newNode(0, (*p_root)->m_height + 1);
        SubNode subNode;
        subNode.m_box = NodeCover(*p_root);
        subNode.m_child = *p_root;
        AddSubNode(subNode, newRoot);

        subNode.m_box = NodeCover(*m_nodeBuffer);
        subNode.m_child = *m_nodeBuffer;
        AddSubNode(subNode, newRoot);

        *p_root = newRoot; 

        return true;
        }
        return false;
    }

    /**
     * @brief Recursively insert a sub-node.
     * @param r_subNode The sub-node to insert.
     * @param p_node The current node to insert into.
     * @return True if the node was split, false otherwise.
     */
    bool RecursivelyInsertSubNode(const SubNode &r_subNode, Node *p_node) {
        assert(p_node); 

        if (p_node->isLeaf()) {
            return AddSubNode(r_subNode, p_node); 
        }
        int index = SelectBestSubNode(r_subNode.m_box,
                                    p_node); 
        bool childSplited = RecursivelyInsertSubNode(
            r_subNode, p_node->m_subNodes[index].m_child); 
        if (childSplited) {
            p_node->m_subNodes[index].m_box = NodeCover(
                p_node->m_subNodes[index].m_child); // Update the bounding box of the child node
            SubNode subnode;
            subnode.m_child = (*m_nodeBuffer);
            subnode.m_box = NodeCover(*m_nodeBuffer);

            return AddSubNode(subnode, p_node);
        } else {
            // Update the bounding box if no split occurred
            p_node->m_subNodes[index].m_box =
                BND::UnionBox(p_node->m_subNodes[index].m_box, r_subNode.m_box);
            return false; // Indicate no split
        }
        return false; 
    }

    /**
     * @brief Compute the bounding box covering a node.
     * @param p_node The node to compute the bounding box for.
     * @return The bounding box covering the node.
     */
    BB nodeCover(Node *p_node) {
        assert(p_node);

        BB t_box = p_node->m_subNodes[0].m_box;
        for (int index = 1; index < p_node->m_csize; index++) {
            t_box = bbf.Union(t_box,
                                p_node->m_subNodes[index]
                                   .m_box); // Union all subnodes' bounding boxes
        }
        return t_box; 
    }

    /**
     * @brief Select the best sub-node to fit a given bounding box.
     * @param r_box The bounding box to fit.
     * @param p_node The current node to search in.
     * @return The index of the best sub-node.
     */
    int SelectBestSubNode(const BND &r_box, Node *p_node) {
         int bestIndex = -1;
        AreaType bestIncr = std::numeric_limits<AreaType>::max();
        AreaType bestArea = std::numeric_limits<AreaType>::max();

        for (int index = 0; index < p_node->m_csize; ++index) {
            BND t_box = BND::UnionBox(
                r_box,
                p_node->m_subNodes[index].m_box); // Compute the union of boxes
            AreaType area = p_node->m_subNodes[index]
                                .m_box.area(); // Area of the current subnode's box
            AreaType increase =
                t_box.area() - area; // Increase in area due to the new box

            // Update best index based on area increase
            if (increase < bestIncr || (increase == bestIncr && area < bestArea)) {
                bestIndex = index;
                bestIncr = increase;
                bestArea = area;
            }
        }
        return bestIndex;
    }

    /**
     * @brief Add a sub-node to a node.
     * @param r_subNode The sub-node to add.
     * @param p_node The node to add the sub-node to.
     * @return True if the node was split, false otherwise.
     */
    bool AddSubNode(const SubNode &r_subNode, Node *p_node) {
         assert(p_node);

        if (p_node->m_csize < MaxChildren) {
                p_node->m_subNodes[p_node->m_csize++] =
                    r_subNode; // Add SubNode to the node
                return false;  // Indicate no split
            } else {
                // Need to split the node
                SplitNode(r_subNode, p_node);
                return true; // Indicate split occurred
            }
        }

        /**
         * @brief Split a node if necessary.
         * @param r_subNode The sub-node to insert.
         * @param p_node The node to split.
         */
    void SplitNode(const SubNode &r_subNode, Node *p_node) {
        assert(p_node);
        assert(p_node->m_csize == MaxChildren);

        GroupAssign t_groupAssign; // Holds the assignment of SubNodes to groups
        SubNode
            t_overflowBuffer[5]; // Buffer to hold nodes during the split process
        BND t_boundingBox =
            p_node->m_subNodes[0].m_box; // Bounding box of the current node
        AreaType t_overflowBufferArea[MaxChildren + 1]; // Areas of the bounding
                                                        // boxes in the buffer

        // Copy existing SubNodes and the new SubNode into t_overflowBuffer
        for (int i_subNode = 0; i_subNode < MaxChildren; i_subNode++) {
            t_overflowBuffer[i_subNode] = p_node->m_subNodes[i_subNode];
            t_boundingBox =
                BND::UnionBox(t_boundingBox, t_overflowBuffer[i_subNode].m_box);
            t_overflowBufferArea[i_subNode] =
                t_overflowBuffer[i_subNode].m_box.area();
        }
        t_overflowBuffer[MaxChildren] =
            r_subNode; // Add the new SubNode to the buffer
        t_boundingBox = BND::UnionBox(
            t_boundingBox,
            r_subNode.m_box); // Update the bounding box with the new SubNode
        t_overflowBufferArea[MaxChildren] =
            r_subNode.m_box.area(); // Update the area of the new SubNode

        int seed0 = 0, seed1 = 0; // Indices of the seeds for splitting
        RangeType bestNormalizedSeparation =
            -std::numeric_limits<RangeType>::infinity(); // Initialize best
                                                        // separation value

        // Find the best pair of seeds for the split based on bounding box
        // dimensions
        for (int dim = 0; dim < Dimensions; ++dim) {
            int minIndex = 0, maxIndex = 0;
            RangeType minLower = t_overflowBuffer[0].m_box.min(dim);
            RangeType maxUpper = t_overflowBuffer[0].m_box.max(dim);

            for (int index = 1; index < MaxChildren + 1; ++index) {
                // Find the minimum and maximum extents for the current dimension
                if (t_overflowBuffer[index].m_box.min(dim) < minLower) {
                    minLower = t_overflowBuffer[index].m_box.min(dim);
                    minIndex = index;
                }
                if (t_overflowBuffer[index].m_box.max(dim) > maxUpper) {
                    maxUpper = t_overflowBuffer[index].m_box.max(dim);
                    maxIndex = index;
                }
            }

            // Calculate normalized separation for the current dimension
            RangeType denominator = t_boundingBox.max(dim) - t_boundingBox.min(dim);
            RangeType reciprocal = 1.0 / denominator;
            RangeType separation = (maxUpper - minLower) * reciprocal;

            // Update best seeds if current separation is better
            if (separation > bestNormalizedSeparation) {
                bestNormalizedSeparation = separation;
                seed0 = minIndex;
                seed1 = maxIndex;
            }
        }

        // Initialize group assignments with the chosen seeds
        AssignGroup(seed0, 0, t_overflowBuffer[seed0].m_box, t_groupAssign);
        AssignGroup(seed1, 1, t_overflowBuffer[seed1].m_box, t_groupAssign);

        bool firstTime = true; // Flag for first iteration
        int chosen = -1;       // Index of the chosen SubNode for assignment
        int betterGroup = -1;  // Index of the better group for assignment
        int group;             // Current group being considered
        AreaType biggestDiff;  // Biggest difference in growth between groups

        // Assign remaining SubNodes to groups based on growth criteria
        while (
            (t_groupAssign.m_groups[0].m_count + t_groupAssign.m_groups[1].m_count <
            t_groupAssign.m_size) &&
            (t_groupAssign.m_groups[0].m_count <
            (t_groupAssign.m_size - MinChildren)) &&
            (t_groupAssign.m_groups[1].m_count <
            (t_groupAssign.m_size - MinChildren))) {

            bool firstTime = true;
            for (int index = 0; index < t_groupAssign.m_size; ++index) {
                if (t_groupAssign.m_groupAssign[index] == -1) {
                    // Compute growth of bounding boxes if the current SubNode is
                    // added to each group
                    BND box0 = BND::UnionBox(t_overflowBuffer[index].m_box,
                                            t_groupAssign.m_groups[0].m_box);
                    BND box1 = BND::UnionBox(t_overflowBuffer[index].m_box,
                                            t_groupAssign.m_groups[1].m_box);
                    AreaType growth0 =
                        box0.area() - t_groupAssign.m_groups[0].m_box.area();
                    AreaType growth1 =
                        box1.area() - t_groupAssign.m_groups[1].m_box.area();
                    AreaType diff = growth1 - growth0;

                    // Determine which group would be better for the current SubNode
                    if (diff >= 0) {
                        group = 0;
                    } else {
                        group = 1;
                        diff = -diff;
                    }

                    // Update the best assignment based on the biggest difference
                    if (firstTime || diff > biggestDiff) {
                        firstTime = false;
                        biggestDiff = diff;
                        chosen = index;
                        betterGroup = group;
                    } else if ((diff == biggestDiff) &&
                            (t_groupAssign.m_groups[group].m_count <
                                t_groupAssign.m_groups[betterGroup].m_count)) {
                        chosen = index;
                        betterGroup = group;
                    }
                }
            }

            // Ensure we found a valid choice
            assert(!firstTime);
            // Assign the chosen SubNode to the better group
            AssignGroup(chosen, betterGroup, t_overflowBuffer[chosen].m_box,
                        t_groupAssign);
        }

        // Final adjustments if the total number of nodes is less than expected
        if ((t_groupAssign.m_groups[0].m_count +
            t_groupAssign.m_groups[1].m_count) < t_groupAssign.m_size) {
            // Determine which group to assign remaining nodes to
            if (t_groupAssign.m_groups[0].m_count >=
                t_groupAssign.m_size - MinChildren) {
                group = 1;
            } else {
                group = 0;
            }
            // Assign remaining nodes to the determined group
            for (int i = 0; i < t_groupAssign.m_size; i++) {
                if (t_groupAssign.m_groupAssign[i] == -1) {
                    AssignGroup(i, group, t_overflowBuffer[i].m_box, t_groupAssign);
                }
            }
        }

        // Ensure valid group assignments
        assert((t_groupAssign.m_groups[0].m_count +
                t_groupAssign.m_groups[1].m_count) == t_groupAssign.m_size);
        assert(t_groupAssign.m_groups[0].m_count >= MinChildren);
        assert(t_groupAssign.m_groups[1].m_count >= MinChildren);

        // Clear the current node and create a new node for one group
        p_node->m_csize = 0;
        Node *t_node = newNode(0, p_node->m_height);
        Node *targetNodes[] = {p_node,
                            t_node}; // Nodes to receive the split SubNodes

        // Ensure the number of nodes in the split is correct
        assert(t_groupAssign.m_size <= MaxChildren + 1);

        for (int index = 0; index < t_groupAssign.m_size; index++) {
            int groupAssignValue = t_groupAssign.m_groupAssign[index];
            assert(groupAssignValue == 0 || groupAssignValue == 1);

            // Add each SubNode to the appropriate group node
            bool nodeSplited =
                AddSubNode(t_overflowBuffer[index], targetNodes[groupAssignValue]);

            assert(!nodeSplited);
        }

        // Ensure the split was performed correctly
        assert((p_node->m_csize + (t_node)->m_csize) == t_groupAssign.m_size);
        m_nodeBuffer = &t_node;
    }

    /**
     * @brief Assign nodes to groups during splitting.
     * @param a_index The index of the node to assign.
     * @param a_group The group to assign the node to.
     * @param r_box The bounding box of the group.
     * @param r_groupAssign The assignment structure to update.
     */
    void AssignGroup(int a_index, int a_group, BND &r_box,
                     GroupAssign &r_groupAssign) {
        assert(a_index < r_groupAssign.m_size);
        assert(a_group < 2);
        assert(r_groupAssign.m_groupAssign[a_index] == -1);

        // Assign the SubNode to the specified group
        r_groupAssign.m_groupAssign[a_index] = a_group;

        // Update the bounding box for the group
        if (r_groupAssign.m_groups[a_group].m_count == 0) {
            // If this is the first SubNode in the group, initialize the bounding
            // box
            r_groupAssign.m_groups[a_group].m_box = r_box;
        } else {
            // Otherwise, expand the existing bounding box to include the new
            // SubNode
            r_groupAssign.m_groups[a_group].m_box =
                BND::UnionBox(r_groupAssign.m_groups[a_group].m_box, r_box);
        }

        // Increment the count of SubNodes in the group
        r_groupAssign.m_groups[a_group].m_count++;
    }

    /**
     * @brief Recursively print nodes starting from a given node.
     * @param p_node The starting node to print.
     * @param p_box The bounding box of the current node.
     */
    void RecursivelyPrint(Node *p_node, BND *p_box) {
        if (p_node == nullptr) {
            return;
        }

        for (int i = 0; i < p_node->m_csize; i++) {
            std::cout << p_node->m_height << " " << p_box->toString() << " -> "
                     << p_node->m_subNodes[i].m_data <<
                    p_node->m_subNodes[i].m_box.toString() << std::endl;
            // Recursively print for child nodes
            RecursivelyPrint(p_node->m_subNodes[i].m_child,
                            &p_node->m_subNodes[i].m_box);
        }
    }

    /**
     * @brief Recursively search for tags within a specified bounding box.
     * @param r_target The bounding box to search within.
     * @param p_node The current node to search in.
     * @param callback The callback to process each result.
     */
    void RecursivelySearchTag(BND &r_target, Node *p_node,
                            ISIndexCallback<SpatialIndex> *callback) {
    if (p_node == nullptr) {
        return;
    }

    // Search through each SubNode in the current node
    for (int i = 0; i < p_node->m_csize; i++) {
        // If the node is a leaf and the target box overlaps with the SubNode's
        // box
        if (p_node->isLeaf() &&
            p_node->m_subNodes[i].m_box.isOverlap(r_target)) {
            // Process the SubNode using the callback
            bool conti = callback->process(p_node->m_subNodes[i]);
            // If the callback returns false, stop further processing
            if (!conti) {
                break;
            }
        }
        // Recursively search child nodes
        RecursivelySearchTag(r_target, p_node->m_subNodes[i].m_child, callback);
    }
    }
};
} // namespace tagfilterdb

#endif // TAGFILTERDB_SPATIAL_INDEX_HPP_