// ==========
// This example demonstrates the implementation of spatial indexing for efficient 
// storage and querying of location data at Chulalongkorn University. It utilizes 
// memory management techniques from the MemTable and BBManager classes to optimize 
// spatial data processing and retrieval.
// ==========

#define SPI_MOVE_COUNT

#include "tagfilterdb/memtable.h"
#include "tagfilterdb/memPool.h"

#include <string>
#include <cstring>
#include <vector>
#include <iomanip>
#include <iostream>
#include <cstdio>

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

class LocationCallBack : public SpICallBack {
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
            std::cout << LocationToString(v[i].data) 
            << bbm->toString(*v[i].box) << std::endl;       
        }
    }
};

void FirstSave() {
    SpatialIndexOptions sop;
    sop.FILENAME = "sExample2.tin";
    MemPoolOpinion mop;
    mop.FILENAME = "mExample2.tin";
    MemTable m(sop,mop);
    auto manager = m.GetSPI()->GetBBManager();
    size_t size = 10000;
    size_t range = 10000;
    Random r(101);
    for (int i = 0; i < size; i++) {
        int id = r.Uniform(range);
        double a = r.Uniform(range);
        double b = r.Uniform(range);
        double c = r.Uniform(range);
        double d = r.Uniform(range);

        auto loc = Location(std::to_string(id), "House", std::min(a, b), std::max(a, b)
                                                            , std::min(c, d), std::max(c, d));
        auto locVE = manager->CreateBox(loc.getLocation());
        SignableData* data = m.GetMempool()->Insert(loc.Serialize());
        m.GetSPI()->Insert(locVE, data);
        locVE.Destroy();
    }

    m.GetMempool()->Flush();

    m.GetSPI()->flush();

    m.GetSPI()->GetManager()->PrintPageInfo();
    std::cout <<  "Memory Usage: " << m.GetArena()->MemoryUsage() << std::endl;  
}

void Save(int seed) {
    SpatialIndexOptions sop;
    sop.FILENAME = "sExample2.tin";
    MemPoolOpinion mop;
    mop.FILENAME = "mExample2.tin";
    MemTable  m(sop,mop);
    auto manager = m.GetSPI()->GetBBManager();
    m.GetSPI()->Load();
    m.GetMempool()->manager_.Load();
    
    size_t size = 10000;
    size_t range = 10000;
    Random r(seed);
    for (int i = 0; i < size; i++) {
        int id = r.Uniform(range);
        double a = r.Uniform(range);
        double b = r.Uniform(range);
        double c = r.Uniform(range);
        double d = r.Uniform(range);

        auto loc = Location(std::to_string(id), "House", std::min(a, b), std::max(a, b)
                                                            , std::min(c, d), std::max(c, d));
        auto locVE = manager->CreateBox(loc.getLocation());
        SignableData* data = m.GetMempool()->Insert(loc.Serialize());
        m.GetSPI()->Insert(locVE, data);
        locVE.Destroy();
    }
   
    m.GetMempool()->Flush();
    m.GetSPI()->flush();
    // m.GetSPI()->GetManager()->PrintPageInfo();
    std::cout << "Time: " << seed << std::endl;
    std::cout <<  "Memory Usage: " << m.GetArena()->MemoryUsage() << std::endl;
}

void DeleteFile() {
    const char* filename = "sExample2.tin";

    if (std::remove(filename) == 0) {
        std::cout << "File deleted successfully: " << filename << std::endl;
    } else {
        std::perror("Error deleting file");
    }

    filename = "mExample2.tin";

    if (std::remove(filename) == 0) {
        std::cout << "File deleted successfully: " << filename << std::endl;
    } else {
        std::perror("Error deleting file");
    }
}

void Scan() {
    SpatialIndexOptions sop;
    sop.FILENAME = "sExample2.tin";
    MemPoolOpinion mop;
    mop.FILENAME = "mExample2.tin";
    MemTable  m(sop,mop);
    auto manager = m.GetSPI()->GetBBManager();
    m.GetSPI()->Load();
    m.GetMempool()->manager_.Load();

    m.GetSPI()->Print(LocationToString);
    std::cout << "Total Node: " << m.GetSPI()->totalNode() << std::endl;
    std::cout << "Cache Node Total Usage:  " << m.GetSPI()->GetCache()->TotalUsage()  << std::endl;
    std::cout << "Cache Data Total Usage:  " << m.GetMempool()->cache_.TotalUsage()  << std::endl;
    std::cout << "Memory Usage: " << m.GetArena()->MemoryUsage() << std::endl;
}

void SearchCover(VE query_v) {
    SpatialIndexOptions sop;
    sop.FILENAME = "sExample2.tin";
    MemPoolOpinion mop;
    mop.FILENAME = "mExample2.tin";
    MemTable  m(sop,mop);
    auto manager = m.GetSPI()->GetBBManager();
    m.GetSPI()->Load();
    m.GetMempool()->manager_.Load();

    LocationCallBack callback;
        auto query_bb = m.GetSPI()->GetBBManager()->CreateBox(query_v);
    std::cout << "Find SearchCover: " << m.GetSPI()->GetBBManager()->toString(query_bb) << std::endl;

    // Test R-tree-based search (searching for "under")
    auto start_time = std::chrono::high_resolution_clock::now();
    m.GetSPI()->SearchCover(query_bb, &callback);
    auto end_time = std::chrono::high_resolution_clock::now();

    std::cout << "Found: " << callback.v.size() << std::endl;

    auto rtree_elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
    std::cout << "R Tree SearchCover Time: " << rtree_elapsed_time << " microseconds";

    #if defined(SPI_MOVE_COUNT)
    std::cout << ", " << callback.move_count << " node";
    #endif

    std::cout << std::endl;

    std::cout << "Sample: " << std::endl;
    callback.Sample(m.GetSPI()->GetBBManager());
    query_bb.Destroy();
    
    std::cout << "Cache Node Total Usage:  " << m.GetSPI()->GetCache()->TotalUsage()  << std::endl;
    std::cout << "Cache Data Total Usage:  " << m.GetMempool()->cache_.TotalUsage()  << std::endl;
    std::cout << "Memory Usage: " << m.GetArena()->MemoryUsage() << std::endl;
}


void SearchOverlap(VE query_v) {
    SpatialIndexOptions sop;
    sop.FILENAME = "sExample2.tin";
    MemPoolOpinion mop;
    mop.FILENAME = "mExample2.tin";
    MemTable  m(sop,mop);
    auto manager = m.GetSPI()->GetBBManager();
    m.GetSPI()->Load();
    m.GetMempool()->manager_.Load();

    LocationCallBack callback;
        auto query_bb = m.GetSPI()->GetBBManager()->CreateBox(query_v);
    std::cout << "Find SearchOverlap: " << m.GetSPI()->GetBBManager()->toString(query_bb) << std::endl;

    // Test R-tree-based search (searching for "under")
    auto start_time = std::chrono::high_resolution_clock::now();
    m.GetSPI()->SearchOverlap(query_bb, &callback);
    auto end_time = std::chrono::high_resolution_clock::now();

    std::cout << "Found: " << callback.v.size() << std::endl;

    auto rtree_elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
    std::cout << "R Tree SearchOverlap Time: " << rtree_elapsed_time << " microseconds";

    #if defined(SPI_MOVE_COUNT)
    std::cout << ", " << callback.move_count << " node";
    #endif

    std::cout << std::endl;
    std::cout << "Sample: " << std::endl;
    callback.Sample(m.GetSPI()->GetBBManager());
    query_bb.Destroy();

    std::cout << "Cache Node Total Usage:  " << m.GetSPI()->GetCache()->TotalUsage()  << std::endl;
    std::cout << "Cache Data Total Usage:  " << m.GetMempool()->cache_.TotalUsage()  << std::endl;
    std::cout << "Memory Usage: " << m.GetArena()->MemoryUsage() << std::endl;
}


void SearchUnder(VE query_v) {
    SpatialIndexOptions sop;
    sop.FILENAME = "sExample2.tin";
    MemPoolOpinion mop;
    mop.FILENAME = "mExample2.tin";
    MemTable  m(sop,mop);
    auto manager = m.GetSPI()->GetBBManager();
    m.GetSPI()->Load();
    m.GetMempool()->manager_.Load();

    LocationCallBack callback;
        auto query_bb = m.GetSPI()->GetBBManager()->CreateBox(query_v);
    std::cout << "Find SearchUnder: " << m.GetSPI()->GetBBManager()->toString(query_bb) << std::endl;

    // Test R-tree-based search (searching for "under")
    auto start_time = std::chrono::high_resolution_clock::now();
    m.GetSPI()->SearchUnder(query_bb, &callback);
    auto end_time = std::chrono::high_resolution_clock::now();

    std::cout << "Found: " << callback.v.size() << std::endl;

    auto rtree_elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
    std::cout << "R Tree SearchUnder Time: " << rtree_elapsed_time << " microseconds";

    #if defined(SPI_MOVE_COUNT)
    std::cout << ", " << callback.move_count << " node";
    #endif

    std::cout << std::endl;

    std::cout << "Sample: " << std::endl;
    callback.Sample(m.GetSPI()->GetBBManager());
    query_bb.Destroy();

    std::cout << "Cache Node Total Usage:  " << m.GetSPI()->GetCache()->TotalUsage()  << std::endl;
    std::cout << "Cache Data Total Usage:  " << m.GetMempool()->cache_.TotalUsage()  << std::endl;
    std::cout << "Memory Usage: " << m.GetArena()->MemoryUsage() << std::endl;
}

int sp_example2() {
    // FirstSave();
    // for (int i = 0; i < 9; i++) {
    //     Save(i);
    // }
    // Scan();

    SearchOverlap({{0,500},{0, 500}});
    SearchCover({{1000,2000},{1000,2000}});
    // SearchUnder({{100,200},{100,200}});

    // DeleteFile();
    return 0;
}