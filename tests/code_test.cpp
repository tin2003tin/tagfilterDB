#include "tagfilterdb/code.hpp"

#include <algorithm>
#include <vector>

#include "gtest/gtest.h"

namespace DBTesting {
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

} // namespace DBTesting