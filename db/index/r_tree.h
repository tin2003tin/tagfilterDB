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
#ifndef STORAGE_TAGFILTERDB_R_TREE_INDEX_H
#define STORAGE_TAGFILTERDB_R_TREE_INDEX_H

#include "db/index/bounding_box.h"
#include "tagfilterdb/dataView.h"
#include "tagfilterdb/status.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <stack>
#include <stddef.h>
#include <vector>

namespace tagfilterdb {
class Cache;
namespace index {

class Index {
  public:
    virtual ~Index() = default;
};

class RTree : public Index {
  public:
    struct Option {
        size_t dimension = 2;
        size_t max_child = 8;
        size_t min_child = max_child / 2;
        size_t max_page_bytes = 1024 * 8;
        long cache_charge = max_page_bytes;
        std::string filename = "rTree.tin";
        bool is_checksum = false;
    };

    RTree(Option *op, Arena *arena, Cache *cache);

    ~RTree();

    Status Insert(const BoundingBox &box, DataView *data);

    Status Remove(const BoundingBox &box, DataView *data);

    bool SearchOverlap(const BoundingBox &box,
                       bool (*callback)(BoundingBox *, DataView *));

    bool SearchUnder(const BoundingBox &box,
                     bool (*callback)(BoundingBox *, DataView *));

    bool SearchCover(const BoundingBox &box,
                     bool (*callback)(BoundingBox *, DataView *));

    void Display(std::string (*formatFunc)(DataView *));

    size_t size() const { return size_; }

  protected:
    struct Node;
    struct Branch;

    struct Node {
        int height_;
        int childSize_;
        Branch *branch_;

        Node() : height_(0), childSize_(0), branch_(nullptr) {}

        bool isLeaf() const { return height_ == 0; }
    };

    struct Branch {
        BoundingBox box_;
        Node *child_;
        DataView *data_;

        Branch() : box_(), child_(nullptr), data_(nullptr) {}
    };

    struct ListNode {
        ListNode *next_;
        Node *node_;

        ListNode() = default;
    };

    struct Group {
        BoundingBox box_;
        AreaType area_;
        int count_;

        Group() : count_(0), area_(0) {}
        ~Group() { delete[] box_.dims; }
    };

    struct GroupAssign {
        int *assign_;
        Group group_[2];

        GroupAssign() = default;
        ~GroupAssign() { delete[] assign_; }
    };

  private:
    Node *newNode();

    GroupAssign *newGroupAssign();

    Status insertBranch(Branch &branch, Node **refNode, size_t height);
    bool recursivelyInsertBranch(Branch &branch, Node *node, Node **nodeBuffer,
                                 size_t height);

    int selectBestBranch(const BoundingBox &box, Node *node);

    bool addBranch(Branch &branch, Node *node, Node **nodeBuffer);

    void splitNode(Branch &branch, Node *node, Node **nodeBuffer);

    void prepareGroup(Branch &branch, Node *node, GroupAssign *groupAssign,
                      Branch *overflowBranch, AreaType *overflowArea);

    void pickSeeds(GroupAssign *groupAssign, Branch *overflowBranch);

    void assignGroup(int index, int group, Branch *overflowBranch,
                     GroupAssign *groupAssign);

    BoundingBox nodeCover(Node *node);

    Node *getChild(Node *node, int index);

    Branch copyBranch(const Branch &s);

    void moveBranch(Branch &dest, Branch &src);

    void recursivelyDisplay(Node *node, BoundingBox *box,
                            std::string (*formatFunc)(DataView *));

    void recursivelySearchOverlap(const BoundingBox &box, Node *node,
                                  bool (*callback)(BoundingBox *, DataView *));

    void recursivelySearchUnder(const BoundingBox &box, Node *node,
                                bool (*callback)(BoundingBox *, DataView *));

    void recursivelySearchCover(const BoundingBox &box, Node *node,
                                bool (*callback)(BoundingBox *, DataView *));

    Option *option_;
    Cache *cache_;

    Node *root_;
    std::size_t size_;

    Arena *arena_;

    mutable std::shared_mutex mutex_;
};

} // namespace index
} // namespace tagfilterdb

#endif