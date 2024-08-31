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

struct SpatialIndexOptions {
    const static int DEFAULT_MAX_CHILD = 4;
    const static int DEFAULT_MIN_CHILD = DEFAULT_MAX_CHILD / 2;
    using DEFAULT_RANGETYPE = double;
    using DEFAULT_AREATYPE = double;
};

template <class SIndex> class ISIndexCallback {
  public:
    using CallBackValue = typename SIndex::CallBackValue;

    virtual bool process(const CallBackValue &value) = 0;
};

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

    struct Node {
        int m_csize;
        int m_height;

        std::array<SubNode, MaxChildren> m_subNodes;

        Node() = default;
        Node(int a_csize = 0, int a_height = -1)
            : m_csize(a_csize), m_height(a_height) {}

        bool isLeaf() const { return m_height == 0; }
    };

    struct SubNode {
        BND m_box;
        Node *m_child = nullptr;
        DataType m_data;

        SubNode() = default;
        SubNode(const BND &box, Node *child = nullptr,
                const DataType &data = DataType())
            : m_box(box), m_child(child), m_data(data) {}
    };

    struct Group {
        BND m_box;
        int m_count = 0;
    };

    struct GroupAssign {
        int m_groupAssign[MaxChildren + 1];
        int m_size;
        Group m_groups[2];
        GroupAssign() {
            for (int i = 0; i < MaxChildren + 1; ++i) {
                m_groupAssign[i] = -1;
            }
            m_size = MaxChildren + 1;
        }
    };

  public:
    using CallBackValue = SubNode;

    SpatialIndex();
    ~SpatialIndex();
    Status Insert(BND a_box, const DataType &r_data);
    void Print();
    void SearchTag(BND r_target, ISIndexCallback<SpatialIndex> *callback);
    std::size_t size() { return m_size; }

  private:
    Node *m_root;
    std::size_t m_size;

    Node **m_nodeBuffer;

  private:
    void RecursivelyDeleteNode(Node *p_node);
    bool InsertSubNode(const SubNode &r_SubNode, Node **p_root);
    bool RecursivelyInsertSubNode(const SubNode &r_subNode, Node *p_node);
    BND NodeCover(Node *p_node);
    int SelectBestSubNode(const BND &r_box, Node *p_node);
    bool AddSubNode(const SubNode &r_subNode, Node *p_node);
    void SplitNode(const SubNode &r_subNode, Node *p_node);
    void AssignGroup(int a_index, int a_group, BND &r_box,
                     GroupAssign &r_groupAssign);

    void RecursivelyPrint(Node *p_node, BND *p_box);
    void RecursielySearchTag(BND &r_target, Node *p_node,
                             ISIndexCallback<SpatialIndex> *callback);
};

} // namespace tagfilterdb

#include "tagfilterdb/spatialIndex/spatialIndex.cpp"

#endif // TAGFILTERDB_SPATIAL_INDEX_HPP_
