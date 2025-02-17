#ifndef TAGFILTERDB_DATAVIEW_H
#define TAGFILTERDB_DATAVIEW_H

#include "arena.h"
#include "murmurHash.h"
#include <cstring>
#include <memory>
#include <string>

namespace tagfilterdb {

class DataView {
  public:
    const char *data_; // Pointer to the data
    size_t size_;      // Size of the data

    DataView() : data_(nullptr), size_(0) {}

    DataView(const char *d, size_t s) : data_(d), size_(s) {}

    DataView(const std::string &s) : data_(s.data()), size_(s.size()) {}

    DataView(const char *d, size_t s, Arena *arena) : data_(d), size_(s) {
        Align(arena);
    }

    std::string ToString() const { return std::string(data_, size_); }

    void Align(Arena *arena) {
        char *memory = arena->AllocateAligned(size_);
        if (!memory) {
            return;
        }

        std::memcpy(memory, data_, size_);
        delete[] data_; // This is incorrect because `data_` may not be
                        // dynamically allocated
        data_ = memory;
    }

    const char &operator[](size_t idx) const { return data_[idx]; }

    bool operator==(const DataView &other) const {
        return size_ == other.size_ &&
               std::memcmp(data_, other.data_, size_) == 0;
    }

    bool operator!=(const DataView &other) const { return !(*this == other); }

    bool operator<(const DataView &b) const {
        size_t min_len = (size_ < b.size_) ? size_ : b.size_;
        int r = std::memcmp(data_, b.data_, min_len);
        if (r == 0) {
            return size_ < b.size_;
        }
        return r < 0;
    }

    bool operator>(const DataView &b) const { return b < *this; }

    bool operator<=(const DataView &b) const { return !(b < *this); }

    bool operator>=(const DataView &b) const { return !(*this < b); }

    bool starts_with(const DataView &x) const {
        return ((size_ >= x.size_) &&
                (std::memcmp(data_, x.data_, x.size_) == 0));
    }

    std::size_t ComputeChecksum() const {
        return (data_ && size_ > 0) ? support::MurmurHash::Hash(data_, size_, 0)
                                    : 0;
    }

    int compare(const DataView &b) const {
        const size_t min_len = (size_ < b.size_) ? size_ : b.size_;
        int r = memcmp(data_, b.data_, min_len);
        if (r == 0) {
            if (size_ < b.size_)
                r = -1;
            else if (size_ > b.size_)
                r = +1;
        }
        return r;
    }

    size_t size() const { return size_; }

    const char *data() const { return data_; }

    std::string toString() const { return std::string(data_, size_); }
};

// Block Address
using PageIDType = long;
using OffsetType = int;

struct BlockAddress {
    PageIDType pageID;
    OffsetType offset;

    bool isSigned() const { return pageID > 0; }

    bool operator==(const BlockAddress &other) const {
        return pageID == other.pageID && offset == other.offset;
    }

    bool operator!=(const BlockAddress &other) const {
        return !(*this == other);
    }
};

// Signable Data
struct SignableData {
    DataView data;
    BlockAddress addr;

    SignableData(DataView aData, BlockAddress aAddr)
        : data(aData), addr(aAddr) {}

    SignableData() : data(), addr(BlockAddress{0, 0}) {}

    bool IsSigned() const { return addr.pageID == 0; }
};

// Adjust Data
struct AdjustData {
    DataView sdata;
    BlockAddress oldAddr;
    BlockAddress newAddr;
};

} // namespace tagfilterdb

#endif
