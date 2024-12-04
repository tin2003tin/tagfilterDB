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

#include <array>
#include <limits>
#include <sstream>
#include <string>
#include <vector>
#include <cassert>

#define BROUNDINGBOX_TYPE                                                      \
    tagfilterdb::BoundingBox<Dimensions, RangeType, AreaType>

namespace tagfilterdb {

/// @brief A template class for representing a multi-dimensional bounding
/// box.
///
/// The `BoundingBox` class stores the minimum and maximum coordinates for
/// each dimension, defining a hyper-rectangle in a multi-dimensional space.
/// It provides utility like access, overlap, union, expand, etc
///
/// @tparam Dimensions The number of dimensions for the bounding box.
/// @tparam RangeType The data type for the coordinates (e.g., `int`,
/// `float`, `double`).
/// @tparam AreaType The area data type for the coordinates (e.g., `int`,
/// `float`, `double`).
template <int Dimensions, class RangeType = double, class AreaType = double>
 class BoundingBox {
    static_assert(std::numeric_limits<RangeType>::is_iec559,
                  "RangeType must be floating-point type");

    static_assert(std::numeric_limits<AreaType>::is_iec559,
                  "AreaType must be floating-point type");

  public:
    /// @brief Alias for a pair representing the minimum and maximum bounds
    /// in a dimension.
    ///
    /// This alias is used to represent an edge of the bounding box in a
    /// specific dimension. The first element of the pair is the minimum
    /// value, and the second element is the maximum value.
    using EDGE = std::pair<RangeType, RangeType>;

    /// @brief Default Constructor
    ///
    /// Initializes a `BoundingBox` with all dimensions set to a default
    /// range of [0, 0]. This effectively creates a point at the origin in
    /// the multi-dimensional space.
    BoundingBox() {
        for (std::size_t i_axis = 0; i_axis < Dimensions; i_axis++) {
            setAxis(i_axis, (RangeType)0, (RangeType)0);
        }
    }

    /// @brief Parameterized Constructor using std::vector
    ///
    /// Constructs a BoundingBox using a vector of coordinate pairs. Each
    /// pair represents the minimum and maximum values for a specific axis.
    ///
    /// @param a_vec A vector of {min, max} pairs for each axis.
    BoundingBox(std::vector<EDGE> a_vec) {
        for (std::size_t i_axis = 0; i_axis < Dimensions; i_axis++) {
            setAxis(i_axis, a_vec[i_axis].first, a_vec[i_axis].second);
        }
    }

    /// @brief Parameterized Constructor using std::array
    ///
    /// Constructs a BoundingBox using a std::array of coordinate pairs.
    /// Each pair represents the minimum and maximum values for a specific
    /// axis.
    ///
    /// @param a_arr An array of {min, max} pairs for each axis.
    BoundingBox(std::array<EDGE, Dimensions> a_arr) {
        for (std::size_t i_axis = 0; i_axis < Dimensions; i_axis++) {
            setAxis(i_axis, a_arr[i_axis].first, a_arr[i_axis].second);
        }
    }

    /// @brief Move Constructor
    ///
    /// Constructs a BoundingBox by transferring ownership of the data from
    /// another BoundingBox object, leaving the source object in a valid but
    /// unspecified state.
    ///
    /// @param other The BoundingBox to move from.
    BoundingBox(BROUNDINGBOX_TYPE &&other) noexcept
        : m_axis(std::move(other.m_axis)) {}

    /// @brief Copy Constructor
    ///
    /// Constructs a BoundingBox as a copy of another BoundingBox, copying
    /// all axis data to create an independent object.
    ///
    /// @param other The BoundingBox to copy from.
    BoundingBox(const BROUNDINGBOX_TYPE &other) : m_axis(other.m_axis) {}

    /// @brief Move Assignment Operator
    ///
    /// Transfers ownership of the data from another BoundingBox to this
    /// one, leaving the source object in a valid but unspecified state.
    ///
    /// @param other The BoundingBox to move from.
    /// @return A reference to this BoundingBox.
    BROUNDINGBOX_TYPE &operator=(BROUNDINGBOX_TYPE &&other) noexcept {
        if (this != &other) {
            m_axis = std::move(other.m_axis);
        }
        return *this;
    }

    /// @brief Copy Assignment Operator
    ///
    /// Copies the data from another BoundingBox to this one, creating an
    /// independent copy of the other BoundingBox.
    ///
    /// @param other The BoundingBox to copy from.
    /// @return A reference to this BoundingBox.
    BROUNDINGBOX_TYPE &operator=(const BROUNDINGBOX_TYPE &other) {
        if (this != &other) {
            for (std::size_t i_axis = 0; i_axis < Dimensions; ++i_axis) {
                m_axis[i_axis] = other.m_axis[i_axis];
            }
        }
        return *this;
    }

    /// @brief Equality Operator
    ///
    /// Compares two BoundingBox objects to determine if they are
    /// equivalent. Returns true if all corresponding axes have the same
    /// {min, max} values; otherwise, returns false.
    ///
    /// @param other The BoundingBox to compare with.
    /// @return true if the BoundingBoxes are equal, false otherwise.
    bool operator==(const BROUNDINGBOX_TYPE &other) {
        for (std::size_t i_axis = 0; i_axis < Dimensions; i_axis++)
            if (m_axis[i_axis].first != other.m_axis[i_axis].first ||
                m_axis[i_axis].second != other.m_axis[i_axis].second)
                return false;

        return true;
    }

    /// @brief Sets the bounds of a specific axis.
    ///
    /// Sets the start and end values for the specified axis. The start
    /// value must be less than or equal to the end value.
    ///
    /// @param a_axis The axis to set.
    /// @param a_start The minimum value for the axis.
    /// @param a_end The maximum value for the axis.
    /// @return Status indicating success or failure.
    bool setAxis(int a_axis, RangeType a_start, RangeType a_end) {
        if (a_axis < 0 || static_cast<std::size_t>(a_axis) >= Dimensions) {
            return false;
        }
        if (a_start > a_end) {
            return false;
        }
        m_axis[a_axis] = std::make_pair(a_start, a_end);
        return true;
    }

    ///   @brief  Sets the bounds for the specified axis using a {min, max}
    ///   pair.
    ///
    ///  @param a_axis The axis to set.
    ///  @param a_edge A pair representing the {min, max} values for the
    ///  axis.
    ///  @return Status indicating success or failure.
    bool setAxis(int a_axis, EDGE a_edge) {
        if (a_axis < 0 || static_cast<std::size_t>(a_axis) >= Dimensions) {
            return false;
        }

        m_axis[a_axis] = a_edge;
        return true;
    }

    /// @brief Retrieves the bounds of a specific axis.
    ///
    /// Returns the {min, max} pair for the specified axis.
    ///
    /// @param a_axis The axis to retrieve.
    /// @return An OperationResult containing the {min, max} pair and
    /// status.
    EDGE get(int a_axis) const {
        if (a_axis < 0 || (int)(a_axis) >= Dimensions) {
            return {};
        }
        return m_axis[a_axis];
    }

    RangeType min(int a_xis) const {
        assert(a_xis < Dimensions);
        return m_axis[a_xis].first;
    }

    RangeType max(int a_xis) const {
        assert(a_xis < Dimensions);
        return m_axis[a_xis].first;
    }

    /// @brief Checks if a point is within the bounds of the box.
    ///
    /// Determines if a point lies within all the dimensions' bounds of the
    /// box.
    ///
    /// @param r_point The point to check.
    /// @return true if the point is within bounds, false otherwise.
    bool containsPoint(const std::array<RangeType, Dimensions> &r_point) const {
        for (std::size_t i_axis = 0; i_axis < Dimensions; ++i_axis) {
            if (r_point[i_axis] < m_axis[i_axis].first ||
                r_point[i_axis] > m_axis[i_axis].second)
                return false;
        }
        return true;
    }

    /// @brief Resets the box's bounds to default values.
    ///
    /// Sets each dimension's bounds to the lowest and highest possible
    /// values for the coordinate type.
    void reset(RangeType min = 0,
               RangeType max = std::numeric_limits<int>::max()) {
        for (std::size_t i_axis = 0; i_axis < Dimensions; ++i_axis) {
            m_axis[i_axis].first = min;
            m_axis[i_axis].second = max;
        }
    }

    /// @brief Computes the center point of the box.
    ///
    /// Calculates the center point for each dimension based on the current
    /// bounds.
    ///
    /// @return An array representing the center point in each dimension.
    std::array<RangeType, Dimensions> center() const {
        std::array<RangeType, Dimensions> t_centers;
        for (std::size_t i_axis = 0; i_axis < Dimensions; ++i_axis) {
            t_centers[i_axis] = (m_axis[i_axis].first + m_axis[i_axis].second) /
                                static_cast<RangeType>(2.0);
        }
        return t_centers;
    }

    /// @brief Computes the edge length for each dimension.
    ///
    /// Calculates the difference between the maximum and minimum bounds for
    /// each dimension.
    ///
    /// @return An array representing the edge lengths in each dimension.
    std::array<RangeType, Dimensions> edgeLength() const {
        std::array<RangeType, Dimensions> t_length{};
        for (std::size_t i_axis = 0; i_axis < Dimensions; ++i_axis) {
            t_length[i_axis] = m_axis[i_axis].second - m_axis[i_axis].first;
        }
        return t_length;
    }

    /// @brief Computes the total edge length of the box.
    ///
    /// Adds up the lengths of each dimension to get the total edge area of
    /// the box.
    ///
    /// @return The total edge length across all dimensions.
    RangeType edgeArea() const {
        RangeType distance = 0;
        for (std::size_t i_axis = 0; i_axis < Dimensions; ++i_axis)
            distance += m_axis[i_axis].second - m_axis[i_axis].first;

        return distance;
    }

    /// @brief Computes the area (or volume) of the bounding box.
    ///
    /// Multiplies the lengths of the edges in all dimensions to compute the
    /// area (or volume in higher dimensions).
    ///
    /// @return The area or volume of the bounding box.
    AreaType area() const {
        AreaType area = static_cast<AreaType>(1.0);
        for (std::size_t i_axis = 0; i_axis < Dimensions; ++i_axis) {
            area *= (m_axis[i_axis].second - m_axis[i_axis].first);
        }
        return area;
    }

    /// @brief Checks if the current box completely encloses another box.
    ///
    /// Determines if the other box is entirely within the bounds of the
    /// current box across all dimensions.
    ///
    /// @param other The box to check for enclosure within the current box.
    /// @return True if the current box encloses the other box, false
    /// otherwise.
    bool encloses(const BROUNDINGBOX_TYPE &other) const {
        for (std::size_t i_axis = 0; i_axis < Dimensions; ++i_axis)
            if (other.m_axis[i_axis].first < m_axis[i_axis].first ||
                other.m_axis[i_axis].second > m_axis[i_axis].second)
                return false;

        return true;
    }

    /// @brief Checks if the current box overlaps with another box.
    ///
    /// Determines if there is any intersection between the current box and
    /// another box.
    ///
    /// @param other The box to check for overlap with the current box.
    /// @return True if the boxes overlap, false otherwise.
    bool isOverlap(const BROUNDINGBOX_TYPE &other) const {
        for (std::size_t i_axis = 0; i_axis < Dimensions; ++i_axis) {
            if (!(m_axis[i_axis].first < other.m_axis[i_axis].second) ||
                !(other.m_axis[i_axis].first < m_axis[i_axis].second))
                return false;
        }

        return true;
    }

    /// @brief Computes the area of the overlapping region between the
    /// current box and another box.
    ///
    /// Calculates the intersection area across all dimensions and returns
    /// the result.
    ///
    /// @param other The box to compute the overlap area with.
    /// @return The area of the overlapping region, or 0 if there is no
    /// overlap.
    RangeType overlap(const BROUNDINGBOX_TYPE &other) {
        RangeType area = static_cast<RangeType>(1.0);
        for (std::size_t i_axis = 0; area && i_axis < Dimensions; ++i_axis) {
            const RangeType t_x1 = m_axis[i_axis].first;
            const RangeType t_x2 = m_axis[i_axis].second;
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

            return static_cast<RangeType>(0.0);
        }

        return area;
    }

    /// @brief Computes the squared distance between the centers of the
    /// current box and another box.
    ///
    /// Calculates the distance between the centers of the two boxes in each
    /// dimension, squares the differences, and sums them up.
    ///
    /// @param other The box to compute the distance from the center.
    /// @return The squared distance between the centers of the two boxes.
    RangeType distanceFromCenter(const BROUNDINGBOX_TYPE &other) const {
        RangeType distance = static_cast<RangeType>(0.0);
        for (std::size_t i_axis = 0; i_axis < Dimensions; ++i_axis) {
            RangeType t_center1 =
                (m_axis[i_axis].first + m_axis[i_axis].second) /
                static_cast<RangeType>(2.0);
            RangeType t_center2 =
                (other.m_axis[i_axis].first + other.m_axis[i_axis].second) /
                static_cast<RangeType>(2.0);
            RangeType t_diff = t_center1 - t_center2;
            distance += t_diff * t_diff;
        }

        return distance;
    }

    /// @brief Expands the bounds of the box by a specified margin in all
    /// dimensions.
    ///
    /// Increases the size of the box by subtracting and adding the margin
    /// to the minimum and maximum bounds, respectively.
    ///
    /// @param a_margin The margin to expand the box by.
    void expand(RangeType a_margin) {
        for (std::size_t i_axis = 0; i_axis < Dimensions; ++i_axis) {
            m_axis[i_axis].first -= a_margin;
            m_axis[i_axis].second += a_margin;
        }
    }

    /// @brief Scales the box's dimensions by a specified factor.
    ///
    /// Adjusts the size of the box by multiplying the width in each
    /// dimension by the given factor, keeping the center point unchanged.
    ///
    /// @param a_factor The factor by which to scale the box's dimensions.
    void scale(RangeType a_factor) {
        for (std::size_t i_axis = 0; i_axis < Dimensions; ++i_axis) {
            RangeType t_center =
                (m_axis[i_axis].first + m_axis[i_axis].second) /
                static_cast<RangeType>(2.0);
            RangeType t_width = m_axis[i_axis].second - m_axis[i_axis].first;
            RangeType t_halfWidth =
                t_width * a_factor / static_cast<RangeType>(2.0);
            m_axis[i_axis].first = t_center - t_halfWidth;
            m_axis[i_axis].second = t_center + t_halfWidth;
        }
    }

    /// @brief Translates (moves) the box by a specified delta vector in all
    /// dimensions.
    ///
    /// Adjusts the position of the box by adding the delta to both the
    /// minimum and maximum bounds in each dimension.
    ///
    /// @param r_delta The delta vector to translate the box by.
    void translate(const std::array<RangeType, Dimensions> &r_delta) {
        for (std::size_t i_axis = 0; i_axis < Dimensions; ++i_axis) {
            m_axis[i_axis].first += r_delta[i_axis];
            m_axis[i_axis].second += r_delta[i_axis];
        }
    }

    /// @brief Resizes the box to fit new dimensions while keeping the
    /// center point unchanged.
    ///
    /// Adjusts the size of the box to match the new dimensions provided,
    /// keeping the center point fixed.
    ///
    /// @param r_newDimensions The new dimensions to resize the box to.
    void resize(const std::array<RangeType, Dimensions> &r_newDimensions) {
        for (std::size_t i_axis = 0; i_axis < Dimensions; ++i_axis) {
            RangeType t_center =
                (m_axis[i_axis].first + m_axis[i_axis].second) /
                static_cast<RangeType>(2.0);
            RangeType t_halfWidth =
                (r_newDimensions[i_axis] - m_axis[i_axis].first) /
                static_cast<RangeType>(2.0);
            m_axis[i_axis].first = t_center - t_halfWidth;
            m_axis[i_axis].second = t_center + t_halfWidth;
        }
    }

    /// @brief Computes the intersection (overlapping region) between the
    /// current box and another box.
    ///
    /// Determines the bounds of the intersection between the two boxes and
    /// returns the resulting bounding box.
    ///
    /// @param other The box to compute the intersection with.
    /// @return A new bounding box representing the intersection of the two
    /// boxes.
    BROUNDINGBOX_TYPE intersection(const BROUNDINGBOX_TYPE &self,
                                   const BROUNDINGBOX_TYPE &other) const {
        BROUNDINGBOX_TYPE t_intersect;
        for (std::size_t i_axis = 0; i_axis < Dimensions; ++i_axis) {
            t_intersect.m_axis[i_axis].first =
                std::max(self.m_axis[i_axis].first, other.m_axis[i_axis].first);
            t_intersect.m_axis[i_axis].second = std::min(
                self.m_axis[i_axis].second, other.m_axis[i_axis].second);
        }
        return t_intersect;
    }

    /// @brief Computes the union (bounding box) of the current box and
    /// another box.
    ///
    /// Determines the smallest bounding box that contains both the current
    /// box and the other box.
    ///
    /// @param other The box to compute the union with.
    /// @return A new bounding box representing the union of the two boxes.
    static BROUNDINGBOX_TYPE UnionBox(const BROUNDINGBOX_TYPE &self,
                                      const BROUNDINGBOX_TYPE &other) {
        BROUNDINGBOX_TYPE t_unionBox;
        for (std::size_t i_axis = 0; i_axis < Dimensions; ++i_axis) {
            t_unionBox.m_axis[i_axis].first =
                std::min(self.m_axis[i_axis].first, other.m_axis[i_axis].first);
            t_unionBox.m_axis[i_axis].second = std::max(
                self.m_axis[i_axis].second, other.m_axis[i_axis].second);
        }
        return t_unionBox;
    }

    /// @brief Returns a box representing the entire universe (default
    /// bounds).
    ///
    /// Creates a box that represents the maximum possible bounds in all
    /// dimensions.
    ///
    /// @return A bounding box with default bounds representing the
    /// universe.
    static BROUNDINGBOX_TYPE
    Universe(RangeType min = 0,
             RangeType max = std::numeric_limits<int>::max()) {
        BROUNDINGBOX_TYPE t_bounds;
        t_bounds.reset(min, max);
        return t_bounds;
    }

    /// @brief Computes the bounding box that contains all given points.
    ///
    /// Determines the minimum and maximum bounds across all points and
    /// returns the resulting bounding box.
    ///
    /// @param r_points A vector of points to compute the bounding box for.
    /// @return A bounding box that contains all the given points.
    static BROUNDINGBOX_TYPE boundingBox(
        const std::vector<std::array<RangeType, Dimensions>> &r_points) {
        BROUNDINGBOX_TYPE t_bbox;
        t_bbox.m_axis[0].first = t_bbox.m_axis[0].second = r_points[0][0];
        for (const auto &r_point : r_points) {
            for (std::size_t i_axis = 0; i_axis < Dimensions; ++i_axis) {
                t_bbox.m_axis[i_axis].first =
                    std::min(t_bbox.m_axis[i_axis].first, r_point[i_axis]);
                t_bbox.m_axis[i_axis].second =
                    std::max(t_bbox.m_axis[i_axis].second, r_point[i_axis]);
            }
        }
        return t_bbox;
    }

    /// @brief Converts the BoundingBox to a string representation.
    ///
    /// Generates a string that represents the bounding box by listing the
    /// {min, max} pairs for each dimension.
    ///
    /// @return A string representation of the BoundingBox.
    std::string toString() const {
        std::ostringstream oss;
        oss << "[";
        for (std::size_t i = 0; i < Dimensions; ++i) {
            if (i > 0)
                oss << ", ";
            oss << "(" << m_axis[i].first << ", " << m_axis[i].second << ")";
        }
        oss << "]";
        return oss.str();
    }

  private:
    std::array<EDGE, Dimensions>
        m_axis; ///< Stores the bounds for each dimension
};
} // namespace tagfilterdb

#undef BROUNDINGBOX_TYPE

#endif // TAGFILTERDB_R_STAR_TREE_BOX_HPP_
