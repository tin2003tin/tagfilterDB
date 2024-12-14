#ifndef TAGFILTERDB_DATAVIEW_H
#define TAGFILTERDB_DATAVIEW_H

#include <string>
#include <memory>
#include "arena.h"

namespace tagfilterdb {
    class DataView {
public:
    const char* data;  // Pointer to the data
    size_t size;       // Size of the data


    DataView() : data(nullptr), size(0) {}

    DataView(const char* d, size_t s) : data(d), size(s) {}

    DataView(const char* d, size_t s, Arena* arena) : data(d), size(s) {
        Align(arena);
    }

    void Align(Arena* arena) {
        char* memory = arena->AllocateAligned(size);
        if (!memory) {
            return; 
        }

        std::memcpy(memory, data, size);
        delete []data;
        data = memory;
    }

    const char& operator[](size_t idx) const {
        return data[idx];
    }

    // Example usage
    std::string toString() const {
        return std::string(data, size);
    }
};


using PageIDType = long;
using OffsetType = int;

struct BlockAddress {
    PageIDType pageID;
    OffsetType offset;
};
    
struct SignableData {
    DataView data;
    BlockAddress addr;

    SignableData(DataView aData, BlockAddress aAddr) 
        : data(aData), addr(aAddr) {}

    SignableData() : data(), addr(BlockAddress{0,0}) {}

    bool IsSigned() {
        return addr.pageID == 0;
    }
};

struct AdjustData {
    DataView sdata;
    BlockAddress oldAddr;
    BlockAddress newAddr;
};

}

#endif