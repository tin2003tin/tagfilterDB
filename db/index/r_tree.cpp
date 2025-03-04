// Copyright (c) 2025-present, tin2003tin, User
//   This source code is part of [TagfilterDB]
//   (https://github.com/tin2003tin/tagfilterDB)
//
/* This implementation is inspired by the N-dimensional RTree implementation
 * from the `nushoin/RTree` repository. The original implementation can be found
 * at: https://github.com/nushoin/RTree
 *
 * Credit: RTree implementation by nushoin.
 *
 * @note This code is based on the original work in the `nushoin/RTree`
 * repository.
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
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "db/index/r_tree.h"
#include "util/arena.h"

namespace tagfilterdb {
namespace index {

RTree::RTree(RTree::Option *option, Arena *arena, Cache *cache)
    : option_(option), cache_(cache), root_(nullptr), size_(0), arena_(arena),
      mutex_() {
    assert(option->dimension > 0);
    root_ = newNode();
}

RTree::~RTree() = default;

Status RTree::Insert(const BoundingBox &box, DataView *data) {
    std::unique_lock lock(mutex_);

    Branch branch;
    branch.box_.dims = box.dims;
    branch.child_ = nullptr;
    branch.data_ = data;

    return insertBranch(branch, &root_, 0);
}

void RTree::Display(std::string (*formatFunc)(DataView *)) {
    std::vector<BoundingBox::Range> vec(option_->dimension, {0, 0});
    BoundingBox box(vec);
    recursivelyDisplay(root_, &box, formatFunc);
    delete[] box.dims;
}

bool RTree::SearchOverlap(const BoundingBox &box,
                          bool (*callback)(BoundingBox *, DataView *)) {
    std::shared_lock lock(mutex_);
    if (callback == nullptr) {
        return false;
    }

    recursivelySearchOverlap(box, root_, callback);
    return true;
}

bool RTree::SearchUnder(const BoundingBox &box,
                        bool (*callback)(BoundingBox *, DataView *)) {
    std::shared_lock lock(mutex_);
    if (callback == nullptr) {
        return false;
    }

    recursivelySearchUnder(box, root_, callback);
    return true;
}

bool RTree::SearchCover(const BoundingBox &box,
                        bool (*callback)(BoundingBox *, DataView *)) {
    std::shared_lock lock(mutex_);
    if (callback == nullptr) {
        return false;
    }

    recursivelySearchCover(box, root_, callback);
    return true;
}

RTree::Node *RTree::newNode() {
    char *node_memory = arena_->AllocateAligned(sizeof(Node));
    Node *node = new (node_memory) Node;
    if (arena_ && option_->max_child > 0) {
        node->branch_ =
            (Branch *)arena_->Allocate(option_->max_child * sizeof(Branch));
    }
    for (int i = 0; i < option_->max_child; i++) {
        node->branch_[i].box_.dims = nullptr;
        node->branch_[i].child_ = nullptr;
        node->branch_[i].data_ = nullptr;
    }
    return node;
}

RTree::GroupAssign *RTree::newGroupAssign() {
    GroupAssign *groupAssign = new GroupAssign;
    groupAssign->assign_ = new int[option_->max_child + 1];
    for (int i = 0; i < option_->max_child + 1; i++) {
        groupAssign->assign_[i] = -1;
    }
    return groupAssign;
}

bool RTree::recursivelyInsertBranch(Branch &branch, Node *node,
                                    Node **nodeBuffer, size_t height) {
    assert(node);

    if (node->height_ == height || node->isLeaf()) {
        return addBranch(branch, node, nodeBuffer);
    }

    int index = selectBestBranch(branch.box_, node);

    bool splitted = recursivelyInsertBranch(branch, getChild(node, index),
                                            nodeBuffer, height);

    if (splitted) {
        auto coverBox1 = nodeCover(getChild(node, index));
        BoundingBoxMgr::alignTo(&coverBox1, arena_, option_->dimension);
        node->branch_[index].box_.dims = coverBox1.dims;
        Branch newBranch;
        newBranch.child_ = (*nodeBuffer);
        newBranch.box_ = nodeCover(*nodeBuffer);
        BoundingBoxMgr::alignTo(&newBranch.box_, arena_, option_->dimension);

        return addBranch(newBranch, node, nodeBuffer);
    } else {
        // Update the bounding box if no split occurred
        auto newBox = BoundingBoxMgr::Union(node->branch_[index].box_,
                                            branch.box_, option_->dimension);
        node->branch_[index].box_.dims = newBox.dims;

        BoundingBoxMgr::alignTo(&node->branch_[index].box_, arena_,
                                option_->dimension);
        return false;
    }
    return false;
}

Status RTree::insertBranch(Branch &branch, Node **refNode, size_t height) {
    assert(refNode);
    assert(height >= 0 && height <= (*refNode)->height_);

    size_++;
    Node *nodeBuffer;
    bool splitted =
        recursivelyInsertBranch(branch, *refNode, &nodeBuffer, height);

    if (splitted) {
        Node *newRoot = newNode();
        newRoot->height_ = (*refNode)->height_ + 1;

        Branch branch1;
        auto nC1 = nodeCover(*refNode);
        BoundingBoxMgr::move(nC1, branch1.box_);
        BoundingBoxMgr::alignTo(&branch1.box_, arena_, option_->dimension);
        branch1.child_ = *refNode;
        addBranch(branch1, newRoot, &nodeBuffer);

        Branch branch2;
        auto nC2 = nodeCover(nodeBuffer);
        BoundingBoxMgr::move(nC2, branch2.box_);
        BoundingBoxMgr::alignTo(&branch2.box_, arena_, option_->dimension);
        branch2.child_ = nodeBuffer;
        addBranch(branch2, newRoot, &nodeBuffer);

        *refNode = newRoot;
    }
    return Status::OK();
}

int RTree::selectBestBranch(const BoundingBox &box, Node *node) {
    int bestIndex = 1;
    AreaType bestIncr = std::numeric_limits<AreaType>::max();
    AreaType bestArea = std::numeric_limits<AreaType>::max();

    for (int i = 0; i < node->childSize_; ++i) {
        BoundingBox unionBox = BoundingBoxMgr::Union(box, node->branch_[i].box_,
                                                     option_->dimension);
        AreaType area =
            BoundingBoxMgr::Area(node->branch_[i].box_, option_->dimension);
        AreaType increase =
            BoundingBoxMgr::Area(unionBox, option_->dimension) - area;
        delete[] unionBox.dims;

        if (increase < bestIncr || (increase == bestIncr && area < bestArea)) {
            bestIndex = i;
            bestIncr = increase;
            bestArea = area;
        }
    }
    return bestIndex;
}

bool RTree::addBranch(Branch &branch, Node *node, Node **nodeBuffer) {
    assert(node);

    if (node->childSize_ < option_->max_child) {
        node->branch_[node->childSize_++] = copyBranch(branch);
        return false;
    } else {
        splitNode(branch, node, nodeBuffer);
        return true;
    }
}

void RTree::splitNode(Branch &branch, Node *node, Node **nodeBuffer) {
    assert(node);
    assert(node->childSize_ == option_->max_child);

    GroupAssign *groupAssign = newGroupAssign();
    Branch overflowBranch[option_->max_child + 1];
    AreaType overflowArea[option_->max_child + 1];
    prepareGroup(branch, node, groupAssign, overflowBranch, overflowArea);

    pickSeeds(groupAssign, overflowBranch);

    bool firstTime;
    AreaType biggestDiff;
    int group, chosen = 0, betterGroup = 0;

    // Assign remaining Branches to groups based on growth criteria
    while ((groupAssign->group_[0].count_ + groupAssign->group_[1].count_ <
            option_->max_child + 1) &&
           (groupAssign->group_[0].count_ <
            (option_->max_child + 1 - option_->min_child)) &&
           (groupAssign->group_[1].count_ <
            (option_->max_child + 1 - option_->min_child))) {

        bool firstTime = true;
        for (int index = 0; index < option_->max_child + 1; ++index) {
            if (groupAssign->assign_[index] == -1) {
                // Compute growth of bounding boxes if the current Branch is
                // added to each group
                auto box0 = BoundingBoxMgr::Union(overflowBranch[index].box_,
                                                  groupAssign->group_[0].box_,
                                                  option_->dimension);
                auto box1 = BoundingBoxMgr::Union(overflowBranch[index].box_,
                                                  groupAssign->group_[1].box_,
                                                  option_->dimension);
                AreaType growth0 =
                    BoundingBoxMgr::Area(box0, option_->dimension) -
                    BoundingBoxMgr::Area(groupAssign->group_[0].box_,
                                         option_->dimension);
                AreaType growth1 =
                    BoundingBoxMgr::Area(box1, option_->dimension) -
                    BoundingBoxMgr::Area(groupAssign->group_[1].box_,
                                         option_->dimension);
                AreaType diff = growth1 - growth0;

                delete[] box0.dims;
                delete[] box1.dims;
                // Determine which group would be better for the current Branch
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
                           (groupAssign->group_[group].count_ <
                            groupAssign->group_[betterGroup].count_)) {
                    chosen = index;
                    betterGroup = group;
                }
            }
        }

        // Ensure we found a valid choice
        assert(!firstTime);
        // Assign the chosen Branch to the better group
        assignGroup(chosen, betterGroup, overflowBranch, groupAssign);
    }

    // Final adjustments if the total number of nodes is less than expected
    if ((groupAssign->group_[0].count_ + groupAssign->group_[1].count_) <
        option_->max_child + 1) {
        // Determine which group to assign remaining nodes to
        if (groupAssign->group_[0].count_ >=
            option_->max_child + 1 - option_->min_child) {
            group = 1;
        } else {
            group = 0;
        }
        // Assign remaining nodes to the determined group
        for (int i = 0; i < option_->max_child + 1; i++) {
            if (groupAssign->assign_[i] == -1) {
                assignGroup(i, group, overflowBranch, groupAssign);
            }
        }
    }

    // Ensure valid group assignments
    assert((groupAssign->group_[0].count_ + groupAssign->group_[1].count_) ==
           option_->max_child + 1);
    assert(groupAssign->group_[0].count_ >= option_->min_child);
    assert(groupAssign->group_[1].count_ >= option_->min_child);

    // Clear the current node and create a new node for one group
    node->childSize_ = 0;
    *nodeBuffer = newNode();
    (*nodeBuffer)->height_ = node->height_;
    Node *targetNodes[] = {node,
                           *nodeBuffer}; // Nodes to receive the split Branches

    for (int index = 0; index < option_->max_child + 1; index++) {
        int groupAssignValue = groupAssign->assign_[index];
        assert(groupAssignValue == 0 || groupAssignValue == 1);

        // Add each Branch to the appropriate group node
        bool splitted = addBranch(overflowBranch[index],
                                  targetNodes[groupAssignValue], nodeBuffer);

        assert(!splitted);
    }

    // Ensure the split was performed correctly
    assert((node->childSize_ + (*nodeBuffer)->childSize_) ==
           option_->max_child + 1);

    delete groupAssign;
}

void RTree::prepareGroup(Branch &branch, Node *node, GroupAssign *groupAssign,
                         Branch *overflowBranch, AreaType *overflowArea) {
    assert(node);
    assert(node->childSize_ == option_->max_child);

    for (int i = 0; i < option_->max_child; i++) {
        moveBranch(node->branch_[i], overflowBranch[i]);
    }
    overflowBranch[option_->max_child] = copyBranch(branch);

    auto coverBox =
        BoundingBoxMgr::copy(overflowBranch[0].box_, option_->dimension);
    for (int index = 1; index < option_->max_child + 1; ++index) {
        auto unionBox = BoundingBoxMgr::Union(
            coverBox, overflowBranch[index].box_, option_->dimension);
        BoundingBoxMgr::move(unionBox, coverBox);
    }
    *overflowArea = BoundingBoxMgr::Area(coverBox, option_->dimension);
    delete[] coverBox.dims;
}

void RTree::pickSeeds(GroupAssign *groupAssign, Branch *overflowBranch) {
    bool firstTime;
    int seed0 = 0, seed1 = 0;
    AreaType worst, waste;
    AreaType area[option_->max_child + 1];

    for (int index = 0; index < option_->max_child + 1; ++index) {
        area[index] = BoundingBoxMgr::Area(overflowBranch[index].box_,
                                           option_->dimension);
    }

    firstTime = true;
    for (int indexA = 0; indexA < option_->max_child - 1; ++indexA) {
        for (int indexB = indexA + 1; indexB < option_->max_child; ++indexB) {
            auto unionBox = BoundingBoxMgr::Union(overflowBranch[indexA].box_,
                                                  overflowBranch[indexB].box_,
                                                  option_->dimension);
            waste = BoundingBoxMgr::Area(unionBox, option_->dimension) -
                    area[indexA] - area[indexB];
            delete[] unionBox.dims;
            if (firstTime || waste > worst) {
                firstTime = false;
                worst = waste;
                seed0 = indexA;
                seed1 = indexB;
            }
        }
    }
    assert(!firstTime);

    assignGroup(seed0, 0, overflowBranch, groupAssign);
    assignGroup(seed1, 1, overflowBranch, groupAssign);
}

void RTree::assignGroup(int index, int group, Branch *overflowBranch,
                        GroupAssign *groupAssign) {
    assert(index < option_->max_child + 1);
    assert(group < 2);
    assert(groupAssign->assign_[index] == -1);

    // Assign the Branch to the specified group
    groupAssign->assign_[index] = group;

    // Update the bounding box for the group
    if (groupAssign->group_[group].count_ == 0) {
        // If this is the first Branch in the group, initialize the bounding
        // box
        groupAssign->group_[group].box_ = BoundingBoxMgr::copy(
            overflowBranch[index].box_, option_->dimension);
    } else {
        // Otherwise, expand the existing bounding box to include the new
        // Branch
        auto unionBox = BoundingBoxMgr::Union(groupAssign->group_[group].box_,
                                              overflowBranch[index].box_,
                                              option_->dimension);
        BoundingBoxMgr::move(unionBox, groupAssign->group_[group].box_);
    }

    groupAssign->group_[group].area_ = BoundingBoxMgr::Area(
        groupAssign->group_[group].box_, option_->dimension);
    // Increment the count of Branches in the group
    groupAssign->group_[group].count_++;
}

BoundingBox RTree::nodeCover(Node *node) {
    assert(node);
    auto box = BoundingBoxMgr::copy(node->branch_[0].box_, option_->dimension);
    for (int index = 1; index < node->childSize_; index++) {
        auto unionBox = BoundingBoxMgr::Union(box, node->branch_[index].box_,
                                              option_->dimension);
        BoundingBoxMgr::move(unionBox, box);
    }
    return box;
}

RTree::Node *RTree::getChild(Node *node, int index) {
    if (node->isLeaf()) {
        return nullptr;
    }
    // TODO: caching nodePage
    return node->branch_[index].child_;
}

RTree::Branch RTree::copyBranch(const Branch &s) {
    Branch ss;
    ss.child_ = s.child_;
    ss.data_ = s.data_;
    ss.box_ = BoundingBoxMgr::copy(s.box_, option_->dimension);
    BoundingBoxMgr::alignTo(&ss.box_, arena_, option_->dimension);
    return ss;
}

void RTree::moveBranch(Branch &src, Branch &dest) {
    dest.box_.dims = src.box_.dims;
    dest.child_ = src.child_;
    dest.data_ = src.data_;
    src.box_.dims = nullptr;
    src.data_ = nullptr;
    src.child_ = nullptr;
}

void RTree::recursivelyDisplay(Node *node, BoundingBox *box,
                               std::string (*formatFunc)(DataView *)) {
    if (node == nullptr) {
        return;
    }

    for (int i = 0; i < node->childSize_; i++) {
        std::cout << node->height_ << " "
                  << BoundingBoxMgr::toString(*box, option_->dimension)
                  << " -> ";
        if (node->isLeaf()) {
            std::cout << formatFunc(node->branch_[i].data_);
        }

        std::cout << BoundingBoxMgr::toString(node->branch_[i].box_,
                                              option_->dimension)
                  << std::endl;
        // Recursively print for child nodes
        if (node->height_ != 0) {
            Branch *branch = &node->branch_[i];
            Node *child = getChild(node, i);
            recursivelyDisplay(child, &node->branch_[i].box_, formatFunc);
        }
    }
}

void RTree::recursivelySearchOverlap(const BoundingBox &box, Node *node,
                                     bool (*callback)(BoundingBox *,
                                                      DataView *)) {
    if (node == nullptr) {
        return;
    }
    for (int i = 0; i < node->childSize_; i++) {
        if (BoundingBoxMgr::IsOverlap(node->branch_[i].box_, box,
                                      option_->dimension)) {
            if (node->isLeaf()) {
                if (!callback(&node->branch_[i].box_, node->branch_[i].data_)) {
                    break;
                }
            } else {
                recursivelySearchOverlap(box, getChild(node, i), callback);
            }
        }
    }
}

void RTree::recursivelySearchUnder(const BoundingBox &box, Node *node,
                                   bool (*callback)(BoundingBox *,
                                                    DataView *)) {
    if (node == nullptr) {
        return;
    }

    for (int i = 0; i < node->childSize_; i++) {
        if (BoundingBoxMgr::ContainsRange(node->branch_[i].box_, box,
                                          option_->dimension)) {
            if (node->isLeaf()) {
                if (!callback(&node->branch_[i].box_, node->branch_[i].data_)) {
                    break;
                }
            } else {
                recursivelySearchUnder(box, getChild(node, i), callback);
            }
        }
    }
}

void RTree::recursivelySearchCover(const BoundingBox &box, Node *node,
                                   bool (*callback)(BoundingBox *,
                                                    DataView *)) {
    if (node == nullptr) {
        return;
    }
    for (int i = 0; i < node->childSize_; i++) {
        if (node->isLeaf() &&
            BoundingBoxMgr::ContainsRange(box, node->branch_[i].box_,
                                          option_->dimension)) {
            if (!callback(&node->branch_[i].box_, node->branch_[i].data_)) {
                break;
            }
        }

        if (!node->isLeaf() &&
            BoundingBoxMgr::IsOverlap(node->branch_[i].box_, box,
                                      option_->dimension)) {
            recursivelySearchCover(box, getChild(node, i), callback);
        }
    }
}

} // namespace index
} // namespace tagfilterdb