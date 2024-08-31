#include "tagfilterdb/spatialIndex/spatialIndex.hpp"
#include "tagfilterdb/logging.hpp"

#define SPATIAL_INDEX_TEMPLATE                                                 \
    template <class DataType, std::size_t Dimensions, std::size_t MaxChildren, \
              std::size_t MinChildren, class RangeType, class AreaType>

#define SPATIAL_INDEX_TYPE                                                     \
    tagfilterdb::SpatialIndex<DataType, Dimensions, MaxChildren, MinChildren,  \
                              RangeType, AreaType>

namespace tagfilterdb {

SPATIAL_INDEX_TEMPLATE
SPATIAL_INDEX_TYPE::SpatialIndex() {
    m_root = new Node(0, 0);
    m_size = 0;
}

SPATIAL_INDEX_TEMPLATE
SPATIAL_INDEX_TYPE::~SpatialIndex() { RecursivelyDeleteNode(m_root); }

SPATIAL_INDEX_TEMPLATE Status
SPATIAL_INDEX_TYPE::Insert(BND a_box, const DataType &r_data) {
    SubNode subNode(a_box, nullptr, r_data);
    InsertSubNode(subNode, &m_root);
    m_size++;
    return Status::OK();
}

SPATIAL_INDEX_TEMPLATE
void SPATIAL_INDEX_TYPE::Print() {
    BND t_b;
    RecursivelyPrint(m_root, &t_b);
}

SPATIAL_INDEX_TEMPLATE
void SPATIAL_INDEX_TYPE::SearchTag(BND r_target,
                                   ISIndexCallback<SpatialIndex> *callback) {
    RecursielySearchTag(r_target, m_root, callback);
}

SPATIAL_INDEX_TEMPLATE
void SPATIAL_INDEX_TYPE::RecursivelyDeleteNode(Node *p_node) {
    if (p_node == nullptr) {
        return;
    }
    for (auto &e : p_node->m_subNodes) {
        RecursivelyDeleteNode(e.m_child);
    }
    delete p_node;
}

SPATIAL_INDEX_TEMPLATE
bool SPATIAL_INDEX_TYPE::InsertSubNode(const SubNode &r_subNode,
                                       Node **p_root) {
    assert(p_root);
    bool splited = RecursivelyInsertSubNode(r_subNode, *p_root);

    if (splited) {
        Node *newRoot = new Node(0, (*p_root)->m_height + 1);
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

SPATIAL_INDEX_TEMPLATE
bool SPATIAL_INDEX_TYPE::RecursivelyInsertSubNode(const SubNode &r_subNode,
                                                  Node *p_node) {
    assert(p_node);

    if (p_node->isLeaf()) {
        return AddSubNode(r_subNode, p_node);
    }
    int index = SelectBestSubNode(r_subNode.m_box, p_node);
    bool childSplited =
        RecursivelyInsertSubNode(r_subNode, p_node->m_subNodes[index].m_child);
    if (childSplited) {
        p_node->m_subNodes[index].m_box =
            NodeCover(p_node->m_subNodes[index].m_child);
        SubNode subnode;
        subnode.m_child = (*m_nodeBuffer);
        subnode.m_box = NodeCover(*m_nodeBuffer);

        return AddSubNode(subnode, p_node);
    } else {
        p_node->m_subNodes[index].m_box =
            BND::UnionBox(p_node->m_subNodes[index].m_box, r_subNode.m_box);
        return false;
    }
    return false;
}
SPATIAL_INDEX_TEMPLATE
typename SPATIAL_INDEX_TYPE::BND SPATIAL_INDEX_TYPE::NodeCover(Node *p_node) {
    assert(p_node);

    BND t_box = p_node->m_subNodes[0].m_box;
    for (int index = 1; index < p_node->m_csize; index++) {
        t_box = BND::UnionBox(t_box, p_node->m_subNodes[index].m_box);
    }
    return t_box;
}

SPATIAL_INDEX_TEMPLATE
int SPATIAL_INDEX_TYPE::SelectBestSubNode(const BND &r_box, Node *p_node) {
    assert(p_node);

    int bestIndex = -1;
    AreaType bestIncr = std::numeric_limits<AreaType>::max();
    AreaType bestArea = std::numeric_limits<AreaType>::max();

    for (int index = 0; index < p_node->m_csize; ++index) {
        BND t_box = BND::UnionBox(r_box, p_node->m_subNodes[index].m_box);
        AreaType area = p_node->m_subNodes[index].m_box.area();
        AreaType increase = t_box.area() - area;

        if (increase < bestIncr || (increase == bestIncr && area < bestArea)) {
            bestIndex = index;
            bestIncr = increase;
            bestArea = area;
        }
    }
    return bestIndex;
}

SPATIAL_INDEX_TEMPLATE
bool SPATIAL_INDEX_TYPE::AddSubNode(const SubNode &r_subNode, Node *p_node) {
    assert(p_node);

    if (p_node->m_csize < MaxChildren) {
        p_node->m_subNodes[p_node->m_csize++] = r_subNode;
        return false;
    } else {
        // need to split
        SplitNode(r_subNode, p_node);
        return true;
    }
}

SPATIAL_INDEX_TEMPLATE
void SPATIAL_INDEX_TYPE::SplitNode(const SubNode &r_subNode, Node *p_node) {
    assert(p_node);
    assert(p_node->m_csize == MaxChildren);

    GroupAssign t_groupAssign;
    SubNode t_overflowBuffer[5];
    BND t_boundingBox = p_node->m_subNodes[0].m_box;
    AreaType t_overflowBufferArea[MaxChildren + 1];

    // Copy existing SubNodes and the new SubNode into t_overflowBuffer
    for (int i_subNode = 0; i_subNode < MaxChildren; i_subNode++) {
        t_overflowBuffer[i_subNode] = p_node->m_subNodes[i_subNode];
        t_boundingBox =
            BND::UnionBox(t_boundingBox, t_overflowBuffer[i_subNode].m_box);
        t_overflowBufferArea[i_subNode] =
            t_overflowBuffer[i_subNode].m_box.area();
    }
    t_overflowBuffer[MaxChildren] = r_subNode;
    t_boundingBox = BND::UnionBox(t_boundingBox, r_subNode.m_box);
    t_overflowBufferArea[MaxChildren] = r_subNode.m_box.area();

    int seed0 = 0, seed1 = 0;
    RangeType bestNormalizedSeparation =
        -std::numeric_limits<RangeType>::infinity();

    // Find the best pair of seeds
    for (int dim = 0; dim < Dimensions; ++dim) {
        int minIndex = 0, maxIndex = 0;
        RangeType minLower = t_overflowBuffer[0].m_box.min(dim);
        RangeType maxUpper = t_overflowBuffer[0].m_box.max(dim);

        for (int index = 1; index < MaxChildren + 1; ++index) {
            if (t_overflowBuffer[index].m_box.min(dim) < minLower) {
                minLower = t_overflowBuffer[index].m_box.min(dim);
                minIndex = index;
            }
            if (t_overflowBuffer[index].m_box.max(dim) > maxUpper) {
                maxUpper = t_overflowBuffer[index].m_box.max(dim);
                maxIndex = index;
            }
        }

        RangeType denominator = t_boundingBox.max(dim) - t_boundingBox.min(dim);
        RangeType reciprocal = 1.0 / denominator;
        RangeType separation = (maxUpper - minLower) * reciprocal;

        if (separation > bestNormalizedSeparation) {
            bestNormalizedSeparation = separation;
            seed0 = minIndex;
            seed1 = maxIndex;
        }
    }

    // Ensure seed0 and seed1 are different
    assert(seed0 != seed1);

    AssignGroup(seed0, 0, t_overflowBuffer[seed0].m_box, t_groupAssign);
    AssignGroup(seed1, 1, t_overflowBuffer[seed1].m_box, t_groupAssign);

    bool firstTime = true;
    int chosen = -1;
    int betterGroup = -1;
    int group;
    AreaType biggestDiff;

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

                BND box0 = BND::UnionBox(t_overflowBuffer[index].m_box,
                                         t_groupAssign.m_groups[0].m_box);
                BND box1 = BND::UnionBox(t_overflowBuffer[index].m_box,
                                         t_groupAssign.m_groups[1].m_box);
                AreaType growth0 =
                    box0.area() - t_groupAssign.m_groups[0].m_box.area();
                AreaType growth1 =
                    box1.area() - t_groupAssign.m_groups[1].m_box.area();
                AreaType diff = growth1 - growth0;

                if (diff >= 0) {
                    group = 0;
                } else {
                    group = 1;
                    diff = -diff;
                }

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

        assert(!firstTime);
        AssignGroup(chosen, betterGroup, t_overflowBuffer[chosen].m_box,
                    t_groupAssign);
    }
    if ((t_groupAssign.m_groups[0].m_count +
         t_groupAssign.m_groups[1].m_count) < t_groupAssign.m_size) {
        if (t_groupAssign.m_groups[0].m_count >=
            t_groupAssign.m_size - MinChildren) {
            group = 1;
        } else {
            group = 0;
        }
        for (int i = 0; i < t_groupAssign.m_size; i++) {
            if (t_groupAssign.m_groupAssign[i] == -1) {
                AssignGroup(i, group, t_overflowBuffer[i].m_box, t_groupAssign);
            }
        }
    }
    // LOG_INFO("OverFlowBuffer: ")
    // for (SubNode &o : t_overflowBuffer) {
    //     LOG_DEBUG(o.m_data, ":", o.m_box.toString())
    // }
    // LOG_INFO("GroupAssign: ")
    // for (int &n : t_groupAssign.m_groupAssign) {
    //     std::cout << n << " ";
    // }
    // std::cout << std::endl;
    // LOG_INFO("Group: ")
    // for (Group &g : t_groupAssign.m_groups) {
    //     LOG_DEBUG(g.m_box.toString())
    // }
    assert((t_groupAssign.m_groups[0].m_count +
            t_groupAssign.m_groups[1].m_count) == t_groupAssign.m_size);
    assert(t_groupAssign.m_groups[0].m_count >= MinChildren);
    assert(t_groupAssign.m_groups[1].m_count >= MinChildren);

    p_node->m_csize = 0;
    Node *t_node = new Node(0, p_node->m_height);
    Node *targetNodes[] = {p_node, t_node};

    assert(t_groupAssign.m_size <= MaxChildren + 1);

    for (int index = 0; index < t_groupAssign.m_size; index++) {
        int groupAssignValue = t_groupAssign.m_groupAssign[index];

        assert(groupAssignValue == 0 || groupAssignValue == 1);

        bool nodeSplited =
            AddSubNode(t_overflowBuffer[index], targetNodes[groupAssignValue]);

        assert(!nodeSplited);
    }
    assert((p_node->m_csize + (t_node)->m_csize) == t_groupAssign.m_size);
    m_nodeBuffer = &t_node;
}

SPATIAL_INDEX_TEMPLATE
void SPATIAL_INDEX_TYPE::AssignGroup(int a_index, int a_group, BND &r_box,
                                     GroupAssign &r_groupAssign) {
    assert(a_index < r_groupAssign.m_size);
    assert(a_group < 2);
    assert(r_groupAssign.m_groupAssign[a_index] == -1);

    r_groupAssign.m_groupAssign[a_index] = a_group;
    if (r_groupAssign.m_groups[a_group].m_count == 0) {
        r_groupAssign.m_groups[a_group].m_box = r_box;
    } else {
        r_groupAssign.m_groups[a_group].m_box =
            BND::UnionBox(r_groupAssign.m_groups[a_group].m_box, r_box);
    }

    r_groupAssign.m_groups[a_group].m_count++;
}

SPATIAL_INDEX_TEMPLATE
void SPATIAL_INDEX_TYPE::RecursivelyPrint(Node *p_node, BND *p_box) {
    if (p_node == nullptr) {
        return;
    }

    for (int i = 0; i < p_node->m_csize; i++) {
        LOG_DEBUG(p_node->m_height, " ", p_box->toString(), " -> ",
                  p_node->m_subNodes[i].m_data,
                  p_node->m_subNodes[i].m_box.toString())
        RecursivelyPrint(p_node->m_subNodes[i].m_child,
                         &p_node->m_subNodes[i].m_box);
    }
}
SPATIAL_INDEX_TEMPLATE
void SPATIAL_INDEX_TYPE::RecursielySearchTag(
    BND &r_target, Node *p_node, ISIndexCallback<SpatialIndex> *callback) {
    if (p_node == nullptr) {
        return;
    }

    for (int i = 0; i < p_node->m_csize; i++) {
        if (p_node->isLeaf() &&
            p_node->m_subNodes[i].m_box.isOverlap(r_target)) {
            bool conti = callback->process(p_node->m_subNodes[i]);
            if (!conti) {
                break;
            }
        }
        RecursielySearchTag(r_target, p_node->m_subNodes[i].m_child, callback);
    }
}

} // namespace tagfilterdb

#undef SPATIAL_INDEX_TEMPLATE
#undef SPATIAL_INDEX_TYPE
