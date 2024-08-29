#include "tagfilterdb/RTree/box.hpp"
#include "tagfilterdb/RTree/tree.hpp"
#include "tagfilterdb/status.hpp"
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

using Box2D = tagfilterdb::BoundingBox<2, double>;
using Tree2D = tagfilterdb::RTree<std::string, double, 2, 3, 1>;

bool BranchComparator(const Tree2D::Branch &b, const Tree2D::Branch &other) {
    if (b.m_box.area() == other.m_box.area()) {
        return b.m_data < other.m_data;
    }
    return b.m_box.area() > other.m_box.area();
}

class CallBack {
  public:
    std::vector<Tree2D::Branch> item;
    bool func(const Tree2D::Branch &value) {
        item.push_back(value);
        return true;
    }
    void print() {
        std::sort(item.begin(), item.end(), BranchComparator);
        for (auto &i : item) {
            LOGIT(i.m_data);
        }
    }
};

int main() {
    Tree2D tree;

    tree.Insert(Box2D::Universe(), "Chula");
    tree.Insert(Box2D({{0, 16}, {0, 12}}), "Engineer_facalty");
    tree.Insert(Box2D({{11, 16}, {5, 8}}), "Building1");
    tree.Insert(Box2D({{5, 11}, {5, 8}}), "Building2");
    tree.Insert(Box2D({{2, 11}, {0, 4}}), "Building3");
    tree.Insert(Box2D({{0, 5}, {9, 12}}), "Building4");
    tree.Insert(Box2D({{0, 5}, {5, 8}}), "Building100");
    tree.Insert(Box2D({{2, 3}, {11, 12}}), "Sky cafe");
    tree.Insert(Box2D({{9, 10}, {1, 2}}), "Cafe Amazon");
    tree.Insert(Box2D({{0, 2}, {0, 4}}), "Icanteen");
    tree.Insert(Box2D({{17, 18}, {0, 12}}), "Road");
    tree.Print();
    LOGIT("======FIND_SKYCAFE_AREA========")
    Box2D skyCafeArea({{2, 3}, {11, 12}});
    CallBack skyCafeCallback;
    tree.SearchTag(skyCafeArea, std::bind(&CallBack::func, &skyCafeCallback,
                                          std::placeholders::_1));

    skyCafeCallback.print();
    return 0;
}