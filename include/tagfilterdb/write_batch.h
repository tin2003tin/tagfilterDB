// Copyright (c) 2025-present, tin2003tin, User
//   This source code is part of [TagfilterDB]
//   (https://github.com/tin2003tin/tagfilterDB)
//
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// WriteBatch holds a collection of updates to apply atomically to a DB.
//
// The updates are applied in the order in which they are added
// to the WriteBatch.  For example, the value of "key" will be "v3"
// after the following batch is written:
//
//    batch.Put("key", "v1");
//    batch.Delete("key");
//    batch.Put("key", "v2");
//    batch.Put("key", "v3");
//
// Multiple threads can invoke const methods on a WriteBatch without
// external synchronization, but if any of the threads may call a
// non-const method, all threads accessing the same WriteBatch must use
// external synchronization.

#ifndef STORAGE_TAGFILTERDB_INCLUDE_WRITE_BATCH_H_
#define STORAGE_TAGFILTERDB_INCLUDE_WRITE_BATCH_H_

#include <string>

#include "tagfilterdb/export.h"
#include "tagfilterdb/status.h"

namespace tagfilterdb {

class DataView;

class TAGFILTERDB_EXPORT WriteBatch {
  public:
    class TAGFILTERDB_EXPORT Handler {
      public:
        virtual ~Handler();
        virtual void Put(const DataView &key, const DataView &value) = 0;
        virtual void Delete(const DataView &key) = 0;
    };

    WriteBatch();

    // Intentionally copyable.
    WriteBatch(const WriteBatch &) = default;
    WriteBatch &operator=(const WriteBatch &) = default;

    ~WriteBatch();

    // Store the mapping "key->value" in the database.
    void Put(const DataView &key, const DataView &value);

    // If the database contains a mapping for "key", erase it.  Else do nothing.
    void Delete(const DataView &key);

    // Clear all updates buffered in this batch.
    void Clear();

    // The size of the database changes caused by this batch.
    //
    // This number is tied to implementation details, and may change across
    // releases. It is intended for LevelDB usage metrics.
    size_t ApproximateSize() const;

    // Copies the operations in "source" to this batch.
    //
    // This runs in O(source size) time. However, the constant factor is better
    // than calling Iterate() over the source batch with a Handler that
    // replicates the operations into this batch.
    void Append(const WriteBatch &source);

    // Support for iterating over the contents of a batch.
    Status Iterate(Handler *handler) const;

  private:
    friend class WriteBatchInternal;

    std::string rep_; // See comment in write_batch.cc for the format of rep_
};

} // namespace tagfilterdb

#endif // STORAGE_TAGFILTERDB_INCLUDE_WRITE_BATCH_H_
