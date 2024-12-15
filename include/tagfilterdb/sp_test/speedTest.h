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

class Test {
private:
    std::string s;

public:
    Test(std::string s) : s(std::move(s)) {}

    std::string ToString() const {
        return s;
    }

    DataView Serialize() const {
        size_t string_size = s.size();
        size_t total_size = sizeof(size_t) + string_size;

        char* memory = new char[total_size];
        char* ptr = memory;

        // Store the size of the string
        memcpy(ptr, &string_size, sizeof(size_t));
        ptr += sizeof(size_t);

        // Store the string content
        memcpy(ptr, s.data(), string_size);

        return DataView(memory, total_size);
    }

    static Test Deserialize(const DataView& dataView) {
        if (!dataView.data || dataView.size == 0) {
            throw std::invalid_argument("Invalid DataView for deserialization.");
        }

        const char* ptr = dataView.data;

        // Read the size of the string
        size_t string_size;
        memcpy(&string_size, ptr, sizeof(size_t));
        ptr += sizeof(size_t);

        // Read the string content
        std::string deserialized_string(ptr, string_size);

        // Construct and return the Test object
        return Test(deserialized_string);
    }

};

std::string TestToString(SignableData* sData) {
    return (Test::Deserialize(sData->data)).ToString();
}

class TestSearchAllCallBack : public SpICallBack {
    public :
    std::vector<SpICallBackValue> v;
    size_t move_count = 0;
    bool Process(SpICallBackValue  value) {
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
            std::cout << bbm->toString(*v[i].box) << std::endl;       
        }
    }
};

void TestSearchOverlapSpeed(VE query_v, std::vector<std::pair<BBManager::BB, Test>>& v, MemTable & m) {
    TestSearchAllCallBack callback;
    auto query_bb = m.GetSPI()->GetBBManager()->CreateBox(query_v);
    std::cout << "Find SearchOverlap: " << m.GetSPI()->GetBBManager()->toString(query_bb) << std::endl;

    // Test vector-based search (searching for overlap)
    auto start_time = std::chrono::high_resolution_clock::now();
    std::vector<Test> vector_results;
    for (auto& entry : v) {
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
    query_bb.Destroy();
}

void TestSearchUnderSpeed(VE query_v,std::vector<std::pair<BBManager::BB, Test>>& v, MemTable & m) {
    TestSearchAllCallBack callback;
    auto query_bb = m.GetSPI()->GetBBManager()->CreateBox(query_v);
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
    query_bb.Destroy();
}

void TestSearchCoverSpeed(VE query_v, std::vector<std::pair<BBManager::BB, Test>>& v, MemTable & m) {
    TestSearchAllCallBack callback;
    auto query_bb = m.GetSPI()->GetBBManager()->CreateBox(query_v);
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
    query_bb.Destroy();
}

int SpeedTest() {
    SpatialIndexOptions sop;
    sop.FILENAME = "SpeedTest_SP.tin";
    MemPoolOpinion mop;
    mop.FILENAME = "SpeedTest_MEM.tin";
    std::vector<std::pair<BBManager::BB, Test>> v;
    MemTable  m(sop,mop);
    size_t size = 1000;
    size_t range = 1000;

    auto manager = m.GetSPI()->GetBBManager();
    // VE chula_locate = {{100,200},{100,200}};
    // auto bb = manager->CreateBox(chula_locate);
    // Test chula = Test("Chula");
    // SignableData tData;
    // tData.data = chula.Serialize();
    // m.GetSPI()->Insert(bb, &tData);
    // v.push_back({manager->Copy(bb), chula});
    Random r(101);
    for (size_t i = 0; i < size; i++) {
        Test t = Test("Location: " + std::to_string(i + 1));
        double a = r.Uniform(range);
        double b = r.Uniform(range);
        double c = r.Uniform(range);
        double d = r.Uniform(range);

        auto bb = manager->CreateBox({{std::min(a, b), std::max(a, b)}, {std::min(c, d), std::max(c, d)}});

        auto sData = m.GetMempool()->Insert(t.Serialize());
        m.GetSPI()->Insert(bb, sData);
        
        v.push_back({manager->Copy(bb), t});
        bb.Destroy();
    }
    std::cout << "Total Size: " << m.GetSPI()->Size() << std::endl;
    std::cout << "Total Node: " << m.GetSPI()->totalNode() << std::endl;
    std::cout << "Total Usage: " << m.GetArena()->MemoryUsage() << std::endl;

    std::cout << "Testing Overlap Search Speed (Vector vs R-tree)..." << std::endl;
    TestSearchOverlapSpeed({{0,500},{0,500}},v, m);

    std::cout << "Testing Under Search Speed (Vector vs R-tree)..." << std::endl;
    TestSearchUnderSpeed({{100,200},{100,200}},v, m);

    std::cout << "Testing Cover Search Speed (Vector vs R-tree)..." << std::endl;
    TestSearchCoverSpeed({{0,500},{0,500}},v, m);


    for (auto& e : v) {
        e.first.Destroy();
    }

    return 0;
}

#undef SPI_MOVE_COUNT