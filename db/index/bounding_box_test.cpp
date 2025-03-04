// Copyright (c) 2025-present, tin2003tin, User
//   This source code is part of [TagfilterDB]
//   (https://github.com/tin2003tin/tagfilterDB)
//
/* This implementation is inspired by the N-dimensional RTree implementation
 * from the `nushoin/RTree` repository. The original implementation can be found
 * at: https://github.com/nushoin/RTree
 *
 * Credit: RTree implementation by nushoin.
 *
 * @note This code is based on the original work in the `nushoin/RTree`
 * repository.
 *
 * @license MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * provided to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "db/index/bounding_box.h"

#include "util/random.h"
#include <gtest/gtest.h>

using namespace tagfilterdb::index;

class BoxMgrTest : public ::testing::Test {
  protected:
    static constexpr int dim_length = 2; // 2D box for testing
};

TEST_F(BoxMgrTest, BoxInitialization) {
    BoundingBox box(dim_length);
    ASSERT_NE(box.dims, nullptr);
    BoundingBoxMgr::deleteBox(box);
}

TEST_F(BoxMgrTest, CopyBox) {
    BoundingBox box1(dim_length);
    box1.dims[0] = {0.0, 1.0};
    box1.dims[1] = {2.0, 3.0};

    BoundingBox box2 = BoundingBoxMgr::copy(box1, dim_length);
    EXPECT_EQ(box2.dims[0].min, 0.0);
    EXPECT_EQ(box2.dims[0].max, 1.0);
    EXPECT_EQ(box2.dims[1].min, 2.0);
    EXPECT_EQ(box2.dims[1].max, 3.0);

    BoundingBoxMgr::deleteBox(box1);
    BoundingBoxMgr::deleteBox(box2);
}

TEST_F(BoxMgrTest, ContainsRange) {
    BoundingBox box1(dim_length);
    box1.dims[0] = {0.0, 5.0};
    box1.dims[1] = {0.0, 5.0};

    BoundingBox box2(dim_length);
    box2.dims[0] = {1.0, 4.0};
    box2.dims[1] = {1.0, 4.0};

    EXPECT_TRUE(BoundingBoxMgr::ContainsRange(box1, box2, dim_length));
    BoundingBoxMgr::deleteBox(box1);
    BoundingBoxMgr::deleteBox(box2);
}

TEST_F(BoxMgrTest, Area) {
    BoundingBox box(dim_length);
    box.dims[0] = {0.0, 5.0};
    box.dims[1] = {0.0, 5.0};

    EXPECT_EQ(BoundingBoxMgr::Area(box, dim_length), 25);

    BoundingBoxMgr::deleteBox(box);
}

TEST_F(BoxMgrTest, OverlapArea) {
    BoundingBox box1(dim_length);
    box1.dims[0] = {0.0, 5.0};
    box1.dims[1] = {0.0, 5.0};

    BoundingBox box2(dim_length);
    box2.dims[0] = {3.0, 7.0};
    box2.dims[1] = {3.0, 7.0};

    EXPECT_EQ(BoundingBoxMgr::OverlapArea(box1, box2, dim_length), 4.0);
    BoundingBoxMgr::deleteBox(box1);
    BoundingBoxMgr::deleteBox(box2);
}

TEST_F(BoxMgrTest, IsOverlap) {
    BoundingBox box1(dim_length);
    box1.dims[0] = {0.0, 5.0};
    box1.dims[1] = {0.0, 5.0};

    BoundingBox box2(dim_length);
    box2.dims[0] = {3.0, 7.0};
    box2.dims[1] = {3.0, 7.0};

    EXPECT_TRUE(BoundingBoxMgr::IsOverlap(box1, box2, dim_length));

    BoundingBoxMgr::deleteBox(box1);
    BoundingBoxMgr::deleteBox(box2);
}

TEST_F(BoxMgrTest, UnionBox) {
    BoundingBox box1(dim_length);
    box1.dims[0] = {0.0, 5.0};
    box1.dims[1] = {0.0, 5.0};

    BoundingBox box2(dim_length);
    box2.dims[0] = {3.0, 7.0};
    box2.dims[1] = {3.0, 7.0};

    BoundingBox unionBox = BoundingBoxMgr::Union(box1, box2, dim_length);

    EXPECT_EQ(unionBox.dims[0].min, 0.0);
    EXPECT_EQ(unionBox.dims[0].max, 7.0);
    EXPECT_EQ(unionBox.dims[1].min, 0.0);
    EXPECT_EQ(unionBox.dims[1].max, 7.0);

    BoundingBoxMgr::deleteBox(unionBox);
    BoundingBoxMgr::deleteBox(box1);
    BoundingBoxMgr::deleteBox(box2);
}