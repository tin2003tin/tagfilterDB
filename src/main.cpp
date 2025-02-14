#include <iostream>
#include "tagfilterdb/boom_filter.h"

using namespace tagfilterdb;

int main() {
    // Create a Bloom filter with 10 bits per key
    BloomFilterPolicy bloomFilter(10);

    // Sample keys to insert
    std::vector<std::string> keys = {"apple", "banana", "cherry"};

    // Convert keys to DataView (assuming DataView is similar to std::string_view)
    std::vector<DataView> keyViews;
    for (const auto& key : keys) {
        keyViews.emplace_back(key);
    }

    // Create the filter
    std::string filter;
    bloomFilter.CreateFilter(keyViews.data(), keyViews.size(), &filter);

    // Test for key existence
    std::vector<std::string> testKeys = {"apple", "grape", "banana", "orange"};
    DataView bloom(filter);
    for (const auto& key : testKeys) {
        if (bloomFilter.KeyMayMatch(DataView(key), bloom)) {
            std::cout << key << " might be in the set.\n";
        } else {
            std::cout << key << " is definitely not in the set.\n";
        }
    }

    return 0;
}