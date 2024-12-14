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
#include "page.h"
#include "dataView.h"

#include <algorithm>
#include <vector>
#include <cassert>
#include <limits>
#include <stddef.h>
#include <iostream>
#include <queue>
#include <stack>
#include <mutex>
#include <shared_mutex>

// #define SPI_MOVE_COUNT

namespace tagfilterdb {

struct SpatialIndexOptions {
    size_t DIMENSION = 2;
    size_t MAX_CHILD = 8; 
    size_t MIN_CHILD = MAX_CHILD / 2;
    size_t PAGE_MAX_BYTES = 4096;
};

struct SpICallBackValue {
    const BBManager::BB* box;
    const SignableData* data;
};

class SpICallBack {
  public:
    #if defined(SPI_MOVE_COUNT)
    virtual bool Move() = 0;
    #endif 
    virtual bool Process(SpICallBackValue value) = 0;
};

class SpatialIndex {
    using BB = BBManager::BB;
    using RangeType = BBManager::RangeType;
    using AreaType = BBManager::AreaType;

    protected:
    struct Node;
    struct Branch;
    struct Group;
    struct GroupAssign;
    struct ListNode;

    struct Node {
        int height_; 
        int childSize_;

        long page_;
        int offset_;

        Branch* branch_;

        Node() : height_(-1), branch_(nullptr), childSize_(0), page_(-1), offset_(-1) {}

        Node(int height , SpatialIndexOptions &op, Arena *arena)
        : height_(height), branch_(nullptr), childSize_(0), page_(-1), offset_(-1) {
            if (arena && op.MAX_CHILD > 0) {
                branch_ = (Branch *)arena->AllocateAligned(op.MAX_CHILD * sizeof(Branch));
            }
        }

        bool isLeaf() const { return height_ == 0; }
    };

    struct ListNode
    {
        ListNode* next_;                             ///< Next in list
        Node* node_;                                 ///< Node
    };

    struct Branch {
        BB box_;          ///< Bounding box of the branch.
        Node *child_;     ///< Pointer to child node, if any.
        SignableData* data_;      ///< Data associated with the branch.

        Branch() = default ;

        Branch(BBManager* bbm_) : box_(bbm_->CreateBox()), child_(nullptr),    
                                    data_(nullptr) {}

        void CreateBox(BBManager* bbm_) {
            if (box_.dims_ == nullptr) {
                box_ = bbm_->CreateBox();
            }
            bbm_->Align(box_);
        }

        static Branch Copy(const Branch& s, BBManager* bbm_) {
            Branch ss(bbm_);
            ss.child_ = s.child_;
            ss.data_ = s.data_;
            ss.box_ = bbm_->Copy(s.box_);
            return ss;
        }

        static void Move(Branch& dest, Branch& src) {
            dest.box_ = std::move(src.box_);
            dest.child_ = src.child_;
            dest.data_ = src.data_;
        }
    };

    struct Group {
        BB box_;           ///< Bounding box of the group.
        int count_;        ///< Number of nodes in the group.
        Group() : count_(0) {}

        ~Group() {
            box_.Destroy();
        }
    };

    struct GroupAssign {
        std::vector<size_t> assign_;      ///< Array for group assignments.
        Group group_[2];                      ///< Two groups used for splitting.

        GroupAssign(SpatialIndexOptions &op, BBManager* bbm_) {
            for (int i = 0; i < op.MAX_CHILD + 1; ++i) {
                assign_.push_back(-1);
            }
        }
    };

    public: 
    SpatialIndex(SpatialIndexOptions &op, Arena* arena) : op_(op), arena_(arena), bbm_(BBManager(op.DIMENSION,arena)) {
        root_ = newNode(0); 
        size_ = 0; 
    }

    bool Insert(const BB &box, SignableData* data) {
        std::unique_lock lock(mutex_);
        Branch tempBranch;
        tempBranch.box_ = bbm_.Copy(box);
        bbm_.Align(tempBranch.box_);
        tempBranch.child_ = nullptr;
        tempBranch.data_ = data;

        insertBranch(tempBranch, &root_, 0);
             
        return true;  
    }

    bool Remove(const BB &box, SignableData* data) {
        std::unique_lock lock(mutex_);
        return removeBranch(box,data,&root_);
    }

    void SearchOverlap(const BB &box, SpICallBack* callback) {
        std::shared_lock lock(mutex_);
        if (callback == nullptr) {
            return;
        }

        recursivelySearchOverlap(box, root_, callback);
    }

    void SearchUnder(BB &box, SpICallBack* callback) {
        std::shared_lock lock(mutex_);
        if (callback == nullptr) {
            return;
        }

        recursivelySearchUnder(box, root_, callback);
    }

    void SearchCover(BB &box, SpICallBack* callback) {
        std::shared_lock lock(mutex_);
        if (callback == nullptr) {
            return;
        }

        recursivelySearchCover(box, root_, callback);
    }

    void Print(std::string (*formatFunc)(SignableData*)) {
        BB box = bbm_.CreateBox();
        recursivelyPrint(root_, &box, formatFunc);
    }


    void Save() {
        std::unique_lock lock(mutex_);
        tagfilterdb::PageNodeManager pageManger(op_.PAGE_MAX_BYTES, getNodeSize());
        flush(&pageManger);  // Call flush to fill the page data

        pageManger.PrintPageInfo();

        Node node(0, op_, arena_); 
        for (int i = 0 ; i < pageManger.Size(); i++) {
            int offset = 0;  
            PageNode* page = pageManger.getPage(i + 1);

            std::cout << "=================== Page " << page->GetPageId() 
                      << " ===================" <<std::endl;
            
            while (offset < (op_.PAGE_MAX_BYTES/ getNodeSize()) * getNodeSize()) {
                std::cout << "Current Offset: " << offset << std::endl;
                // Deserialize the node properties from the page
                std::memcpy(&node.height_, page->GetData(offset), sizeof(node.height_));
                offset += sizeof(node.height_);

                std::memcpy(&node.childSize_, page->GetData(offset), sizeof(node.childSize_));
                offset += sizeof(node.childSize_);

                std::memcpy(&node.page_, page->GetData(offset), sizeof(node.page_));
                offset += sizeof(node.page_);

                std::memcpy(&node.offset_, page->GetData(offset), sizeof(node.offset_));
                offset += sizeof(node.offset_);

                std::vector<std::pair<int,int>> childOffset(node.childSize_);
                for (int i = 0; i < node.childSize_; i++) {
                    node.branch_[i].box_ = bbm_.CreateBox();
                    for (int d = 0; d < op_.DIMENSION; d++) {
                        std::memcpy(&node.branch_[i].box_.dims_[d].first, page->GetData(offset), sizeof(node.branch_[i].box_.dims_[d].first));
                        offset += sizeof(node.branch_[i].box_.dims_[d].first);
                        
                        std::memcpy(&node.branch_[i].box_.dims_[d].second, page->GetData(offset), sizeof(node.branch_[i].box_.dims_[d].second));
                        offset += sizeof(node.branch_[i].box_.dims_[d].second);
                    }

                    std::memcpy(&childOffset[i].first, page->GetData(offset), sizeof(long));
                    offset += sizeof(long);
                    std::memcpy(&childOffset[i].second, page->GetData(offset), sizeof(int));
                    offset += sizeof(int);
                }

                // Fill empty space if the node has fewer children than the maximum
                for (int i = node.childSize_ + 1; i <= op_.MAX_CHILD; i++) {
                    offset += sizeof(RangeType) * 2 * op_.DIMENSION;
                    offset += sizeof(long) + sizeof(int);
                }

                // Print the deserialized node's values
                std::cout << "Node - Height: " << node.height_ << ", CSize: " << node.childSize_ 
                        << ", PageID: " << node.page_ << ", Offset: " << node.offset_ << std::endl;

                // Print the bounding boxes for each branch
                for (int i = 0; i < node.childSize_; i++) {
                    std::cout << bbm_.toString(node.branch_[i].box_) << " PageID: " 
                        << childOffset[i].first << " ,Offset: " <<  childOffset[i].second << std::endl;
                }
            }

        }
    }

    size_t totalNode() {
        return recursivelyTotalNode(root_);
    }

    size_t getNodeSize() {
        return 20 + (sizeof(long) * 2 * op_.DIMENSION + 12) * op_.MAX_CHILD;
    }

   void flush(PageNodeManager* pageManager) { 
        int node_size = getNodeSize();
        char bufferNode[node_size]; 

        auto p = pageManager->Assign(1);
        root_->page_ = p.first;
        root_->offset_ = p.second;

        std::queue<Node*> q;
        q.push(root_);  // Start with the root node

        while (!q.empty()) {
            int offset = 0; 
            Node* node = q.front();
            q.pop();

            // Serialize node properties into the page buffer at the current offset
            std::memcpy(bufferNode + offset, &node->height_, sizeof(node->height_));
            offset += sizeof(node->height_);
            
            std::memcpy(bufferNode + offset, &node->childSize_, sizeof(node->childSize_));
            offset += sizeof(node->childSize_);

            std::memcpy(bufferNode + offset, &node->page_, sizeof(node->page_));
            offset += sizeof(node->page_);

            std::memcpy(bufferNode + offset, &node->offset_, sizeof(node->offset_));
            offset += sizeof(node->offset_);

            // Iterate through the branches of the node and serialize the bounding boxes
            for (int i = 0; i < node->childSize_; i++) {
                for (int j = 0; j < op_.DIMENSION; j++) {
                    std::memcpy(bufferNode + offset, &node->branch_[i].box_.dims_[j].first, sizeof(RangeType));
                    offset += sizeof(RangeType);

                    std::memcpy(bufferNode + offset, &node->branch_[i].box_.dims_[j].second, sizeof(RangeType));
                    offset += sizeof(RangeType);
                }

                if (node->branch_[i].child_ != nullptr) {
                    auto a = pageManager->Assign(node->page_);
                    node->branch_[i].child_->page_ = a.first;
                    node->branch_[i].child_->offset_ = a.second; 
                    q.push(node->branch_[i].child_);

                    std::memcpy(bufferNode + offset, &node->branch_[i].child_->page_, sizeof(long));
                    offset += sizeof(node->branch_[i].child_->page_);

                    std::memcpy(bufferNode + offset, &node->branch_[i].child_->offset_, sizeof(int));
                    offset += sizeof(node->branch_[i].child_->offset_);
                } else {
                    std::memset(bufferNode + offset, -1, sizeof(long));
                    offset += sizeof(node->branch_[i].child_->page_);

                    std::memset(bufferNode + offset, -1, sizeof(int));
                    offset += sizeof(node->branch_[i].child_->offset_);
                }
            }
            // Fill empty space if the node has fewer children than the maximum
            for (int i = node->childSize_ + 1; i <= op_.MAX_CHILD; i++) {
                std::memset(bufferNode + offset, 0, sizeof(RangeType) * 2 * op_.DIMENSION);
                offset += sizeof(RangeType) * 2 * op_.DIMENSION;

                std::memset(bufferNode + offset, 0, sizeof(long) + sizeof(int));
                offset += sizeof(long) + sizeof(int);
            }

        PageNode* p = pageManager->getPage(node->page_);
        p->SetData(node->offset_, bufferNode);
        }
    }


    size_t Size() const {
        return size_;
    }

    int Height() const {
        return root_->height_;
    }

    BBManager* GetBBManager() {
        return &bbm_;
    }

    private:
    Node* newNode(size_t height = -1) {
        char* node_memory = arena_->AllocateAligned(sizeof(Node));
        Node* node = new (node_memory) Node(height, op_, arena_);
        return node;
    }

    bool insertBranch(Branch &branch, Node **refNode, size_t height) {
        assert(refNode);
        assert(height >= 0 && height <= (*refNode)->height_);
        
        size_++;    
        Node* nodeBuffer;
        bool splited = recursivelyInsertBranch(branch, *refNode, &nodeBuffer, height); 

        if (splited) {
        Node *newRoot = newNode((*refNode)->height_ + 1);
        Branch tempBranch1(&bbm_);
        BB nC1 = nodeCover(*refNode);
        bbm_.Move(tempBranch1.box_, nC1);
        bbm_.Align(tempBranch1.box_);
        tempBranch1.child_ = *refNode;
        addBranch(tempBranch1, newRoot, &nodeBuffer);

        Branch tempBranch2(&bbm_);
        BB nC2 = nodeCover(*refNode);
        bbm_.Move(tempBranch2.box_, nC2);
        bbm_.Align(tempBranch2.box_);
        tempBranch2.child_ = nodeBuffer;
        addBranch(tempBranch2, newRoot,&nodeBuffer);

        *refNode = newRoot; 

        return true;
        }
        return false;
    }

    bool removeBranch(const BB &box, SignableData* data , Node **refNode) {
        assert(*refNode);
        ListNode* reInsertList = NULL;
        if (!recursivelyRemove(box,data, *refNode, &reInsertList)) {
            while (reInsertList != nullptr)
            {
                Node* tempNode = reInsertList->node_;
                for (int index = 0; index < tempNode->childSize_; index++) {
                    insertBranch(tempNode->branch_[index], refNode,tempNode->height_);
                }
                ListNode* remLNode = reInsertList;
                reInsertList = reInsertList->next_;
                delete remLNode;
            }
            if((*refNode)->childSize_ == 1 && !(*refNode)->isLeaf())
            {
                Node* tempNode = (*refNode)->branch_[0].child_;

                assert(tempNode);
                *refNode = tempNode;
            }
            return false;
        } else
        {
            return true;
        }
        
    }

    bool recursivelyInsertBranch(Branch &branch, Node *node, Node** nodeBuffer, size_t height) {
        assert(node); 

        if (node->height_ == height) {
            return addBranch(branch,node,nodeBuffer);
        }

        if (node->isLeaf()) {
            return addBranch(branch, node, nodeBuffer); 
        }
        int index = selectBestBranch(branch.box_,
                                    node); 
      
        bool childSplited = recursivelyInsertBranch(
            branch, node->branch_[index].child_, nodeBuffer,height); 
        if (childSplited) {
            BB coverBox1 = nodeCover(node->branch_[index].child_);
            bbm_.Align(coverBox1);
            node->branch_[index].box_.dims_ = coverBox1.dims_;
            Branch tempBranch(&bbm_);
            tempBranch.child_ = (*nodeBuffer);
            BB coverBox2 = nodeCover(node->branch_[index].child_);
             bbm_.Align(coverBox2);
            bbm_.Move(tempBranch.box_, coverBox2);

            return addBranch(tempBranch, node,nodeBuffer);
        } else {
            // Update the bounding box if no split occurred
            BB newBox = bbm_.Union(node->branch_[index].box_, branch.box_);
            bbm_.Align(newBox);
            node->branch_[index].box_.dims_ = newBox.dims_;
            return false; // Indicate no split
        }
        return false; 
    }

    bool recursivelyRemove(const BB& box, const SignableData* data, Node* node, ListNode** listNode) {
        assert(node && listNode);
        assert(node->height_ >= 0);

        if (node->isLeaf()) {
            for (size_t index = 0; index < node->childSize_; index++) {
                if ((node->branch_[index].data_->data) == (data->data)) {
                        deleteBranch(node,index);
                        return false;
                }
            }
            return true;
        } else {
            for(int index = 0; index < node->childSize_; ++index) {
                if (bbm_.IsOverlap(box,node->branch_[index].box_)) {
                    if (!recursivelyRemove(box,data,node->branch_[index].child_,listNode)) {
                        if (node->branch_[index].child_->childSize_ >= op_.MIN_CHILD) {
                            BB coverBox = nodeCover(node->branch_[index].child_);
                            bbm_.Align(coverBox);
                            node->branch_[index].box_.dims_ = coverBox.dims_; 
                        } else {
                            addListNode(node->branch_[index].child_,listNode);
                            deleteBranch(node, index);
                        }
                    }
                    return false;
                }  
            }
            return true;
        }
    }

    bool addBranch(Branch &branch, Node *node,Node** nodeBuffer) {
         assert(node);

        if (node->childSize_ < op_.MAX_CHILD) {
                Branch::Move(node->branch_[node->childSize_++], branch);
                return false;  // Indicate no split
            } else {
                // Need to split the node
                splitNode(branch, node,nodeBuffer);
                return true; // Indicate split occurred
            }
    }

    void deleteBranch(Node* node, int index) {
        assert(node && (index >= 0) && (index < op_.MAX_CHILD));
        assert(node->childSize_ > 0);

        node->branch_[index].box_.dims_ = node->branch_[node->childSize_ - 1].box_.dims_;
        node->branch_[index].child_ = node->branch_[node->childSize_ - 1].child_;
        node->branch_[index].data_ = node->branch_[node->childSize_ - 1].data_;

        size_--;
        node->childSize_--;
    }

    BB nodeCover(Node *node) {
        assert(node);
        BB box = bbm_.Copy(node->branch_[0].box_);
        for (int index = 1; index < node->childSize_; index++) {
            BB uBox = bbm_.Union(box, node->branch_[index].box_);
            bbm_.Move(box, uBox);
        }
        return box; 
    }

    int selectBestBranch(const BB &box, Node *node) {
        int bestIndex = -1;
        BBManager::AreaType bestIncr = std::numeric_limits<BBManager::AreaType>::max();
        BBManager::AreaType bestArea = std::numeric_limits<BBManager::AreaType>::max();

        for (int index = 0; index < node->childSize_; ++index) {
            BB box = bbm_.Union(box,node->branch_[index].box_);
            BBManager::AreaType area = bbm_.Area(node->branch_[index]
                                .box_); // Area of the current tempBranch's box
            BBManager::AreaType increase =
                bbm_.Area(box) - area; // Increase in area due to the new box
            box.Destroy();
            // Update best index based on area increase
            if (increase < bestIncr || (increase == bestIncr && area < bestArea)) {
                bestIndex = index;
                bestIncr = increase;
                bestArea = area;
            }
        }
        return bestIndex;
    }

    void splitNode(Branch &branch, Node *node, Node** nodeBuffer) {
        assert(node);
        assert(node->childSize_ == op_.MAX_CHILD);
        bool firstTime;

        GroupAssign groupAssign(op_, &bbm_); 

        Branch overflowBuffer[op_.MAX_CHILD + 1];

        AreaType overflowBufferArea[op_.MAX_CHILD + 1];

        BB box = bbm_.Copy(
            node->branch_[0].box_);

        // Copy existing Branchs and the new Branch into overflowBuffer
        for (int i_tempBranch = 0; i_tempBranch < op_.MAX_CHILD; i_tempBranch++) {
            Branch::Move(overflowBuffer[i_tempBranch], node->branch_[i_tempBranch]);
            BB uBox = bbm_.Union(box, overflowBuffer[i_tempBranch].box_);
            bbm_.Move(box, uBox);
            overflowBufferArea[i_tempBranch] =
                bbm_.Area(overflowBuffer[i_tempBranch].box_);
        }
        Branch::Move(overflowBuffer[op_.MAX_CHILD], branch);
        BB uBox = bbm_.Union(box, branch.box_); // Update the bounding box with the new Branch
        bbm_.Move(box, uBox);
        overflowBufferArea[op_.MAX_CHILD] = bbm_.Area(branch.box_); // Update the area of the new Branch
        box.Destroy();

        int seed0 = 0, seed1 = 0; // Indices of the seeds for splitting
        RangeType bestNormalizedSeparation =
            -std::numeric_limits<RangeType>::infinity(); // Initialize best
        
        AreaType worst, waste;

        firstTime = true;
        for(int indexA=0; indexA < op_.MAX_CHILD-1; ++indexA)
        {
            for(int indexB = indexA+1; indexB < op_.MAX_CHILD; ++indexB)
            {
            BB tempBox = bbm_.Union(overflowBuffer[indexA].box_, overflowBuffer[indexB].box_);
            waste = bbm_.Area(tempBox) - overflowBufferArea[indexA] - overflowBufferArea[indexB];
            tempBox.Destroy();
            if(firstTime || waste > worst)
            {
                firstTime = false;
                worst = waste;
                seed0 = indexA;
                seed1 = indexB;
            }
            }
        }                                        

        // Initialize group assignments with the chosen seeds
        assignGroup(seed0, 0, overflowBuffer[seed0].box_, groupAssign);
        assignGroup(seed1, 1, overflowBuffer[seed1].box_, groupAssign);

        firstTime = true;       // Flag for first iteration
        int chosen = -1;       // Index of the chosen Branch for assignment
        int betterGroup = -1;  // Index of the better group for assignment
        int group;             // Current group being considered
        AreaType biggestDiff;  // Biggest difference in growth between groups

        // Assign remaining Branchs to groups based on growth criteria
        while (
            (groupAssign.group_[0].count_ + groupAssign.group_[1].count_ <
            groupAssign.assign_.size()) &&
            (groupAssign.group_[0].count_ <
            (groupAssign.assign_.size() - op_.MIN_CHILD)) &&
            (groupAssign.group_[1].count_ <
            (groupAssign.assign_.size() - op_.MIN_CHILD))) {

            bool firstTime = true;
            for (int index = 0; index < groupAssign.assign_.size(); ++index) {
                if (groupAssign.assign_[index] == -1) {
                    // Compute growth of bounding boxes if the current Branch is
                    // added to each group
                    BB box0 = bbm_.Union(overflowBuffer[index].box_,
                                            groupAssign.group_[0].box_);
                    BB box1 = bbm_.Union(overflowBuffer[index].box_,
                                            groupAssign.group_[1].box_);
                    AreaType growth0 =
                        bbm_.Area(box0) -  bbm_.Area(groupAssign.group_[0].box_);
                    AreaType growth1 =
                         bbm_.Area(box1) -  bbm_.Area(groupAssign.group_[1].box_);
                    AreaType diff = growth1 - growth0;
                    box0.Destroy(); box1.Destroy();
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
                            (groupAssign.group_[group].count_ <
                                groupAssign.group_[betterGroup].count_)) {
                        chosen = index;
                        betterGroup = group;
                    }
                }
            }

            // Ensure we found a valid choice
            assert(!firstTime);
            // Assign the chosen Branch to the better group
            assignGroup(chosen, betterGroup, overflowBuffer[chosen].box_,
                        groupAssign);
        }

        // Final adjustments if the total number of nodes is less than expected
        if ((groupAssign.group_[0].count_ +
            groupAssign.group_[1].count_) < groupAssign.assign_.size()) {
            // Determine which group to assign remaining nodes to
            if (groupAssign.group_[0].count_ >=
                groupAssign.assign_.size() - op_.MIN_CHILD) {
                group = 1;
            } else {
                group = 0;
            }
            // Assign remaining nodes to the determined group
            for (int i = 0; i < groupAssign.assign_.size(); i++) {
                if (groupAssign.assign_[i] == -1) {
                    assignGroup(i, group, overflowBuffer[i].box_, groupAssign);
                }
            }
        }

        // Ensure valid group assignments
        assert((groupAssign.group_[0].count_ +
                groupAssign.group_[1].count_) == groupAssign.assign_.size());
        assert(groupAssign.group_[0].count_ >= op_.MIN_CHILD);
        assert(groupAssign.group_[1].count_ >= op_.MIN_CHILD);

        // Clear the current node and create a new node for one group
        node->childSize_ = 0;
        *nodeBuffer = newNode(node->height_);
        Node *targetNodes[] = {node, *nodeBuffer}; // Nodes to receive the split Branchs

        // Ensure the number of nodes in the split is correct
        assert(groupAssign.assign_.size() <= op_.MAX_CHILD + 1);

        for (int index = 0; index < groupAssign.assign_.size(); index++) {
            int groupAssignValue = groupAssign.assign_[index];
            assert(groupAssignValue == 0 || groupAssignValue == 1);

            // Add each Branch to the appropriate group node
            bool nodeSplited =
                addBranch(overflowBuffer[index], targetNodes[groupAssignValue], nodeBuffer);

            assert(!nodeSplited);
        }

        // Ensure the split was performed correctly
        assert((node->childSize_ + (*nodeBuffer)->childSize_) == groupAssign.assign_.size());
    }

    /**
     * @brief Assign nodes to groups during splitting.
     * @param index The index of the node to assign.
     * @param group The group to assign the node to.
     * @param box The bounding box of the group.
     * @param groupAssign The assignment structure to update.
     */
    void assignGroup(int index, int group,const BB &box,
                     GroupAssign &groupAssign) {
        assert(index < groupAssign.assign_.size());
        assert(group < 2);
        assert(groupAssign.assign_[index] == -1);

        // Assign the Branch to the specified group
        groupAssign.assign_[index] = group;

        // Update the bounding box for the group
        if (groupAssign.group_[group].count_ == 0) {
            // If this is the first Branch in the group, initialize the bounding
            // box
            groupAssign.group_[group].box_ = bbm_.Copy(box);
        } else {
            // Otherwise, expand the existing bounding box to include the new
            // Branch
            BB uBox = bbm_.Union(groupAssign.group_[group].box_, box);
            bbm_.Move(groupAssign.group_[group].box_,uBox);
                
        }

        // Increment the count of Branchs in the group
        groupAssign.group_[group].count_++;
    }

    void addListNode(Node* node, ListNode** listNode)
    {
    ListNode* newListNode;

    newListNode = new ListNode;
    newListNode->node_ = node;
    newListNode->next_ = *listNode;
    *listNode = newListNode;
    }

    void recursivelyPrint(Node *node, BB *box, std::string(*formatFunc)(SignableData*)) {
        if (node == nullptr) {
            return;
        }

        for (int i = 0; i < node->childSize_; i++) {
            std::cout << node->height_ << " " << bbm_.toString(*box) << " -> ";
            if (node->branch_[i].data_ != nullptr) {
                std::cout << formatFunc(node->branch_[i].data_);
            }
                    
            std::cout << bbm_.toString(node->branch_[i].box_) << std::endl;
            // Recursively print for child nodes
            recursivelyPrint(node->branch_[i].child_,
                            &node->branch_[i].box_,formatFunc);
        }
    }

    void recursivelySearchOverlap(const BB &box, Node *node, SpICallBack* callback) {
        if (node == nullptr) {
            return;
        }
        #if defined(SPI_MOVE_COUNT)
            callback->Move();
        #endif 
        for (int i = 0; i < node->childSize_; i++) {
            if (bbm_.IsOverlap(node->branch_[i].box_, box)) {
                if (node->isLeaf()) {
                    SpICallBackValue cv;
                    cv.box = &node->branch_[i].box_;
                    cv.data = node->branch_[i].data_;
                    bool conti = callback->Process(cv);
                    
                    if (!conti) {
                        break;
                    }
                } else {
                    recursivelySearchOverlap(box, node->branch_[i].child_, callback);
                }
            }
        }
    }

    void recursivelySearchUnder(const BB &box, Node *node, SpICallBack* callback) {
        if (node == nullptr) {
            return;
        }
        #if defined(SPI_MOVE_COUNT)
            callback->Move();
        #endif 
        for (int i = 0; i < node->childSize_; i++) {
            if (bbm_.ContainsRange(node->branch_[i].box_, box)) {
                if (node->isLeaf()) {
                    SpICallBackValue cv;
                    cv.box = &node->branch_[i].box_;
                    cv.data = node->branch_[i].data_;
                    bool conti = callback->Process(cv);

                    if (!conti) {
                        break;
                    }
                } else {
                    recursivelySearchUnder(box, node->branch_[i].child_, callback);
                }
            }
        }
    }

    void recursivelySearchCover(const BB &box, Node *node, SpICallBack* callback) {
        if (node == nullptr) {
            return;
        }
        #if defined(SPI_MOVE_COUNT)
            callback->Move();
        #endif 
        for (int i = 0; i < node->childSize_; i++) {
            if (node->isLeaf() && bbm_.ContainsRange(box, node->branch_[i].box_)) {
                    SpICallBackValue cv;
                    cv.box = &node->branch_[i].box_;
                    cv.data = node->branch_[i].data_;
                    bool conti = callback->Process(cv);
                   
                    if (!conti) {
                        return; 
                    }
            }

            if (!node->isLeaf() && bbm_.IsOverlap(node->branch_[i].box_, box)) {
                recursivelySearchCover(box, node->branch_[i].child_, callback);
            }
        }
    }

    size_t recursivelyTotalNode(Node *node) {
        if (node == nullptr) {
            return 0;
        }
        size_t n = 1;
        for (int i = 0; i < node->childSize_; i++) {
            n += recursivelyTotalNode(node->branch_[i].child_);
        }
        return n;
    }

    public:
       class Iterator {
        private: 
            struct TraversalContext {
                Node* node;
                int branchIndex;
                Node* prev;
            };

        public:
            Iterator(Node* node, int index) {
                stack_.push({node,index});
            }

            bool isEnd() {
                return stack_.empty();
            }

            SignableData* operator*()
            {
                assert(!stack_.empty());
                TraversalContext top = stack_.top();
                return top.node->branch_[top.branchIndex].data_;
            }

            BB& getBox()
            {
                assert(!stack_.empty());
                TraversalContext top = stack_.top();
                return top.node->branch_[top.branchIndex].box_;
            }

            int getHeight()
            {
                assert(!stack_.empty());
                TraversalContext top = stack_.top();
                return top.node->height_;
            }

            SignableData* getValue()
            {
                assert(!stack_.empty());
                TraversalContext top = stack_.top();
                return top.node->branch_[top.branchIndex].data_;
            }

            bool operator++() {
                return FindNextData();
            }

            bool FindNextData()
            {
                while (true)
                {
                    if(stack_.empty())
                    {
                        return false;
                    }
                    TraversalContext top = stack_.top();
                    stack_.pop();

                    if(top.node->isLeaf())
                    {
                    if(top.branchIndex + 1 < top.node->childSize_)
                    {
                        stack_.push({top.node, top.branchIndex + 1});
                        return true;
                    }
                    }
                    else
                    {
                    if(top.branchIndex+1 < top.node->childSize_)
                    {
                        stack_.push({top.node, top.branchIndex + 1});
                    }
                    Node* nextLevelnode = top.node->branch_[top.branchIndex].child_;
                    stack_.push({nextLevelnode, 0});

                    if(nextLevelnode->isLeaf())
                    {
                        return true;
                    }
                    }
                }
                }

        private:
            std::stack<TraversalContext> stack_;
        };

    Iterator begin() {
        return Iterator(root_, 0);
    }

    protected:
    BBManager bbm_;

    private:
    SpatialIndexOptions op_;

    Node *root_;       ///< Pointer to the root node of the index.
    std::size_t size_; ///< Number of elements in the index.

    Arena* arena_;

    mutable std::shared_mutex mutex_; 
};




}; // namespace tagfilterdb


#endif

