#ifndef RTREE_H
#define RTREE_H

// NOTE This file compiles under MSVC 6 SP5 and MSVC .Net 2003 it may not work on other compilers without modification.

// NOTE These next few lines may be win32 specific, you may need to modify them to compile on other platform
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <stdlib.h>

#include <algorithm>
#include <functional>
#include <vector>
#include <queue>

#define RTREE_ASSERT assert // RTree uses RTREE_ASSERT( condition )
#ifdef Min
#define RTREE_MIN Min
#else
#define RTREE_MIN std::min
#endif // Min
#ifdef Max
#define RTREE_MAX Max
#else
#define RTREE_MAX std::max
#endif // Max

#define RTREE_TEMPLATE template <class DATATYPE, class ELEMTYPE, int NUMDIMS, class ELEMTYPEREAL, int TMAXNODES, int TMINNODES>
#define RTREE_QUAL RTree<DATATYPE, ELEMTYPE, NUMDIMS, ELEMTYPEREAL, TMAXNODES, TMINNODES>

#define RTREE_DONT_USE_MEMPOOLS    // This version does not contain a fixed memory allocator, fill in lines with EXAMPLE to implement one.
#define RTREE_USE_SPHERICAL_VOLUME // Better split classification, may be slower on some systems

template <class DATATYPE, class ELEMTYPE, int NUMDIMS,
          class ELEMTYPEREAL = ELEMTYPE, int TMAXNODES = 8, int TMINNODES = TMAXNODES / 2>
class RTree
{
    static_assert(std::numeric_limits<ELEMTYPEREAL>::is_iec559, "'ELEMTYPEREAL' accepts floating-point types only");

protected:
    struct Node; // Fwd decl.  Used by other internal structs and iterator

public:
    // These constant must be declared after Branch and before Node struct
    // Stuck up here for MSVC 6 compiler.  NSVC .NET 2003 is much happier.
    enum
    {
        MAXNODES = TMAXNODES, ///< Max elements in node
        MINNODES = TMINNODES, ///< Min elements in node
    };

public:
    RTree();
    RTree(const RTree &other);
    virtual ~RTree();

    /// Insert entry
    /// \param a_min Min of bounding rect
    /// \param a_max Max of bounding rect
    /// \param a_dataId Positive Id of data.  Maybe zero, but negative numbers not allowed.
    void Insert(const ELEMTYPE a_min[NUMDIMS], const ELEMTYPE a_max[NUMDIMS], const DATATYPE &a_dataId);

protected:
    /// Minimal bounding rectangle (n-dimensional)
    struct Rect
    {
        ELEMTYPE m_min[NUMDIMS]; ///< Min dimensions of bounding box
        ELEMTYPE m_max[NUMDIMS]; ///< Max dimensions of bounding box
    };

    /// May be data or may be another subtree
    /// The parents level determines this.
    /// If the parents level is 0, then this is data
    struct Branch
    {
        Rect m_rect;     ///< Bounds
        Node *m_child;   ///< Child node
        DATATYPE m_data; ///< Data Id
    };

    /// Node for each branch level
    struct Node
    {
        bool IsInternalNode() { return (m_level > 0); } // Not a leaf, but a internal node
        bool IsLeaf() { return (m_level == 0); }        // A leaf, contains data

        int m_count;               ///< Count
        int m_level;               ///< Leaf is zero, others positive
        Branch m_branch[MAXNODES]; ///< Branch
    };

    /// A link list of nodes for reinsertion after a delete operation
    struct ListNode
    {
        ListNode *m_next; ///< Next in list
        Node *m_node;     ///< Node
    };

    /// Variables for finding a split partition
    struct PartitionVars
    {
        enum
        {
            NOT_TAKEN = -1
        }; // indicates that position

        int m_partition[MAXNODES + 1];
        int m_total;
        int m_minFill;
        int m_count[2];
        Rect m_cover[2];
        ELEMTYPEREAL m_area[2];

        Branch m_branchBuf[MAXNODES + 1];
        int m_branchCount;
        Rect m_coverSplit;
        ELEMTYPEREAL m_coverSplitArea;
    };

    Node *AllocNode();
    void InitNode(Node *a_node);
    void InitRect(Rect *a_rect);
    bool InsertRectRec(const Branch &a_branch, Node *a_node, Node **a_newNode, int a_level);
    bool InsertRect(const Branch &a_branch, Node **a_root, int a_level);
    Rect NodeCover(Node *a_node);
    bool AddBranch(const Branch *a_branch, Node *a_node, Node **a_newNode);
    int PickBranch(const Rect *a_rect, Node *a_node);
    Rect CombineRect(const Rect *a_rectA, const Rect *a_rectB);
    void SplitNode(Node *a_node, const Branch *a_branch, Node **a_newNode);
    ELEMTYPEREAL RectSphericalVolume(Rect *a_rect);
    ELEMTYPEREAL RectVolume(Rect *a_rect);
    ELEMTYPEREAL CalcRectVolume(Rect *a_rect);
    void GetBranches(Node *a_node, const Branch *a_branch, PartitionVars *a_parVars);
    void ChoosePartition(PartitionVars *a_parVars, int a_minFill);
    void LoadNodes(Node *a_nodeA, Node *a_nodeB, PartitionVars *a_parVars);
    void InitParVars(PartitionVars *a_parVars, int a_maxRects, int a_minFill);
    void PickSeeds(PartitionVars *a_parVars);
    void Classify(int a_index, int a_group, PartitionVars *a_parVars);

    Node *m_root;
    ELEMTYPEREAL m_unitSphereVolume;

public:
    // return all the AABBs that form the RTree
    std::vector<Rect> ListTree() const;
};

RTREE_TEMPLATE
RTREE_QUAL::RTree()
{
    RTREE_ASSERT(MAXNODES > MINNODES);
    RTREE_ASSERT(MINNODES > 0);

    // Precomputed volumes of the unit spheres for the first few dimensions
    const float UNIT_SPHERE_VOLUMES[] = {
        0.000000f, 2.000000f, 3.141593f, // Dimension  0,1,2
        4.188790f, 4.934802f, 5.263789f, // Dimension  3,4,5
        5.167713f, 4.724766f, 4.058712f, // Dimension  6,7,8
        3.298509f, 2.550164f, 1.884104f, // Dimension  9,10,11
        1.335263f, 0.910629f, 0.599265f, // Dimension  12,13,14
        0.381443f, 0.235331f, 0.140981f, // Dimension  15,16,17
        0.082146f, 0.046622f, 0.025807f, // Dimension  18,19,20
    };

    m_root = AllocNode();
    m_root->m_level = 0;
    m_unitSphereVolume = (ELEMTYPEREAL)UNIT_SPHERE_VOLUMES[NUMDIMS];
}

RTREE_TEMPLATE
RTREE_QUAL::~RTree()
{
    // Reset(); // Free, or reset node memory
}

RTREE_TEMPLATE
void RTREE_QUAL::Insert(const ELEMTYPE a_min[NUMDIMS], const ELEMTYPE a_max[NUMDIMS], const DATATYPE &a_dataId)
{
#ifdef _DEBUG
    for (int index = 0; index < NUMDIMS; ++index)
    {
        RTREE_ASSERT(a_min[index] <= a_max[index]);
    }
#endif //_DEBUG

    Branch branch;
    branch.m_data = a_dataId;
    branch.m_child = NULL;

    for (int axis = 0; axis < NUMDIMS; ++axis)
    {
        branch.m_rect.m_min[axis] = a_min[axis];
        branch.m_rect.m_max[axis] = a_max[axis];
    }

    InsertRect(branch, &m_root, 0);
}

RTREE_TEMPLATE
typename RTREE_QUAL::Node *RTREE_QUAL::AllocNode()
{
    Node *newNode;
#ifdef RTREE_DONT_USE_MEMPOOLS
    newNode = new Node;
#else  // RTREE_DONT_USE_MEMPOOLS
    // EXAMPLE
#endif // RTREE_DONT_USE_MEMPOOLS
    InitNode(newNode);
    return newNode;
}

RTREE_TEMPLATE
void RTREE_QUAL::InitNode(Node *a_node)
{
    a_node->m_count = 0;
    a_node->m_level = -1;
}

RTREE_TEMPLATE
void RTREE_QUAL::InitRect(Rect *a_rect)
{
    for (int index = 0; index < NUMDIMS; ++index)
    {
        a_rect->m_min[index] = (ELEMTYPE)0;
        a_rect->m_max[index] = (ELEMTYPE)0;
    }
}

// Inserts a new data rectangle into the index structure.
// Recursively descends tree, propagates splits back up.
// Returns 0 if node was not split.  Old node updated.
// If node was split, returns 1 and sets the pointer pointed to by
// new_node to point to the new node.  Old node updated to become one of two.
// The level argument specifies the number of steps up from the leaf
// level to insert; e.g. a data rectangle goes in at level = 0.
RTREE_TEMPLATE
bool RTREE_QUAL::InsertRectRec(const Branch &a_branch, Node *a_node, Node **a_newNode, int a_level)
{
    RTREE_ASSERT(a_node && a_newNode);
    RTREE_ASSERT(a_level >= 0 && a_level <= a_node->m_level);

    // recurse until we reach the correct level for the new record. data records
    // will always be called with a_level == 0 (leaf)
    if (a_node->m_level > a_level)
    {
        // Still above level for insertion, go down tree recursively
        Node *otherNode;

        // find the optimal branch for this record
        int index = PickBranch(&a_branch.m_rect, a_node);

        // recursively insert this record into the picked branch
        bool childWasSplit = InsertRectRec(a_branch, a_node->m_branch[index].m_child, &otherNode, a_level);

        if (!childWasSplit)
        {
            // Child was not split. Merge the bounding box of the new record with the
            // existing bounding box
            a_node->m_branch[index].m_rect = CombineRect(&a_branch.m_rect, &(a_node->m_branch[index].m_rect));
            return false;
        }
        else
        {
            // Child was split. The old branches are now re-partitioned to two nodes
            // so we have to re-calculate the bounding boxes of each node
            a_node->m_branch[index].m_rect = NodeCover(a_node->m_branch[index].m_child);
            Branch branch;
            branch.m_child = otherNode;
            branch.m_rect = NodeCover(otherNode);

            // The old node is already a child of a_node. Now add the newly-created
            // node to a_node as well. a_node might be split because of that.
            return AddBranch(&branch, a_node, a_newNode);
        }
    }
    else if (a_node->m_level == a_level)
    {
        // We have reached level for insertion. Add rect, split if necessary
        return AddBranch(&a_branch, a_node, a_newNode);
    }
    else
    {
        // Should never occur
        RTREE_ASSERT(0);
        return false;
    }
}

// Insert a data rectangle into an index structure.
// InsertRect provides for splitting the root;
// returns 1 if root was split, 0 if it was not.
// The level argument specifies the number of steps up from the leaf
// level to insert; e.g. a data rectangle goes in at level = 0.
// InsertRect2 does the recursion.
//
RTREE_TEMPLATE
bool RTREE_QUAL::InsertRect(const Branch &a_branch, Node **a_root, int a_level)
{
    RTREE_ASSERT(a_root);
    RTREE_ASSERT(a_level >= 0 && a_level <= (*a_root)->m_level);
#ifdef _DEBUG
    for (int index = 0; index < NUMDIMS; ++index)
    {
        RTREE_ASSERT(a_branch.m_rect.m_min[index] <= a_branch.m_rect.m_max[index]);
    }
#endif //_DEBUG

    Node *newNode;

    if (InsertRectRec(a_branch, *a_root, &newNode, a_level)) // Root split
    {
        // Grow tree taller and new root
        Node *newRoot = AllocNode();
        newRoot->m_level = (*a_root)->m_level + 1;

        Branch branch;

        // add old root node as a child of the new root
        branch.m_rect = NodeCover(*a_root);
        branch.m_child = *a_root;
        AddBranch(&branch, newRoot, NULL);

        // add the split node as a child of the new root
        branch.m_rect = NodeCover(newNode);
        branch.m_child = newNode;
        AddBranch(&branch, newRoot, NULL);

        // set the new root as the root node
        *a_root = newRoot;

        return true;
    }

    return false;
}

// Find the smallest rectangle that includes all rectangles in branches of a node.
RTREE_TEMPLATE
typename RTREE_QUAL::Rect RTREE_QUAL::NodeCover(Node *a_node)
{
    RTREE_ASSERT(a_node);

    Rect rect = a_node->m_branch[0].m_rect;
    for (int index = 1; index < a_node->m_count; ++index)
    {
        rect = CombineRect(&rect, &(a_node->m_branch[index].m_rect));
    }

    return rect;
}

// Add a branch to a node.  Split the node if necessary.
// Returns 0 if node not split.  Old node updated.
// Returns 1 if node split, sets *new_node to address of new node.
// Old node updated, becomes one of two.
RTREE_TEMPLATE
bool RTREE_QUAL::AddBranch(const Branch *a_branch, Node *a_node, Node **a_newNode)
{
    RTREE_ASSERT(a_branch);
    RTREE_ASSERT(a_node);

    if (a_node->m_count < MAXNODES) // Split won't be necessary
    {
        a_node->m_branch[a_node->m_count] = *a_branch;
        ++a_node->m_count;

        return false;
    }
    else
    {
        RTREE_ASSERT(a_newNode);

        SplitNode(a_node, a_branch, a_newNode);
        return true;
    }
}

// Pick a branch.  Pick the one that will need the smallest increase
// in area to accomodate the new rectangle.  This will result in the
// least total area for the covering rectangles in the current node.
// In case of a tie, pick the one which was smaller before, to get
// the best resolution when searching.
RTREE_TEMPLATE
int RTREE_QUAL::PickBranch(const Rect *a_rect, Node *a_node)
{
    RTREE_ASSERT(a_rect && a_node);

    bool firstTime = true;
    ELEMTYPEREAL increase;
    ELEMTYPEREAL bestIncr = (ELEMTYPEREAL)-1;
    ELEMTYPEREAL area;
    ELEMTYPEREAL bestArea;
    int best = 0;
    Rect tempRect;

    for (int index = 0; index < a_node->m_count; ++index)
    {
        Rect *curRect = &a_node->m_branch[index].m_rect;
        area = CalcRectVolume(curRect);
        tempRect = CombineRect(a_rect, curRect);
        increase = CalcRectVolume(&tempRect) - area;
        if ((increase < bestIncr) || firstTime)
        {
            best = index;
            bestArea = area;
            bestIncr = increase;
            firstTime = false;
        }
        else if ((increase == bestIncr) && (area < bestArea))
        {
            best = index;
            bestArea = area;
            bestIncr = increase;
        }
    }
    return best;
}

// Combine two rectangles into larger one containing both
RTREE_TEMPLATE
typename RTREE_QUAL::Rect RTREE_QUAL::CombineRect(const Rect *a_rectA, const Rect *a_rectB)
{
    RTREE_ASSERT(a_rectA && a_rectB);

    Rect newRect;

    for (int index = 0; index < NUMDIMS; ++index)
    {
        newRect.m_min[index] = RTREE_MIN(a_rectA->m_min[index], a_rectB->m_min[index]);
        newRect.m_max[index] = RTREE_MAX(a_rectA->m_max[index], a_rectB->m_max[index]);
    }

    return newRect;
}

// Split a node.
// Divides the nodes branches and the extra one between two nodes.
// Old node is one of the new ones, and one really new one is created.
// Tries more than one method for choosing a partition, uses best result.
RTREE_TEMPLATE
void RTREE_QUAL::SplitNode(Node *a_node, const Branch *a_branch, Node **a_newNode)
{
    RTREE_ASSERT(a_node);
    RTREE_ASSERT(a_branch);

    // Could just use local here, but member or external is faster since it is reused
    PartitionVars localVars;
    PartitionVars *parVars = &localVars;

    // Load all the branches into a buffer, initialize old node
    GetBranches(a_node, a_branch, parVars);

    // Find partition
    ChoosePartition(parVars, MINNODES);

    // Create a new node to hold (about) half of the branches
    *a_newNode = AllocNode();
    (*a_newNode)->m_level = a_node->m_level;

    // Put branches from buffer into 2 nodes according to the chosen partition
    a_node->m_count = 0;
    LoadNodes(a_node, *a_newNode, parVars);

    RTREE_ASSERT((a_node->m_count + (*a_newNode)->m_count) == parVars->m_total);
}

// Calculate the n-dimensional volume of a rectangle
RTREE_TEMPLATE
ELEMTYPEREAL RTREE_QUAL::RectVolume(Rect *a_rect)
{
    RTREE_ASSERT(a_rect);

    ELEMTYPEREAL volume = (ELEMTYPEREAL)1;

    for (int index = 0; index < NUMDIMS; ++index)
    {
        volume *= a_rect->m_max[index] - a_rect->m_min[index];
    }

    RTREE_ASSERT(volume >= (ELEMTYPEREAL)0);

    return volume;
}

// The exact volume of the bounding sphere for the given Rect
RTREE_TEMPLATE
ELEMTYPEREAL RTREE_QUAL::RectSphericalVolume(Rect *a_rect)
{
    RTREE_ASSERT(a_rect);

    ELEMTYPEREAL sumOfSquares = (ELEMTYPEREAL)0;
    ELEMTYPEREAL radius;

    for (int index = 0; index < NUMDIMS; ++index)
    {
        ELEMTYPEREAL halfExtent = ((ELEMTYPEREAL)a_rect->m_max[index] - (ELEMTYPEREAL)a_rect->m_min[index]) * (ELEMTYPEREAL)0.5;
        sumOfSquares += halfExtent * halfExtent;
    }

    radius = (ELEMTYPEREAL)sqrt(sumOfSquares);

    // Pow maybe slow, so test for common dims like 2,3 and just use x*x, x*x*x.
    if (NUMDIMS == 3)
    {
        return (radius * radius * radius * m_unitSphereVolume);
    }
    else if (NUMDIMS == 2)
    {
        return (radius * radius * m_unitSphereVolume);
    }
    else
    {
        return (ELEMTYPEREAL)(pow(radius, NUMDIMS) * m_unitSphereVolume);
    }
}

// Use one of the methods to calculate retangle volume
RTREE_TEMPLATE
ELEMTYPEREAL RTREE_QUAL::CalcRectVolume(Rect *a_rect)
{
#ifdef RTREE_USE_SPHERICAL_VOLUME
    return RectSphericalVolume(a_rect); // Slower but helps certain merge cases
#else                                   // RTREE_USE_SPHERICAL_VOLUME
    return RectVolume(a_rect); // Faster but can cause poor merges
#endif                                  // RTREE_USE_SPHERICAL_VOLUME
}

// Load branch buffer with branches from full node plus the extra branch.
RTREE_TEMPLATE
void RTREE_QUAL::GetBranches(Node *a_node, const Branch *a_branch, PartitionVars *a_parVars)
{
    RTREE_ASSERT(a_node);
    RTREE_ASSERT(a_branch);

    RTREE_ASSERT(a_node->m_count == MAXNODES);

    // Load the branch buffer
    for (int index = 0; index < MAXNODES; ++index)
    {
        a_parVars->m_branchBuf[index] = a_node->m_branch[index];
    }
    a_parVars->m_branchBuf[MAXNODES] = *a_branch;
    a_parVars->m_branchCount = MAXNODES + 1;

    // Calculate rect containing all in the set
    a_parVars->m_coverSplit = a_parVars->m_branchBuf[0].m_rect;
    for (int index = 1; index < MAXNODES + 1; ++index)
    {
        a_parVars->m_coverSplit = CombineRect(&a_parVars->m_coverSplit, &a_parVars->m_branchBuf[index].m_rect);
    }
    a_parVars->m_coverSplitArea = CalcRectVolume(&a_parVars->m_coverSplit);
}

// Method #0 for choosing a partition:
// As the seeds for the two groups, pick the two rects that would waste the
// most area if covered by a single rectangle, i.e. evidently the worst pair
// to have in the same group.
// Of the remaining, one at a time is chosen to be put in one of the two groups.
// The one chosen is the one with the greatest difference in area expansion
// depending on which group - the rect most strongly attracted to one group
// and repelled from the other.
// If one group gets too full (more would force other group to violate min
// fill requirement) then other group gets the rest.
// These last are the ones that can go in either group most easily.
RTREE_TEMPLATE
void RTREE_QUAL::ChoosePartition(PartitionVars *a_parVars, int a_minFill)
{
    RTREE_ASSERT(a_parVars);

    bool firstTime;
    ELEMTYPEREAL biggestDiff;
    int group, chosen = 0, betterGroup = 0;

    InitParVars(a_parVars, a_parVars->m_branchCount, a_minFill);
    PickSeeds(a_parVars);

    while (((a_parVars->m_count[0] + a_parVars->m_count[1]) < a_parVars->m_total) && (a_parVars->m_count[0] < (a_parVars->m_total - a_parVars->m_minFill)) && (a_parVars->m_count[1] < (a_parVars->m_total - a_parVars->m_minFill)))
    {
        firstTime = true;
        for (int index = 0; index < a_parVars->m_total; ++index)
        {
            if (PartitionVars::NOT_TAKEN == a_parVars->m_partition[index])
            {
                Rect *curRect = &a_parVars->m_branchBuf[index].m_rect;
                Rect rect0 = CombineRect(curRect, &a_parVars->m_cover[0]);
                Rect rect1 = CombineRect(curRect, &a_parVars->m_cover[1]);
                ELEMTYPEREAL growth0 = CalcRectVolume(&rect0) - a_parVars->m_area[0];
                ELEMTYPEREAL growth1 = CalcRectVolume(&rect1) - a_parVars->m_area[1];
                ELEMTYPEREAL diff = growth1 - growth0;
                if (diff >= 0)
                {
                    group = 0;
                }
                else
                {
                    group = 1;
                    diff = -diff;
                }

                if (firstTime || diff > biggestDiff)
                {
                    firstTime = false;
                    biggestDiff = diff;
                    chosen = index;
                    betterGroup = group;
                }
                else if ((diff == biggestDiff) && (a_parVars->m_count[group] < a_parVars->m_count[betterGroup]))
                {
                    chosen = index;
                    betterGroup = group;
                }
            }
        }
        RTREE_ASSERT(!firstTime);
        Classify(chosen, betterGroup, a_parVars);
    }

    // If one group too full, put remaining rects in the other
    if ((a_parVars->m_count[0] + a_parVars->m_count[1]) < a_parVars->m_total)
    {
        if (a_parVars->m_count[0] >= a_parVars->m_total - a_parVars->m_minFill)
        {
            group = 1;
        }
        else
        {
            group = 0;
        }
        for (int index = 0; index < a_parVars->m_total; ++index)
        {
            if (PartitionVars::NOT_TAKEN == a_parVars->m_partition[index])
            {
                Classify(index, group, a_parVars);
            }
        }
    }

    RTREE_ASSERT((a_parVars->m_count[0] + a_parVars->m_count[1]) == a_parVars->m_total);
    RTREE_ASSERT((a_parVars->m_count[0] >= a_parVars->m_minFill) &&
                 (a_parVars->m_count[1] >= a_parVars->m_minFill));
}

// Copy branches from the buffer into two nodes according to the partition.
RTREE_TEMPLATE
void RTREE_QUAL::LoadNodes(Node *a_nodeA, Node *a_nodeB, PartitionVars *a_parVars)
{
    RTREE_ASSERT(a_nodeA);
    RTREE_ASSERT(a_nodeB);
    RTREE_ASSERT(a_parVars);

    for (int index = 0; index < a_parVars->m_total; ++index)
    {
        RTREE_ASSERT(a_parVars->m_partition[index] == 0 || a_parVars->m_partition[index] == 1);

        int targetNodeIndex = a_parVars->m_partition[index];
        Node *targetNodes[] = {a_nodeA, a_nodeB};

        // It is assured that AddBranch here will not cause a node split.
        bool nodeWasSplit = AddBranch(&a_parVars->m_branchBuf[index], targetNodes[targetNodeIndex], NULL);
        RTREE_ASSERT(!nodeWasSplit);
    }
}

// Initialize a PartitionVars structure.
RTREE_TEMPLATE
void RTREE_QUAL::InitParVars(PartitionVars *a_parVars, int a_maxRects, int a_minFill)
{
    RTREE_ASSERT(a_parVars);

    a_parVars->m_count[0] = a_parVars->m_count[1] = 0;
    a_parVars->m_area[0] = a_parVars->m_area[1] = (ELEMTYPEREAL)0;
    a_parVars->m_total = a_maxRects;
    a_parVars->m_minFill = a_minFill;
    for (int index = 0; index < a_maxRects; ++index)
    {
        a_parVars->m_partition[index] = PartitionVars::NOT_TAKEN;
    }
}

RTREE_TEMPLATE
void RTREE_QUAL::PickSeeds(PartitionVars *a_parVars)
{
    bool firstTime;
    int seed0 = 0, seed1 = 0;
    ELEMTYPEREAL worst, waste;
    ELEMTYPEREAL area[MAXNODES + 1];

    for (int index = 0; index < a_parVars->m_total; ++index)
    {
        area[index] = CalcRectVolume(&a_parVars->m_branchBuf[index].m_rect);
    }

    firstTime = true;
    for (int indexA = 0; indexA < a_parVars->m_total - 1; ++indexA)
    {
        for (int indexB = indexA + 1; indexB < a_parVars->m_total; ++indexB)
        {
            Rect oneRect = CombineRect(&a_parVars->m_branchBuf[indexA].m_rect, &a_parVars->m_branchBuf[indexB].m_rect);
            waste = CalcRectVolume(&oneRect) - area[indexA] - area[indexB];
            if (firstTime || waste > worst)
            {
                firstTime = false;
                worst = waste;
                seed0 = indexA;
                seed1 = indexB;
            }
        }
    }
    RTREE_ASSERT(!firstTime);

    Classify(seed0, 0, a_parVars);
    Classify(seed1, 1, a_parVars);
}

// Put a branch in one of the groups.
RTREE_TEMPLATE
void RTREE_QUAL::Classify(int a_index, int a_group, PartitionVars *a_parVars)
{
    RTREE_ASSERT(a_parVars);
    RTREE_ASSERT(PartitionVars::NOT_TAKEN == a_parVars->m_partition[a_index]);

    a_parVars->m_partition[a_index] = a_group;

    // Calculate combined rect
    if (a_parVars->m_count[a_group] == 0)
    {
        a_parVars->m_cover[a_group] = a_parVars->m_branchBuf[a_index].m_rect;
    }
    else
    {
        a_parVars->m_cover[a_group] = CombineRect(&a_parVars->m_branchBuf[a_index].m_rect, &a_parVars->m_cover[a_group]);
    }

    // Calculate volume of combined rect
    a_parVars->m_area[a_group] = CalcRectVolume(&a_parVars->m_cover[a_group]);

    ++a_parVars->m_count[a_group];
}

#undef RTREE_TEMPLATEw
#undef RTREE_QUAL

#endif // RTREE_H
