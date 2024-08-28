#include "tagfilterdb/rStarTree/box.hpp"
#include "tagfilterdb/rStarTree/tree.hpp"
#include "tagfilterdb/status.hpp"
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

bool BranchComparator(
    const tagfilterdb::RStarTree<std::string, double, 2, 3, 1>::Branch &b,
    const tagfilterdb::RStarTree<std::string, double, 2, 3, 1>::Branch &other) {
    if (b.m_box.area() == other.m_box.area()) {
        return b.m_data < other.m_data;
    }
    return b.m_box.area() > other.m_box.area();
}

class CallBack {
  public:
    std::vector<tagfilterdb::RStarTree<std::string, double, 2, 3, 1>::Branch>
        item;
    bool func(const tagfilterdb::RStarTree<std::string, double, 2, 3, 1>::Branch
                  &value) {
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
    using Box2D = tagfilterdb::BoundingBox<2, double>;
    tagfilterdb::RStarTree<std::string, double, 2, 3, 1> rStarTree;
    rStarTree.Insert(Box2D::Universe(), "Chula");
    rStarTree.Insert(Box2D({{0, 16}, {0, 12}}), "Engineer_facalty");
    rStarTree.Insert(Box2D({{11, 16}, {5, 8}}), "Building1");
    rStarTree.Insert(Box2D({{5, 11}, {5, 8}}), "Building2");
    rStarTree.Insert(Box2D({{2, 11}, {0, 4}}), "Building3");
    rStarTree.Insert(Box2D({{0, 5}, {9, 12}}), "Building4");
    rStarTree.Insert(Box2D({{0, 5}, {5, 8}}), "Building100");
    rStarTree.Insert(Box2D({{2, 3}, {11, 12}}), "Sky cafe");
    rStarTree.Insert(Box2D({{9, 10}, {1, 2}}), "Cafe Amazon");
    rStarTree.Insert(Box2D({{0, 2}, {0, 4}}), "Icanteen");
    rStarTree.Insert(Box2D({{17, 18}, {0, 12}}), "Road");
    rStarTree.Print();
    LOGIT("======FIND_SKYCAFE_AREA========")
    Box2D skyCafeArea({{2, 3}, {11, 12}});
    CallBack skyCafeCallback;
    rStarTree.SearchTag(
        skyCafeArea,
        std::bind(&CallBack::func, &skyCafeCallback, std::placeholders::_1));

    skyCafeCallback.print();
    return 0;
}