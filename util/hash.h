// Copyright (c) 2025-present, tin2003tin, User
//   This source code is part of [TagfilterDB]
//   (https://github.com/tin2003tin/tagfilterDB)
//
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// Simple hash function used for internal data structures

#ifndef STORAGE_TAGFILTERDB_UTIL_HASH_H_
#define STORAGE_TAGFILTERDB_UTIL_HASH_H_

#include <cstddef>
#include <cstdint>

namespace tagfilterdb
{

    uint32_t Hash(const char *data, size_t n, uint32_t seed);

} // namespace tagfilterdb

#endif // STORAGE_TAGFILTERDB_UTIL_HASH_H_
