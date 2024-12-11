#include "tagfilterdb/pageH.h"
#include "tagfilterdb/skiplist.h"
#include "tagfilterdb/arena.h"
#include <iostream>

using namespace tagfilterdb;

struct BlockAddressComparator {
  int operator()(const BlockAddress& a, const BlockAddress& b) const {
    // Compare the PageID first, then Offset if necessary
    if (a.first != b.first) {
      return a.first - b.first;
    } else {
      return a.second - b.second;
    }
  }
};

int main() {
  Arena arena;
  
  SkipList<BlockAddress, BlockAddress, BlockAddressComparator> skiplist(BlockAddressComparator(), &arena);

  // Insert BlockAddress elements
  skiplist.Insert({5, 100}, {5, 100});
  skiplist.Insert({10, 200}, {10, 200});
  skiplist.Insert({3, 50}, {3, 50});

  if (skiplist.Contains({10, 200})) {
    std::cout << "BlockAddress (10, 200) found in the skip list!" << std::endl;
  }

  // Iterate over BlockAddress elements
  SkipList<BlockAddress, BlockAddress, BlockAddressComparator>::Iterator it(&skiplist);
  for (it.SeekToFirst(); it.Valid(); it.Next()) {
    std::cout << "(" << it.key().first << ", " << it.key().second << ") -> "
              << "(" << it.value().first << ", " << it.value().second << ") ";
  }
  std::cout << std::endl;

  return 0;
}
