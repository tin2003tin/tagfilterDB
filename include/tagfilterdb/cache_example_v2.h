#pragma once

#include "tagfilterdb/cache_v2.h"
#include <iostream>

using namespace tagfilterdb;
void deleter(const DataView &key, void *value) {
    std::cout << "Deleting " << "Key: " << key.toString()
              << ", Value: " << ((DataView *)value)->toString() << std::endl;
}

void cache_example_v2() {
    using namespace tagfilterdb;

    // Create a cache with 4 LRU caches and total charge 1000
    ShareLRUCache::Config config =
        ShareLRUCache::Config{.SHARECACHE_BIT = 2,
                              .SHARECACHE_N = 1 << 2,
                              .SHARECACHE_TOTAL_CHARGE = 1000};
    Cache *cache = NewShareLRUCache(config);

    // Case 1: Insert a new key-value pair
    std::cout << "Case 1: Insert a new key - value pair" << std::endl;
    std::string name1 = "John Doe";
    DataView nameData1(name1);
    cache->Release(
        cache->Insert(DataView("101"), &nameData1, nameData1.size(), deleter));
    std::string name2 = "Siriwid";
    DataView nameData2(name2);
    cache->Release(
        cache->Insert(DataView("102"), &nameData2, nameData2.size(), deleter));
    std::string name3 = "Thongon";
    DataView nameData3(name3);
    cache->Release(
        cache->Insert(DataView("103"), &nameData3, nameData3.size(), deleter));
    std::string name4 = "Job";
    DataView nameData4(name4);
    cache->Release(
        cache->Insert(DataView("104"), &nameData4, nameData4.size(), deleter));
    cache->Print();

    // Case 2: Insert another key-value pair
    std::cout << "Case 2: Insert another key-value pair" << std::endl;
    std::string svalue = "John Doe";
    DataView value(svalue);
    CacheResponse *res = cache->Release(cache->Insert(
        DataView("65123"), (void *)&value, value.size(), deleter));
    if (res != nullptr) {
        std::cout << "Found 65123: " << value.toString() << std::endl;
    }
    // Case 3: Remove a key-value pair
    std::cout << "Case 3: Remove a key-value pair" << std::endl;
    cache->Remove(DataView("65123"));

    // Case 4: Retrieve a removed value from cache
    std::cout << "Case 4: Retrieve a removed value from cache" << std::endl;
    if (auto n = cache->Release(cache->Get(DataView("65123"))); n != nullptr) {
        std::cout << "Found 65123: " << n->GetValue() << std::endl;
    } else {
        std::cout << "Key 65123 not found in cache->" << std::endl;
    }

    // Case 5: Check total cache usage
    std::cout << "Case 5: Check total cache usage" << std::endl;
    std::cout << "Total cache usage: " << cache->TotalUsage() << std::endl;

    // Case 6: Prune cache to remove stale entries
    auto res101 = cache->Get(DataView("101"));
    auto res102 = cache->Get(DataView("102"));

    std::cout << "Case 6: Prune cache to remove stale entries" << std::endl;
    cache->Prune();

    // Case 7: Print all caches
    std::cout << "Case 7: Print all caches" << std::endl;
    cache->Print();

    // Case 8: Print all after released and Pruned
    std::cout << "Case 8: Print all after released and Pruned" << std::endl;
    cache->Release(res101);
    cache->Release(res102);
    cache->Prune();
    cache->Print();

    // Case 9: Remove a key that doesn't exist (shouldn't cause errors)
    std::cout << "Case 9: Remove a key that doesn't exist" << std::endl;
    cache->Remove(DataView("99999"));

    delete cache;
}
