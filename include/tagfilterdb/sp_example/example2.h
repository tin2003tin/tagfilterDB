// ==========
// This example demonstrates the implementation of spatial indexing for efficient 
// storage, removal, and querying of location data. It showcases memory management 
// techniques using the MemTable and BBManager classes, including the insertion, 
// removal, and re-insertion of location entries, with performance tracking for 
// memory usage and size of the spatial index.
// ==========

#include "tagfilterdb/memtable.h"
#include <string>
#include <cstring>
#include <vector>
#include <iomanip>
#include <iostream>

using namespace tagfilterdb;
using VE = std::vector<BBManager::BB::Edge>;

class Location : public Interface {
    std::string id;
    std::string name;
    double x_min, x_max, y_min, y_max;

public:
    Location(std::string id, std::string name, double x_min, double x_max, double y_min, double y_max) 
        : id(std::move(id)), name(std::move(name)), x_min(x_min), x_max(x_max), y_min(y_min), y_max(y_max) {}

    Interface* Align(Arena* arena) override {
        char* obj_memory = arena->Allocate(sizeof(Location));

        char* id_memory = arena->Allocate(id.size() + 1);
        std::memcpy(id_memory, id.data(), id.size());
        id_memory[id.size()] = '\0';

        char* name_memory = arena->Allocate(name.size() + 1);
        std::memcpy(name_memory, name.data(), name.size());
        name_memory[name.size()] = '\0';

        // Create a new Location object in the arena with aligned strings
        return new (obj_memory) Location(std::string(id_memory), std::string(name_memory), x_min, x_max, y_min, y_max);
    }

    std::string ToString() const override {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << "Location[ID: " << id
            << ", Name: " << name
            << ", X(" << x_min
            << ", " << x_max
            << "), Y(" << y_min
            << ", " << y_max
            << ")";
        return oss.str();
    }

    bool operator==(Interface* other) override {
        auto* other_location = dynamic_cast<Location*>(other);
        if (!other_location) {
            return false;
        }
        return id == other_location->id && name == other_location->name &&
               x_min == other_location->x_min && x_max == other_location->x_max &&
               y_min == other_location->y_min && y_max == other_location->y_max;
    }

    VE getLocation() const {
        return {
            BBManager::BB::Edge{x_min, x_max},
            BBManager::BB::Edge{y_min, y_max}
        };
    }
};

class TestSearchAllCallBack : public SpICallBack {
public:
    std::vector<SpICallBackValue> v;
    size_t move_count = 0;

    bool Process(SpICallBackValue value) override {
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
        std::cout << "Sample Callback Results:" << std::endl;
        for (int i = 0; i < 5 && i < v.size(); i++) {
            std::cout << bbm->toString(*v[i].box) << (*v[i].data)->ToString() << std::endl;
        }
    }
};

int sp_example2() {
    SpatialIndexOptions op;
    MemTable m(op);
    auto manager = m.GetSPI()->GetBBManager();

    // Create and insert multiple Location objects
    std::vector<Location> locations = {
        Location("001", "Home", 0.0, 100.0, 0.0, 50.0),
        Location("002", "Park", 150.0, 200.0, 50.0, 100.0),
        Location("003", "School", 50.0, 120.0, 100.0, 150.0),
        Location("004", "Mall", 200.0, 300.0, 20.0, 70.0),
        Location("005", "Library", 80.0, 130.0, 10.0, 60.0),
        Location("006", "Hospital", 250.0, 320.0, 90.0, 140.0),
        Location("007", "Office", 30.0, 80.0, 30.0, 80.0),
        Location("008", "Stadium", 400.0, 500.0, 200.0, 300.0),
        Location("009", "Airport", 0.0, 50.0, 300.0, 350.0),
        Location("010", "Train Station", 50.0, 100.0, 350.0, 400.0)
    };

    for (auto& loc : locations) {
        auto locVE = manager->CreateBox(loc.getLocation());
        m.InsertSpiral(locVE, &loc);
    }

    // // Print the spatial index structure
    m.GetSPI()->Print();
    // std::cout << "Total Size: " << m.GetSPI()->Size() << std::endl;
    // std::cout << "Total Usage: " << m.GetArena()->MemoryUsage() << std::endl;
    // std::cout << "============ Remove All ===============" << std::endl;

    // for (auto& loc : locations) {
    //     auto locVE = manager->CreateBB(loc.getLocation());
    //      m.GetSPI()->Remove(locVE, &loc);
    // }
    
    // m.GetSPI()->Print();
    // std::cout << "Total Size: " << m.GetSPI()->Size() << std::endl;
    // std::cout << "Total Usage: " << m.GetArena()->MemoryUsage() << std::endl;

    // std::cout << "============= Add All =================" << std::endl;
      
    // for (auto& loc : locations) {
    //     auto locVE = manager->CreateBB(loc.getLocation());
    //     m.InsertSpiral(locVE, &loc);
    // }

    // m.GetSPI()->Print();
    // std::cout << "Total Size: " << m.GetSPI()->Size() << std::endl;
    // std::cout << "Total Usage: " << m.GetArena()->MemoryUsage() << std::endl;
    
    // std::cout << "========== Remove Home, 1 =============" << std::endl;
    // // After Remove Home
    // auto locVE = manager->CreateBB(locations[0].getLocation());
    // m.GetSPI()->Remove(locVE, &locations[0]);

    // m.GetSPI()->Print();
    // std::cout << "Total Size: " << m.GetSPI()->Size() << std::endl;
    // std::cout << "Total Usage: " << m.GetArena()->MemoryUsage() << std::endl;

    // std::cout << "========== Remove Library, 5 =============" << std::endl;

    // // After Remove Library
    // locVE = manager->CreateBB(locations[4].getLocation());
    // m.GetSPI()->Remove(locVE, &locations[4]);

    // m.GetSPI()->Print();
    // std::cout << "Total Size: " << m.GetSPI()->Size() << std::endl;
    // std::cout << "Total Usage: " << m.GetArena()->MemoryUsage() << std::endl;

    m.GetSPI()->Save();

    return 0;
}