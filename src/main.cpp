#include <iostream>
#include "tagfilterdb/rStarTree/temp.hpp"

class MyClass
{
public:
    int index = 0;
    MyClass(int i) : index(i) {}
};

int main()
{
    RTree<MyClass *, double, 3> tree;
    double min[3] = {0., 0., 0.};
    double max[3] = {1., 1., 1.};
    MyClass *mc1 = new MyClass(10);
    MyClass *mc2 = new MyClass(20);
    tree.Insert(min, max, mc1);
    tree.Insert(min, max, mc2);
}