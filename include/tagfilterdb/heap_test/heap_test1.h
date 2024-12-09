#pragma once

#include "tagfilterdb/pageH.h"
#include "json.hpp"
#include <string>
#include <iostream>
#include <cstring>

#include <cstdlib>  // For rand() and srand()
#include <ctime>    // For seeding rand()

using namespace nlohmann;
using namespace tagfilterdb;

BlockAddress SetJson(PageHeapManager* pageManager, const json& json) {
    std::string jsonData = json.dump();  

    return pageManager->AddRecord(jsonData.data(), jsonData.size());
}

json GetJson(PageHeapManager* pageManager, int PageID, int offset) {
    std::pair<char*, int> result = pageManager->GetData(PageID, offset);

    if (!result.first || result.second <= 0) {
        throw std::runtime_error("Invalid data retrieved from PageHeapManager");
    }

    std::string jsonString(result.first, result.second);
    std::cout << jsonString << std::endl;
    delete[] result.first;

    try {
        return json::parse(jsonString);
    } catch (const json::parse_error& e) {
        throw std::runtime_error("Failed to parse JSON data: " + std::string(e.what()));
    }
}


void Scan(PageHeapManager *pageManager) {
    
    auto iter = pageManager->begin();
    auto endIter = pageManager->end();

    while (iter != pageManager->end()) {
        auto result = GetJson(pageManager, (*iter).first,(*iter).second);
        std::cout << "Retrieved JSON: " << result.dump(4) << std::endl;
        ++iter;
    }

    for (int i = 1 ; i <= pageManager->Size(); i++) {
        pageManager->getPage(i)->PrintFree();
    }
}

BlockAddress Move(PageHeapManager* pageManager, int m) {
     auto iter = pageManager->begin();
     for (int i = 0 ; i < m; i++) {
        ++iter;
     }
     return *iter;
}


void RandomTestCase(int seed, PageHeapManager& pageManager, int numOperations) {
    std::srand(seed); // Seed randomness

    for (int i = 0; i < numOperations; ++i) {
        int operation = std::rand() % 3; // 0: Add record, 1: Free record
        if (operation != 0) {
            std::cout << "Trying to add.....\n";
            // Add a random record
            json data;
            data["name"] = "User " + std::to_string(i + 1);
            data["age"] = 18 + (std::rand() % 50); // Random age between 18 and 67
            data["gpx"] = 2.0 + ((std::rand() % 200) / 100.0); // Random GPA between 2.0 and 4.0

            BlockAddress blockAddress = SetJson(&pageManager, data);
            std::string jsonString = data.dump();
            std::cout << "Added: " << jsonString << ", Size " << jsonString.size() << "\n";
            std::cout << "At Page: " << blockAddress.first << ", Offset: " << blockAddress.second << "\n";
        } else {
             std::cout << "Trying to delete.....\n";
            // Free a random record using the iterator
            auto iter = pageManager.begin();
            int count = pageManager.TotalCount();
            if (count == 0) {
                std::cerr << "No records available to free.\n";
                continue; // Skip freeing
            }

            int rn = std::rand();
            int skip =  rn % count; // Random record to free
            std::cout << "Skip: "<<  skip << std::endl; 
            for (int j = 0; j < skip && iter != pageManager.end(); ++j) {
                ++iter;
            }

            if (iter != pageManager.end()) {
                auto loc = *iter;
                int freedSize = pageManager.FreeData(loc.first, loc.second);
                std::cout << "Freed record at PageID: " << loc.first << ", Offset: " << loc.second << "\n";
                std::cout << "Freed Size: " << freedSize << std::endl;;
            }
        }
        // pageManager.PrintPageInfo();
    }
}

int heap_test1() {
int numOperations = 10000;
    for (int i = 0; i < 2; i ++) {
        PageHeapManager pageManager(1024 * 4);
        RandomTestCase(i, pageManager, numOperations);
        std::cout << "\n=== Final Scan ===\n";
        Scan(&pageManager);
    }

    return 0;
}