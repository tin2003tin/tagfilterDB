#include "tagfilterdb/spatialIndex/spatialIndex.hpp"
#include "tagfilterdb/status.hpp"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

using Sp2D = tagfilterdb::SpatialIndex<std::string, 2>;
using Box2D = tagfilterdb::BoundingBox<2>;

class CallBack : public tagfilterdb::ISIndexCallback<Sp2D> {
  public:
    std::vector<CallBackValue> items;

    bool process(const CallBackValue &r_item) override {
        items.push_back(r_item);
        return true;
    }

    void sort() {
        std::sort(items.begin(), items.end(),
                  [this](const CallBackValue &b, const CallBackValue &other) {
                      return SubNodeComparator(b, other);
                  });
    }

    bool SubNodeComparator(const CallBackValue &b, const CallBackValue &other) {
        if (b.m_box.area() == other.m_box.area()) {
            return b.m_data < other.m_data;
        }
        return b.m_box.area() > other.m_box.area();
    }
};

int main() {
    Sp2D sp;
    sp.Insert(Box2D::Universe(), "Chula");
    sp.Insert(Box2D({{0, 16}, {0, 12}}), "Engineer_facalty");
    sp.Insert(Box2D({{11, 16}, {5, 8}}), "Building1");
    sp.Insert(Box2D({{5, 11}, {5, 8}}), "Building2");
    sp.Insert(Box2D({{2, 11}, {0, 4}}), "Building3");
    sp.Insert(Box2D({{0, 5}, {9, 12}}), "Building4");
    sp.Insert(Box2D({{0, 5}, {5, 8}}), "Building100");
    sp.Insert(Box2D({{2, 3}, {11, 12}}), "Sky Cafe");
    sp.Insert(Box2D({{9, 10}, {1, 2}}), "Cafe Amazon");
    sp.Insert(Box2D({{0, 2}, {0, 4}}), "Icanteen");
    sp.Insert(Box2D({{17, 18}, {0, 12}}), "Road");

    sp.Print();

    CallBack callback;
    Box2D targetArea({{2, 3}, {11, 12}});
    sp.SearchTag(targetArea, &callback);

    callback.sort();
    LOG_INFO("Search Area: ", targetArea.toString())
    for (const auto &items : callback.items) {
        LOG_DEBUG(items.m_data)
    }

    return 0;
}