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
#include "db/index/bounding_box.h"

#include "util/arena.h"

namespace tagfilterdb::index {

BoundingBox::BoundingBox(std::vector<BoundingBox::Range> vec) {
    dims = new Range[vec.size()];
    for (int i = 0; i < vec.size(); i++) {
        dims[i].min = vec[i].min;
        dims[i].max = vec[i].max;
    }
}

BoundingBoxMgr::~BoundingBoxMgr() = default;

void BoundingBoxMgr::move(BoundingBox &from, BoundingBox &to) {
    delete[] to.dims;
    to.dims = from.dims;
    from.dims = nullptr;
}

BoundingBox BoundingBoxMgr::copy(const BoundingBox &box, int dim_length) {
    assert(dim_length > 0);
    BoundingBox newBox(dim_length);
    copyTo(box, newBox, dim_length);
    return newBox;
}

void BoundingBoxMgr::copyTo(const BoundingBox &from, BoundingBox &to,
                            int dim_length) {
    assert(dim_length > 0);
    if (&from != &to) {
        for (int i = 0; i < dim_length; ++i) {
            to.dims[i] = from.dims[i];
        }
    }
}

void BoundingBoxMgr::alignTo(BoundingBox *box, Arena *arena, int dim_length) {
    assert(dim_length > 0);
    char *memory =
        arena->AllocateAligned(sizeof(BoundingBox::Range) * dim_length);
    std::memcpy(memory, box->dims, sizeof(BoundingBox::Range) * dim_length);
    delete[] box->dims;
    box->dims = (BoundingBox::Range *)memory;
}

BoundingBox BoundingBoxMgr::newBox(std::vector<BoundingBox::Range> vec,
                                   int dim_length) {
    assert(dim_length > 0);
    BoundingBox box = newBox(dim_length);
    for (int i = 0; i < dim_length; i++) {
        box.dims[i].min = vec[i].min;
        box.dims[i].max = vec[i].max;
    }
    return box;
}

BoundingBox::Range &BoundingBoxMgr::get(const BoundingBox &box, int dim_length,
                                        int dim) {
    assert(dim >= 0 && dim < dim_length);
    return box.dims[dim];
}

bool BoundingBoxMgr::equal(const BoundingBox &a, const BoundingBox &b,
                           int dim_length) {
    for (int i = 0; i < dim_length; i++) {
        if (a.dims[i].max != b.dims[i].max)
            return false;
        if (a.dims[i].min != b.dims[i].min)
            return false;
    }
    return true;
}

bool BoundingBoxMgr::ContainsRange(const BoundingBox &a, const BoundingBox &b,
                                   int dim_length) {
    for (int i = 0; i < dim_length; ++i) {
        if (a.dims[i].min > b.dims[i].min || a.dims[i].max < b.dims[i].max) {
            // If 'a' does not fully contain 'b' in this axis
            return false;
        }
    }
    return true; // 'a' contains 'b' in all dimensions
}

AreaType BoundingBoxMgr::Area(const BoundingBox &box, int dim_length) {
    AreaType area = static_cast<AreaType>(1.0);
    for (int i = 0; i < dim_length; ++i) {
        area *= (box.dims[i].max - box.dims[i].min);
    }
    return area;
}

bool BoundingBoxMgr::IsOverlap(const BoundingBox &a, const BoundingBox &b,
                               int dim_length) {
    for (int i = 0; i < dim_length; ++i) {
        if ((a.dims[i].min > b.dims[i].max) || (b.dims[i].min > a.dims[i].max))
            return false;
    }

    return true;
}

AreaType BoundingBoxMgr::OverlapArea(const BoundingBox &a, const BoundingBox &b,
                                     int dim_length) {
    AreaType area = static_cast<AreaType>(1.0);
    for (int i = 0; area && i < dim_length; ++i) {
        const RangeType t_x1 = a.dims[i].min;
        const RangeType t_x2 = a.dims[i].max;
        const RangeType t_y1 = b.dims[i].min;
        const RangeType t_y2 = b.dims[i].max;

        if (t_x1 < t_y1) {
            if (t_y1 < t_x2) {
                if (t_y2 < t_x2)
                    area *= (t_y2 - t_y1);
                else
                    area *= (t_x2 - t_y1);
                continue;
            }
        } else if (t_x1 < t_y2) {
            if (t_x2 < t_y2)
                area *= (t_x2 - t_x1);
            else
                area *= (t_y2 - t_x1);
            continue;
        }

        return static_cast<AreaType>(0.0);
    }

    return area;
}

BoundingBox BoundingBoxMgr::Intersection(const BoundingBox &a,
                                         const BoundingBox &b, int dim_length) {
    BoundingBox t_intersect = BoundingBox(dim_length);
    for (int i = 0; i < dim_length; ++i) {
        t_intersect.dims[i].min = std::max(a.dims[i].min, b.dims[i].min);
        t_intersect.dims[i].max = std::min(a.dims[i].max, b.dims[i].max);
    }
    return t_intersect;
}

BoundingBox BoundingBoxMgr::Union(const BoundingBox &a, const BoundingBox &b,
                                  int dim_length) {
    BoundingBox unionBox = BoundingBox(dim_length);
    for (int i = 0; i < dim_length; ++i) {
        unionBox.dims[i].min = std::min(a.dims[i].min, b.dims[i].min);
        unionBox.dims[i].max = std::max(a.dims[i].max, b.dims[i].max);
    }
    return unionBox;
}

std::string BoundingBoxMgr::toString(const BoundingBox &box, int dim_length) {
    std::ostringstream oss;
    oss << "[";
    for (std::size_t i = 0; i < dim_length; ++i) {
        if (i > 0)
            oss << ", ";
        oss << "(" << box.dims[i].min << ", " << box.dims[i].max << ")";
    }
    oss << "]";
    return oss.str();
}

void BoundingBoxMgr::deleteBox(BoundingBox &box) { delete[] box.dims; }

} // namespace tagfilterdb::index