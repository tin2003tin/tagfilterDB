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

#ifndef STORAGE_TAGFILTERDB_R_STAR_TREE_BOX_H
#define STORAGE_TAGFILTERDB_R_STAR_TREE_BOX_H

#include <cassert>
#include <cstring>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace tagfilterdb {

class Arena;

namespace index {

using RangeType = double;
using AreaType = long double;

struct BoundingBox {
    static_assert(std::numeric_limits<RangeType>::is_iec559,
                  "RangeType must be floating-point type");

    static_assert(std::numeric_limits<AreaType>::is_iec559,
                  "AreaType must be floating-point type");

    struct Range {
        RangeType min;
        RangeType max;
    };

    Range *dims;

    BoundingBox() : dims(nullptr) {}

    BoundingBox(int dimension) : dims(new Range[dimension]) {}

    BoundingBox(std::vector<BoundingBox::Range> vec);

    BoundingBox(BoundingBox &&other) noexcept : dims(other.dims) {
        other.dims = nullptr;
    }

    BoundingBox &operator=(BoundingBox &&other) noexcept {
        if (this != &other) {
            delete[] dims;
            dims = other.dims;
            other.dims = nullptr;
        }
        return *this;
    }

    // No destructor use BoundingBoxMgr to delete
};

class BoundingBoxMgr {
  public:
    BoundingBoxMgr(int dim_length) = delete;

    BoundingBoxMgr(const BoundingBoxMgr &) = delete;
    BoundingBoxMgr &operator=(const BoundingBoxMgr &) = delete;

    ~BoundingBoxMgr();

    static void move(BoundingBox &from, BoundingBox &to);

    static BoundingBox copy(const BoundingBox &box, int dim_length);

    static void copyTo(const BoundingBox &from, BoundingBox &to,
                       int dim_length);

    static void alignTo(BoundingBox *box, Arena *arena, int dim_length);

    static bool equal(const BoundingBox &a, const BoundingBox &b,
                      int dim_length);

    static BoundingBox newBox(int dim_length) {
        return BoundingBox(dim_length);
    }

    static BoundingBox newBox(std::vector<BoundingBox::Range> vec,
                              int dim_length);

    static BoundingBox::Range &get(const BoundingBox &box, int dim_length,
                                   int dim);

    static bool ContainsRange(const BoundingBox &a, const BoundingBox &b,
                              int dim_length);

    static AreaType Area(const BoundingBox &box, int dim_length);

    static bool IsOverlap(const BoundingBox &a, const BoundingBox &b,
                          int dim_length);

    static AreaType OverlapArea(const BoundingBox &a, const BoundingBox &b,
                                int dim_length);

    static BoundingBox Intersection(const BoundingBox &a, const BoundingBox &b,
                                    int dim_length);

    static BoundingBox Union(const BoundingBox &a, const BoundingBox &b,
                             int dim_length);

    static std::string toString(const BoundingBox &box, int dim_length);

    static void deleteBox(BoundingBox &box);
};
} // namespace index
} // namespace tagfilterdb

#endif