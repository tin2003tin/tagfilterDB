// ==========
// This code demonstrates the performance testing of spatial index operations using 
// an R-tree and vector-based approaches. It focuses on searching for overlaps, 
// containment, and coverage in a spatial data set. The program compares the 
// speed of these search operations between a vector-based method and an R-tree-based 
// method for different spatial queries. The code also tracks memory usage during 
// the insertion of spatial data and provides detailed timing for each search type.
// ========== 

#pragma once

#define SPI_MOVE_COUNT

#include <iostream>
#include "tagfilterdb/broundingbox.h"
#include "tagfilterdb/memtable.h"
#include "tagfilterdb/arena.h"
#include "tagfilterdb/spatialIndex.h"
#include "tagfilterdb/random.h"
#include <cstring>
#include <chrono>

using namespace tagfilterdb;
using VE = std::vector<std::pair<BBManager::RangeType, BBManager::RangeType>>;

class Test : public tagfilterdb::Interface {
private:
    std::string s;

public:
    Test(std::string s) : s(std::move(s)) {}

    Interface* Align(Arena* arena) override {
        char* obj_memory = arena->Allocate(sizeof(Test));
        char* str_memory = arena->Allocate(s.size() + 1);
        std::memcpy(str_memory, s.data(), s.size());
        str_memory[s.size()] = '\0';

        // Create a new Test object in the arena with the string in arena memory
        return new (obj_memory) Test(std::string(str_memory));
    }

    std::string ToString() const override {
        return s;
    }

    bool operator==(Interface *other) override {
        return s == ((Test*) other)->s;
    }
};

class TestSearchAllCallBack : public SpICallBack {
    public :
    std::vector<SpICallBackValue> v;
    size_t move_count = 0;
    bool Process(SpICallBackValue value) {
        v.push_back(value);
        return true;
    }

    #if defined(SPI_MOVE_COUNT)
        bool Move() {
            move_count++;
            return true;
        }
    #endif 

    void Sample(BBManager* bbm) const {
        for (int i = 0; i < 5 && i < v.size(); i++) {
            std::cout << bbm->toString(*v[i].bb) << std::endl;       
        }
    }
};

void TestSearchOverlapSpeed(VE query_v, std::vector<std::pair<BBManager::BB, Test>>& v, MemTable& m) {
    TestSearchAllCallBack callback;
    auto query_bb = m.GetSPI()->GetBBManager()->CreateBB(query_v);
    std::cout << "Find SearchOverlap: " << m.GetSPI()->GetBBManager()->toString(query_bb) << std::endl;

    // Test vector-based search (searching for overlap)
    auto start_time = std::chrono::high_resolution_clock::now();
    std::vector<Test> vector_results;
    for (const auto& entry : v) {
        if (m.GetSPI()->GetBBManager()->IsOverlap(query_bb, entry.first)) {
            vector_results.push_back(entry.second);
        }
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    auto vector_elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
    std::cout << "Vector SearchOverlap Time: " << vector_elapsed_time << " microseconds" << std::endl;

    // Test R-tree-based search (searching for overlap)
    start_time = std::chrono::high_resolution_clock::now();
    m.GetSPI()->SearchOverlap(query_bb, &callback);
    end_time = std::chrono::high_resolution_clock::now();
    auto rtree_elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
    std::cout << "R Tree SearchOverlap Time: " << rtree_elapsed_time << " microseconds";

    #if defined(SPI_MOVE_COUNT)
    std::cout << ", " << callback.move_count << " node";
    #endif

    std::cout << std::endl;

    std::cout << "Vector results: " << vector_results.size() << ", R-tree results: " << callback.v.size() << std::endl;

    callback.Sample(m.GetSPI()->GetBBManager());
}

void TestSearchUnderSpeed(VE query_v,std::vector<std::pair<BBManager::BB, Test>>& v, MemTable& m) {
    TestSearchAllCallBack callback;
    auto query_bb = m.GetSPI()->GetBBManager()->CreateBB(query_v);
    std::cout << "Find SearchContain: " << m.GetSPI()->GetBBManager()->toString(query_bb) << std::endl;

    // Test vector-based search (searching for containment)
    auto start_time = std::chrono::high_resolution_clock::now();
    std::vector<Test> vector_results;
    for (const auto& entry : v) {
        if (m.GetSPI()->GetBBManager()->ContainsRange(entry.first, query_bb)) {
            vector_results.push_back(entry.second);
        }
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    auto vector_elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
    std::cout << "Vector SearchUnder Time: " << vector_elapsed_time << " microseconds" << std::endl;

    // Test R-tree-based search (searching for containment)
    start_time = std::chrono::high_resolution_clock::now();
    m.GetSPI()->SearchUnder(query_bb, &callback);
    end_time = std::chrono::high_resolution_clock::now();
    auto rtree_elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
    std::cout << "R Tree SearchUnder Time: " << rtree_elapsed_time << " microseconds";

    #if defined(SPI_MOVE_COUNT)
    std::cout << ", " << callback.move_count << " node";
    #endif

    std::cout << std::endl;

    std::cout << "Vector results: " << vector_results.size() << ", R-tree results: " << callback.v.size() << std::endl;

    callback.Sample(m.GetSPI()->GetBBManager());
}

void TestSearchCoverSpeed(VE query_v, std::vector<std::pair<BBManager::BB, Test>>& v, MemTable& m) {
    TestSearchAllCallBack callback;
    auto query_bb = m.GetSPI()->GetBBManager()->CreateBB(query_v);
    std::cout << "Find SearchCover: " << m.GetSPI()->GetBBManager()->toString(query_bb) << std::endl;

    // Test vector-based search (searching for "under")
    auto start_time = std::chrono::high_resolution_clock::now();
    std::vector<Test> vector_results;
    for (const auto& entry : v) {
        if (m.GetSPI()->GetBBManager()->ContainsRange(query_bb, entry.first)) { 
            vector_results.push_back(entry.second);
        }
    }
   
    auto end_time = std::chrono::high_resolution_clock::now();
    auto vector_elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
    std::cout << "Vector SearchCover Time: " << vector_elapsed_time << " microseconds" << std::endl;

    // Test R-tree-based search (searching for "under")
    start_time = std::chrono::high_resolution_clock::now();
    m.GetSPI()->SearchCover(query_bb, &callback);
    end_time = std::chrono::high_resolution_clock::now();
    auto rtree_elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
    std::cout << "R Tree SearchCover Time: " << rtree_elapsed_time << " microseconds";

    #if defined(SPI_MOVE_COUNT)
    std::cout << ", " << callback.move_count << " node";
    #endif

    std::cout << std::endl;

    std::cout << "Vector results: " << vector_results.size() << ", R-tree results: " << callback.v.size() << std::endl;

      callback.Sample(m.GetSPI()->GetBBManager());
}

int SpeedTest() {
    SpatialIndexOptions op;
    std::vector<std::pair<BBManager::BB, Test>> v;
    MemTable m(op);
    size_t size = 100000;
    size_t range = 100000;

    auto manager = m.GetSPI()->GetBBManager();
    VE chula_locate = {{100,200},{100,200}};
    auto bb = manager->CreateBB(chula_locate);
    Test chula = Test("Chula");
    m.InsertSpiral(bb, &chula);
    v.push_back({manager->Copy(bb), chula});
    Random r(100);
    for (size_t i = 0; i < size; i++) {
        Test t = Test("Location: " + std::to_string(i + 1));
        double a = r.Uniform(range);
        double b = r.Uniform(range);
        double c = r.Uniform(range);
        double d = r.Uniform(range);

        auto bb = manager->CreateBB({{std::min(a, b), std::max(a, b)}, {std::min(c, d), std::max(c, d)}});
        m.InsertSpiral(bb, &t);
        v.push_back({manager->Copy(bb), t});
    }
    std::cout << "Total Usage: " << m.GetArena()->MemoryUsage() << std::endl;

    std::cout << "Testing Overlap Search Speed (Vector vs R-tree)..." << std::endl;
    TestSearchOverlapSpeed(chula_locate,v, m);

    std::cout << "Testing Under Search Speed (Vector vs R-tree)..." << std::endl;
    TestSearchUnderSpeed(chula_locate,v, m);

    std::cout << "Testing Cover Search Speed (Vector vs R-tree)..." << std::endl;
    TestSearchCoverSpeed(chula_locate,v, m);

    return 0;
}

#undef SPI_MOVE_COUNT