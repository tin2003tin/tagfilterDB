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
            for (int i = 0; i < DIMS; i++)
            {
                setAxis(i, (RANGETYPE)0, (RANGETYPE)0);
            }
        }

        /// Parameterized Constructor
        /// Constructs a Box using a vector of coordinate pairs.
        /// Box<2> box({{1,10},{1,5}});
        BoundingBox(std::vector<EDGE> vec)
        {
            for (int i = 0; i < DIMS; i++)
            {
                setAxis(i, vec[i].first, vec[i].second);
            }
        }

        /// Move Constructor
        BoundingBox(BROUNDINGBOX_QUAL &&other) noexcept : axis_(std::move(other.axis_)) {}

        /// Copy Constructor
        BoundingBox(const BROUNDINGBOX_QUAL &other) : axis_(other.axis_) {}

        /// Equality Operator
        bool operator==(const BROUNDINGBOX_QUAL &bb)
        {
            for (std::size_t axis = 0; axis < DIMS; axis++)
                if (axis_[axis].first != bb.axis_[axis].first || axis_[axis].second != bb.axis_[axis].second)
                    return false;

            return true;
        }

        BROUNDINGBOX_QUAL &operator=(BROUNDINGBOX_QUAL &&other) noexcept
        {
            if (this != &other)
            {
                axis_ = std::move(other.axis_);
            }
            return *this;
        }

        BROUNDINGBOX_QUAL &operator=(const BROUNDINGBOX_QUAL &other)
        {
            if (this != &other)
            {
                for (std::size_t i = 0; i < DIMS; ++i)
                {
                    axis_[i] = other.axis_[i];
                }
            }
            return *this;
        }

        // Set Method
        Status setAxis(int axis, RANGETYPE s, RANGETYPE e)
        {
            if (axis < 0 || static_cast<std::size_t>(axis) >= DIMS)
            {
                return Status::OutOfRange("Axis is out of bounds");
            }
            if (s > e)
            {
                return Status::OutOfRange("Start value should less than End value");
            }
            axis_[axis] = std::make_pair(s, e);
            return Status::OK();
        }

        Status setAxis(int axis, EDGE e)
        {
            if (axis < 0 || static_cast<std::size_t>(axis) >= DIMS)
            {
                return Status::OutOfRange("Axis is out of bounds");
            }
            // if (s > e)
            // {
            //     return Status::OutOfRange("Start value should less than End value");
            // }
            axis_[axis] = e;
            return Status::OK();
        }

        // Get Method
        OperationResult<EDGE> get(int axis) const
        {
            if (axis < 0 || static_cast<std::size_t>(axis) >= DIMS)
            {
                return OperationResult<EDGE>({}, Status::OutOfRange("Axis is out of bounds"));
            }
            return OperationResult<EDGE>(axis_[axis], Status::OK());
        }

        // Contains Point Method
        // Returns true if the point is within all dimensions' bounds, false otherwise.
        bool containsPoint(const std::array<RANGETYPE, DIMS> &point) const
        {
            for (std::size_t axis = 0; axis < DIMS; ++axis)
            {
                if (point[axis] < axis_[axis].first || point[axis] > axis_[axis].second)
                    return false;
            }
            return true;
        }

        // Reset Method
        // Resets the box's bounds to default values.
        void reset()
        {
            for (std::size_t axis = 0; axis < DIMS; ++axis)
            {
                axis_[axis].first = std::numeric_limits<RANGETYPE>::lowest();
                axis_[axis].second = std::numeric_limits<RANGETYPE>::max();
            }
        }

        // Center Method
        // Computes the center point of the box for each dimension.
        std::array<RANGETYPE, DIMS> center() const
        {
            std::array<RANGETYPE, DIMS> centers;
            for (std::size_t axis = 0; axis < DIMS; ++axis)
            {
                centers[axis] = (axis_[axis].first + axis_[axis].second) / static_cast<RANGETYPE>(2.0);
            }
            return centers;
        }

        // Edge Area Method
        // Computes the sum of the lengths of all edges of the box.
        RANGETYPE edgeArea() const
        {
            RANGETYPE distance = 0;
            for (std::size_t axis = 0; axis < DIMS; ++axis)
                distance += axis_[axis].second - axis_[axis].first;

            return distance;
        }

        // Area Method
        // Computes the area of the box.
        RANGETYPE area() const
        {
            RANGETYPE area = static_cast<RANGETYPE>(1.0);
            for (std::size_t axis = 0; axis < DIMS; ++axis)
            {
                area *= (axis_[axis].second - axis_[axis].first);
            }
            return area;
        }

        // Encloses Method
        // Checks if the current box completely encloses another box.
        bool encloses(const BROUNDINGBOX_QUAL &bb) const
        {
            for (std::size_t axis = 0; axis < DIMS; ++axis)
                if (bb.axis_[axis].first < axis_[axis].first || bb.axis_[axis].second > axis_[axis].second)
                    return false;

            return true;
        }

        // Is Overlap Method
        // Checks if the current box overlaps with another box.
        bool isOverlap(const BROUNDINGBOX_QUAL &bb) const
        {
            for (std::size_t axis = 0; axis < DIMS; ++axis)
            {
                if (!(axis_[axis].first < bb.axis_[axis].second) || !(bb.axis_[axis].first < axis_[axis].second))
                    return false;
            }

            return true;
        }

        // Overlap Method
        // Computes the area of the overlapping region between the current box and another box.
        RANGETYPE overlap(const BROUNDINGBOX_QUAL &bb) const
        {
            RANGETYPE area = static_cast<RANGETYPE>(1.0);
            for (std::size_t axis = 0; area && axis < DIMS; ++axis)
            {
                const RANGETYPE x1 = axis_[axis].first;
                const RANGETYPE x2 = axis_[axis].second;
                const RANGETYPE y1 = bb.axis_[axis].first;
                const RANGETYPE y2 = bb.axis_[axis].second;

                if (x1 < y1)
                {
                    if (y1 < x2)
                    {
                        if (y2 < x2)
                            area *= (y2 - y1);
                        else
                            area *= (x2 - y1);
                        continue;
                    }
                }
                else if (x1 < y2)
                {
                    if (x2 < y2)
                        area *= (x2 - x1);
                    else
                        area *= (y2 - x1);
                    continue;
                }

                return static_cast<RANGETYPE>(0.0);
            }

            return area;
        }

        // Distance From Center Method
        // Computes the squared distance between the centers of the current box and another box.
        RANGETYPE distanceFromCenter(const BROUNDINGBOX_QUAL &bb) const
        {
            RANGETYPE distance = static_cast<RANGETYPE>(0.0);
            for (std::size_t axis = 0; axis < DIMS; ++axis)
            {
                RANGETYPE center1 = (axis_[axis].first + axis_[axis].second) / static_cast<RANGETYPE>(2.0);
                RANGETYPE center2 = (bb.axis_[axis].first + bb.axis_[axis].second) / static_cast<RANGETYPE>(2.0);
                RANGETYPE diff = center1 - center2;
                distance += diff * diff;
            }

            return distance;
        }

        // Expand Method
        // Expands the bounds of the box by a specified margin in all dimensions.
        void expand(RANGETYPE margin)
        {
            for (std::size_t axis = 0; axis < DIMS; ++axis)
            {
                axis_[axis].first -= margin;
                axis_[axis].second += margin;
            }
        }

        // Scale Method
        // Scales the box's dimensions by a specified factor.
        void scale(RANGETYPE factor)
        {
            for (std::size_t axis = 0; axis < DIMS; ++axis)
            {
                RANGETYPE center = (axis_[axis].first + axis_[axis].second) / static_cast<RANGETYPE>(2.0);
                RANGETYPE width = axis_[axis].second - axis_[axis].first;
                RANGETYPE halfWidth = width * factor / static_cast<RANGETYPE>(2.0);
                axis_[axis].first = center - halfWidth;
                axis_[axis].second = center + halfWidth;
            }
        }
        // Translate Method
        // Translates (moves) the box by a specified delta vector in all dimensions.
        void translate(const std::array<RANGETYPE, DIMS> &delta)
        {
            for (std::size_t axis = 0; axis < DIMS; ++axis)
            {
                axis_[axis].first += delta[axis];
                axis_[axis].second += delta[axis];
            }
        }

        // Resize Method
        // Resizes the box to fit new dimensions while keeping the center point unchanged.
        void resize(const std::array<RANGETYPE, DIMS> &newDimensions)
        {
            for (std::size_t axis = 0; axis < DIMS; ++axis)
            {
                RANGETYPE center = (axis_[axis].first + axis_[axis].second) / static_cast<RANGETYPE>(2.0);
                RANGETYPE halfWidth = (newDimensions[axis] - axis_[axis].first) / static_cast<RANGETYPE>(2.0);
                axis_[axis].first = center - halfWidth;
                axis_[axis].second = center + halfWidth;
            }
        }

        // Intersection Method
        // Computes the intersection (overlapping region) between the current box and another box.
        BROUNDINGBOX_QUAL intersection(const BROUNDINGBOX_QUAL &bb) const
        {
            BROUNDINGBOX_QUAL intersect;
            for (std::size_t axis = 0; axis < DIMS; ++axis)
            {
                intersect.axis_[axis].first = std::max(axis_[axis].first, bb.axis_[axis].first);
                intersect.axis_[axis].second = std::min(axis_[axis].second, bb.axis_[axis].second);
            }
            return intersect;
        }

        // Union Box Method
        // Computes the union (bounding box) of the current box and another box.
        BROUNDINGBOX_QUAL unionBox(const BROUNDINGBOX_QUAL &bb) const
        {
            BROUNDINGBOX_QUAL unionBox;
            for (std::size_t axis = 0; axis < DIMS; ++axis)
            {
                unionBox.axis_[axis].first = std::min(axis_[axis].first, bb.axis_[axis].first);
                unionBox.axis_[axis].second = std::max(axis_[axis].second, bb.axis_[axis].second);
            }
            return unionBox;
        }

        // Universe Method
        // Returns a Box representing the entire universe (default bounds).
        static BROUNDINGBOX_QUAL Universe()
        {
            BROUNDINGBOX_QUAL bounds;
            bounds.reset();
            return bounds;
        }

        // Bounding Box Method
        // Computes the bounding box that contains all given points.
        static BROUNDINGBOX_QUAL boundingBox(const std::vector<std::array<RANGETYPE, DIMS>> &points)
        {
            BROUNDINGBOX_QUAL bbox;
            bbox.axis_[0].first = bbox.axis_[0].second = points[0][0];
            for (const auto &point : points)
            {
                for (std::size_t axis = 0; axis < DIMS; ++axis)
                {
                    bbox.axis_[axis].first = std::min(bbox.axis_[axis].first, point[axis]);
                    bbox.axis_[axis].second = std::max(bbox.axis_[axis].second, point[axis]);
                }
            }
            return bbox;
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
                oss << "(" << axis_[i].first << ", " << axis_[i].second << ")";
            }
            oss << "]";
            return oss.str();
        }

    private:
        /// Array storing the bounds for each dimension.
        EDGE axis_[DIMS];
    };
}

#endif // TAGFILTERDB_R_STAR_TREE_BOX_HPP_
