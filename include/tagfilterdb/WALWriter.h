#ifndef TAGFILTERDB_WAL_WRITER_H
#define TAGFILTERDB_WAL_WRITER_H

#include <fstream>
#include <cstdint>
#include <string>
#include <cassert>
#include <iostream>
#include "WALFormat.h" 

namespace tagfilterdb {

class WALWriter {
 public:
  explicit WALWriter(std::ofstream* dest);
  WALWriter(std::ofstream* dest, uint64_t dest_length);
  WALWriter(const WALWriter&) = delete;
  WALWriter& operator=(const WALWriter&) = delete;
  ~WALWriter();

  void AddRecord(const std::string& record);

 private:
  void EmitPhysicalRecord(tagfilterdb::wal::RecordType type, const char* ptr, size_t length);
  void InitTypeCrc();

  std::ofstream* dest_;  // Output file stream
  int block_offset_;     // Current offset in block
  uint32_t type_crc_[tagfilterdb::wal::kMaxRecordType + 1]; // CRC values for record types

};

void WALWriter::InitTypeCrc() {
  // Initialize CRC values for all record types
  for (int i = 0; i <= tagfilterdb::wal::kMaxRecordType; ++i) {
    type_crc_[i] = 0;  // Placeholder CRC value, you can replace it with actual CRC logic
  }
}

WALWriter::WALWriter(std::ofstream* dest)
    : dest_(dest), block_offset_(0) {
  InitTypeCrc();
}

WALWriter::WALWriter(std::ofstream* dest, uint64_t dest_length)
    : dest_(dest), block_offset_(dest_length % tagfilterdb::wal::kBlockSize) {
  InitTypeCrc();
}

WALWriter::~WALWriter() {
  if (dest_->is_open()) {
    dest_->close();
  }
}

void WALWriter::AddRecord(const std::string& record) {
  const char* ptr = record.c_str();
  size_t left = record.size();

  bool begin = true;
  while (left > 0) {
    const int leftover = tagfilterdb::wal::kBlockSize - block_offset_;
    if (leftover < tagfilterdb::wal::kHeaderSize) {
      // Switch to a new block
      if (leftover > 0) {
        // Fill the remaining space in the current block (if any)
        dest_->write("\x00\x00\x00\x00\x00\x00", leftover);
      }
      block_offset_ = 0;
    }

    // Ensure space for the record header
    const size_t avail = tagfilterdb::wal::kBlockSize - block_offset_ - tagfilterdb::wal::kHeaderSize;
    const size_t fragment_length = (left < avail) ? left : avail;

    tagfilterdb::wal::RecordType type;
    const bool end = (left == fragment_length);
    if (begin && end) {
      type = tagfilterdb::wal::kFullType;
    } else if (begin) {
      type = tagfilterdb::wal::kFirstType;
    } else if (end) {
      type = tagfilterdb::wal::kLastType;
    } else {
      type = tagfilterdb::wal::kMiddleType;
    }

    EmitPhysicalRecord(type, ptr, fragment_length);
    ptr += fragment_length;
    left -= fragment_length;
    begin = false;
  }
}

void WALWriter::EmitPhysicalRecord(tagfilterdb::wal::RecordType type, const char* ptr, size_t length) {
  assert(length <= 0xffff);  // Ensure length fits in two bytes
  assert(block_offset_ + tagfilterdb::wal::kHeaderSize + length <= tagfilterdb::wal::kBlockSize);

  // Format the header
  char buf[tagfilterdb::wal::kHeaderSize];
  buf[4] = static_cast<char>(length & 0xff);
  buf[5] = static_cast<char>(length >> 8);
  buf[6] = static_cast<char>(type);

  // Compute the CRC of the record type and the payload
  uint32_t crc = type_crc_[type];
  crc ^= static_cast<uint32_t>(*ptr); // Simple CRC calculation placeholder
  crc ^= length;

  // Write the header and the payload
  dest_->write(buf, tagfilterdb::wal::kHeaderSize);
  dest_->write(ptr, length);

  block_offset_ += tagfilterdb::wal::kHeaderSize + length;
}

}  // namespace tagfilterdb


#endif