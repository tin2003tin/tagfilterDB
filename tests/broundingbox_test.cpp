#include "tagfilterdb/spatialIndex/broundingbox.hpp"

#include <algorithm>
#include <vector>

#include "gtest/gtest.h"

namespace DBTesting {

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

} // namespace DBTesting