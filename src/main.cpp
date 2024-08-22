#include <iostream>
#include "tagfilterdb/rStarTree/box.hpp"

int main()
{
    // x = 1 to 10, y = 1 to 10
    tagfilterdb::Box<2> box1({{1, 10}, {1, 10}});
    // x = 2 to 5, y = 2 to 5
    tagfilterdb::Box<2> box2({{2, 5}, {2, 5}});

    std::cout << box1.toString() << std::endl;
    std::cout << box2.toString() << std::endl;

    if (box1.isOverlap(box2))
    {
        std::cout << "overlap" << std::endl;
    }
    else
    {
        std::cout << "no overlap" << std::endl;
    }
    std::cout << "box1 Area: " << box1.area() << std::endl;
    std::cout << "box2 Area: " << box2.area() << std::endl;
    std::cout << "overlap Area box1 and box2: " << box1.overlap(box2) << std::endl;

    tagfilterdb::Box<2> u = tagfilterdb::Box<2>::Universe();
    auto [axis, status] = u.get(1);
    if (status.ok())
    {
        std::cout << "Universe edge: " << axis.first << " " << axis.second << std::endl;
    }
    std::cout << "Universe Area: " << u.area() << std::endl;
    std::cout << "overlap Box1 & Universe Area: " << box1.overlap(u) << std::endl;
    std::cout << "overlap Box2 & Universe Area: " << box2.overlap(u) << std::endl;

    // more..
    return 0;
}
