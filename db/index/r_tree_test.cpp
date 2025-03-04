#include "db/index/r_tree.h"
#include "util/arena.h"
#include "util/random.h"
#include <chrono>
#include <gtest/gtest.h>
#include <vector>

using namespace tagfilterdb;

class RTreeTest : public ::testing::Test {
  protected:
    static RTreeTest *current_;
    Arena arena;
    Cache *cache = nullptr;
    index::RTree::Option option;
    index::RTree *rtree;
    DataView data;
    std::vector<std::pair<index::BoundingBox *, DataView *>> vec;

    RTreeTest() : data("This is a data") {}

    void SetUp() override {
        current_ = this;
        rtree = new index::RTree(&option, &arena, cache); // Allocate the RTree
    }

    void TearDown() override {
        delete rtree; // Free memory after each test
        rtree = nullptr;
    }

    static bool callback(index::BoundingBox *box, DataView *data) {
        current_->vec.push_back({box, data});
        return true;
    }
};
RTreeTest *RTreeTest::current_ = nullptr;

// Test searching an empty tree
TEST_F(RTreeTest, SearchEmptyTree) {
    index::BoundingBox targetBox({{0, 100}, {0, 100}});

    rtree->SearchUnder(targetBox, RTreeTest::callback);

    // Since no elements were inserted, we expect no results
    ASSERT_TRUE(vec.empty());
}

// Test inserting and searching an edge case where all elements have the same
// coordinates
TEST_F(RTreeTest, InsertIdenticalBoxes) {
    index::BoundingBox box({{50, 50}, {50, 50}});

    for (int i = 0; i < 10; i++) {
        (rtree->Insert(box, &data));
    }

    index::BoundingBox targetBox({{50, 50}, {50, 50}});

    rtree->SearchUnder(targetBox, RTreeTest::callback);

    // Expect all inserted elements to be found
    ASSERT_EQ(vec.size(), 10);
}

TEST_F(RTreeTest, InsertOverlappingBoxes) {
    index::BoundingBox box1({{10, 20}, {10, 20}});
    index::BoundingBox box2({{30, 40}, {30, 40}});
    index::BoundingBox box3({{50, 50}, {50, 60}});

    rtree->Insert(box1, &data);
    rtree->Insert(box2, &data);
    rtree->Insert(box3, &data);

    index::BoundingBox targetBox({{25, 35}, {25, 35}});
    rtree->SearchOverlap(targetBox, RTreeTest::callback);

    ASSERT_EQ(vec.size(), 1);
    ASSERT_TRUE(index::BoundingBoxMgr::equal(*vec[0].first, box2, 2));
    delete[] box1.dims;
    delete[] box2.dims;
    delete[] box3.dims;
}

TEST_F(RTreeTest, InsertCoverBoxes) {
    index::BoundingBox box1({{10, 20}, {10, 20}});
    index::BoundingBox box2({{30, 40}, {30, 40}});
    index::BoundingBox box3({{50, 50}, {50, 60}});

    rtree->Insert(box1, &data);
    rtree->Insert(box2, &data);
    rtree->Insert(box3, &data);

    index::BoundingBox targetBox({{10, 40}, {10, 40}});
    rtree->SearchCover(targetBox, RTreeTest::callback);

    ASSERT_EQ(vec.size(), 2);
    delete[] box1.dims;
    delete[] box2.dims;
    delete[] box3.dims;
}

TEST_F(RTreeTest, InsertUnderBoxes) {
    index::BoundingBox box1({{10, 20}, {10, 20}});
    index::BoundingBox box2({{30, 40}, {30, 40}});
    index::BoundingBox box3({{50, 50}, {50, 60}});

    rtree->Insert(box1, &data);
    rtree->Insert(box2, &data);
    rtree->Insert(box3, &data);

    index::BoundingBox targetBox({{12, 15}, {12, 15}});
    rtree->SearchUnder(targetBox, RTreeTest::callback);

    ASSERT_EQ(vec.size(), 1);
    ASSERT_TRUE(index::BoundingBoxMgr::equal(*vec[0].first, box1, 2));
    delete[] box1.dims;
    delete[] box2.dims;
    delete[] box3.dims;
}

TEST_F(RTreeTest, InsertDifferentRangeBoxes) {
    for (int i = 0; i < 1000; i++) {
        index::BoundingBox box(
            {{static_cast<double>(i), static_cast<double>(i * 2)},
             {static_cast<double>(i), static_cast<double>(i * 2)}});

        rtree->Insert(box, &data);

        delete[] box.dims;
    }

    index::BoundingBox overlapBox({{0, 500}, {0, 500}});
    rtree->SearchOverlap(overlapBox, RTreeTest::callback);
    ASSERT_EQ(vec.size(), 501);
    vec.clear();
    index::BoundingBox coverBox({{0, 500}, {0, 500}});
    rtree->SearchCover(coverBox, RTreeTest::callback);
    ASSERT_EQ(vec.size(), 251);
    vec.clear();
    index::BoundingBox underBox({{500, 550}, {500, 550}});
    rtree->SearchUnder(underBox, RTreeTest::callback);
    ASSERT_EQ(vec.size(), 226);
    vec.clear();
}

TEST_F(RTreeTest, SearchLargeSet) {
    int sample = 100000;
    size_t range = 1000;
    Random r(std::chrono::system_clock::now().time_since_epoch().count());

    // Inserting bounding boxes
    for (int i = 0; i < sample; i++) {
        double a = r.Uniform(range);
        double b = r.Uniform(range);
        double c = r.Uniform(range);
        double d = r.Uniform(range);
        index::BoundingBox box({{std::min(a, b), std::max(a, b)},
                                {std::min(c, d), std::max(c, d)}});
        rtree->Insert(box, &data);
    }

    // Set target box for search
    index::BoundingBox targetBox({{250, 750}, {250, 750}});
    vec.clear();
    // SearchOverlap Test
    auto start_overlap = std::chrono::high_resolution_clock::now();
    rtree->SearchOverlap(targetBox, RTreeTest::callback);
    auto end_overlap = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration_overlap =
        end_overlap - start_overlap;
    std::chrono::milliseconds ms_overlap =
        std::chrono::duration_cast<std::chrono::milliseconds>(duration_overlap);
    std::chrono::microseconds us_overlap =
        std::chrono::duration_cast<std::chrono::microseconds>(duration_overlap);
    std::cout << "Time taken for SearchOverlap: " << ms_overlap.count()
              << " milliseconds, " << us_overlap.count()
              << " microseconds. Found: " << vec.size() << " Sample"
              << std::endl;

    vec.clear();
    // SearchCover Test
    auto start_cover = std::chrono::high_resolution_clock::now();
    rtree->SearchCover(targetBox, RTreeTest::callback);
    auto end_cover = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration_cover = end_cover - start_cover;
    std::chrono::milliseconds ms_cover =
        std::chrono::duration_cast<std::chrono::milliseconds>(duration_cover);
    std::chrono::microseconds us_cover =
        std::chrono::duration_cast<std::chrono::microseconds>(duration_cover);
    std::cout << "Time taken for SearchCover: " << ms_cover.count()
              << " milliseconds, " << us_cover.count()
              << " microseconds. Found: " << vec.size() << " Sample"
              << std::endl;
    vec.clear();
    // SearchUnder Test
    auto start_under = std::chrono::high_resolution_clock::now();
    rtree->SearchUnder(targetBox, RTreeTest::callback);
    auto end_under = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration_under = end_under - start_under;
    std::chrono::milliseconds ms_under =
        std::chrono::duration_cast<std::chrono::milliseconds>(duration_under);
    std::chrono::microseconds us_under =
        std::chrono::duration_cast<std::chrono::microseconds>(duration_under);
    std::cout << "Time taken for SearchUnder: " << ms_under.count()
              << " milliseconds, " << us_under.count()
              << " microseconds. Found: " << vec.size() << " Sample"
              << std::endl;

    // Verify size
    ASSERT_TRUE(rtree->size() == sample);
}