#include "tagfilterdb/pageH.h"
#include "json.hpp"
#include <string>
#include <iostream>
#include <cstring>

using namespace nlohmann;
using namespace tagfilterdb;

void SetJson(PageHeap* page, const json& json) {
    std::string jsonData = json.dump();  
    int jsonSize = jsonData.size(); 

    bool flagAssigned = true; 
    bool flagIsAppend = false;  

    int dataSize = sizeof(flagAssigned) + sizeof(flagIsAppend) +
                   sizeof(jsonSize) + jsonSize;

    char buffer[dataSize]; 
    int offset = 0;

    std::memcpy(buffer + offset, &flagAssigned, sizeof(flagAssigned));
    offset += sizeof(flagAssigned);

    std::memcpy(buffer + offset, &flagIsAppend, sizeof(flagIsAppend));
    offset += sizeof(flagIsAppend);

    std::memcpy(buffer + offset, &jsonSize, sizeof(jsonSize));
    offset += sizeof(jsonSize);

    std::memcpy(buffer + offset, jsonData.data(), jsonSize);

    page->AppendData(buffer, dataSize); 
}

std::pair<json,Flag> GetJson(PageHeap* page, int offset) {
    Flag flag = page->LoadFlag(offset);
    int jsonSize = page->LoadSize(offset);

    std::string jsonData(jsonSize, '\0'); 
    page->LoadData(offset, &jsonData[0], jsonSize);
    return {json::parse(jsonData), flag};
}


int main() {
    PageHeap page(1, 1024 * 4);

    for (int i = 0; i < 10; ++i) {
        json data;
        data["name"] = "User " + std::to_string(i+1);
        data["age"] = 20 + i;
        data["gpx"] = 3.0 + (i * 0.1);

        SetJson(&page, data);
    }

    auto iter = page.begin();
    for (int i = 0; i < 4;i++) {
        ++iter;
    }
    page.Free(*iter);

    iter = page.begin();
    for (int i = 0; i < 5;i++) {
        ++iter;
    }
    page.Free(*iter);

    iter = page.begin();
    for (int i = 0; i < 6;i++) {
        ++iter;
    }
    page.Free(*iter);

    iter = page.begin();
    for (int i = 0; i < 5;i++) {
        ++iter;
    }
    page.Free(*iter);

    iter = page.begin();
    for (int i = 0; i < 3;i++) {
        ++iter;
    }
    page.Free(*iter);

      iter = page.begin();
    for (int i = 0; i < 1;i++) {
        ++iter;
    }
    page.Free(*iter);

       iter = page.begin();
    for (int i = 0; i < 2;i++) {
        ++iter;
    }
    page.Free(*iter);
    
    page.PrintFree();
    iter = page.begin();

    while (iter != page.end()) {

        auto result = GetJson(&page, *iter);
        std::cout << "Size: " << page.getOffsetSize(*iter) << std::endl;
        if (result.second.flagAssigned) {
            std::cout << "Flag Assigned" << ": " << result.second.flagAssigned << std::endl;
            std::cout << "Flag Append" << ": " << result.second.flagIsAppend << std::endl;
            std::cout << "Retrieved JSON: " << result.first.dump(4) << std::endl;
        }
        ++iter;
    }

    return 0;
}