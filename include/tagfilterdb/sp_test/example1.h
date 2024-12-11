// ==========
// This example demonstrates the implementation of spatial indexing for efficient 
// storage and querying of location data at Chulalongkorn University. It utilizes 
// memory management techniques from the MemTable and BBManager classes to optimize 
// spatial data processing and retrieval.
// ==========



#include "tagfilterdb/memtable.h"
#include <string>
#include <cstring>
#include <vector>
#include <iomanip>
#include <iostream>

using namespace tagfilterdb;
using VE = std::vector<BBManager::BB::Edge>;

class Location {
    std::string id;
    std::string name;
    double x_min, x_max, y_min, y_max;

public:
    Location() = default;
    explicit Location(std::string id, std::string name, double x_min, double x_max, double y_min, double y_max) 
        : id(std::move(id)), name(std::move(name)), x_min(x_min), x_max(x_max), y_min(y_min), y_max(y_max) {}

    Location* Align(Arena* arena) {
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

    std::string ToString() const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << "[ID: " << id
            << ", Name: " << name
            << ", X(" << x_min
            << ", " << x_max
            << "), Y(" << y_min
            << ", " << y_max
            << ")";
        return oss.str();
    }

    VE getLocation() const {
        return {
            BBManager::BB::Edge{x_min, x_max},
            BBManager::BB::Edge{y_min, y_max}
        };
    }
};


std::string LocationToString(Location** loc) {
    return (*loc)->ToString();
}

int sp_example1() {
    SpatialIndexOptions op;
    MemTable<Location*> m(op);
    auto manager = m.GetSPI()->GetBBManager();

        std::vector<Location> locations = {
            Location("001", "Chulalongkorn University", 0.0, 12000.0, 0.0, 12000.0), 
            Location("002", "Faculty of Engineering", 6919, 6919 + 1960, 7796 - 1175, 7796 ),
            Location("003", "Building 4 of Engineering", 8353, 8353 + 361, 7780 - 258, 7780 ),
            Location("004", "Building 3 of Engineering", 7076, 7076 + 1406, 7134 - 463, 7134 ),
            Location("005", "Building 2 of Engineering", 7782, 7782 + 523, 7450 - 265, 7450 ),
            Location("006", "Building 1 of Engineering", 7145, 7145 + 499, 7461 - 288, 7461),
            Location("006", "ENG Centenial Memorial Building", 8365, 8365 + 361, 7467 - 301, 7467),
            Location("007", "Icanteen", 8429, 8429 + 213, 6908 - 223, 6908),
            Location("008", "LarnGear", 8143, 8143 + 354, 7125 - 264, 7125),
            Location("009", "Faculty of Science", 4863, 4863 + 1632, 6616, 6616 + 1208),
            Location("010", "Faculty of Science", 4855, 4855 + 1185, 7920, 7920 + 1160),
            Location("011", "Science Canteen", 6111, 6111 + 248, 8016, 8016 + 728),
            Location("012", "Sala Phra Kiao ", 6351, 6351 + 40, 8192, 8192 + 425),
        };


    for (auto& loc : locations) {
        auto locVE = manager->CreateBox(loc.getLocation());
        m.GetSPI()->Insert(locVE, &loc);
    }


    m.GetSPI()->Print(LocationToString);
    // std::cout << "Total Size: " << m.GetSPI()->Size() << std::endl;
    // std::cout << "Total Usage: " << m.GetArena()->MemoryUsage() << std::endl;

    // m.GetSPI()->Save();

    return 0;
}