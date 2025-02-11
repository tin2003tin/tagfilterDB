#ifndef TAGFILTERDB_DATAVIEW_H
#define TAGFILTERDB_DATAVIEW_H

#include <string>
#include <memory>
#include "arena.h"
#include "murmurHash.h"

namespace tagfilterdb {
    class DataView {
public:
    const char* data_;  // Pointer to the data
    size_t size_;       // Size of the data


    DataView() : data_(nullptr), size_(0) {}

    DataView(const char* d, size_t s) : data_(d), size_(s) {}
    
    DataView(const std::string& s) : data_(s.data()), size_(s.size()) {}

    DataView(const char* d, size_t s, Arena* arena) : data_(d), size_(s) {
        Align(arena);
    }

    std::string ToString() const { return std::string(data_, size_); }

    void Align(Arena* arena) {
        char* memory = arena->AllocateAligned(size_);
        if (!memory) {
            return; 
        }

        std::memcpy(memory, data_, size_);
        delete []data_;
        data_ = memory;
    }

    const char& operator[](size_t idx) const {
        return data_[idx];
    }

    bool operator==(const DataView& other) const {
        auto thisChecksum = ComputeChecksum();
        auto otherChecksum = other.ComputeChecksum();

        return thisChecksum == otherChecksum;
    }

    bool starts_with(const DataView& x) const {
        return ((size_ >= x.size_) && (memcmp(data_, x.data_, x.size_) == 0));
    }

    std::size_t ComputeChecksum() const {
        std::size_t hash = 0;

        if (data_ && size_ > 0) {
            hash = support::MurmurHash::Hash(data_, size_, 0); 
        }

        return hash;
    }

    size_t size() const {
        return size_;
    }

    const char* data() const {
        return data_;
    }

    std::string toString() const {
        return std::string(data_, size_);
    }
};

using PageIDType = long;
using OffsetType = int;

struct BlockAddress {
    PageIDType pageID;
    OffsetType offset;

    bool isSigned() {
        return pageID > 0;
    }

    bool operator==(const BlockAddress& other) const {
        return pageID == other.pageID && offset == other.offset;
    } 
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