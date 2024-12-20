// ==========
// This example demonstrates the implementation of spatial indexing for efficient 
// storage and querying of location data at Chulalongkorn University. It utilizes 
// memory management techniques from the MemTable and BBManager classes to optimize 
// spatial data processing and retrieval.
// ==========



#include "tagfilterdb/memtable.h"
#include "tagfilterdb/memPool.h"
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

    DataView Serialize() const {
        size_t id_size = id.size();
        size_t name_size = name.size();
        size_t total_size = sizeof(size_t) * 2 + id_size + name_size + sizeof(double) * 4;

        char* memory = new char[total_size];
        char* ptr = memory;

        memcpy(ptr, &id_size, sizeof(size_t));
        ptr += sizeof(size_t);
        memcpy(ptr, id.data(), id_size);
        ptr += id_size;

        memcpy(ptr, &name_size, sizeof(size_t));
        ptr += sizeof(size_t);
        memcpy(ptr, name.data(), name_size);
        ptr += name_size;

        memcpy(ptr, &x_min, sizeof(double));
        ptr += sizeof(double);
        memcpy(ptr, &x_max, sizeof(double));
        ptr += sizeof(double);
        memcpy(ptr, &y_min, sizeof(double));
        ptr += sizeof(double);
        memcpy(ptr, &y_max, sizeof(double));

        return DataView(memory, total_size);
    }

    static Location Deserialize(const DataView& dataView) {
        if (!dataView.data || dataView.size == 0) {
           return Location();
        }

        const char* ptr = dataView.data;

        // Extract ID size and ID
        size_t id_size;
        memcpy(&id_size, ptr, sizeof(size_t));
        ptr += sizeof(size_t);
        std::string id(ptr, id_size);
        ptr += id_size;

        // Extract name size and name
        size_t name_size;
        memcpy(&name_size, ptr, sizeof(size_t));
        ptr += sizeof(size_t);
        std::string name(ptr, name_size);
        ptr += name_size;

        // Extract x_min, x_max, y_min, y_max
        double x_min, x_max, y_min, y_max;
        memcpy(&x_min, ptr, sizeof(double));
        ptr += sizeof(double);
        memcpy(&x_max, ptr, sizeof(double));
        ptr += sizeof(double);
        memcpy(&y_min, ptr, sizeof(double));
        ptr += sizeof(double);
        memcpy(&y_max, ptr, sizeof(double));

        // Construct and return the Location object
        return Location(id, name, x_min, x_max, y_min, y_max);
    }
};


std::string LocationToString(SignableData* sData) {
    return (Location::Deserialize(sData->data)).ToString();
}

void Load() {
    SpatialIndexOptions sop;
    MemPoolOpinion mop;
    mop.FILENAME = "locMemPool.tin";
    MemTable  m(sop,mop);
    auto manager = m.GetSPI()->GetBBManager();
    m.GetSPI()->Load();
    m.GetMempool()->manager_.Load();

    // for (int i = 100; i < 1000; i++) {
    //     auto loc = Location("00" + std::to_string(i), "Test", 0.0, 1000.0 + i * 10, 0.0, 1000.0 + i * 10);
    //     auto locVE = manager->CreateBox(loc.getLocation());
    //     SignableData* data = m.GetMempool()->Insert(loc.Serialize());
    //     m.GetSPI()->Insert(locVE, data);
    //     locVE.Destroy();
    // }
   
    m.GetSPI()->Print(LocationToString);
    // m.GetSPI()->GetCache()->Detail();

    // m.GetMempool()->Flush();
    // m.GetSPI()->flush();
    // m.GetSPI()->GetManager()->PrintPageInfo();
    // std::cout <<  "Memory Usage: " << m.GetArena()->MemoryUsage() << std::endl;
}

int sp_example1() {
    Load();
    return 0;
    SpatialIndexOptions sop;
    MemPoolOpinion mop;
    mop.FILENAME = "locMemPool.tin";
    MemTable m(sop,mop);
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
        SignableData* data = m.GetMempool()->Insert(loc.Serialize());
        m.GetSPI()->Insert(locVE, data);
        locVE.Destroy();
    }

    m.GetSPI()->Print(LocationToString);
    std::cout << "Total Node: " << m.GetSPI()->totalNode() << std::endl;
    std::cout << "Total Size: " << m.GetSPI()->Size() << std::endl;
    std::cout << "Total Usage: " << m.GetArena()->MemoryUsage() << std::endl;

    m.GetMempool()->Flush();
    m.GetSPI()->flush();
    m.GetSPI()->GetManager()->PrintPageInfo();

    return 0;
}