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

std::string JsonToString(SignableData* sData) {
    return (JsonMgr::ToJson(sData->data)).dump();
}

class JsonCallBack : public SpICallBack {
    public :
    std::vector<std::pair<BBManager::BB,SignableData*>> v;
    size_t move_count = 0;

    bool Process(SpICallBackValue& value) {
        v.push_back({BBManager::BB(),value.data});
        BBManager::Move(v[v.size()-1].first, value.box);
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
            std::cout << JsonToString(v[i].second) 
            << bbm->toString(v[i].first) << std::endl;       
        }
    }

    json getAt(int index) {
        if (index > v.size()) {
            return json();
        }
        if (v[index].second->data.data == nullptr) {
            return json();
        }
        return JsonMgr::ToJson(v[index].second->data);    
    }

    ~JsonCallBack() {
        for (int i =0; i < v.size(); i++) {
            v[i].first.Destroy();            
        }
    }
};

void FirstSave() {
    SpatialIndexOptions sop;
    sop.DNAME = {{"x1","x2"},{"y1","y2"}};
    sop.FILENAME = "sExample3.tin";
    MemPoolOpinion mop;
    mop.FILENAME = "mExample3.tin";
    MemTable m(sop,mop);
    auto manager = m.GetSPI()->GetBBManager();

    size_t size = 100;
    size_t range = 100;
    Random r(101);
    for (int i = 0; i < size; i++) {
        int id = r.Uniform(range);
        double a = r.Uniform(range);
        double b = r.Uniform(range);
        double c = r.Uniform(range);
        double d = r.Uniform(range);

        json data;
        data["x1"] = std::min(a, b);
        data["x2"] = std::max(a, b);
        data["y1"] = std::min(c, d);
        data["y2"] = std::max(c, d);
        auto locVE = manager->CreateBox({{std::min(a, b), std::max(a, b)}
                                        , {std::min(c, d), std::max(c, d)}});
       
        std::string sData = data.dump();
        char* mem = new char[sData.size()];
        std::memcpy(mem,sData.data(),sData.size());
        DataView view(mem,sData.size());

        SignableData* signData = m.GetMempool()->Insert(view);
        m.GetSPI()->Insert(locVE, signData);
        locVE.Destroy();
    }

    m.Flush();
    m.GetSPI()->GetManager()->PrintPageInfo();
    std::cout <<  "Memory Usage: " << m.GetArena()->MemoryUsage() << std::endl;  
}

void Save(int seed) {
    SpatialIndexOptions sop;
    sop.DNAME = {{"x1","x2"},{"y1","y2"}};
    sop.FILENAME = "sExample3.tin";
    MemPoolOpinion mop;
    mop.FILENAME = "mExample3.tin";
    MemTable m(sop,mop);
    auto manager = m.GetSPI()->GetBBManager();

    m.GetSPI()->Load();
    m.GetMempool()->manager_.Load();
    
    size_t size = 10;
    size_t range = 100;
    Random r(seed);
    for (int i = 0; i < size; i++) {
        int id = r.Uniform(range);
        double a = r.Uniform(range);
        double b = r.Uniform(range);
        double c = r.Uniform(range);
        double d = r.Uniform(range);

        json data;
        data["x1"] = std::min(a, b);
        data["x2"] = std::max(a, b);
        data["y1"] = std::min(c, d);
        data["y2"] = std::max(c, d);
        auto locVE = manager->CreateBox({{std::min(a, b), std::max(a, b)}
                                        , {std::min(c, d), std::max(c, d)}});
        std::string sData = data.dump();
        char* mem = new char[sData.size()];
        std::memcpy(mem,sData.data(),sData.size());
        DataView view(mem,sData.size());

        SignableData* signData = m.GetMempool()->Insert(view);
        m.GetSPI()->Insert(locVE, signData);
        locVE.Destroy();
    }
   
    m.Flush();
    // m.GetSPI()->GetManager()->PrintPageInfo();
    std::cout << "Time: " << seed << std::endl;
    std::cout <<  "Memory Usage: " << m.GetArena()->MemoryUsage() << std::endl;
}

void DeleteFile() {
    const char* filename = "sExample3.tin";

    if (std::remove(filename) == 0) {
        std::cout << "File deleted successfully: " << filename << std::endl;
    } else {
        std::perror("Error deleting file");
    }

    filename = "mExample3.tin";

    if (std::remove(filename) == 0) {
        std::cout << "File deleted successfully: " << filename << std::endl;
    } else {
        std::perror("Error deleting file");
    }
}

void Scan() {
    SpatialIndexOptions sop;
    sop.DNAME = {{"x1","x2"},{"y1","y2"}};
    sop.FILENAME = "sExample3.tin";
    MemPoolOpinion mop;
    mop.FILENAME = "mExample3.tin";
    MemTable m(sop,mop);
    auto manager = m.GetSPI()->GetBBManager();

    m.GetSPI()->Load();
    m.GetMempool()->manager_.Load();

    m.GetSPI()->Print(JsonToString);
    std::cout << "Total Node: " << m.GetSPI()->totalNode() << std::endl;
    std::cout << "Cache Node Total Usage:  " << m.GetSPI()->GetCache()->TotalUsage()  << std::endl;
    std::cout << "Cache Data Total Usage:  " << m.GetMempool()->cache_.TotalUsage()  << std::endl;
    std::cout << "Memory Usage: " << m.GetArena()->MemoryUsage() << std::endl;
}

void SearchCover(VE query_v) {
    SpatialIndexOptions sop;
    sop.DNAME = {{"x1","x2"},{"y1","y2"}};
    sop.FILENAME = "sExample3.tin";
    MemPoolOpinion mop;
    mop.FILENAME = "mExample3.tin";
    MemTable  m(sop,mop);
    auto manager = m.GetSPI()->GetBBManager();
    m.GetSPI()->Load();
    m.GetMempool()->manager_.Load();

    JsonCallBack callback;
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
    sop.FILENAME = "sExample3.tin";
    sop.DNAME = {{"x1","x2"},{"y1","y2"}};
    MemPoolOpinion mop;
    mop.FILENAME = "mExample3.tin";
    MemTable  m(sop,mop);
    auto manager = m.GetSPI()->GetBBManager();
    m.GetSPI()->Load();
    m.GetMempool()->manager_.Load();

    JsonCallBack callback;
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
    sop.FILENAME = "sExample3.tin";
    sop.DNAME = {{"x1","x2"},{"y1","y2"}};
    MemPoolOpinion mop;
    mop.FILENAME = "mExample3.tin";
    MemTable  m(sop,mop);
    auto manager = m.GetSPI()->GetBBManager();
    m.GetSPI()->Load();
    m.GetMempool()->manager_.Load();

    JsonCallBack callback;
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

void DeleteAllOverlap(VE query_v) {
    SpatialIndexOptions sop;
    sop.FILENAME = "sExample3.tin";
    sop.DNAME = {{"x1","x2"},{"y1","y2"}};
    MemPoolOpinion mop;
    mop.FILENAME = "mExample3.tin";
    MemTable  m(sop,mop);
    auto manager = m.GetSPI()->GetBBManager();
    m.GetSPI()->Load();
    m.GetMempool()->manager_.Load();

    JsonCallBack callback;
    auto query_bb = m.GetSPI()->GetBBManager()->CreateBox(query_v);
    m.GetSPI()->SearchOverlap(query_bb, &callback);
    query_bb.Destroy();

    std::cout << "Found: " << callback.v.size() << std::endl;
    for (int i = 0; i < callback.v.size(); i++) {
        if (callback.v[i].second->data.data == nullptr) {
            callback.v[i].second = m.GetMempool()->Get(callback.v[i].second->addr);
        }
        json loc = callback.getAt(i);
        std::cout << "Delete: "<< loc.dump() << std::endl;
        assert(callback.v[i].second->data.data);
        m.GetSPI()->Remove(callback.v[i].first, callback.v[i].second);
        // m.GetSPI()->Print(LocationToString);
        int temp;
    }
    m.Flush();
    std::cout << "Memory Usage: " << m.GetArena()->MemoryUsage() << std::endl;
}

int sp_example3() {
    DeleteFile();
    FirstSave();
    Scan();
    DeleteAllOverlap({{0,50},{0, 50}});
    Scan();
    Save(1);
    DeleteAllOverlap({{80,100},{80, 100}});
    Scan();
    DeleteAllOverlap({{0,100},{0, 100}});
    Scan();

    DeleteFile();
    return 0;
}