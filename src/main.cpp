#include "tagfilterdb/memPool.h"
#include "tagfilterdb/heap_test/heap_test.h"
#include <vector>
#include <memory>

using namespace tagfilterdb;
using namespace  nlohmann;


class MemPoolTest {

  Arena arena_;
  MemPool memPool_;
  std::vector<BlockAddress> sample_;
  const int sampleSize = 100;

  public:
  MemPoolTest(int seed) : arena_(Arena()), 
  memPool_(MemPool(MemPoolOpinion(),&arena_))  {
      SetupHeapFile(seed);
      FetchData();
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
    ShareLRUCache<PageHeap> tempCache(op.CACHE_CHARGE);
    PageHeapManager manager(1024*4, &tempCache);
    std::cout << "Sample Size: " << sample_.size() << std::endl;
    manager.Load();
    std::cout << "TotalPage: " << manager.LastPageID() << std::endl;

    for (auto& addr : sample_) {
      std::pair<char*, int> result  = manager.FetchData(addr.first,addr.second);
      if (!result.first || result.second <= 0) {
          throw std::runtime_error("Invalid data retrieved from PageHeapManager");
      }
      std::string jsonString(result.first, result.second);
      delete[] result.first;

      try {
          json data = json::parse(jsonString);
          // std::cout << "Retrieved JSON: " << data.dump(4) << std::endl;
      } catch (const json::parse_error& e) {
          throw std::runtime_error("Failed to parse JSON data: " + std::string(e.what()));
      }
    } 
    tempCache.Detail();
    
  }


};

int main() {
  // std::cout << "Hello World" << std::endl;
  for (int i = 0; i < 1;i ++) {
     MemPoolTest m(i);
  }
 
}