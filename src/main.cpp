#include "tagfilterdb/pageH.h"
#include "json.hpp"
#include <string>
#include <iostream>
#include <cstring>

#include <cstdlib>  // For rand() and srand()
#include <ctime>    // For seeding rand()

using namespace nlohmann;
using namespace tagfilterdb;

void SetJson(PageHeapManager* pageManager, const json& json) {
    std::string jsonData = json.dump();  

    pageManager->AddRecord(jsonData.data(), jsonData.size());
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

std::pair<long,int> Move(PageHeapManager* pageManager, int m) {
     auto iter = pageManager->begin();
     for (int i = 0 ; i < m; i++) {
        ++iter;
     }
     return *iter;
}


void RandomTestCase(PageHeapManager& pageManager, int numOperations) {
    std::srand(0); // Seed randomness

    for (int i = 0; i < numOperations; ++i) {
        int operation = std::rand() % 2; // 0: Add record, 1: Free record

        if (operation == 0) {
            // Add a random record
            json data;
            data["name"] = "User " + std::to_string(i + 1);
            data["age"] = 18 + (std::rand() % 50); // Random age between 18 and 67
            data["gpx"] = 2.0 + ((std::rand() % 200) / 100.0); // Random GPA between 2.0 and 4.0

            SetJson(&pageManager, data);
            std::string jsonString = data.dump();
        std::cout << "Added: " << jsonString << " " << jsonString.size() << "\n";
        } else {
            // Free a random record using the iterator
            auto iter = pageManager.begin();
            int count = pageManager.TotalCount();
            if (count == 0) {
                std::cerr << "No records available to free.\n";
                continue; // Skip freeing
            }

            int rn = std::rand();
            int skip =  rn % count; // Random record to free
            for (int j = 0; j < skip && iter != pageManager.end(); ++j) {
                ++iter;
            }

            if (iter != pageManager.end()) {
                auto loc = *iter;
                pageManager.FreeData(loc.first, loc.second);
                std::cout << "Freed record at PageID: " << loc.first << ", Offset: " << loc.second << "\n";
            }
        }
        pageManager.PrintPageInfo();
    }
}

int main() {
    PageHeapManager pageManager(1024 * 4); // Initialize PageHeapManager with 4KB pages

    int numOperations = 100; // Total number of operations to simulate

    RandomTestCase(pageManager, numOperations);

    std::cout << "\n=== Final Scan ===\n";
    Scan(&pageManager);

    return 0;
}

// int main() {
//     PageHeapManager pageManager(1024 * 4);

// for (int i = 0; i < 100; ++i) {
//     if (i == 6247) {
//         int temp = 0;
//         temp++;
//     }
//         json data;
//         data["name"] = "User " +  std::to_string(i+1);
//         data["age"] = 20 + i;
//         data["gpx"] = 3.0 + (i * 0.1);

//         SetJson(&pageManager, data);
//     }

// //    auto m = Move(&pageManager, 85);
// //    pageManager.FreeData(m.first, m.second);

//    auto m = Move(&pageManager, 0);
//    pageManager.FreeData(m.first, m.second);

//    Scan(&pageManager); 

//     json data;
//     data["name"] = "Tin";
//     data["age"] = 21;

//     std::cout << "==========================\n";
//     SetJson(&pageManager, data);

//     Scan(&pageManager); 

//    return 0;
// }