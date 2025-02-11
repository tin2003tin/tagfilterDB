#ifndef TAGFILTERDB_WAL_FORMAT_H
#define TAGFILTERDB_WAL_FORMAT_H

namespace tagfilterdb::wal {
    enum RecordType {
    // Zero is reserved for preallocated files
    kZeroType = 0,

    kFullType = 1,

    // For fragments
    kFirstType = 2,
    kMiddleType = 3,
    kLastType = 4
    };
    static const int kMaxRecordType = kLastType;

    static const int kBlockSize = 32768;

    // Header is checksum (4 bytes), length (2 bytes), type (1 byte).
    static const int kHeaderSize = 4 + 2 + 1;
}

#endif
