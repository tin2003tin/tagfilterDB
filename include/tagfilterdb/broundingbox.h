/**
 * @file broundingbox.h
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

#ifndef TAGFILTERDB_R_STAR_TREE_BOX_H
#define TAGFILTERDB_R_STAR_TREE_BOX_H

#include "arena.h"

#include <array>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>
#include <cassert>

namespace tagfilterdb {

class BBManager;

template <class RangeType = double, class AreaType = double>
 class BoundingBox {
    static_assert(std::numeric_limits<RangeType>::is_iec559,
                  "RangeType must be floating-point type");

    static_assert(std::numeric_limits<AreaType>::is_iec559,
                  "AreaType must be floating-point type");
    friend BBManager;
  public:
    using Edge = std::pair<RangeType, RangeType>;
    using BB = BoundingBox<RangeType, AreaType>; 

    Edge* dims_; ///< Stores the bounds for each dimension

    BoundingBox() {
        dims_ = nullptr;
    }

    BoundingBox(size_t dimension, Arena *arena) {
        setup(dimension, arena);
    }

    BoundingBox(BB &&other) noexcept
        : dims_(std::move(other.dims_)) {}

    BB& operator=(BB &&other) noexcept {
        if (this != &other) {
            dims_ = std::move(other.dims_);
        }
        return *this;
    }
    
  protected:

    void setup(size_t dimension, Arena *arena) {
        assert(dimension > 0);
        assert(arena != nullptr);
        char* ptr = arena->AllocateAligned(sizeof(Edge) * dimension);
        dims_ = (Edge*) ptr;
    }
};


class BBManager {
    public :
    using RangeType = double;
    using AreaType = long double;
    using BB = BoundingBox<RangeType,AreaType>; 

    friend BB;
    private: 
        size_t dimension_;
        Arena* arena_;

    public:    
    BBManager(size_t d, Arena* arena) : dimension_(d), arena_(arena) {
        assert(dimension_ > 0);
        assert(arena_ != nullptr);
    }
    
    BB Copy(BB& box) {
        BB t(dimension_, arena_);
        CopyTo(box, t);
        return t;
    }

    BB CreateBox() {
        BB box(dimension_, arena_);
        return box;
    }

    BB CreateBox(std::vector<BB::Edge> a_vec) {
        if (a_vec.size() > dimension_) {
            a_vec.resize(dimension_);
        }
        BB b = CreateBox();
        for (std::size_t i_axis = 0; i_axis < a_vec.size(); i_axis++) {
            SetAxis(b, i_axis, a_vec[i_axis].first, a_vec[i_axis].second);
        }
        return b;
    }

    void CopyTo(const BB &self, const BB &other) {
        if (&self != &other) {
            for (std::size_t i_axis = 0; i_axis < dimension_; ++i_axis) {
                other.dims_[i_axis] = self.dims_[i_axis];
            }
        }
    }

    bool Equal(const BB &self, const BB &other) {
        for (std::size_t i_axis = 0; i_axis < dimension_; i_axis++)
            if (self.dims_[i_axis].first != other.dims_[i_axis].first ||
                self.dims_[i_axis].second != other.dims_[i_axis].second)
                return false;

        return true;
    }

    bool SetAxis(BB& box, int a_axis, RangeType a_start, RangeType a_end) {
        if (a_axis < 0 || a_axis > dimension_) {
            return false;
        }
        if (a_start > a_end) {
            return false;
        }
        box.dims_[a_axis] = std::make_pair(a_start, a_end);
        return true;
    }
   
    bool SetAxis(BB& box,int a_axis, BB::Edge a_edge) {
            if (a_axis < 0 || a_axis > dimension_) {
                return false;
            }

            box.dims_[a_axis] = a_edge;
            return true;
    }

    BB::Edge Get(BB& box, size_t a_axis) const {
       if (a_axis < 0 || a_axis > dimension_) {
            return {};
        }
        return box.dims_[a_axis];
    }

    
    double Min(BB& box,size_t a_axis) const {
        if (a_axis < 0 || a_axis > dimension_) {
            return 0;
        }
        return box.dims_[a_axis].first;
    }

    double Max(BB& box,size_t a_axis) const {
        if (a_axis < 0 || a_axis > dimension_) {
            return 0;
        }
        return box.dims_[a_axis].second;
    }

    bool ContainsRange(const BB& self, const BB& other) const {
        for (std::size_t i_axis = 0; i_axis < dimension_; ++i_axis) {
            if (self.dims_[i_axis].first > other.dims_[i_axis].first || 
                self.dims_[i_axis].second < other.dims_[i_axis].second) {
                // If 'self' does not fully contain 'other' in this axis
                return false;
            }
        }
        return true; // 'self' contains 'other' in all dimensions
    }

    void Reset(BB& box, RangeType min = 0,
               RangeType max = std::numeric_limits<RangeType>::max()) {
        for (std::size_t i_axis = 0; i_axis < dimension_; ++i_axis) {
            box.dims_[i_axis].first = min;
            box.dims_[i_axis].second = max;
        }
    }

    double Area(BB& box) const {
        AreaType area = static_cast<AreaType>(1.0);
        for (std::size_t i_axis = 0; i_axis < dimension_; ++i_axis) {
            area *= (box.dims_[i_axis].second - box.dims_[i_axis].first);
        }
        return area;
    }

    bool IsOverlap(const BB &self, const BB &other) const {
        for (std::size_t i_axis = 0; i_axis < dimension_; ++i_axis) {
            if (!(self.dims_[i_axis].first < other.dims_[i_axis].second) ||
                !(other.dims_[i_axis].first < self.dims_[i_axis].second))
                return false;
        }

        return true;
    }

    AreaType OverlapArea(const BB &self, const BB &other) {
        AreaType area = static_cast<AreaType>(1.0);
        for (std::size_t i_axis = 0; area && i_axis < dimension_; ++i_axis) {
            const RangeType t_x1 = self.dims_[i_axis].first;
            const RangeType t_x2 = self.dims_[i_axis].second;
            const RangeType t_y1 = other.dims_[i_axis].first;
            const RangeType t_y2 = other.dims_[i_axis].second;

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

    BB Intersection(const BB &self, const BB &other) {
        BB t_intersect = CreateBox();
        for (std::size_t i_axis = 0; i_axis < dimension_; ++i_axis) {
            t_intersect.dims_[i_axis].first =
                std::max(self.dims_[i_axis].first, other.dims_[i_axis].first);
            t_intersect.dims_[i_axis].second = std::min(
                self.dims_[i_axis].second, other.dims_[i_axis].second);
        }
        return t_intersect;
    }

    BB Union(const BB &self, const BB &other) {
        BB t_unionBox = CreateBox();
        for (std::size_t i_axis = 0; i_axis < dimension_; ++i_axis) {
            t_unionBox.dims_[i_axis].first =
                std::min(self.dims_[i_axis].first, other.dims_[i_axis].first);
            t_unionBox.dims_[i_axis].second = std::max(
                self.dims_[i_axis].second, other.dims_[i_axis].second);
        }
        return t_unionBox;
    }

    BB Universe(RangeType min = 0,
             RangeType max = std::numeric_limits<int>::max()) {
        BB t_bounds = CreateBox();
        Reset(t_bounds, min, max);
        return t_bounds;
    }

    std::string toString(const BB& box) const {
        std::ostringstream oss;
        oss << "[";
        for (std::size_t i = 0; i < dimension_; ++i) {
            if (i > 0)
                oss << ", ";
            oss << "(" << box.dims_[i].first << ", " << box.dims_[i].second << ")";
        }
        oss << "]";
        return oss.str();
    }

    void Print(BB &b) const {
        std::cout << toString(b) << std::endl;
    }

};
} // namespace tagfilterdb

#endif // TAGFILTERDB_R_STAR_TREE_BOX_HPP_