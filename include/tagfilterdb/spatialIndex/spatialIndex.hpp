#ifndef TAGFILTERDB_SPATIAL_INDEX_HPP_
#define TAGFILTERDB_SPATIAL_INDEX_HPP_

#include "tagfilterdb/export.hpp"
#include "tagfilterdb/spatialIndex/broundingbox.hpp"
#include "tagfilterdb/status.hpp"

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
    static const int DEFAULT_MIN_CHILD =
        DEFAULT_MAX_CHILD / 2; ///< Default minimum number of children per node.
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
class TAGFILTERDB_EXPORT SpatialIndex {
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

    SpatialIndex();  ///< Default constructor.
    ~SpatialIndex(); ///< Destructor.

    /**
     * @brief Insert a new bounding box and associated data into the index.
     * @param a_box The bounding box to insert.
     * @param r_data The data to associate with the bounding box.
     * @return Status of the insertion operation.
     */
    Status Insert(BND a_box, const DataType &r_data);

    /**
     * @brief Print the contents of the spatial index.
     *
     * This function prints a textual representation of the spatial index,
     * useful for debugging and visualization.
     */
    void Print();

    /**
     * @brief Search for tags within a specified bounding box.
     * @param r_target The bounding box to search within.
     * @param callback The callback to process each result.
     *
     * This function performs a search within the index and uses the provided
     * callback to handle each result found within the bounding box.
     */
    void SearchTag(BND r_target, ISIndexCallback<SpatialIndex> *callback);

    /**
     * @brief Get the number of elements in the spatial index.
     * @return The number of elements.
     */
    std::size_t size() { return m_size; }

  private:
    Node *m_root;       ///< Pointer to the root node of the index.
    std::size_t m_size; ///< Number of elements in the index.

    Node **m_nodeBuffer; ///< Buffer for node pointers.

  private:
    /**
     * @brief Recursively delete nodes starting from a given node.
     * @param p_node The starting node for deletion.
     *
     * This function deletes the given node and all its children recursively.
     */
    void RecursivelyDeleteNode(Node *p_node);

    /**
     * @brief Insert a sub-node into the index.
     * @param r_SubNode The sub-node to insert.
     * @param p_root Pointer to the root node.
     * @return True if the node was split, false otherwise.
     */
    bool InsertSubNode(const SubNode &r_SubNode, Node **p_root);

    /**
     * @brief Recursively insert a sub-node.
     * @param r_subNode The sub-node to insert.
     * @param p_node The current node to insert into.
     * @return True if the node was split, false otherwise.
     */
    bool RecursivelyInsertSubNode(const SubNode &r_subNode, Node *p_node);

    /**
     * @brief Compute the bounding box covering a node.
     * @param p_node The node to compute the bounding box for.
     * @return The bounding box covering the node.
     */
    BND NodeCover(Node *p_node);

    /**
     * @brief Select the best sub-node to fit a given bounding box.
     * @param r_box The bounding box to fit.
     * @param p_node The current node to search in.
     * @return The index of the best sub-node.
     */
    int SelectBestSubNode(const BND &r_box, Node *p_node);

    /**
     * @brief Add a sub-node to a node.
     * @param r_subNode The sub-node to add.
     * @param p_node The node to add the sub-node to.
     * @return True if the node was split, false otherwise.
     */
    bool AddSubNode(const SubNode &r_subNode, Node *p_node);

    /**
     * @brief Split a node if necessary.
     * @param r_subNode The sub-node to insert.
     * @param p_node The node to split.
     */
    void SplitNode(const SubNode &r_subNode, Node *p_node);

    /**
     * @brief Assign nodes to groups during splitting.
     * @param a_index The index of the node to assign.
     * @param a_group The group to assign the node to.
     * @param r_box The bounding box of the group.
     * @param r_groupAssign The assignment structure to update.
     */
    void AssignGroup(int a_index, int a_group, BND &r_box,
                     GroupAssign &r_groupAssign);

    /**
     * @brief Recursively print nodes starting from a given node.
     * @param p_node The starting node to print.
     * @param p_box The bounding box of the current node.
     */
    void RecursivelyPrint(Node *p_node, BND *p_box);

    /**
     * @brief Recursively search for tags within a specified bounding box.
     * @param r_target The bounding box to search within.
     * @param p_node The current node to search in.
     * @param callback The callback to process each result.
     */
    void RecursivelySearchTag(BND &r_target, Node *p_node,
                              ISIndexCallback<SpatialIndex> *callback);
};

} // namespace tagfilterdb

#include "tagfilterdb/spatialIndex/spatialIndex.cpp"

#endif // TAGFILTERDB_SPATIAL_INDEX_HPP_
