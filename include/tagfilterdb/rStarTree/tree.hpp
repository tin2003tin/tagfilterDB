#ifndef TAGFILTERDB_R_STAR_TREE_HPP_
#define TAGFILTERDB_R_STAR_TREE_HPP_

#include "tagfilterdb/export.hpp"
#include "tagfilterdb/rStarTree/box.hpp"
#include "tagfilterdb/status.hpp"

#include <cassert>
#include <iostream>
#include <vector>

#define LOGIT(e) std::cout << "Log: " << e << std::endl;

namespace tagfilterdb {

#define RSTARTREE_TEMPLATE                                                     \
    template <class DATATYPE, class RANGETYPE, int DIMS,                       \
              std::size_t TMAXCHILD, std::size_t TMINCHILD>
#define RSTARTREE_QUAL                                                         \
    RStarTree<DATATYPE, RANGETYPE, DIMS, TMAXCHILD, TMINCHILD>

template <class DATATYPE, class RANGETYPE, int DIMS, std::size_t TMAXCHILD = 8,
          std::size_t TMINCHILD = TMAXCHILD / 2>
class RStarTree {
    static_assert(std::numeric_limits<RANGETYPE>::is_iec559,
                  "'ELEMTYPEREAL' accepts floating-point types only");
    using BND = BoundingBox<DIMS, RANGETYPE>;

  protected:
    class Node;

  public:
    class Branch {
      public:
        BND m_box;
        Node *m_child;
        DATATYPE m_data;
    };

  protected:
    enum {
        MAXCHILD = TMAXCHILD,
        MINCHILD = TMINCHILD,
    };

    class Node {
      public:
        Node(int a_count = 0, int a_level = -1)
            : m_count(a_count), m_level(a_level) {}
        bool isLeaf() { return m_level == 0; }
        bool isInterNode() { return m_level > 0; }

        int m_count;
        int m_level;
        bool Intern;
        Branch m_branch[MAXCHILD];
    };

    struct PartitionVars {
        enum { NOT_TAKEN = -1 };

        int m_partition[MAXCHILD + 1];
        int m_total;
        int m_minFill;
        int m_count[2];
        BND m_cover[2];
        RANGETYPE m_area[2];

        Branch m_branchBuf[MAXCHILD + 1];
        int m_branchCount;
        BND m_coverSplit;
        RANGETYPE m_coverSplitArea;
    };

  public:
    RStarTree();
    Status Insert(BND r_box, const DATATYPE &r_data);
    void Print() {
        BND t_b;
        RecursivelyPrint(m_root, &t_b);
    }

    void SearchTag(BND a_target, std::function<bool(const Branch &)> callback) {
        RecursielySearchTag(a_target, m_root, callback);
    }

  private:
    bool InsertBox(const Branch &r_newItem, Node **p_root, int a_level);
    bool RecursivelyInsertBox(const Branch &r_newItem, Node *p_node,
                              Node **p_newNode, int a_level);
    int SelectBestBranch(const BND &r_box, Node *p_node);
    BND NodeCover(Node *p_node);
    bool AddBranch(const Branch &r_newItem, Node *p_node, Node **p_newNode);
    void SplitNode(Node *p_node, const Branch &r_newItem, Node **p_newNode);
    void GetBranches(Node *p_node, const Branch &r_branch,
                     PartitionVars *p_parVars);
    void ChoosePartition(PartitionVars *p_parVars, int a_minFill);
    void InitParVars(PartitionVars *p_parVars, int a_maxBoxes, int a_minFill);
    void PickSeeds(PartitionVars *p_parVars);
    void Classify(int a_index, int a_group, PartitionVars *p_parVars);
    void LoadNodes(Node *p_nodeA, Node *p_nodeB, PartitionVars *p_parVars);
    void RecursielySearchTag(BND &a_target, Node *p_node,
                             std::function<bool(const Branch &)> callback) {
        if (p_node == nullptr) {
            return;
        }
        for (auto &b : p_node->m_branch) {
            if (b.m_box.isOverlap(a_target)) {
                if (p_node->isLeaf()) {
                    callback(b);
                }
                RecursielySearchTag(a_target, b.m_child, callback);
            }
        }
    }

    Node *m_root;

    void RecursivelyPrint(Node *p_node, BND *p_box) {
        if (p_node == nullptr) {
            return;
        }
        for (auto &e : p_node->m_branch) {
            LOGIT((std::to_string(p_node->m_level) + " " + p_box->toString() +
                   " -> " + e.m_data + e.m_box.toString()))
            RecursivelyPrint(e.m_child, &e.m_box);
        }
    }
};

RSTARTREE_TEMPLATE
RSTARTREE_QUAL::RStarTree() {
    assert(MINCHILD > 0);
    assert(MAXCHILD > MINCHILD);

    m_root = new Node(0, 0);
}

RSTARTREE_TEMPLATE
Status RSTARTREE_QUAL::Insert(BND a_box, const DATATYPE &r_data) {
    Branch branch;
    branch.m_data = r_data;
    branch.m_child = nullptr;
    branch.m_box = a_box;

    InsertBox(branch, &m_root, 0);

    return Status::OK();
}

RSTARTREE_TEMPLATE
bool RSTARTREE_QUAL::InsertBox(const Branch &r_newBranch, Node **p_root,
                               int a_level) {
    assert(p_root);
    assert(a_level >= 0 && a_level <= (*p_root)->m_level);
    Node *t_node;
    bool splited = RecursivelyInsertBox(r_newBranch, *p_root, &t_node, a_level);
    if (splited) {
        Node *newRoot = new Node(0, (*p_root)->m_level + 1);
        Branch branch;

        branch.m_box = NodeCover(*p_root);
        branch.m_child = *p_root;
        AddBranch(branch, newRoot, NULL);

        branch.m_box = NodeCover(t_node);
        branch.m_child = t_node;
        AddBranch(branch, newRoot, NULL);

        *p_root = newRoot;
        return true;
    }
    return false;
}

RSTARTREE_TEMPLATE
bool RSTARTREE_QUAL::RecursivelyInsertBox(const Branch &r_newItem, Node *p_root,
                                          Node **p_newNode, int a_level) {
    assert(p_root && p_newNode);
    assert(a_level >= 0 && a_level <= (*p_root).m_level);

    if (p_root->m_level > a_level) {
        Node *p_otherNode;
        int index = SelectBestBranch(r_newItem.m_box, p_root);
        bool childSplited = RecursivelyInsertBox(
            r_newItem, p_root->m_branch[index].m_child, &p_otherNode, a_level);
        if (!childSplited) {
            p_root->m_branch[index].m_box =
                r_newItem.m_box.unionBox(p_root->m_branch[index].m_box);
            return false;
        } else {
            // child splited
            p_root->m_branch[index].m_box =
                NodeCover(p_root->m_branch[index].m_child);
            Branch t_branch;
            t_branch.m_child = p_otherNode;
            t_branch.m_box = NodeCover(p_otherNode);

            return AddBranch(t_branch, p_root, p_newNode);
        }
    } else if (p_root->m_level == a_level) {
        return AddBranch(r_newItem, p_root, p_newNode);
    }

    return false;
}

RSTARTREE_TEMPLATE
int RSTARTREE_QUAL::SelectBestBranch(const BND &r_box, Node *p_node) {
    assert(p_node);
    bool firstTime = true;
    RANGETYPE increase;
    RANGETYPE bestIncrease = (RANGETYPE)-1;
    RANGETYPE area;
    RANGETYPE bestArea;
    int best = 0;
    BND t_box;
    for (std::size_t i_branch = 0; i_branch < p_node->m_count; i_branch++) {
        area = ((p_node->m_branch[i_branch]).m_box).area();
        t_box = r_box.unionBox((p_node->m_branch[i_branch]).m_box);
        increase = t_box.area() - area;
        if ((increase < bestIncrease) || firstTime) {
            best = i_branch;
            bestArea = area;
            bestIncrease = increase;
            firstTime = false;
        } else if ((increase == bestIncrease) && (area < bestArea)) {
            best = i_branch;
            bestArea = area;
            bestIncrease = increase;
        }
    }
    return best;
}

RSTARTREE_TEMPLATE
bool RSTARTREE_QUAL::AddBranch(const Branch &r_newItem, Node *p_node,
                               Node **p_newNode) {
    assert(p_node);
    if (p_node->m_count < MAXCHILD) {
        p_node->m_branch[p_node->m_count] = r_newItem;
        p_node->m_count++;
    } else {
        assert(p_newNode);
        SplitNode(p_node, r_newItem, p_newNode);
        return true;
    }
    return false;
}

RSTARTREE_TEMPLATE
typename RSTARTREE_QUAL::BND RSTARTREE_QUAL::NodeCover(Node *p_node) {
    assert(p_node);

    BND t_unionBox = p_node->m_branch[0].m_box;
    for (std::size_t i_branch = 1; i_branch < p_node->m_count; i_branch++) {
        t_unionBox = t_unionBox.unionBox(p_node->m_branch[i_branch].m_box);
    }
    return t_unionBox;
}

RSTARTREE_TEMPLATE
void RSTARTREE_QUAL::SplitNode(Node *p_node, const Branch &r_newItem,
                               Node **p_newNode) {
    assert(p_node);

    PartitionVars t_localVars;
    PartitionVars *p_loclVars = &t_localVars;

    GetBranches(p_node, r_newItem, p_loclVars);

    ChoosePartition(p_loclVars, MINCHILD);
    *p_newNode = new Node();
    (*p_newNode)->m_level = p_node->m_level;

    p_node->m_count = 0;
    LoadNodes(p_node, *p_newNode, p_loclVars);
    assert((p_node->m_count + (*p_newNode)->m_count) == p_loclVars->m_total);
}

RSTARTREE_TEMPLATE
void RSTARTREE_QUAL::GetBranches(Node *p_node, const Branch &r_branch,
                                 PartitionVars *p_parVars) {
    assert(p_node);
    assert(p_node->m_count == MAXCHILD);

    for (std::size_t i_buff = 0; i_buff < MAXCHILD; i_buff++) {
        p_parVars->m_branchBuf[i_buff] = p_node->m_branch[i_buff];
    }
    p_parVars->m_branchBuf[MAXCHILD] = r_branch;
    p_parVars->m_branchCount = MAXCHILD + 1;

    p_parVars->m_coverSplit = p_parVars->m_branchBuf[0].m_box;
    for (std::size_t i_buff = 1; i_buff < MAXCHILD + 1; i_buff++) {
        p_parVars->m_coverSplit = p_parVars->m_coverSplit.unionBox(
            p_parVars->m_branchBuf[i_buff].m_box);
    }
    p_parVars->m_coverSplitArea = p_parVars->m_coverSplit.area();
}
RSTARTREE_TEMPLATE
void RSTARTREE_QUAL::ChoosePartition(PartitionVars *p_parVars, int a_minFill) {
    assert(p_parVars);
    bool firstTime;
    RANGETYPE biggestDiff;
    int group, chosen = 0, betterGroup = 0;

    InitParVars(p_parVars, p_parVars->m_branchCount, a_minFill);

    while (
        ((p_parVars->m_count[0] + p_parVars->m_count[1]) <
         p_parVars->m_total) &&
        (p_parVars->m_count[0] < (p_parVars->m_total - p_parVars->m_minFill)) &&
        (p_parVars->m_count[1] < (p_parVars->m_total - p_parVars->m_minFill))) {
        firstTime = true;

        for (std::size_t i_par = 0; i_par < p_parVars->m_total; i_par++) {
            if (PartitionVars::NOT_TAKEN == p_parVars->m_partition[i_par]) {
                BND box0 = p_parVars->m_branchBuf[i_par].m_box.unionBox(
                    p_parVars->m_cover[0]);
                BND box1 = p_parVars->m_branchBuf[i_par].m_box.unionBox(
                    p_parVars->m_cover[1]);
                RANGETYPE growth0 = box0.area() - p_parVars->m_area[0];
                RANGETYPE growth1 = box1.area() - p_parVars->m_area[1];
                RANGETYPE diff = growth1 - growth0;
                if (diff >= 0) {
                    group = 0;
                } else {
                    group = 1;
                    diff = -diff;
                }
                if (firstTime || diff > biggestDiff) {
                    firstTime = false;
                    biggestDiff = diff;
                    chosen = i_par;
                    betterGroup = group;
                } else if ((diff == biggestDiff) &&
                           (p_parVars->m_count[group] <
                            p_parVars->m_count[betterGroup])) {
                    chosen = i_par;
                    betterGroup = group;
                }
            }
        }
        assert(!firstTime);
        Classify(chosen, betterGroup, p_parVars);
    }
    if ((p_parVars->m_count[0] + p_parVars->m_count[1]) < p_parVars->m_total) {
        if (p_parVars->m_count[0] >=
            p_parVars->m_total - p_parVars->m_minFill) {
            group = 1;
        } else {
            group = 0;
        }
        for (int i_par = 0; i_par < p_parVars->m_total; ++i_par) {
            if (PartitionVars::NOT_TAKEN == p_parVars->m_partition[i_par]) {
                Classify(i_par, group, p_parVars);
            }
        }
    }
    assert((p_parVars->m_count[0] + p_parVars->m_count[1]) ==
           p_parVars->m_total);
    assert((p_parVars->m_count[0] >= p_parVars->m_minFill) &&
           (p_parVars->m_count[1] >= p_parVars->m_minFill));
}

RSTARTREE_TEMPLATE
void RSTARTREE_QUAL::InitParVars(PartitionVars *p_parVars, int a_maxBoxes,
                                 int a_minFill) {
    assert(p_parVars);
    p_parVars->m_count[0] = p_parVars->m_count[1] = 0;
    p_parVars->m_area[0] = p_parVars->m_area[1] = (RANGETYPE)0;
    p_parVars->m_total = a_maxBoxes;
    p_parVars->m_minFill = a_minFill;
    for (std::size_t i_par = 0; i_par < a_maxBoxes; i_par++) {
        p_parVars->m_partition[i_par] = PartitionVars::NOT_TAKEN;
    }
}
RSTARTREE_TEMPLATE
void RSTARTREE_QUAL::PickSeeds(PartitionVars *p_parVars) {
    bool firstTime = true;
    int seed0 = 0, seed1 = 0;
    RANGETYPE worst, waste;
    RANGETYPE area[MAXCHILD + 1];
    for (std::size_t i_buff; i_buff < p_parVars->m_total; i_buff++) {
        area[i_buff] = p_parVars->m_branchBuf[index].m_box.area();
    }
    for (std::size_t i_A = 0; i_A < p_parVars->m_total - 1; i_A++) {
        for (std::size_t i_B = i_A + 1; i_B < p_parVars->m_total; i_B++) {
            BND box = p_parVars->m_branchBuf[i_A].m_box.unionBox(
                p_parVars->m_branchBuf[i_B].m_box);
            waste = box.area() - area[i_A] - area[i_B];
            if (firstTime || waste > worst) {
                firstTime = false;
                worst = waste;
                seed0 = i_A;
                seed1 = i_B;
            }
        }
    }
    assert(!firstTime);
    Classify(seed0, 0, p_parVars);
    Classify(seed1, 1, p_parVars);
}

RSTARTREE_TEMPLATE
void RSTARTREE_QUAL::Classify(int a_index, int a_group,
                              PartitionVars *p_parVars) {
    assert(p_parVars);
    assert(PartitionVars::NOT_TAKEN == p_parVars->m_partition[a_index]);

    p_parVars->m_partition[a_index] = a_group;

    if (p_parVars->m_count[a_group] == 0) {
        p_parVars->m_cover[a_group] = p_parVars->m_branchBuf[a_index].m_box;
    } else {
        p_parVars->m_cover[a_group] = p_parVars->m_cover[a_group].unionBox(
            p_parVars->m_branchBuf[a_index].m_box);
    }
    p_parVars->m_area[a_group] = p_parVars->m_cover[a_group].area();

    p_parVars->m_count[a_group]++;
}

RSTARTREE_TEMPLATE
void RSTARTREE_QUAL::LoadNodes(Node *p_nodeA, Node *p_nodeB,
                               PartitionVars *p_parVars) {
    assert(p_nodeA && p_nodeB && p_parVars);
    for (std::size_t i_par = 0; i_par < p_parVars->m_total; i_par++) {

        assert(p_parVars->m_partition[i_par] == 0 ||
               p_parVars->m_partition[i_par] == 1);
        int nodeIndex = p_parVars->m_partition[i_par];
        Node *nodes[] = {p_nodeA, p_nodeB};

        bool nodeSplited =
            AddBranch(p_parVars->m_branchBuf[i_par], nodes[nodeIndex], nullptr);
        assert(!nodeSplited);
    }
}

} // namespace tagfilterdb

#endif // TAGFILTERDB_R_STAR_TREE_HPP_
