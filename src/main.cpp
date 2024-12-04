#include <iostream>
#include "tagfilterdb/broundingbox.h"
#include "tagfilterdb/arena.h"
#include "tagfilterdb/spatialIndex.h"
#include "tagfilterdb/random.h"
#include <cstring>

using namespace tagfilterdb;

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

    std::string ToString() override {
        return s;
    }
};

int main() {
    SpatialIndexOptions op;
    Arena a;
    BBManager f(2, &a);
    SpatialIndex sp(op, &a);
    Random r(0);
    for (int i = 0 ; i < 10000; i++) {
        Test t  = Test("" + std::to_string(i + 1));
        double a = r.Uniform(100);
        double b = r.Uniform(100);
        double c = r.Uniform(100);
        double d = r.Uniform(100);

        std::vector<BBManager::BB::EDGE> v = {{std::min(a,b),std::max(a,b)},{std::min(c,d),std::max(c,d)}};
        
        BBManager::BB bb = f.CreateBB(v);

        sp.Insert(bb, &t);
    }
    sp.Print();

    std::cout << "Total Usage: " << a.MemoryUsage() << std::endl;
}

