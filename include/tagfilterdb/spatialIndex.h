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

#include <algorithm>
#include <vector>
#include <cassert>
#include <limits>
#include <stddef.h>
#include <iostream>

namespace tagfilterdb {

struct SpatialIndexOptions {
    size_t DIMANSION = 2;
    size_t MAX_CHILD = 4; 
    size_t MIN_CHILD = MAX_CHILD / 2;
};

class Interface {
    public :
    virtual Interface* Align(Arena *arena) = 0;
    virtual std::string ToString() = 0;
    virtual ~Interface() {}
};

class SpatialIndex {
    using BB = BBManager::BB;
    using RangeType = BBManager::RangeType;
    using AreaType = BBManager::AreaType;

    protected:
    BBManager bbm;

    protected:
    struct Node;
    struct Branch;
    struct Group;
    struct GroupAssign;

    struct Node {
        size_t m_height; ///< Height of the node in the tree.

        Branch* m_branches; ///< Array of branchs.
        size_t m_csize;

        Node() : m_height(-1), m_branches(nullptr), m_csize(0) {}

        Node(int a_height , SpatialIndexOptions &op, Arena *arena)
        : m_height(a_height), m_branches(nullptr), m_csize(0) {
            if (arena && op.MAX_CHILD > 0) {
                m_branches = (Branch *)arena->AllocateAligned(op.MAX_CHILD * sizeof(Branch));
            }
        }

        /**
         * @brief Check if the node is a leaf.
         * @return True if the node is a leaf (height is 0), false otherwise.
         */
        bool isLeaf() const { return m_height == 0; }
    };

    struct Branch {
        BB m_box;          ///< Bounding box of the branch.
        Node *m_child;     ///< Pointer to child node, if any.
        Interface* m_data; ///< Data associated with the branch.

        Branch() : m_box(BB()) , m_child(nullptr), m_data(nullptr) {}

        Branch(BBManager* bbm) : m_box(bbm->CreateBB()), m_child(nullptr), m_data(nullptr) {}

        void CreateBox(BBManager* bbm) {
            if (m_box.m_axis == nullptr) {
                m_box = bbm->CreateBB();
            }
        }

        static Branch Copy(Branch& s, BBManager* bbm) {
            Branch ss(bbm);
            ss.m_child = s.m_child;
            ss.m_data = s.m_data;
            ss.m_box = bbm->Copy(s.m_box);
            return ss;
        }
    };

    struct Group {
        BB m_box;           ///< Bounding box of the group.
        int m_count = 0;    ///< Number of nodes in the group.
        Group() = default;

        void CreateBox(BBManager* bbm) {
            if (m_box.m_axis == nullptr) {
                m_box = bbm->CreateBB();
            }
        }

        Group(BBManager* bbm) : m_box(bbm->CreateBB()) {}
    };

    struct GroupAssign {
        std::vector<size_t> m_groupAssign;      ///< Array for group assignments.
        Group m_groups[2];                      ///< Two groups used for splitting.

        GroupAssign(SpatialIndexOptions &op, BBManager* bbm)
            : m_groups{Group(bbm), Group(bbm)} {
            for (int i = 0; i < op.MAX_CHILD + 1; ++i) {
                m_groupAssign.push_back(-1);
            }
        }
    };

    public: 
    SpatialIndex(SpatialIndexOptions &op, Arena* arena) : m_op(op), m_arena(arena), bbm(BBManager(op.DIMANSION,arena)) {
        m_root = newNode(0); 
        m_size = 0; 
    }

    /**
     * @brief Insert a new bounding box and associated data into the index.
     * @param b The bounding box to insert.
     * @param data The data to associate with the bounding box.
     * @return Status of the insertion operation.
     */
    bool Insert(BB &b, Interface *data) {
        Branch subNode(&bbm);
        subNode.m_box = bbm.Copy(b);
        subNode.m_child = nullptr;
        Interface* dataArena = data->Align(m_arena);
        subNode.m_data = dataArena;

        insertBranch(subNode, &m_root);
        m_size++;                 
        return true;  
    }

    void Print() {
        BB t_b = bbm.CreateBB({{0,0},{0,0}});                      
        RecursivelyPrint(m_root, &t_b);
    }

    private:

    Node* newNode(size_t a_height = -1) {
        char* node_memory = m_arena->AllocateAligned(sizeof(Node));
        Node* node = new (node_memory) Node(a_height, m_op, m_arena);
        return node;
    }

    bool insertBranch(Branch &r_Branch, Node **p_root) {
        assert(p_root); 
        Node* t_nodeBuffer;
        bool splited = recursivelyInsertBranch(r_Branch, *p_root, &t_nodeBuffer); 

        if (splited) {
        Node *newRoot = newNode((*p_root)->m_height + 1);
        Branch subNode(&bbm);
        bbm.CopyTo(nodeCover(*p_root), subNode.m_box);
        subNode.m_child = *p_root;
        addBranch(subNode, newRoot, &t_nodeBuffer);

        bbm.CopyTo(nodeCover(t_nodeBuffer), subNode.m_box);
        subNode.m_child = t_nodeBuffer;
        addBranch(subNode, newRoot,&t_nodeBuffer);

        *p_root = newRoot; 

        return true;
        }
        return false;
    }

    bool recursivelyInsertBranch(Branch &r_subNode, Node *p_node, Node** nodeBuffer) {
        assert(p_node); 

        if (p_node->isLeaf()) {
            return addBranch(r_subNode, p_node, nodeBuffer); 
        }
        int index = SelectBestBranch(r_subNode.m_box,
                                    p_node); 
      
        bool childSplited = recursivelyInsertBranch(
            r_subNode, p_node->m_branches[index].m_child, nodeBuffer); 
        if (childSplited) {
            bbm.CopyTo(nodeCover(p_node->m_branches[index].m_child),
                 p_node->m_branches[index].m_box);
            Branch subnode(&bbm);
            subnode.m_child = (*nodeBuffer);
            bbm.CopyTo(nodeCover(*nodeBuffer), subnode.m_box);

            return addBranch(subnode, p_node,nodeBuffer);
        } else {
            // Update the bounding box if no split occurred
            bbm.CopyTo(bbm.Union(p_node->m_branches[index].m_box, r_subNode.m_box),p_node->m_branches[index].m_box);
            return false; // Indicate no split
        }
        return false; 
    }

    bool addBranch(Branch &r_subNode, Node *p_node,Node** nodeBuffer) {
         assert(p_node);

        if (p_node->m_csize < m_op.MAX_CHILD) {
                p_node->m_branches[p_node->m_csize++] =  Branch::Copy(r_subNode,&bbm);
                return false;  // Indicate no split
            } else {
                // Need to split the node
                SplitNode(r_subNode, p_node,nodeBuffer);
                return true; // Indicate split occurred
            }
    }

    BB nodeCover(Node *p_node) {
        assert(p_node);
        BB t_box = bbm.Copy(p_node->m_branches[0].m_box);
        for (int index = 1; index < p_node->m_csize; index++) {
            t_box = bbm.Union(t_box,
                                p_node->m_branches[index]
                                   .m_box);
        }
        return t_box; 
    }

    int SelectBestBranch(const BB &r_box, Node *p_node) {
         int bestIndex = -1;
        BBManager::AreaType bestIncr = std::numeric_limits<BBManager::AreaType>::max();
        BBManager::AreaType bestArea = std::numeric_limits<BBManager::AreaType>::max();

        for (int index = 0; index < p_node->m_csize; ++index) {
            BB t_box = bbm.Union(
                r_box,
                p_node->m_branches[index].m_box);
            BBManager::AreaType area = bbm.Area(p_node->m_branches[index]
                                .m_box); // Area of the current subnode's box
            BBManager::AreaType increase =
                bbm.Area(t_box) - area; // Increase in area due to the new box

            // Update best index based on area increase
            if (increase < bestIncr || (increase == bestIncr && area < bestArea)) {
                bestIndex = index;
                bestIncr = increase;
                bestArea = area;
            }
        }
        return bestIndex;
    }
    void SplitNode(Branch &r_subNode, Node *p_node, Node** nodeBuffer) {
        assert(p_node);
        assert(p_node->m_csize == m_op.MAX_CHILD);
        bool firstTime;

        GroupAssign t_groupAssign(m_op, &bbm); 

        Branch t_overflowBuffer[m_op.MAX_CHILD + 1];

        AreaType t_overflowBufferArea[m_op.MAX_CHILD + 1];

        BB t_boundingBox = bbm.Copy(
            p_node->m_branches[0].m_box);

        // Copy existing Branchs and the new Branch into t_overflowBuffer
        for (int i_subNode = 0; i_subNode < m_op.MAX_CHILD; i_subNode++) {
            t_overflowBuffer[i_subNode] = Branch::Copy(p_node->m_branches[i_subNode],&bbm);
            t_boundingBox = bbm.Union(t_boundingBox, t_overflowBuffer[i_subNode].m_box);
            t_overflowBufferArea[i_subNode] =
                bbm.Area(t_overflowBuffer[i_subNode].m_box);
        }
        t_overflowBuffer[m_op.MAX_CHILD] = Branch::Copy(r_subNode, &bbm); // Add the new Branch to the buffer
        t_boundingBox = bbm.Union(t_boundingBox, r_subNode.m_box); // Update the bounding box with the new Branch
        t_overflowBufferArea[m_op.MAX_CHILD] = bbm.Area(r_subNode.m_box); // Update the area of the new Branch

        int seed0 = 0, seed1 = 0; // Indices of the seeds for splitting
        RangeType bestNormalizedSeparation =
            -std::numeric_limits<RangeType>::infinity(); // Initialize best
        
        AreaType worst, waste;

        firstTime = true;
        for(int indexA=0; indexA < m_op.MAX_CHILD-1; ++indexA)
        {
            for(int indexB = indexA+1; indexB < m_op.MAX_CHILD; ++indexB)
            {
            BB b = bbm.Union(t_overflowBuffer[indexA].m_box, t_overflowBuffer[indexB].m_box);
            waste = bbm.Area(b) - t_overflowBufferArea[indexA] - t_overflowBufferArea[indexB];
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
        AssignGroup(seed0, 0, t_overflowBuffer[seed0].m_box, t_groupAssign);
        AssignGroup(seed1, 1, t_overflowBuffer[seed1].m_box, t_groupAssign);

        firstTime = true;       // Flag for first iteration
        int chosen = -1;       // Index of the chosen Branch for assignment
        int betterGroup = -1;  // Index of the better group for assignment
        int group;             // Current group being considered
        AreaType biggestDiff;  // Biggest difference in growth between groups

        // Assign remaining Branchs to groups based on growth criteria
        while (
            (t_groupAssign.m_groups[0].m_count + t_groupAssign.m_groups[1].m_count <
            t_groupAssign.m_groupAssign.size()) &&
            (t_groupAssign.m_groups[0].m_count <
            (t_groupAssign.m_groupAssign.size() - m_op.MIN_CHILD)) &&
            (t_groupAssign.m_groups[1].m_count <
            (t_groupAssign.m_groupAssign.size() - m_op.MIN_CHILD))) {

            bool firstTime = true;
            for (int index = 0; index < t_groupAssign.m_groupAssign.size(); ++index) {
                if (t_groupAssign.m_groupAssign[index] == -1) {
                    // Compute growth of bounding boxes if the current Branch is
                    // added to each group
                    BB box0 = bbm.Union(t_overflowBuffer[index].m_box,
                                            t_groupAssign.m_groups[0].m_box);
                    BB box1 = bbm.Union(t_overflowBuffer[index].m_box,
                                            t_groupAssign.m_groups[1].m_box);
                    AreaType growth0 =
                        bbm.Area(box0) -  bbm.Area(t_groupAssign.m_groups[0].m_box);
                    AreaType growth1 =
                         bbm.Area(box1) -  bbm.Area(t_groupAssign.m_groups[1].m_box);
                    AreaType diff = growth1 - growth0;

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
                            (t_groupAssign.m_groups[group].m_count <
                                t_groupAssign.m_groups[betterGroup].m_count)) {
                        chosen = index;
                        betterGroup = group;
                    }
                }
            }

            // Ensure we found a valid choice
            assert(!firstTime);
            // Assign the chosen Branch to the better group
            AssignGroup(chosen, betterGroup, t_overflowBuffer[chosen].m_box,
                        t_groupAssign);
        }

        // Final adjustments if the total number of nodes is less than expected
        if ((t_groupAssign.m_groups[0].m_count +
            t_groupAssign.m_groups[1].m_count) < t_groupAssign.m_groupAssign.size()) {
            // Determine which group to assign remaining nodes to
            if (t_groupAssign.m_groups[0].m_count >=
                t_groupAssign.m_groupAssign.size() - m_op.MIN_CHILD) {
                group = 1;
            } else {
                group = 0;
            }
            // Assign remaining nodes to the determined group
            for (int i = 0; i < t_groupAssign.m_groupAssign.size(); i++) {
                if (t_groupAssign.m_groupAssign[i] == -1) {
                    AssignGroup(i, group, t_overflowBuffer[i].m_box, t_groupAssign);
                }
            }
        }

        // Ensure valid group assignments
        assert((t_groupAssign.m_groups[0].m_count +
                t_groupAssign.m_groups[1].m_count) == t_groupAssign.m_groupAssign.size());
        assert(t_groupAssign.m_groups[0].m_count >= m_op.MIN_CHILD);
        assert(t_groupAssign.m_groups[1].m_count >= m_op.MIN_CHILD);

        // Clear the current node and create a new node for one group
        p_node->m_csize = 0;
        *nodeBuffer = newNode(p_node->m_height);
        Node *targetNodes[] = {p_node, *nodeBuffer}; // Nodes to receive the split Branchs

        // Ensure the number of nodes in the split is correct
        assert(t_groupAssign.m_groupAssign.size() <= m_op.MAX_CHILD + 1);

        for (int index = 0; index < t_groupAssign.m_groupAssign.size(); index++) {
            int groupAssignValue = t_groupAssign.m_groupAssign[index];
            assert(groupAssignValue == 0 || groupAssignValue == 1);

            // Add each Branch to the appropriate group node
            bool nodeSplited =
                addBranch(t_overflowBuffer[index], targetNodes[groupAssignValue], nodeBuffer);

            assert(!nodeSplited);
        }

        // Ensure the split was performed correctly
        assert((p_node->m_csize + (*nodeBuffer)->m_csize) == t_groupAssign.m_groupAssign.size());
    }

    /**
     * @brief Assign nodes to groups during splitting.
     * @param a_index The index of the node to assign.
     * @param a_group The group to assign the node to.
     * @param r_box The bounding box of the group.
     * @param r_groupAssign The assignment structure to update.
     */
    void AssignGroup(int a_index, int a_group, BB &r_box,
                     GroupAssign &r_groupAssign) {
        assert(a_index < r_groupAssign.m_groupAssign.size());
        assert(a_group < 2);
        assert(r_groupAssign.m_groupAssign[a_index] == -1);

        // Assign the Branch to the specified group
        r_groupAssign.m_groupAssign[a_index] = a_group;

        // Update the bounding box for the group
        if (r_groupAssign.m_groups[a_group].m_count == 0) {
            // If this is the first Branch in the group, initialize the bounding
            // box
            r_groupAssign.m_groups[a_group].m_box = bbm.Copy(r_box);
        } else {
            // Otherwise, expand the existing bounding box to include the new
            // Branch
            r_groupAssign.m_groups[a_group].m_box =
                bbm.Union(r_groupAssign.m_groups[a_group].m_box, r_box);
        }

        // Increment the count of Branchs in the group
        r_groupAssign.m_groups[a_group].m_count++;
    }

    /**
     * @brief Recursively print nodes starting from a given node.
     * @param p_node The starting node to print.
     * @param p_box The bounding box of the current node.
     */
    void RecursivelyPrint(Node *p_node, BB *p_box) {
        if (p_node == nullptr) {
            return;
        }

        for (int i = 0; i < p_node->m_csize; i++) {
            std::cout << p_node->m_height << " " << bbm.toString(*p_box) << " -> ";
            if (p_node->m_branches[i].m_data != nullptr) {
                std::cout << ((Interface *) p_node->m_branches[i].m_data)->ToString();
            }
                    
            std::cout << bbm.toString(p_node->m_branches[i].m_box) << std::endl;
            // Recursively print for child nodes
            RecursivelyPrint(p_node->m_branches[i].m_child,
                            &p_node->m_branches[i].m_box);
        }
    }

    private:
    SpatialIndexOptions m_op;

    Node *m_root;       ///< Pointer to the root node of the index.
    std::size_t m_size; ///< Number of elements in the index.

    Arena* m_arena;
};




}; // namespace tagfilterdb


#endif

