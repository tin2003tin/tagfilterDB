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
#include <cmath>

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

int sp_example3() {
    SpatialIndexOptions op;
    op.DIMENSION = 2;
    op.MAX_CHILD = 8;
    op.MIN_CHILD = 4;
    op.PAGE_MAX_BYTES = 1024 * 4;

    MemTable m(op);
    size_t size = 100;
    size_t range = 100000;

    auto manager = m.GetSPI()->GetBBManager();
    // VE chula_locate = {{100,200},{100,200}};
    // auto bb = manager->CreateBB(chula_locate);
    // Test chula = Test("Chula");
    // m.InsertSpiral(bb, &chula);

    Random r(100);
    for (size_t i = 0; i < size; i++) {
        Test t = Test("3D: " + std::to_string(i + 1));
        double a = r.Uniform(range);
        double b = r.Uniform(range);
        double c = r.Uniform(range);
        double d = r.Uniform(range);
        double e = r.Uniform(range);
        double f = r.Uniform(range);

        auto bb = manager->CreateBox({{std::min(a, b), std::max(a, b)}, 
                                    {std::min(c, d), std::max(c, d)},
                                    {std::min(e, f), std::max(e, f)}});
        m.InsertSpiral(bb, &t);
    }
    std::cout << "Total Size: " << m.GetSPI()->Size() << std::endl;
    std::cout << "Total Height: " << m.GetSPI()->Height() << std::endl;
    m.GetSPI()->Save();

    return 0;
}

#undef SPI_MOVE_COUNT