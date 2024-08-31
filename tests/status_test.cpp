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
} // namespace DBTesting