#ifndef TAGFILTERDB_OPTIONS_H
#define TAGFILTERDB_OPTIONS_H

#include "cache_v2.h"
#include "comparator.h"
#include "env.h"
#include "filter_policy.h"

namespace tagfilterdb {

class Snapshot {
  protected:
    virtual ~Snapshot() = default;
};

enum CompressionType {
    // NOTE: do not change the values of existing entries, as these are
    // part of the persistent format on disk.
    kNoCompression = 0x0,
    kSnappyCompression = 0x1,
    kZstdCompression = 0x2,
};

struct Options {
    Options() : comparator(BytewiseComparator()), env(Env::Default()) {}

    const Comparator *comparator;
    bool create_if_missing = false;
    bool error_if_exists = false;
    bool paranoid_checks = false;
    Env *env;
    // Logger* info_log = nullptr;
    size_t write_buffer_size = 4 * 1024 * 1024;
    int max_open_files = 1000;
    Cache *block_cache = nullptr;
    size_t block_size = 4 * 1024;
    int block_restart_interval = 16;
    size_t max_file_size = 2 * 1024 * 1024;
    CompressionType compression = kSnappyCompression;
    bool reuse_logs = false;
    const FilterPolicy *filter_policy = nullptr;
};

struct ReadOptions {
    // If true, all data read from underlying storage will be
    // verified against corresponding checksums.
    bool verify_checksums = false;

    // Should the data read for this iteration be cached in memory?
    // Callers may wish to set this field to false for bulk scans.
    bool fill_cache = true;

    // If "snapshot" is non-null, read as of the supplied snapshot
    // (which must belong to the DB that is being read and which must
    // not have been released).  If "snapshot" is null, use an implicit
    // snapshot of the state at the beginning of this read operation.
    const Snapshot *snapshot = nullptr;
};

} // namespace tagfilterdb

#endif