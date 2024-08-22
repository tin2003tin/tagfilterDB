#include <iostream>
#include "tagfilterdb/rStarTree/box.hpp"
#include "tagfilterdb/rStarTree/tree.hpp"

int main()
{
    // x = 1 to 10, y = 1 to 10
    tagfilterdb::Box<2> box1({{1, 10}, {1, 10}});
    // x = 2 to 5, y = 2 to 5
    tagfilterdb::Box<2> box2({{2, 5}, {2, 5}});

    std::cout << "Box1: " << box1.toString() << std::endl;
    std::cout << "Box2: " << box2.toString() << std::endl;

    if (box1.isOverlap(box2))
    {
        std::cout << "Overlap detected between Box1 and Box2" << std::endl;
    }
    else
    {
        std::cout << "No overlap between Box1 and Box2" << std::endl;
    }

    std::cout << "Box1 Area: " << box1.area() << std::endl;
    std::cout << "Box2 Area: " << box2.area() << std::endl;
    std::cout << "Overlap Area between Box1 and Box2: " << box1.overlap(box2) << std::endl;

    // Create a universal box (infinite bounding box)
    tagfilterdb::Box<2> u = tagfilterdb::Box<2>::Universe();
    auto [axis, status] = u.get(1);
    if (status.ok())
    {
        std::cout << "Universe box edge on dimension 1: " << axis.first << " to " << axis.second << std::endl;
    }
    std::cout << "Universe Box Area: " << u.area() << std::endl;
    std::cout << "Overlap Area between Box1 and Universe: " << box1.overlap(u) << std::endl;
    std::cout << "Overlap Area between Box2 and Universe: " << box2.overlap(u) << std::endl;

    // More tests...

    // Creating an R*-Tree and inserting data with bounding boxes
    tagfilterdb::RStarTree<int, 2, 1, 3> tree;

    // Insert some sample data into the R*-Tree
    tree.insert(box1, 1); // Insert with Box1 and associated data 1
    tree.insert(box2, 2); // Insert with Box2 and associated data 2

    // Print a confirmation message
    std::cout << "Inserted Box1 and Box2 into the R*-Tree" << std::endl;

    return 0;
}
