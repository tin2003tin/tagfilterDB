#ifndef TAGFILTER_TABLE_FORMAT_H
#define TAGFILTER_TABLE_FORMAT_H

#include <cstdint>
#include <string>

#include "coding.h"
#include "tagfilterdb/dataView.h"
#include "tagfilterdb/env.h"
#include "tagfilterdb/options.h"
#include "tagfilterdb/status.h"

namespace tagfilterdb {

static const uint64_t kTableMagicNumber = 0xdb4775248b80fb57ull;

static const size_t kBlockTrailerSize = 5;

// BlockHandle is a pointer to the extent of a file that stores a data
// block or a meta block.
class BlockHandle {
  public:
    enum { kMaxEncodedLength = 10 + 10 };

    BlockHandle()
        : offset_(~static_cast<uint64_t>(0)), size_(~static_cast<uint64_t>(0)) {
    }

    uint64_t GetOffset() const { return offset_; }
    uint64_t GetSize() const { return size_; }

    void SetOffset(uint64_t offset) { offset_ = offset; }
    void SetSize(uint64_t size) { size_ = size; }

    void EncodeTo(std::string *dst) const {
        assert(offset_ != ~static_cast<uint64_t>(0));
        assert(size_ != ~static_cast<uint64_t>(0));
        PutVarint64(dst, offset_);
        PutVarint64(dst, size_);
    }

    Status DecodeFrom(DataView *input) {
        if (GetVarint64(input, &offset_) && GetVarint64(input, &size_)) {
            return Status::OK();
        } else {
            return Status::Corruption("bad block handle");
        }
    }
    uint64_t GetSize() const { return size_; }

  private:
    uint64_t offset_ = 0;
    uint64_t size_ = 0;
};

// Footer encapsulates the fixed information stored at the tail
// end of every table file.
class Footer {
  public:
    enum { kEncodedLength = 2 * BlockHandle::kMaxEncodedLength + 8 };

    Footer() = default;

    const BlockHandle &GetMetaindexHandle() const { return metaindex_handle_; }
    void SetMetaindexHandle(const BlockHandle &h) { metaindex_handle_ = h; }

    const BlockHandle &GetIndexHandle() const { return index_handle_; }
    void SetIndexHandle(const BlockHandle &h) { index_handle_ = h; }

    void EncodeTo(std::string *dst) const {
        const size_t original_size = dst->size();
        metaindex_handle_.EncodeTo(dst);
        index_handle_.EncodeTo(dst);
        dst->resize(2 * BlockHandle::kMaxEncodedLength); // Padding
        PutFixed32(dst, static_cast<uint32_t>(kTableMagicNumber & 0xffffffffu));
        PutFixed32(dst, static_cast<uint32_t>(kTableMagicNumber >> 32));
        assert(dst->size() == original_size + kEncodedLength);
        (void)original_size; // Disable unused variable warning.
    }

    Status DecodeFrom(DataView *input) {
        if (input->size() < kEncodedLength) {
            return Status::Corruption("not an sstable (footer too short)");
        }

        const char *magic_ptr = input->data() + kEncodedLength - 8;
        const uint32_t magic_lo = DecodeFixed32(magic_ptr);
        const uint32_t magic_hi = DecodeFixed32(magic_ptr + 4);
        const uint64_t magic = ((static_cast<uint64_t>(magic_hi) << 32) |
                                (static_cast<uint64_t>(magic_lo)));
        if (magic != kTableMagicNumber) {
            return Status::Corruption("not an sstable (bad magic number)");
        }

        Status result = metaindex_handle_.DecodeFrom(input);
        if (result.ok()) {
            result = index_handle_.DecodeFrom(input);
        }
        if (result.ok()) {
            // We skip over any leftover data (just padding for now) in "input"
            const char *end = magic_ptr + 8;
            *input = DataView(end, input->data() + input->size() - end);
        }
        return result;
    }

  private:
    BlockHandle metaindex_handle_;
    BlockHandle index_handle_;
};

struct BlockContents {
    DataView data;       // Actual contents of data
    bool cacheable;      // True iff data can be cached
    bool heap_allocated; // True iff caller should delete[] data.data()
};

Status ReadBlock(RandomAccessFile *file, const ReadOptions &options,
                 const BlockHandle &handle, BlockContents *result) {
    result->data = DataView();
    result->cacheable = false;
    result->heap_allocated = false;

    size_t n = (size_t)(handle.GetSize());
    char *buf = new char[n + kBlockTrailerSize];
    DataView contents;
    Status s =
        file->Read(handle.GetOffset(), n + kBlockTrailerSize, &contents, buf);
    if (!s.ok()) {
        delete[] buf;
        return s;
    }
    if (contents.size() != n + kBlockTrailerSize) {
        delete[] buf;
        return Status::Corruption("truncated block read");
    }

    const char *data = contents.data(); // Pointer to where block data starts
    if (options.verify_checksums) {
        // TODO: Implement checksum verification
    }

    switch (data[n]) {
    case kNoCompression:
        if (data != buf) {
            // File implementation gave us pointer to some other data.
            // Use it directly under the assumption that it will be live
            // while the file is open.
            delete[] buf;
            result->data = DataView(data, n);
            result->heap_allocated = false;
            result->cacheable = false; // Need to copy to make contiguous
        } else {
            result->data = DataView(buf, n);
            result->heap_allocated = true;
            result->cacheable = true;
        }
        break;
    case kSnappyCompression: {
        // TODO: Implement Snappy compression
    }
    case kZstdCompression: {
        // TODO: Implement Zstd compression
    }
    default:
        delete[] buf;
        return Status::Corruption("bad block type");
    }
    return Status::OK();
}

} // namespace tagfilterdb

#endif