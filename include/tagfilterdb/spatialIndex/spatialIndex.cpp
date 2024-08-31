#include "tagfilterdb/spatialIndex/spatialIndex.hpp"
#include "tagfilterdb/logging.hpp"

#define SPATIAL_INDEX_TEMPLATE                                                 \
    template <class DataType, std::size_t Dimensions, std::size_t MaxChildren, \
              std::size_t MinChildren, class RangeType, class AreaType>

#define SPATIAL_INDEX_TYPE                                                     \
    tagfilterdb::SpatialIndex<DataType, Dimensions, MaxChildren, MinChildren,  \
                              RangeType, AreaType>

namespace tagfilterdb {

// Constructor for SpatialIndex
SPATIAL_INDEX_TEMPLATE
SPATIAL_INDEX_TYPE::SpatialIndex() {
    // Initialize the root node with size 0 and height 0
    m_root = new Node(0, 0);
    m_size = 0; // Set the initial size of the index to 0
}

// Destructor for SpatialIndex
SPATIAL_INDEX_TEMPLATE
SPATIAL_INDEX_TYPE::~SpatialIndex() {
    RecursivelyDeleteNode(
        m_root); // Recursively delete all nodes starting from the root
}

// Insert a new bounding box with associated data into the spatial index
SPATIAL_INDEX_TEMPLATE Status
SPATIAL_INDEX_TYPE::Insert(BND a_box, const DataType &r_data) {
    SubNode subNode(a_box, nullptr, r_data); // Create a new SubNode
    InsertSubNode(subNode, &m_root); // Insert the SubNode into the index
    m_size++;                        // Increment the size of the index
    return Status::OK();             // Return success status
}

// Print the spatial index starting from the root
SPATIAL_INDEX_TEMPLATE
void SPATIAL_INDEX_TYPE::Print() {
    BND t_b;                        // Temporary bounding box
    RecursivelyPrint(m_root, &t_b); // Recursively print the index
}

// Search for overlapping nodes with the target bounding box
SPATIAL_INDEX_TEMPLATE
void SPATIAL_INDEX_TYPE::SearchTag(BND r_target,
                                   ISIndexCallback<SpatialIndex> *callback) {
    RecursivelySearchTag(r_target, m_root,
                         callback); // Recursively search for matching nodes
}

// Recursively delete nodes starting from the given node
SPATIAL_INDEX_TEMPLATE
void SPATIAL_INDEX_TYPE::RecursivelyDeleteNode(Node *p_node) {
    if (p_node == nullptr) {
        return; // Base case: if node is null, do nothing
    }
    for (auto &e : p_node->m_subNodes) {
        RecursivelyDeleteNode(e.m_child); // Recursively delete child nodes
    }
    delete p_node; // Delete the current node
}

// Insert a SubNode into the spatial index, splitting if necessary
SPATIAL_INDEX_TEMPLATE
bool SPATIAL_INDEX_TYPE::InsertSubNode(const SubNode &r_subNode,
                                       Node **p_root) {
    assert(p_root); // Ensure p_root is not null
    bool splited = RecursivelyInsertSubNode(
        r_subNode, *p_root); // Recursively insert the SubNode

    if (splited) {
        // If the node was split, create a new root node
        Node *newRoot = new Node(0, (*p_root)->m_height + 1);
        SubNode subNode;
        subNode.m_box = NodeCover(*p_root);
        subNode.m_child = *p_root;
        AddSubNode(subNode, newRoot);

        subNode.m_box = NodeCover(*m_nodeBuffer);
        subNode.m_child = *m_nodeBuffer;
        AddSubNode(subNode, newRoot);

        *p_root = newRoot; // Update root to the new node

        return true; // Indicate that the node was split
    }
    return false; // Indicate that no split was necessary
}

// Recursively insert a SubNode into a node
SPATIAL_INDEX_TEMPLATE
bool SPATIAL_INDEX_TYPE::RecursivelyInsertSubNode(const SubNode &r_subNode,
                                                  Node *p_node) {
    assert(p_node); // Ensure p_node is not null

    if (p_node->isLeaf()) {
        return AddSubNode(r_subNode, p_node); // Add SubNode to a leaf node
    }
    int index = SelectBestSubNode(r_subNode.m_box,
                                  p_node); // Select the best child node
    bool childSplited = RecursivelyInsertSubNode(
        r_subNode, p_node->m_subNodes[index]
                       .m_child); // Recursively insert into the best child
    if (childSplited) {
        p_node->m_subNodes[index].m_box = NodeCover(
            p_node->m_subNodes[index]
                .m_child); // Update the bounding box of the child node
        SubNode subnode;
        subnode.m_child = (*m_nodeBuffer);
        subnode.m_box = NodeCover(*m_nodeBuffer);

        return AddSubNode(subnode,
                          p_node); // Add the subnode to the current node
    } else {
        // Update the bounding box if no split occurred
        p_node->m_subNodes[index].m_box =
            BND::UnionBox(p_node->m_subNodes[index].m_box, r_subNode.m_box);
        return false; // Indicate no split
    }
    return false; // Indicate no split
}

// Compute the covering bounding box for a node
SPATIAL_INDEX_TEMPLATE
typename SPATIAL_INDEX_TYPE::BND SPATIAL_INDEX_TYPE::NodeCover(Node *p_node) {
    assert(p_node); // Ensure p_node is not null

    BND t_box = p_node->m_subNodes[0].m_box;
    for (int index = 1; index < p_node->m_csize; index++) {
        t_box = BND::UnionBox(t_box,
                              p_node->m_subNodes[index]
                                  .m_box); // Union all subnodes' bounding boxes
    }
    return t_box; // Return the covering bounding box
}

// Select the best subnode to insert into based on bounding box enlargement
SPATIAL_INDEX_TEMPLATE
int SPATIAL_INDEX_TYPE::SelectBestSubNode(const BND &r_box, Node *p_node) {
    assert(p_node); // Ensure p_node is not null

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
    return bestIndex; // Return the best index
}

// Add a SubNode to a node, splitting if necessary
SPATIAL_INDEX_TEMPLATE
bool SPATIAL_INDEX_TYPE::AddSubNode(const SubNode &r_subNode, Node *p_node) {
    assert(p_node); // Ensure p_node is not null

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

SPATIAL_INDEX_TEMPLATE
void SPATIAL_INDEX_TYPE::SplitNode(const SubNode &r_subNode, Node *p_node) {
    // Ensure the node to be split is valid and full
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

    // Ensure the chosen seeds are different
    // assert(seed0 != seed1);

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
    Node *t_node = new Node(0, p_node->m_height);
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
    m_nodeBuffer = &t_node; // Update the node buffer with the new node
}

SPATIAL_INDEX_TEMPLATE
void SPATIAL_INDEX_TYPE::AssignGroup(int a_index, int a_group, BND &r_box,
                                     GroupAssign &r_groupAssign) {
    // Ensure the index is within bounds and the group is valid
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
SPATIAL_INDEX_TEMPLATE
void SPATIAL_INDEX_TYPE::RecursivelyPrint(Node *p_node, BND *p_box) {
    // Base case: return if the node is nullptr
    if (p_node == nullptr) {
        return;
    }

    // Print information for each SubNode in the current node
    for (int i = 0; i < p_node->m_csize; i++) {
        LOG_DEBUG(p_node->m_height, " ", p_box->toString(), " -> ",
                  p_node->m_subNodes[i].m_data,
                  p_node->m_subNodes[i].m_box.toString())
        // Recursively print for child nodes
        RecursivelyPrint(p_node->m_subNodes[i].m_child,
                         &p_node->m_subNodes[i].m_box);
    }
}

SPATIAL_INDEX_TEMPLATE
void SPATIAL_INDEX_TYPE::RecursivelySearchTag(
    BND &r_target, Node *p_node, ISIndexCallback<SpatialIndex> *callback) {
    // Base case: return if the node is nullptr
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

} // namespace tagfilterdb

#undef SPATIAL_INDEX_TEMPLATE
#undef SPATIAL_INDEX_TYPE
