#pragma once

#include "cache.h"

void cache_example() {
    using namespace tagfilterdb;
    using SLC = ShareLRUCache<std::string>;

    // Create a cache with 4 LRU caches and total charge 1000
    SLC cache(1000);

    // Case 1: Insert a new key-value pair
    cache.Release(cache.Insert("630414821", "Siriwid Thongon"));

    // Case 2: Insert another key-value pair
    cache.Release(cache.Insert("65123", "John Doe"));

    // Case 3: Remove a key-value pair
    cache.Remove("65123");

    // Case 4: Retrieve a value from cache
    if (auto n = cache.Release(cache.Get("630414821")); n != nullptr) {
        std::cout << "Found 630414821: " << ShareLRUCache<std::string>::GetValue(n) << std::endl;
    }

    // Case 5: Try to retrieve a non-existent key
    if (auto n = cache.Release(cache.Get("123456789")); n == nullptr) {
        std::cout << "Key 123456789 not found in cache." << std::endl;
    }

    // Case 6: Insert a key with a custom charge
    cache.Release(cache.Insert("10001", "Alice", 200));

    // Case 7: Retrieve a key with custom charge
    if (auto n = cache.Release(cache.Get("10001")); n != nullptr) {
        std::cout << "Found 10001: " << ShareLRUCache<std::string>::GetValue(n) << std::endl;
    }

    // Case 8: Check total cache usage
    std::cout << "Total cache usage: " << cache.TotalUsage() << std::endl;

    // Case 9: Prune cache to remove stale entries
    cache.Prune();

    // Case 10: Print all caches
    cache.Print();

    // Case 11: Remove a key that doesn't exist (shouldn't cause errors)
    cache.Remove("99999");

    // Case 12: Add more keys and check if eviction happens
    cache.Release(cache.Insert("999", "Bob"));
    cache.Release(cache.Insert("1000", "Charlie"));
    cache.Release(cache.Insert("1001", "Dave"));
    cache.Release(cache.Insert("1002", "Eve"));

    // Case 13: Print caches after inserting more items
    cache.Print();

    // Case 14: Insert multiple keys and verify eviction and order
    cache.Release(cache.Insert("1500", "Grace"));
    cache.Release(cache.Insert("1501", "Heidi"));
    cache.Release(cache.Insert("1502", "Ivy"));
    cache.Release(cache.Insert("1503", "Jack"));
    
    cache.Print();

    // Case 15: Try to get value after cache eviction
    if (auto n = cache.Release(cache.Get("630414821")); n == nullptr) {
        std::cout << "Key 630414821 has been evicted from the cache." << std::endl;
    }

    // Case 16: Insert another key after eviction and check cache status
    cache.Release(cache.Insert("2000", "Zack"));
    cache.Print();

    // Case 17: Check if a key exists after being removed
    if (auto n = cache.Release(cache.Get("65123")); n == nullptr) {
        std::cout << "Key 65123 was removed and is no longer in the cache." << std::endl;
    }

    // Case 18: Release a handle to free cache space
    if (auto n = cache.Release(cache.Get("1001")); n != nullptr) {
        cache.Release(n);  // Release handle explicitly
        std::cout << "Released handle for key 1001." << std::endl;
    }

    // Case 19: Print detailed information of cache states
    cache.Detail();

    // Case 20: Attempt to get a key again after handle release
    if (auto n = cache.Release(cache.Get("1001")); n == nullptr) {
        std::cout << "Key 1001 is not found after handle release." << std::endl;
    }
}
