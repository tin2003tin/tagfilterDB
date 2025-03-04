// Copyright (c) 2025-present, tin2003tin, User
//   This source code is part of [TagfilterDB]
//   (https://github.com/tin2003tin/tagfilterDB)
//
// Copyright 2017 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_TAGFILTERDB_UTIL_ENV_POSIX_TEST_HELPER_H_
#define STORAGE_TAGFILTERDB_UTIL_ENV_POSIX_TEST_HELPER_H_

namespace tagfilterdb
{

  class EnvPosixTest;

  // A helper for the POSIX Env to facilitate testing.
  class EnvPosixTestHelper
  {
  private:
    friend class EnvPosixTest;

    // Set the maximum number of read-only files that will be opened.
    // Must be called before creating an Env.
    static void SetReadOnlyFDLimit(int limit);

    // Set the maximum number of read-only files that will be mapped via mmap.
    // Must be called before creating an Env.
    static void SetReadOnlyMMapLimit(int limit);
  };

} // namespace tagfilterdb

#endif // STORAGE_TAGFILTERDB_UTIL_ENV_POSIX_TEST_HELPER_H_
