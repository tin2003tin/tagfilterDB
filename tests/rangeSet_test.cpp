#include "tagfilterdb/support/rangeSet.hpp"

#include <algorithm>
#include <vector>

#include "gtest/gtest.h"

namespace DBTesting {
TEST(TEST_RANGESET, RangeSet) {
    using namespace tagfilterdb::support;
    RangeSet r;
    r.add(1, 3);
    r.add(2, 10);
    r.add(15, 20);
    r.add(100);
    ASSERT_EQ(r.toString(), "{1..10, 15..20, 100}");
    RangeSet temp;
    temp.add(1, 5);
    temp.add(10);
    temp.add(18, 25);
    ASSERT_EQ(temp.toString(), "{1..5, 10, 18..25}");
    ASSERT_EQ((r.And(temp)).toString(), "{1..5, 10, 18..20}");
    ASSERT_TRUE(r.contains((size_t)10));
    ASSERT_FALSE(r.contains((size_t)200));
    ASSERT_FALSE(r.empty());
    r.remove((size_t)5);
    ASSERT_EQ(r.toString(), "{1..4, 6..10, 15..20, 100}");
}
} // namespace DBTesting