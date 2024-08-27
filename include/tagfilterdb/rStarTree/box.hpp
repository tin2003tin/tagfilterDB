#ifndef TAGFILTERDB_R_STAR_TREE_BOX_HPP_
#define TAGFILTERDB_R_STAR_TREE_BOX_HPP_

#include <string>
#include <array>
#include <tuple>
#include <sstream>
#include <limits>
#include <vector>
#include "tagfilterdb/export.hpp"
#include "tagfilterdb/status.hpp"

// TODO: Implement move assignment operator to complement the move constructor
// TODO: Add range checks and error handling for methods that may operate on invalid ranges
// TODO: Optimize the area, intersection, and union methods for performance, especially with large dimensions
// TODO: Add unit tests to verify the correctness of all methods, including edge cases and invalid inputs
// TODO: Document all methods thoroughly, including examples of usage and explanations of parameters and return values
// TODO: Implement a method to serialize and deserialize the Box object for file I/O or network transfer
// TODO: Add a method to verify if the box is valid (i.e., bounds are properly defined)
// TODO: Refactor to avoid redundant computations in methods like `overlap` and `intersection`
// TODO: Include boundary checks for methods that modify the box, like `expand`, `scale`, and `translate`
// TODO: Implement a method to generate a random box within a given range for testing purposes

namespace tagfilterdb
{

#define BROUNDINGBOX_TEMPLATE template <int DIMS, class RANGETYPE>
#define BROUNDINGBOX_QUAL BoundingBox<DIMS, RANGETYPE>

    BROUNDINGBOX_TEMPLATE class TAGFILTERDB_EXPORT BoundingBox
    {
        static_assert(std::numeric_limits<RANGETYPE>::is_iec559, "RANGETYPE must be floating-point type");

    public:
        /// @brief {min, max} for axis
        using EDGE = std::pair<RANGETYPE, RANGETYPE>;

        /// Default Constructor
        /// BoundingBox<2, double> = 2 dimension, {{1, 10},{1, 5}} mean X axis start from 1 to 10 and Y axis start from 1 to 5
        /// Note 1 < 10 and 1 < 5 the frist value should less than second Value
        BoundingBox()
        {
            for (std::size_t i_axis = 0; i_axis < DIMS; i_axis++)
            {
                setAxis(i_axis, (RANGETYPE)0, (RANGETYPE)0);
            }
        }

        /// Parameterized Constructor
        /// Constructs a Box using a vector of coordinate pairs.
        /// Box<2> box({{1,10},{1,5}});
        BoundingBox(std::vector<EDGE> a_vec)
        {
            for (std::size_t i_axis = 0; i_axis < DIMS; i_axis++)
            {
                setAxis(i_axis, a_vec[i_axis].first, a_vec[i_axis].second);
            }
        }

        /// Move Constructor
        BoundingBox(BROUNDINGBOX_QUAL &&other) noexcept : m_axis(std::move(other.m_axis)) {}

        /// Copy Constructor
        BoundingBox(const BROUNDINGBOX_QUAL &other) : m_axis(other.m_axis) {}

        /// Equality Operator
        bool operator==(const BROUNDINGBOX_QUAL &other)
        {
            for (std::size_t i_axis = 0; i_axis < DIMS; i_axis++)
                if (m_axis[i_axis].first != other.m_axis[i_axis].first || m_axis[i_axis].second != other.m_axis[i_axis].second)
                    return false;

            return true;
        }

        BROUNDINGBOX_QUAL &operator=(BROUNDINGBOX_QUAL &&other) noexcept
        {
            if (this != &other)
            {
                m_axis = std::move(other.m_axis);
            }
            return *this;
        }

        BROUNDINGBOX_QUAL &operator=(const BROUNDINGBOX_QUAL &other)
        {
            if (this != &other)
            {
                for (std::size_t i_axis = 0; i_axis < DIMS; ++i_axis)
                {
                    m_axis[i_axis] = other.m_axis[i_axis];
                }
            }
            return *this;
        }

        // Set Method
        Status setAxis(int a_axis, RANGETYPE a_start, RANGETYPE a_end)
        {
            if (a_axis < 0 || static_cast<std::size_t>(a_axis) >= DIMS)
            {
                return Status::Error(Status::e_OutOfRange, "Axis is out of bounds");
            }
            if (a_start > a_end)
            {
                return Status::Error(Status::e_OutOfRange, "Start value should be less than End value");
            }
            m_axis[a_axis] = std::make_pair(a_start, a_end);
            return Status::OK();
        }

        Status setAxis(int a_axis, EDGE a_edge)
        {
            if (a_axis < 0 || static_cast<std::size_t>(a_axis) >= DIMS)
            {
                return Status::Error(Status::e_OutOfRange, "Axis is out of bounds");
            }

            m_axis[a_axis] = a_edge;
            return Status::OK();
        }

        // Get Method
        OperationResult<EDGE> get(int a_axis) const
        {
            if (a_axis < 0 || (int)(a_axis) >= DIMS)
            {
                return OperationResult<EDGE>({}, Status::Error(Status::e_OutOfRange, "Axis is out of bounds"));
            }
            return OperationResult<EDGE>(m_axis[a_axis], Status::OK());
        }

        // Contains Point Method
        // Returns true if the point is within all dimensions' bounds, false otherwise.
        bool containsPoint(const std::array<RANGETYPE, DIMS> &r_point) const
        {
            for (std::size_t i_axis = 0; i_axis < DIMS; ++i_axis)
            {
                if (r_point[i_axis] < m_axis[i_axis].first || r_point[i_axis] > m_axis[i_axis].second)
                    return false;
            }
            return true;
        }

        // Reset Method
        // Resets the box's bounds to default values.
        void reset()
        {
            for (std::size_t i_axis = 0; i_axis < DIMS; ++i_axis)
            {
                m_axis[i_axis].first = std::numeric_limits<RANGETYPE>::lowest();
                m_axis[i_axis].second = std::numeric_limits<RANGETYPE>::max();
            }
        }

        // Center Method
        // Computes the center point of the box for each dimension.
        std::array<RANGETYPE, DIMS> center() const
        {
            std::array<RANGETYPE, DIMS> t_centers;
            for (std::size_t i_axis = 0; i_axis < DIMS; ++i_axis)
            {
                t_centers[i_axis] = (m_axis[i_axis].first + m_axis[i_axis].second) / static_cast<RANGETYPE>(2.0);
            }
            return t_centers;
        }

        // Edge Area Method
        // Computes the sum of the lengths of all edges of the box.
        RANGETYPE edgeArea() const
        {
            RANGETYPE distance = 0;
            for (std::size_t i_axis = 0; i_axis < DIMS; ++i_axis)
                distance += m_axis[i_axis].second - m_axis[i_axis].first;

            return distance;
        }

        // Area Method
        // Computes the area of the box.
        RANGETYPE area() const
        {
            RANGETYPE area = static_cast<RANGETYPE>(1.0);
            for (std::size_t i_axis = 0; i_axis < DIMS; ++i_axis)
            {
                area *= (m_axis[i_axis].second - m_axis[i_axis].first);
            }
            return area;
        }

        // Encloses Method
        // Checks if the current box completely encloses another box.
        bool encloses(const BROUNDINGBOX_QUAL &other) const
        {
            for (std::size_t i_axis = 0; i_axis < DIMS; ++i_axis)
                if (other.m_axis[i_axis].first < m_axis[i_axis].first || other.m_axis[i_axis].second > m_axis[i_axis].second)
                    return false;

            return true;
        }

        // Is Overlap Method
        // Checks if the current box overlaps with another box.
        bool isOverlap(const BROUNDINGBOX_QUAL &other) const
        {
            for (std::size_t i_axis = 0; i_axis < DIMS; ++i_axis)
            {
                if (!(m_axis[i_axis].first < other.m_axis[i_axis].second) || !(other.m_axis[i_axis].first < m_axis[i_axis].second))
                    return false;
            }

            return true;
        }

        // Overlap Method
        // Computes the area of the overlapping region between the current box and another box.
        RANGETYPE overlap(const BROUNDINGBOX_QUAL &other) const
        {
            RANGETYPE area = static_cast<RANGETYPE>(1.0);
            for (std::size_t i_axis = 0; area && i_axis < DIMS; ++i_axis)
            {
                const RANGETYPE t_x1 = m_axis[i_axis].first;
                const RANGETYPE t_x2 = m_axis[i_axis].second;
                const RANGETYPE t_y1 = other.m_axis[i_axis].first;
                const RANGETYPE t_y2 = other.m_axis[i_axis].second;

                if (t_x1 < t_y1)
                {
                    if (t_y1 < t_x2)
                    {
                        if (t_y2 < t_x2)
                            area *= (t_y2 - t_y1);
                        else
                            area *= (t_x2 - t_y1);
                        continue;
                    }
                }
                else if (t_x1 < t_y2)
                {
                    if (t_x2 < t_y2)
                        area *= (t_x2 - t_x1);
                    else
                        area *= (t_y2 - t_x1);
                    continue;
                }

                return static_cast<RANGETYPE>(0.0);
            }

            return area;
        }

        // Distance From Center Method
        // Computes the squared distance between the centers of the current box and another box.
        RANGETYPE distanceFromCenter(const BROUNDINGBOX_QUAL &other) const
        {
            RANGETYPE distance = static_cast<RANGETYPE>(0.0);
            for (std::size_t i_axis = 0; i_axis < DIMS; ++i_axis)
            {
                RANGETYPE t_center1 = (m_axis[i_axis].first + m_axis[i_axis].second) / static_cast<RANGETYPE>(2.0);
                RANGETYPE t_center2 = (other.m_axis[i_axis].first + other.m_axis[i_axis].second) / static_cast<RANGETYPE>(2.0);
                RANGETYPE t_diff = t_center1 - t_center2;
                distance += t_diff * t_diff;
            }

            return distance;
        }

        // Expand Method
        // Expands the bounds of the box by a specified margin in all dimensions.
        void expand(RANGETYPE a_margin)
        {
            for (std::size_t i_axis = 0; i_axis < DIMS; ++i_axis)
            {
                m_axis[i_axis].first -= a_margin;
                m_axis[i_axis].second += a_margin;
            }
        }

        // Scale Method
        // Scales the box's dimensions by a specified factor.
        void scale(RANGETYPE a_factor)
        {
            for (std::size_t i_axis = 0; i_axis < DIMS; ++i_axis)
            {
                RANGETYPE t_center = (m_axis[i_axis].first + m_axis[i_axis].second) / static_cast<RANGETYPE>(2.0);
                RANGETYPE t_width = m_axis[i_axis].second - m_axis[i_axis].first;
                RANGETYPE t_halfWidth = t_width * a_factor / static_cast<RANGETYPE>(2.0);
                m_axis[i_axis].first = t_center - t_halfWidth;
                m_axis[i_axis].second = t_center + t_halfWidth;
            }
        }
        // Translate Method
        // Translates (moves) the box by a specified delta vector in all dimensions.
        void translate(const std::array<RANGETYPE, DIMS> &r_delta)
        {
            for (std::size_t i_axis = 0; i_axis < DIMS; ++i_axis)
            {
                m_axis[i_axis].first += r_delta[i_axis];
                m_axis[i_axis].second += r_delta[i_axis];
            }
        }

        // Resize Method
        // Resizes the box to fit new dimensions while keeping the center point unchanged.
        void resize(const std::array<RANGETYPE, DIMS> &r_newDimensions)
        {
            for (std::size_t i_axis = 0; i_axis < DIMS; ++i_axis)
            {
                RANGETYPE t_center = (m_axis[i_axis].first + m_axis[i_axis].second) / static_cast<RANGETYPE>(2.0);
                RANGETYPE t_halfWidth = (r_newDimensions[i_axis] - m_axis[i_axis].first) / static_cast<RANGETYPE>(2.0);
                m_axis[i_axis].first = t_center - t_halfWidth;
                m_axis[i_axis].second = t_center + t_halfWidth;
            }
        }

        // Intersection Method
        // Computes the intersection (overlapping region) between the current box and another box.
        BROUNDINGBOX_QUAL intersection(const BROUNDINGBOX_QUAL &other) const
        {
            BROUNDINGBOX_QUAL t_intersect;
            for (std::size_t i_axis = 0; i_axis < DIMS; ++i_axis)
            {
                t_intersect.m_axis[i_axis].first = std::max(m_axis[i_axis].first, other.m_axis[i_axis].first);
                t_intersect.m_axis[i_axis].second = std::min(m_axis[i_axis].second, other.m_axis[i_axis].second);
            }
            return t_intersect;
        }

        // Union Box Method
        // Computes the union (bounding box) of the current box and another box.
        BROUNDINGBOX_QUAL unionBox(const BROUNDINGBOX_QUAL &other) const
        {
            BROUNDINGBOX_QUAL t_unionBox;
            for (std::size_t i_axis = 0; i_axis < DIMS; ++i_axis)
            {
                t_unionBox.m_axis[i_axis].first = std::min(m_axis[i_axis].first, other.m_axis[i_axis].first);
                t_unionBox.m_axis[i_axis].second = std::max(m_axis[i_axis].second, other.m_axis[i_axis].second);
            }
            return t_unionBox;
        }

        // Universe Method
        // Returns a Box representing the entire universe (default bounds).
        static BROUNDINGBOX_QUAL Universe()
        {
            BROUNDINGBOX_QUAL t_bounds;
            t_bounds.reset();
            return t_bounds;
        }

        // Bounding Box Method
        // Computes the bounding box that contains all given points.
        static BROUNDINGBOX_QUAL boundingBox(const std::vector<std::array<RANGETYPE, DIMS>> &r_points)
        {
            BROUNDINGBOX_QUAL t_bbox;
            t_bbox.m_axis[0].first = t_bbox.m_axis[0].second = r_points[0][0];
            for (const auto &r_point : r_points)
            {
                for (std::size_t i_axis = 0; i_axis < DIMS; ++i_axis)
                {
                    t_bbox.m_axis[i_axis].first = std::min(t_bbox.m_axis[i_axis].first, r_point[i_axis]);
                    t_bbox.m_axis[i_axis].second = std::max(t_bbox.m_axis[i_axis].second, r_point[i_axis]);
                }
            }
            return t_bbox;
        }

        // toString Method
        // Returns a string representation of the box's dimensions and bounds.
        std::string toString() const
        {
            std::ostringstream oss;
            oss << "[";
            for (std::size_t i = 0; i < DIMS; ++i)
            {
                if (i > 0)
                    oss << ", ";
                oss << "(" << m_axis[i].first << ", " << m_axis[i].second << ")";
            }
            oss << "]";
            return oss.str();
        }

    private:
        /// Array storing the bounds for each dimension.
        EDGE m_axis[DIMS];
    };
}

#endif // TAGFILTERDB_R_STAR_TREE_BOX_HPP_
