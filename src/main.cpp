#include "tagfilterdb/memPool.h"
#include "tagfilterdb/heap_test/heap_test.h"
#include <vector>
#include <memory>

using namespace tagfilterdb;

class MemPoolTest {

  Arena arena_;
  MemPool memPool_;
  std::vector<BlockAddress> sample_;

  public:
  MemPoolTest() : arena_(Arena()), 
  memPool_(MemPool(MemPoolOpinion(),&arena_))  {
    SetupHeapFile();
  }
  private:
  void SetupHeapFile() {
    int rn = std::rand() % 1000; 
    RandomTestCase1(rn, &memPool_.manager_, rn, sample_);
    Scan(&memPool_.manager_);
    std::cout << "Sample Size: " << sample_.size() << std::endl;
    memPool_.manager_.Save();
  }


};

int main() {
    MemPoolTest();
}