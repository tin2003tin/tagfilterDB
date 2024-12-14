#pragma once

#include "tagfilterdb/memPool.h"
#include "heap_test.h"
#include "tagfilterdb/memtable.h"
#include <vector>
#include <memory>

using namespace tagfilterdb;
using namespace nlohmann;

class MemPoolTest {
  public :
  Arena arena_;
  MemPool memPool_;
  std::vector<BlockAddress> sample_;
  const int sampleSize = 100;

  public:
  MemPoolTest(int seed) :
  memPool_(MemPool(MemPoolOpinion(), &arena_))  {
      SetupHeapFile(seed);
    //   FetchData();
    //   FlushTest();
    //   ScanTest();
  }

  private:
  void SetupHeapFile(int seed) {
    int numOperations = 2000;
    std::cout << "Trying.. Add/Free data and Save to file" << std::endl;

    MemPoolOpinion op;
    ShareLRUCache<PageHeap> tempCache(op.CACHE_CHARGE);
    PageHeapManager manager(1024*4, &tempCache);
    RandomTestCase1(seed, &manager, numOperations, sample_, sampleSize);
    std::cout << "Finished!! Add/Free data and Save to file" << std::endl;
    // Scan(&memPool_.manager_);
    manager.Save();
  }

  void FetchData() {
    MemPoolOpinion op;
    std::cout << "Sample Size: " << sample_.size() << std::endl;
    memPool_.manager_.Load();
    std::cout << "TotalPage: " << memPool_.manager_.LastPageID() << std::endl;

    for (auto& addr : sample_) {
      DataView* data = memPool_.Get(addr);
      if (data == nullptr) {
        continue;
      }
      try {
        std::string jsonString = std::string(data->data, data->size);
        json jsonData = json(jsonString);
        std::cout << "Retrieved JSON: " << jsonData.dump(4) << std::endl;
      } catch (const json::parse_error& e) {
        throw std::runtime_error("Failed to parse JSON data: " + std::string(e.what()));
      }
    } 
    // memPool_.cache_.Detail();
  }

  void FlushTest() {
    std::vector<SignableData*> vec;
    // Free First 5 items
    for (int i = 0; i < 5; i++) {
        auto iter = memPool_.manager_.begin();
        int n = i;
        while (n > 0 && iter != memPool_.manager_.end()) {
            ++iter;
            n--;
        }
        memPool_.Delete(*iter);
    }
    // Add Unsigned 10 items
    for (int i = 0; i < 10; i++) {
        json data;
        data["name"] = "user " + std::to_string(i);
        data["grade"] = 3.5;
        data["gender"] = "male";  

        std::string dataString = data.dump();
        char* rawData = new char[dataString.size()];
        std::memcpy(rawData, dataString.data(), dataString.size());

        vec.push_back(memPool_.Insert(DataView{rawData, dataString.size()}));   
    }
    // Scan(&memPool_.manager_);
    memPool_.Flush();
  }

  void ScanTest() {
    MemPoolOpinion op;
    ShareLRUCache<PageHeap> tempCache(op.CACHE_CHARGE);
    PageHeapManager manager(1024*4, &tempCache);
    manager.Load();
    std::cout << "-----Scan-----" << std::endl;
    Scan(&manager);
  }
};

void mempool_test() {
  MemPoolTest(0);
}