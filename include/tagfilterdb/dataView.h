// Copyright (c) 2025-present, tin2003tin, User
//   This source code is part of [TagfilterDB]
//   (https://github.com/tin2003tin/tagfilterDB)
//
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// DataView is a simple structure containing a pointer into some external
// storage and a size.  The user of a DataView must ensure that the DataView
// is not used after the corresponding external storage has been
// deallocated.
//
// Multiple threads can invoke const methods on a DataView without
// external synchronization, but if any of the threads may call a
// non-const method, all threads accessing the same DataView must use
// external synchronization.

#ifndef STORAGE_TAGFILTERDB_INCLUDE_DataView_H_
#define STORAGE_TAGFILTERDB_INCLUDE_DataView_H_

#include <cassert>
#include <cstddef>
#include <cstring>
#include <string>

#include "tagfilterdb/export.h"

namespace tagfilterdb {

class TAGFILTERDB_EXPORT DataView {
  public:
    // Create an empty DataView.
    DataView() : data_(""), size_(0) {}

    // Create a DataView that refers to d[0,n-1].
    DataView(const char *d, size_t n) : data_(d), size_(n) {}

    // Create a DataView that refers to the contents of "s"
    DataView(const std::string &s) : data_(s.data()), size_(s.size()) {}

    // Create a DataView that refers to s[0,strlen(s)-1]
    DataView(const char *s) : data_(s), size_(strlen(s)) {}

    // Intentionally copyable.
    DataView(const DataView &) = default;
    DataView &operator=(const DataView &) = default;

    // Return a pointer to the beginning of the referenced data
    const char *data() const { return data_; }

    // Return the length (in bytes) of the referenced data
    size_t size() const { return size_; }

    // Return true iff the length of the referenced data is zero
    bool empty() const { return size_ == 0; }

    const char *begin() const { return data(); }
    const char *end() const { return data() + size(); }

    // Return the ith byte in the referenced data.
    // REQUIRES: n < size()
    char operator[](size_t n) const {
        assert(n < size());
        return data_[n];
    }

    // Change this DataView to refer to an empty array
    void clear() {
        data_ = "";
        size_ = 0;
    }

    // Drop the first "n" bytes from this DataView.
    void remove_prefix(size_t n) {
        assert(n <= size());
        data_ += n;
        size_ -= n;
    }

    // Return a string that contains the copy of the referenced data.
    std::string ToString() const { return std::string(data_, size_); }

    // Three-way comparison.  Returns value:
    //   <  0 iff "*this" <  "b",
    //   == 0 iff "*this" == "b",
    //   >  0 iff "*this" >  "b"
    int compare(const DataView &b) const;

    // Return true iff "x" is a prefix of "*this"
    bool starts_with(const DataView &x) const {
        return ((size_ >= x.size_) && (memcmp(data_, x.data_, x.size_) == 0));
    }

  private:
    const char *data_;
    size_t size_;
};

inline bool operator==(const DataView &x, const DataView &y) {
    return ((x.size() == y.size()) &&
            (memcmp(x.data(), y.data(), x.size()) == 0));
}

inline bool operator!=(const DataView &x, const DataView &y) {
    return !(x == y);
}

inline int DataView::compare(const DataView &b) const {
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

} // namespace tagfilterdb

#endif // STORAGE_TAGFILTERDB_INCLUDE_DataView_H_
