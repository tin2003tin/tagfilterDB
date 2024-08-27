#include "tagfilterdb/rStarTree/box.hpp"
#include "tagfilterdb/status.hpp"
#include <iostream>
#include <vector>

int main() {
    using Box2D = tagfilterdb::BoundingBox<2, double>;
    Box2D box;
    box.setAxis(0, {1, 10});
    box.setAxis(1, Box2D::EDGE(1, 2));
    auto status = box.setAxis(2, {3, 3});

    if (status.IsError()) {
        std::cout << status.ToString() << std::endl;
    }
    std::cout << box.toString() << std::endl;
}