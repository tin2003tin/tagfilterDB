#pragma once

#include "tagfilterdb/pageH.h"
#include "json.hpp"
#include <string>
#include <iostream>
#include <cstring>
#include "timer.h"

#include <cstdlib>  // For rand() and srand()
#include <ctime>    // For seeding rand()

using namespace nlohmann;
using namespace tagfilterdb;

BlockAddress SetJson(PageHeapManager* pageManager, const json& json) {
    std::string jsonData = json.dump();  

    return pageManager->AddRecord(jsonData.data(), jsonData.size());
}

json GetJson(PageHeapManager* pageManager, BlockAddress addr) {
    auto result = pageManager->GetData(addr);

    std::string jsonString(result.data, result.size);
    // std::cout << jsonString << std::endl;
    delete[] result.data;

    try {
        return json::parse(jsonString);
    } catch (const json::parse_error& e) {
        throw std::runtime_error("Failed to parse JSON data: " + std::string(e.what()));
    }
}


void Scan(PageHeapManager *pageManager) {
    if (pageManager->Size() == 0) {
        return;
    }
    auto iter = pageManager->begin();
    auto endIter = pageManager->end();

    while (iter != pageManager->end()) {
        auto result = GetJson(pageManager, (*iter));
        std::cout << "Retrieved JSON: " << result.dump(4) << std::endl;
        ++iter;
    }

    // for (int i = 1 ; i <= pageManager->Size(); i++) {
    //     pageManager->getPage(i)->Print Free();
    // }
}

BlockAddress Move(PageHeapManager* pageManager, int m) {
     auto iter = pageManager->begin();
     for (int i = 0 ; i < m; i++) {
        ++iter;
     }
     return *iter;
}


void RandomTestCase1(int seed, PageHeapManager* pageManager, int numOperations,
                    std::vector<BlockAddress>& sample, int sampleSize) {
    std::srand(seed); // Seed randomness

    for (int i = 0; i < numOperations; ++i) {
        if (i % 10000 == 0) {
            std::cout << "- " << i << std::endl;
        }
        int operation = std::rand() % 3; // 0: Add record, 1: Free record
        if (operation != 0) {
            // std::cout << "Trying to add.....\n";
            // Add a random record
            json data;
            data["name"] = "User " + std::to_string(i + 1);
            data["age"] = 18 + (std::rand() % 50); // Random age between 18 and 67
            data["gpx"] = 2.0 + ((std::rand() % 200) / 100.0); // Random GPA between 2.0 and 4.0

            BlockAddress blockAddress = SetJson(pageManager, data);
            // std::string jsonString = data.dump();
            // std::cout << "Added: " << jsonString << ", Size " << jsonString.size() << "\n";
            // std::cout << "At Page: " << blockAddress.first << ", Offset: " << blockAddress.second << "\n";
        } else {
            //  std::cout << "Trying to delete.....\n";
            // Free a random record using the iterator
            auto iter = pageManager->begin();
            int count = pageManager->TotalCount();
            if (count == 0) {
                // std::cerr << "No records available to free.\n";
                continue; // Skip freeing
            }

            

            int rn = std::rand();
            int skip =  rn % count; // Random record to free
            // std::cout << "Skip: "<<  skip << std::endl; 
            if (i == 601) {

                int temp = 0;

            }
            { 
                for (int j = 0; j < skip && iter != pageManager->end(); ++j) {
                if (j == 78) {

                    int temp1 = 1;
                }
                ++iter;
            }
            }
            


            if (iter != pageManager->end()) {
                auto loc = *iter;
                int freedSize = pageManager->FreeBlock(loc.pageID, loc.offset);
                // std::cout << "Freed record at PageID: " << loc.first << ", Offset: " << loc.second << "\n";
                // std::cout << "Freed Size: " << freedSize << std::endl;;
            }
        }
        // pageManager.PrintPageInfo();
    }

    int count = pageManager->TotalCount();
    for (int i = 0; i < sampleSize; i++) {
        while (true) {
        auto iter = pageManager->begin();
        int rn = std::rand();
        int skip =  rn % count;
         for (int j = 0; j < skip && iter != pageManager->end(); ++j) {
                ++iter;
        }
        if (iter != pageManager->end()) {
             sample.push_back(*iter);
             break;
        }
        }   
    }
}



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
            
            int sizeOpeartion = std::rand() % 20;
            json data;
            if (sizeOpeartion == 0) {
                GenerateBigData(data,i);
            } else if (sizeOpeartion < 5 ) {
                GenerateLargeData(data,i);
            } else {
                GenerateSmallData(data,i);
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

void heap_test() {
    for (int i = 0; i < 10000; i++) {
        {
            int rn = std::rand() % 10000; 
            ShareLRUCache<PageHeap> cache;
            std::vector<BlockAddress> sample;
            PageHeapManager pageManager(1024 * 4, &cache);
            RandomTestCase1(rn, &pageManager, rn, sample, 10);
            // pageManager.PrintPageInfo();
            // Scan(&pageManager);
            std::cout << i << ": Finished test1 Save! ";
            pageManager.Save();
        }
        {
            ShareLRUCache<PageHeap> cache;
            PageHeapManager pageManager(1024 * 4, &cache);
            pageManager.Load();
            // pageManager.LoadAtPage(1);
            // pageManager.LoadAtPage(2);
            // pageManager.PrintPageInfo();
            Scan(&pageManager);
             std::cout << " Finished test1 Load/Scan! " << std::endl;
        }
         {
            int rn = std::rand() % 2000; 
            ShareLRUCache<PageHeap> cache;
            PageHeapManager pageManager(1024 * 4, &cache);
            RandomTestCase2(rn, pageManager, rn);
            // pageManager.PrintPageInfo();
            Scan(&pageManager);
            std::cout << i << ": Finished test2 Save! ";
            pageManager.Save();
        }
        {
            ShareLRUCache<PageHeap> cache;
            PageHeapManager pageManager(1024 * 4, &cache);
            pageManager.Load();
            // pageManager.LoadAtPage(1);
            // pageManager.LoadAtPage(2);
            // pageManager.PrintPageInfo();
            Scan(&pageManager);
             std::cout << " Finished test2 Load/Scan! " << std::endl;
        }
    }
}