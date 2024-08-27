#ifndef TAGFILTERDB_R_STAR_TREE_HPP_
#define TAGFILTERDB_R_STAR_TREE_HPP_

#include "tagfilterdb/export.hpp"
#include "tagfilterdb/rStarTree/box.hpp"
#include "tagfilterdb/status.hpp"

#include <cassert>
#include <iostream>
#include <vector>

namespace tagfilterdb {
#define RSTARTREE_TEMPLATE                                                     \
    template <class DATATYPE, class RANGETYPE, int DIMS, int MAXCHILD,         \
              int MINCHILD>
#define RSTARTREE_QUAL RStarTree<DATATYPE, RANGETYPE, DIMS, MAXCHILD, MINCHILD>

RSTARTREE_TEMPLATE
class TAGFILTERDB_EXPORT RStarTree {
  public:
    RStarTree();
    RStarTree(const RStarTree &other);
    virtual ~RStarTree();

  protected:
    using BBox = BoundingBox<DIMS, RANGETYPE>;

    struct Node;
    struct Branch {
        BBox m_box;
        Node *m_child = nullptr;
        DATATYPE m_data;
    };
    struct Node {
        bool IsInternalNode() const { return (m_level > 0); }
        bool IsLeaf() const { return (m_level == 0); }

        int m_cout = 0;
        int m_level = -1;
        Branch m_branch[MAXCHILD];
    };
    struct ListNode {
        ListNode *m_next = nullptr; ///< Next in linked list
        Node *m_node = nullptr;     ///< Pointer to node
    };

    Node *m_root = nullptr;
    RANGETYPE m_unitSphereVolume;

    void Insert(const BBox &bbox, const DATATYPE &data);
    void InsertBox(const Branch &branch, Node **root, int level);
    bool RecursiveInsertBox(const Branch &branch, Node *node, Node **newNode,
                            int level);

    void InitNode(Node *node);
    BBox NodeCover(Node *node) const;

  public:
    std::vector<BBox> ListTree() const;
};

RSTARTREE_TEMPLATE
RSTARTREE_QUAL::RStarTree() {
    assert(MAXCHILD > 0 && MINCHILD > 0);
    assert(MAXCHILD > MINCHILD);

    const float UNIT_SPHERE_VOLUMES[] = {
        0.000000f, 2.000000f, 3.141593f, // Dimension  0,1,2
        4.188790f, 4.934802f, 5.263789f, // Dimension  3,4,5
        5.167713f, 4.724766f, 4.058712f, // Dimension  6,7,8
        3.298509f, 2.550164f, 1.884104f, // Dimension  9,10,11
        1.335263f, 0.910629f, 0.599265f, // Dimension  12,13,14
        0.381443f, 0.235331f, 0.140981f, // Dimension  15,16,17
        0.082146f, 0.046622f, 0.025807f, // Dimension  18,19,20
    };

    // Initialize root node
    m_root = new Node;
    InitNode(m_root);
    m_root->m_level = 0; // Root level set to 0 (leaf level)
    m_unitSphereVolume = static_cast<RANGETYPE>(UNIT_SPHERE_VOLUMES[DIMS]);
}

RSTARTREE_TEMPLATE
void RSTARTREE_QUAL::InitNode(Node *node) {
    node.= 0;
    node->l = -1;
}

RSTARTREE_TEMPLATE
void RSTARTREE_QUAL::Insert(const BBox &bbox, const DATATYPE &data) {
    Subtree branch;
    branch.data = data;
    branch.box = bbox;
    branch.child = nullptr;

    InsertBox(branch, &m_root, 0);
}

RSTARTREE_TEMPLATE
bool RSTARTREE_QUAL::RecursiveInsertBox(const Branch &branch, Node *node,
                                        Node **newNode, int level) {}

RSTARTREE_TEMPLATE
void RSTARTREE_QUAL::InsertBox(const Branch &branch, Node **root, int level) {
    assert(a_node && a_newNode);
    assert(a_level >= 0 && a_level <= (*root)->l);

    Node *node;
    if ()
}

} // namespace tagfilterdb

#endif // TAGFILTERDB_R_STAR_TREE_HPP_
