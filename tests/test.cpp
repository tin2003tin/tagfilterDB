#include <iostream>

#include "tagfilterdb/complier/LRcomplier.hpp"
#include "tagfilterdb/rStarTree/box.hpp"
#include "tagfilterdb/code.hpp"
#include "tagfilterdb/status.hpp"

#include "gtest/gtest.h"

namespace DBTesting
{
    TEST(TEST_STATUS, Status)
    {
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

    TEST(TEST_R_STAR_TREE, BROUNDINGBOX)
    {
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

    TEST(TEST_CODE, ENCODE32)
    {
        std::string s;
        for (uint32_t v = 0; v < 100000; v++)
        {
            tagfilterdb::AppendEncode32(&s, v);
        }

        const char *p = s.data();
        for (uint32_t v = 0; v < 100000; v++)
        {
            uint32_t actual = tagfilterdb::Decode32(p);
            ASSERT_EQ(v, actual);
            p += sizeof(uint32_t);
        }
    }
}