#include "tagfilterdb/heap_test/heap_test1.h"
#include <iostream>
#include <string>

using json = nlohmann::json;


void GenerateLargeData(json& data, int seed) {
    for (int i = 0; i < 50; ++i) {
        data["large_key_" + std::to_string(seed) + "_" + std::to_string(i)] = {
            {"nested_key_1", "Large data string " + std::to_string(i)},
            {"nested_key_2", seed * i},
            {"nested_array", {i, i + 1, i + 2, i + 3, i + 4}},
            {"nested_object", {
                {"sub_key_1", "Large value " + std::to_string(seed)},
                {"sub_key_2", i * seed * 42},
                {"sub_key_3", "another_large_value"}
            }}
        };
    }
}

void GenerateBigData(json& data, int seed) {
    for (int i = 0; i < 10; ++i) {
        data["large_key_" + std::to_string(seed) + "_" + std::to_string(i)] = {
            {"nested_key_1", "Large data string " + std::to_string(i)},
            {"nested_key_2", seed * i},
            {"nested_array", {i, i + 1, i + 2, i + 3, i + 4}},
            {"nested_object", {
                {"sub_key_1", "Large value " + std::to_string(seed)},
                {"sub_key_2", i * seed * 42},
                {"sub_key_3", "another_large_value"}
            }}
        };
    }
}

void GenerateSmallData(json& data, int seed) {
    data["small_key_" + std::to_string(seed)] = {
        {"small_value", "Small data " + std::to_string(seed)},
        {"number", seed}
    };
}

void RandomTestCase2(int seed, PageHeapManager& pageManager, int numOperations) {
    std::srand(seed); 

    for (int i = 0; i < numOperations; ++i) {
        int operation = std::rand() % 3;
        if (operation != 0) {
            // std::cout << "Trying to add.....\n";
            
            int sizeOpeartion = std::rand() % 8;
            json data;
            if (sizeOpeartion == 0) {
                GenerateBigData(data,seed);
            } else if (sizeOpeartion == 1 || sizeOpeartion == 2 ) {
                GenerateLargeData(data,seed);
            } else {
                GenerateSmallData(data,seed);
            }

            BlockAddress blockAddress = SetJson(&pageManager, data);
            // std::string jsonString = data.dump();
            // std::cout << "Added: Size " << jsonString.size() << "\n";
            // std::cout << "At Page: " << blockAddress.first << ", Offset: " << blockAddress.second << "\n";
        } else {
            //  std::cout << "Trying to delete.....\n";
            // Free a random record using the iterator
            auto iter = pageManager.begin();
            int count = pageManager.TotalCount();
            if (count == 0) {
                // std::cerr << "No records available to free.\n";
                continue; // Skip freeing
            }

            int rn = std::rand();
            int skip =  rn % count; // Random record to free
            // std::cout << "Skip: "<<  skip << std::endl; 
            for (int j = 0; j < skip && iter != pageManager.end(); ++j) {
                ++iter;
            }

            // if (iter != pageManager.end()) {
            //     auto loc = *iter;
            //     int freedSize = pageManager.FreeData(loc.first, loc.second);
            //     std::cout << "Freed record at PageID: " << loc.first << ", Offset: " << loc.second << "\n";
            //     std::cout << "Freed Size: " << freedSize << std::endl;;
            // } 
        }
        // pageManager.PrintPageInfo();
    }
}

int main() {
    for (int i = 0; i < 1; i++) {
        // int rn = std::rand() % 1000; 
        PageHeapManager pageManager(1024 * 4);
        RandomTestCase1(0, pageManager, 9500);
        pageManager.PrintPageInfo();
        Scan(&pageManager);
        std::cout << "finished: " << i << std::endl;
    }

    return 0;
}