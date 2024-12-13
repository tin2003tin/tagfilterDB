#include "tagfilterdb/pageH.h"
#include "tagfilterdb/skiplist.h"
#include "tagfilterdb/arena.h"
#include <iostream>

using namespace tagfilterdb;

struct BlockAddressCmp {
  int operator()(const BlockAddress& a, const BlockAddress& b) const {
    if (a.first != b.first) {
      return a.first - b.first;
    } else {
      return a.second - b.second;
    }
  }
};

int main() {
  Arena arena;

  SkipList<BlockAddress, BlockAddress, BlockAddressCmp> skiplist(BlockAddressCmp(), &arena);

  skiplist.Insert({5, 100}, {5, 100});
  skiplist.Insert({10, 200}, {10, 200});
  skiplist.Insert({3, 50}, {3, 50});

  if (skiplist.Contains({10, 200})) {
    std::cout << "BlockAddress (10, 200) found in the skip list!" << std::endl;
  }

  SkipList<BlockAddress, BlockAddress, BlockAddressCmp>::Iterator it(&skiplist);
  for (it.SeekToFirst(); it.Valid(); it.Next()) {
    std::cout << "(" << it.key().first << ", " << it.key().second << ") -> "
              << "(" << it.value().first << ", " << it.value().second << ") ";
  }

  std::cout << std::endl;

  return 0;
}
