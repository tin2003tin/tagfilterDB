#ifndef TAGFILTERDB_WAL_READER_H
#define TAGFILTERDB_WAL_READER_H

#include <fstream>
#include <string>
#include <memory>
#include <cstdio>
#include "WALFormat.h"  // Assuming this contains necessary enums, constants, etc.

namespace tagfilterdb {

class WALReader {
 public:
  // Interface for reporting errors.
  class Reporter {
   public:
    virtual ~Reporter() {}

    virtual void Corruption(size_t bytes, const std::string& status) = 0;
  };

  // Create a reader that will return log records from "*file".
  // "*file" must remain live while this Reader is in use.
  // If "reporter" is non-null, it is notified whenever some data is
  // dropped due to a detected corruption. "*reporter" must remain
  // live while this Reader is in use.
  // If "checksum" is true, verify checksums if available.
  WALReader(std::ifstream* file, Reporter* reporter, bool checksum,
            uint64_t initial_offset);

  // Deleted copy constructor and assignment operator
  WALReader(const WALReader&) = delete;
  WALReader& operator=(const WALReader&) = delete;

  // Destructor
  ~WALReader();

  // Read the next record into *record. Returns true if read successfully, false if we hit the end of the input.
  bool ReadRecord(std::string* record, std::string* scratch);

  // Returns the physical offset of the last record returned by ReadRecord.
  uint64_t LastRecordOffset();

 private:
  // Extend record types with special values
  enum {
    kEof = wal::kMaxRecordType + 1,
    kBadRecord = wal::kMaxRecordType + 2
  };

  // Skips blocks before the initial offset
  bool SkipToInitialBlock();

  // Reads a physical record from the file
  unsigned int ReadPhysicalRecord(std::string* result);

  // Reports corruption or other issues
  void ReportCorruption(uint64_t bytes, const char* reason);
  void ReportDrop(uint64_t bytes, const std::string& reason);

  // Member variables
  std::ifstream* const file_;
  Reporter* const reporter_;
  bool const checksum_;
  char* const backing_store_;
  std::string buffer_;
  bool eof_;

  // Offsets for the last record and the buffer
  uint64_t last_record_offset_;
  uint64_t end_of_buffer_offset_;
  uint64_t const initial_offset_;

  bool resyncing_;
};

WALReader::WALReader(std::ifstream* file, Reporter* reporter, bool checksum, uint64_t initial_offset)
    : file_(file), reporter_(reporter), checksum_(checksum),
      backing_store_(new char[wal::kBlockSize]), buffer_(""), eof_(false),
      last_record_offset_(0), end_of_buffer_offset_(0),
      initial_offset_(initial_offset), resyncing_(initial_offset > 0) {}

WALReader::~WALReader() {
  delete[] backing_store_;
}

bool WALReader::SkipToInitialBlock() {
  const size_t offset_in_block = initial_offset_ % wal::kBlockSize;
  uint64_t block_start_location = initial_offset_ - offset_in_block;

  // Don't search a block if we'd be in the trailer
  if (offset_in_block > wal::kBlockSize - 6) {
    block_start_location += wal::kBlockSize;
  }

  end_of_buffer_offset_ = block_start_location;

  // Skip to start of first block that can contain the initial record
  if (block_start_location > 0) {
    file_->seekg(block_start_location, std::ios::beg);
    if (!file_->good()) {
      ReportDrop(block_start_location, "Seek failed");
      return false;
    }
  }

  return true;
}

bool WALReader::ReadRecord(std::string* record, std::string* scratch) {
  if (last_record_offset_ < initial_offset_) {
    if (!SkipToInitialBlock()) {
      return false;
    }
  }

  scratch->clear();
  record->clear();
  bool in_fragmented_record = false;
  uint64_t prospective_record_offset = 0;

  std::string fragment;
  while (true) {
    const unsigned int record_type = ReadPhysicalRecord(&fragment);

    uint64_t physical_record_offset =
        end_of_buffer_offset_ - buffer_.size() - wal::kHeaderSize - fragment.size();

    if (resyncing_) {
      if (record_type == wal::kMiddleType) {
        continue;
      } else if (record_type == wal::kLastType) {
        resyncing_ = false;
        continue;
      } else {
        resyncing_ = false;
      }
    }

    switch (record_type) {
      case wal::kFullType:
        if (in_fragmented_record) {
          if (!scratch->empty()) {
            ReportCorruption(scratch->size(), "partial record without end(1)");
          }
        }
        prospective_record_offset = physical_record_offset;
        scratch->clear();
        *record = fragment;
        last_record_offset_ = prospective_record_offset;
        return true;

      case wal::kFirstType:
        if (in_fragmented_record) {
          if (!scratch->empty()) {
            ReportCorruption(scratch->size(), "partial record without end(2)");
          }
        }
        prospective_record_offset = physical_record_offset;
        scratch->assign(fragment.data(), fragment.size());
        in_fragmented_record = true;
        break;

      case wal::kMiddleType:
        if (!in_fragmented_record) {
          ReportCorruption(fragment.size(), "missing start of fragmented record(1)");
        } else {
          scratch->append(fragment.data(), fragment.size());
        }
        break;

      case wal::kLastType:
        if (!in_fragmented_record) {
          ReportCorruption(fragment.size(), "missing start of fragmented record(2)");
        } else {
          scratch->append(fragment.data(), fragment.size());
          *record = *scratch;
          last_record_offset_ = prospective_record_offset;
          return true;
        }
        break;

      case kEof:
        if (in_fragmented_record) {
          scratch->clear();
        }
        return false;

      case kBadRecord:
        if (in_fragmented_record) {
          ReportCorruption(scratch->size(), "error in middle of record");
          in_fragmented_record = false;
          scratch->clear();
        }
        break;

      default: {
        char buf[40];
        std::snprintf(buf, sizeof(buf), "unknown record type %u", record_type);
        ReportCorruption(
            (fragment.size() + (in_fragmented_record ? scratch->size() : 0)),
            buf);
        in_fragmented_record = false;
        scratch->clear();
        break;
      }
    }
  }
  return false;
}

unsigned int WALReader::ReadPhysicalRecord(std::string* result) {
  while (true) {
    if (buffer_.size() < wal::kHeaderSize) {
      if (!eof_) {
        buffer_.clear();
        file_->read(backing_store_, wal::kBlockSize);
        end_of_buffer_offset_ += file_->gcount();
        if (file_->fail()) {
          ReportDrop(wal::kBlockSize, "File read error");
          eof_ = true;
          return kEof;
        } else if (file_->gcount() < wal::kBlockSize) {
          eof_ = true;
        }
        continue;
      } else {
        buffer_.clear();
        return kEof;
      }
    }

    const char* header = buffer_.data();
    const uint32_t a = static_cast<uint32_t>(header[4]) & 0xff;
    const uint32_t b = static_cast<uint32_t>(header[5]) & 0xff;
    const unsigned int type = header[6];
    const uint32_t length = a | (b << 8);

    if (wal::kHeaderSize + length > buffer_.size()) {
      buffer_.clear();
      if (!eof_) {
        ReportCorruption(buffer_.size(), "bad record length");
        return kBadRecord;
      }
      return kEof;
    }

    *result = std::string(header + wal::kHeaderSize, length);
    return type;
  }
}

void WALReader::ReportCorruption(uint64_t bytes, const char* reason) {
  ReportDrop(bytes, reason);
}

void WALReader::ReportDrop(uint64_t bytes, const std::string& reason) {
  if (reporter_ != nullptr) {
    reporter_->Corruption(static_cast<size_t>(bytes), reason);
  }
}

uint64_t WALReader::LastRecordOffset() {
  return last_record_offset_;
}

}  // namespace tagfilterdb

#endif  // TAGFILTERDB_WAL_H
