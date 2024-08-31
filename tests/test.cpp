#include <iostream>

#include "tagfilterdb/code.hpp"
#include "tagfilterdb/spatialIndex/broundingbox.hpp"
#include "tagfilterdb/spatialIndex/spatialIndex.hpp"
#include "tagfilterdb/status.hpp"

#include <algorithm>
#include <vector>

#include "gtest/gtest.h"

namespace DBTesting {
TEST(TEST_STATUS, Status) {
    using STATUS = tagfilterdb::Status;

    STATUS s = STATUS::OK();
    ASSERT_TRUE(s.ok());
    ASSERT_FALSE(s.IsError());

    s = STATUS::Error(STATUS::e_NotFound, "Where are you now");
    ASSERT_EQ(s.ToString(), "NotFound: Where are you now");
    ASSERT_FALSE(s.ok());
    ASSERT_TRUE(s == STATUS::e_NotFound);

    STATUS e = STATUS::Error(STATUS::e_NotFound);
    ASSERT_TRUE(s == e);

    auto arg = STATUS::Error(STATUS::e_InvalidArgument, "EER", "Error");
    s = arg;
    ASSERT_TRUE(s.IsError());
    ASSERT_FALSE(s.ok());
    ASSERT_TRUE(s == STATUS::e_InvalidArgument);
    ASSERT_EQ(s.ToString(), "Invalid argument: EER: Error");
}

TEST(TEST_BROUNDINGBOX, BROUNDINGBOX) {
    using testBB = tagfilterdb::BoundingBox<2, double>;

    testBB box1({{1, 10}, {1, 10}});
    testBB box2({{2, 5}, {2, 5}});

    ASSERT_EQ(box1.toString(), "[(1, 10), (1, 10)]");
    ASSERT_EQ(box2.toString(), "[(2, 5), (2, 5)]");
    ASSERT_TRUE(box1.isOverlap(box2));
    ASSERT_EQ(box1.area(), 81);
    ASSERT_EQ(box2.area(), 9);
    ASSERT_EQ(box1.overlap(box2), 9);

    testBB u = testBB::Universe();
    ASSERT_EQ(box1.overlap(u), 81);
    ASSERT_EQ(box2.overlap(u), 9);
}

TEST(TEST_CODE, ENCODE32) {
    std::string s;
    for (uint32_t v = 0; v < 100000; v++) {
        tagfilterdb::AppendEncode32(&s, v);
    }

    const char *p = s.data();
    for (uint32_t v = 0; v < 100000; v++) {
        uint32_t actual = tagfilterdb::Decode32(p);
        ASSERT_EQ(v, actual);
        p += sizeof(uint32_t);
    }
}

using Sp2D = tagfilterdb::SpatialIndex<std::string, 2>;
using Box2D = tagfilterdb::BoundingBox<2>;

class CallBack : public tagfilterdb::ISIndexCallback<Sp2D> {
  public:
    std::vector<CallBackValue> item;

    bool process(const CallBackValue &value) override {
        item.push_back(value);
        return true;
    }

    void sort() {
        std::sort(item.begin(), item.end(),
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

TEST(TEST_SPATIALINDEX, SPATIALINDEX) {
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

    Box2D skyCafeArea({{2, 3}, {11, 12}});
    CallBack skyCafeCallback;
    sp.SearchTag(skyCafeArea, &skyCafeCallback);
    skyCafeCallback.sort();
    ASSERT_EQ(skyCafeCallback.item[0].m_data, "Chula");
    ASSERT_EQ(skyCafeCallback.item[1].m_data, "Engineer_facalty");
    ASSERT_EQ(skyCafeCallback.item[2].m_data, "Building4");
    ASSERT_EQ(skyCafeCallback.item[3].m_data, "Sky Cafe");

    Box2D AmazonArea({{8, 10}, {0, 4}});
    CallBack AmazonAreaCallback;
    sp.SearchTag(AmazonArea, &AmazonAreaCallback);
    AmazonAreaCallback.sort();
    ASSERT_EQ(AmazonAreaCallback.item[0].m_data, "Chula");
    ASSERT_EQ(AmazonAreaCallback.item[1].m_data, "Engineer_facalty");
    ASSERT_EQ(AmazonAreaCallback.item[2].m_data, "Building3");
    ASSERT_EQ(AmazonAreaCallback.item[3].m_data, "Cafe Amazon");

    Box2D EngineerArea({{0, 16}, {0, 12}});
    CallBack EngineerAreaCallBack;
    sp.SearchTag(EngineerArea, &EngineerAreaCallBack);

    EngineerAreaCallBack.sort();
    ASSERT_EQ(EngineerAreaCallBack.item[0].m_data, "Chula");
    ASSERT_EQ(EngineerAreaCallBack.item[1].m_data, "Engineer_facalty");
    ASSERT_EQ(EngineerAreaCallBack.item[2].m_data, "Building3");
    ASSERT_EQ(EngineerAreaCallBack.item[3].m_data, "Building2");
    ASSERT_EQ(EngineerAreaCallBack.item[4].m_data, "Building1");
    ASSERT_EQ(EngineerAreaCallBack.item[5].m_data, "Building100");
    ASSERT_EQ(EngineerAreaCallBack.item[6].m_data, "Building4");
    ASSERT_EQ(EngineerAreaCallBack.item[7].m_data, "Icanteen");
    ASSERT_EQ(EngineerAreaCallBack.item[8].m_data, "Cafe Amazon");
    ASSERT_EQ(EngineerAreaCallBack.item[9].m_data, "Sky Cafe");

    Box2D RoadArea({{17, 18}, {0, 4}});
    CallBack RoadAreaCallBack;
    sp.SearchTag(RoadArea, &RoadAreaCallBack);

    RoadAreaCallBack.sort();
    ASSERT_EQ(RoadAreaCallBack.item[0].m_data, "Chula");
    ASSERT_EQ(RoadAreaCallBack.item[1].m_data, "Road");
}
} // namespace DBTesting