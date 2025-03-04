// Copyright (c) 2025-present, tin2003tin, User
//   This source code is part of [TagfilterDB]
//   (https://github.com/tin2003tin/tagfilterDB)
//
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_TAGFILTERDB_INCLUDE_COMPARATOR_H_
#define STORAGE_TAGFILTERDB_INCLUDE_COMPARATOR_H_

#include <string>

#include "tagfilterdb/export.h"

namespace tagfilterdb {

class DataView;

// A Comparator object provides a total order across DataViews that are
// used as keys in an sstable or a database.  A Comparator implementation
// must be thread-safe since tagfilterdb may invoke its methods concurrently
// from multiple threads.
class TAGFILTERDB_EXPORT Comparator {
  public:
    virtual ~Comparator();

    // Three-way comparison.  Returns value:
    //   < 0 iff "a" < "b",
    //   == 0 iff "a" == "b",
    //   > 0 iff "a" > "b"
    virtual int Compare(const DataView &a, const DataView &b) const = 0;

    // The name of the comparator.  Used to check for comparator
    // mismatches (i.e., a DB created with one comparator is
    // accessed using a different comparator.
    //
    // The client of this package should switch to a new name whenever
    // the comparator implementation changes in a way that will cause
    // the relative ordering of any two keys to change.
    //
    // Names starting with "tagfilterdb." are reserved and should not be used
    // by any clients of this package.
    virtual const char *Name() const = 0;

    // Advanced functions: these are used to reduce the space requirements
    // for internal data structures like index blocks.

    // If *start < limit, changes *start to a short string in [start,limit).
    // Simple comparator implementations may return with *start unchanged,
    // i.e., an implementation of this method that does nothing is correct.
    virtual void FindShortestSeparator(std::string *start,
                                       const DataView &limit) const = 0;

    // Changes *key to a short string >= *key.
    // Simple comparator implementations may return with *key unchanged,
    // i.e., an implementation of this method that does nothing is correct.
    virtual void FindShortSuccessor(std::string *key) const = 0;
};

// Return a builtin comparator that uses lexicographic byte-wise
// ordering.  The result remains the property of this module and
// must not be deleted.
TAGFILTERDB_EXPORT const Comparator *BytewiseComparator();

} // namespace tagfilterdb

#endif // STORAGE_TAGFILTERDB_INCLUDE_COMPARATOR_H_
