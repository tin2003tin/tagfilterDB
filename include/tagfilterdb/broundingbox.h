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
    using EDGE = std::pair<RangeType, RangeType>;
    using BB = BoundingBox<RangeType, AreaType>; 

    EDGE* m_axis; ///< Stores the bounds for each dimension

    BoundingBox() {
        m_axis = nullptr;
    }

    BoundingBox(size_t dimension, Arena *arena) {
        setup(dimension, arena);
    }

    BoundingBox(BB &&other) noexcept
        : m_axis(std::move(other.m_axis)) {}

    BB& operator=(BB &&other) noexcept {
        if (this != &other) {
            m_axis = std::move(other.m_axis);
        }
        return *this;
    }
    
  protected:

    void setup(size_t dimension, Arena *arena) {
        assert(dimension > 0);
        assert(arena != nullptr);
        char* ptr = arena->AllocateAligned(sizeof(EDGE) * dimension);
        m_axis = (EDGE*) ptr;
    }
};


class BBManager {
    public :
    using RangeType = double;
    using AreaType = long double;
    using BB = BoundingBox<RangeType,AreaType>; 

    friend BB;
    private: 
        size_t m_dimension;
        Arena* m_arena;

    public:    
    BBManager(size_t d, Arena* arena) : m_dimension(d), m_arena(arena) {
        assert(m_dimension > 0);
        assert(m_arena != nullptr);
    }
    
    BB Copy(BB& b) {
        BB t(m_dimension, m_arena);
        CopyTo(b, t);
        return t;
    }

    BB CreateBB() {
        BB b(m_dimension, m_arena);
        return b;
    }

    BB CreateBB(std::vector<BB::EDGE> a_vec) {
        if (a_vec.size() > m_dimension) {
            a_vec.resize(m_dimension);
        }
        BB b = CreateBB();
        for (std::size_t i_axis = 0; i_axis < a_vec.size(); i_axis++) {
            SetAxis(b, i_axis, a_vec[i_axis].first, a_vec[i_axis].second);
        }
        return b;
    }

    void CopyTo(const BB &self, const BB &other) {
        if (&self != &other) {
            for (std::size_t i_axis = 0; i_axis < m_dimension; ++i_axis) {
                other.m_axis[i_axis] = self.m_axis[i_axis];
            }
        }
    }

    bool Equal(const BB &self, const BB &other) {
        for (std::size_t i_axis = 0; i_axis < m_dimension; i_axis++)
            if (self.m_axis[i_axis].first != other.m_axis[i_axis].first ||
                self.m_axis[i_axis].second != other.m_axis[i_axis].second)
                return false;

        return true;
    }

    bool SetAxis(BB& b, int a_axis, RangeType a_start, RangeType a_end) {
        if (a_axis < 0 || a_axis > m_dimension) {
            return false;
        }
        if (a_start > a_end) {
            return false;
        }
        b.m_axis[a_axis] = std::make_pair(a_start, a_end);
        return true;
    }
   
    bool SetAxis(BB& b,int a_axis, BB::EDGE a_edge) {
            if (a_axis < 0 || a_axis > m_dimension) {
                return false;
            }

            b.m_axis[a_axis] = a_edge;
            return true;
    }

    BB::EDGE Get(BB& b, size_t a_axis) const {
       if (a_axis < 0 || a_axis > m_dimension) {
            return {};
        }
        return b.m_axis[a_axis];
    }

    
    double Min(BB& b,size_t a_axis) const {
        if (a_axis < 0 || a_axis > m_dimension) {
            return 0;
        }
        return b.m_axis[a_axis].first;
    }

    double Max(BB& b,size_t a_axis) const {
        if (a_axis < 0 || a_axis > m_dimension) {
            return 0;
        }
        return b.m_axis[a_axis].second;
    }

    bool ContainsRange(BB b, const std::vector<BB::EDGE> &r_point) const {
        for (std::size_t i_axis = 0; i_axis < r_point.size(); ++i_axis) {
            if (r_point[i_axis].first < b.m_axis[i_axis].first ||
                r_point[i_axis].second > b.m_axis[i_axis].second)
                return false;
        }
        return true;
    }

    void Reset(BB& b, RangeType min = 0,
               RangeType max = std::numeric_limits<RangeType>::max()) {
        for (std::size_t i_axis = 0; i_axis < m_dimension; ++i_axis) {
            b.m_axis[i_axis].first = min;
            b.m_axis[i_axis].second = max;
        }
    }

    double Area(BB& b) const {
        AreaType area = static_cast<AreaType>(1.0);
        for (std::size_t i_axis = 0; i_axis < m_dimension; ++i_axis) {
            area *= (b.m_axis[i_axis].second - b.m_axis[i_axis].first);
        }
        return area;
    }

    bool IsOverlap(const BB &self, const BB &other) const {
        for (std::size_t i_axis = 0; i_axis < m_dimension; ++i_axis) {
            if (!(self.m_axis[i_axis].first < other.m_axis[i_axis].second) ||
                !(other.m_axis[i_axis].first < self.m_axis[i_axis].second))
                return false;
        }

        return true;
    }

    AreaType OverlapArea(const BB &self, const BB &other) {
        AreaType area = static_cast<AreaType>(1.0);
        for (std::size_t i_axis = 0; area && i_axis < m_dimension; ++i_axis) {
            const RangeType t_x1 = self.m_axis[i_axis].first;
            const RangeType t_x2 = self.m_axis[i_axis].second;
            const RangeType t_y1 = other.m_axis[i_axis].first;
            const RangeType t_y2 = other.m_axis[i_axis].second;

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
        BB t_intersect = CreateBB();
        for (std::size_t i_axis = 0; i_axis < m_dimension; ++i_axis) {
            t_intersect.m_axis[i_axis].first =
                std::max(self.m_axis[i_axis].first, other.m_axis[i_axis].first);
            t_intersect.m_axis[i_axis].second = std::min(
                self.m_axis[i_axis].second, other.m_axis[i_axis].second);
        }
        return t_intersect;
    }

    BB Union(const BB &self, const BB &other) {
        BB t_unionBox = CreateBB();
        for (std::size_t i_axis = 0; i_axis < m_dimension; ++i_axis) {
            t_unionBox.m_axis[i_axis].first =
                std::min(self.m_axis[i_axis].first, other.m_axis[i_axis].first);
            t_unionBox.m_axis[i_axis].second = std::max(
                self.m_axis[i_axis].second, other.m_axis[i_axis].second);
        }
        return t_unionBox;
    }

    BB Universe(RangeType min = 0,
             RangeType max = std::numeric_limits<int>::max()) {
        BB t_bounds = CreateBB();
        Reset(t_bounds, min, max);
        return t_bounds;
    }

    std::string toString(const BB& b) const {
        std::ostringstream oss;
        oss << "[";
        for (std::size_t i = 0; i < m_dimension; ++i) {
            if (i > 0)
                oss << ", ";
            oss << "(" << b.m_axis[i].first << ", " << b.m_axis[i].second << ")";
        }
        oss << "]";
        return oss.str();
    }

    void Print(BB &b) {
        std::cout << toString(b) << std::endl;
    }

};
} // namespace tagfilterdb

#endif // TAGFILTERDB_R_STAR_TREE_BOX_HPP_