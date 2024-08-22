#ifndef TAGFILTERDB_R_STAR_TREE_BOX_HPP_
#define TAGFILTERDB_R_STAR_TREE_BOX_HPP_

#include <cstddef>
#include <string>
#include <array>
#include <tuple>
#include <sstream>
#include <limits>
#include <vector>
#include "tagfilterdb/export.hpp"
#include "tagfilterdb/status.hpp"

namespace tagfilterdb
{
    template <std::size_t dimensions>
    class TAGFILTERDB_EXPORT Box
    {
    public:
        // Default Constructor
        // Box<2> = 2 dimension, {{1, 10},{1, 5}} mean X axis start from 1 to 10 and Y axis start from 1 to 5
        // Note 1 < 10 and 1 < 5 the frist value should less than second Value
        Box() : axis_{} {}

        // Parameterized Constructor
        // Constructs a Box using a vector of coordinate pairs.
        // Box<2> box({{1,10},{1,5}});
        Box(std::vector<std::pair<int, int>> vec)
        {
            std::size_t i = 0;
            for (const auto &p : vec)
            {
                if (i >= dimensions)
                {
                    break;
                }
                set(i++, p.first, p.second);
            }
        }

        // Move Constructor
        Box(Box &&other) noexcept : axis_(std::move(other.axis_)) {}

        // Copy Constructor
        Box(const Box<dimensions> &other) : axis_(other.axis_) {}

        // Equality Operator
        bool operator==(const Box<dimensions> &bb)
        {
            for (std::size_t axis = 0; axis < dimensions; axis++)
                if (axis_[axis].first != bb.axis_[axis].first || axis_[axis].second != bb.axis_[axis].second)
                    return false;

            return true;
        }

        // Destructor
        ~Box() = default;

        // Set Method
        Status set(int axis, int s, int e)
        {
            if (axis < 0 || static_cast<std::size_t>(axis) >= dimensions)
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

        // Get Method
        // Returns a tuple with the bounds and a status code.
        std::tuple<std::pair<int, int>, Status> get(int axis) const
        {
            if (axis < 0 || static_cast<std::size_t>(axis) >= dimensions)
            {
                return std::make_tuple(std::pair<int, int>(), Status::OutOfRange("Axis is out of bounds"));
            }
            return std::make_tuple(axis_[axis], Status::OK());
        }

        // Contains Point Method
        // Returns true if the point is within all dimensions' bounds, false otherwise.
        bool containsPoint(const std::array<int, dimensions> &point) const
        {
            for (std::size_t axis = 0; axis < dimensions; ++axis)
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
            for (std::size_t axis = 0; axis < dimensions; ++axis)
            {
                axis_[axis].first = std::numeric_limits<int>::min();
                axis_[axis].second = std::numeric_limits<int>::max();
            }
        }

        // Center Method
        // Computes the center point of the box for each dimension.
        std::array<double, dimensions> center() const
        {
            std::array<double, dimensions> centers;
            for (std::size_t axis = 0; axis < dimensions; ++axis)
            {
                centers[axis] = (axis_[axis].first + axis_[axis].second) / 2.0;
            }
            return centers;
        }

        // Edge Area Method
        // Computes the sum of the lengths of all edges of the box.
        int edgeArea() const
        {
            int distance = 0;
            for (std::size_t axis = 0; axis < dimensions; ++axis)
                distance += axis_[axis].second - axis_[axis].first;

            return distance;
        }

        // Area Method
        // Computes the area of the box.
        double area() const
        {
            double area = 1.0;
            for (std::size_t axis = 0; axis < dimensions; ++axis)
            {
                double min = static_cast<double>(axis_[axis].first);
                double max = static_cast<double>(axis_[axis].second);
                double width = max - min;
                area *= width;
            }

            return area;
        }

        // Encloses Method
        // Checks if the current box completely encloses another box.
        bool encloses(const Box<dimensions> &bb) const
        {
            for (std::size_t axis = 0; axis < dimensions; ++axis)
                if (bb.axis_[axis].first < axis_[axis].first || axis_[axis].second < bb.axis_[axis].second)
                    return false;

            return true;
        }

        // Is Overlap Method
        // Checks if the current box overlaps with another box.
        bool isOverlap(const Box<dimensions> &bb) const
        {
            for (std::size_t axis = 0; axis < dimensions; ++axis)
            {
                if (!(axis_[axis].first < bb.axis_[axis].second) || !(bb.axis_[axis].first < axis_[axis].second))
                    return false;
            }

            return true;
        }

        // Overlap Method
        // Computes the area of the overlapping region between the current box and another box.
        double overlap(const Box<dimensions> &bb) const
        {
            double area = 1.0;
            for (std::size_t axis = 0; area && axis < dimensions; ++axis)
            {
                const int x1 = axis_[axis].first;
                const int x2 = axis_[axis].second;
                const int y1 = bb.axis_[axis].first;
                const int y2 = bb.axis_[axis].second;

                if (x1 < y1)
                {
                    if (y1 < x2)
                    {
                        if (y2 < x2)
                            area *= static_cast<double>(y2 - y1);
                        else
                            area *= static_cast<double>(x2 - y1);
                        continue;
                    }
                }
                else if (x1 < y2)
                {
                    if (x2 < y2)
                        area *= static_cast<double>(x2 - x1);
                    else
                        area *= static_cast<double>(y2 - x1);
                    continue;
                }

                return 0.0;
            }

            return area;
        }

        // Distance From Center Method
        // Computes the squared distance between the centers of the current box and another box.
        double distanceFromCenter(const Box<dimensions> &bb) const
        {
            double distance = 0.0;
            for (std::size_t axis = 0; axis < dimensions; ++axis)
            {
                double center1 = (axis_[axis].first + axis_[axis].second) / 2.0;
                double center2 = (bb.axis_[axis].first + bb.axis_[axis].second) / 2.0;
                double diff = center1 - center2;
                distance += diff * diff;
            }

            return distance;
        }

        // Expand Method
        // Expands the bounds of the box by a specified margin in all dimensions.
        void expand(int margin)
        {
            for (std::size_t axis = 0; axis < dimensions; ++axis)
            {
                axis_[axis].first -= margin;
                axis_[axis].second += margin;
            }
        }

        // Scale Method
        // Scales the box's dimensions by a specified factor.
        void scale(double factor)
        {
            for (std::size_t axis = 0; axis < dimensions; ++axis)
            {
                int center = (axis_[axis].first + axis_[axis].second) / 2;
                int width = axis_[axis].second - axis_[axis].first;
                int halfWidth = static_cast<int>(width * factor / 2);
                axis_[axis].first = center - halfWidth;
                axis_[axis].second = center + halfWidth;
            }
        }

        // Translate Method
        // Translates (moves) the box by a specified delta vector in all dimensions.
        void translate(const std::array<int, dimensions> &delta)
        {
            for (std::size_t axis = 0; axis < dimensions; ++axis)
            {
                axis_[axis].first += delta[axis];
                axis_[axis].second += delta[axis];
            }
        }

        // Resize Method
        // Resizes the box to fit new dimensions while keeping the center point unchanged.
        void resize(const std::array<int, dimensions> &newDimensions)
        {
            for (std::size_t axis = 0; axis < dimensions; ++axis)
            {
                int center = (axis_[axis].first + axis_[axis].second) / 2;
                int halfWidth = (newDimensions[axis] - axis_[axis].first) / 2;
                axis_[axis].first = center - halfWidth;
                axis_[axis].second = center + halfWidth;
            }
        }

        // Intersection Method
        // Computes the intersection (overlapping region) between the current box and another box.
        Box intersection(const Box<dimensions> &bb) const
        {
            Box intersect;
            for (std::size_t axis = 0; axis < dimensions; ++axis)
            {
                intersect.axis_[axis].first = std::max(axis_[axis].first, bb.axis_[axis].first);
                intersect.axis_[axis].second = std::min(axis_[axis].second, bb.axis_[axis].second);
            }
            return intersect;
        }

        // Union Box Method
        // Computes the union (bounding box) of the current box and another box.
        Box unionBox(const Box<dimensions> &bb) const
        {
            Box unionBox;
            for (std::size_t axis = 0; axis < dimensions; ++axis)
            {
                unionBox.axis_[axis].first = std::min(axis_[axis].first, bb.axis_[axis].first);
                unionBox.axis_[axis].second = std::max(axis_[axis].second, bb.axis_[axis].second);
            }
            return unionBox;
        }

        // Universe Method
        // Returns a Box representing the entire universe (default bounds).
        static Box Universe()
        {
            Box bounds;
            bounds.reset();
            return bounds;
        }

        // Bounding Box Method
        // Computes the bounding box that contains all given points.
        static Box boundingBox(const std::vector<std::array<int, dimensions>> &points)
        {
            Box bbox;
            bbox.axis_[0].first = bbox.axis_[0].second = points[0][0];
            for (const auto &point : points)
            {
                for (std::size_t axis = 0; axis < dimensions; ++axis)
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
            for (std::size_t i = 0; i < dimensions; ++i)
            {
                if (i > 0)
                    oss << ", ";
                oss << "(" << axis_[i].first << ", " << axis_[i].second << ")";
            }
            oss << "]";
            return oss.str();
        }

    private:
        // Array storing the bounds for each dimension.
        std::array<std::pair<int, int>, dimensions> axis_;
    };
}

#endif // TAGFILTERDB_R_STAR_TREE_BOX_HPP_
