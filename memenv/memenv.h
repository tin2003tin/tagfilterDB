// Copyright (c) 2025-present, tin2003tin, User
//   This source code is part of [TagfilterDB]
//   (https://github.com/tin2003tin/tagfilterDB)
//
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_TAGFILTERDB_HELPERS_MEMENV_MEMENV_H_
#define STORAGE_TAGFILTERDB_HELPERS_MEMENV_MEMENV_H_

#include "tagfilterdb/export.h"

namespace tagfilterdb
{

    class Env;

    // Returns a new environment that stores its data in memory and delegates
    // all non-file-storage tasks to base_env. The caller must delete the result
    // when it is no longer needed.
    // *base_env must remain live while the result is in use.
    TAGFILTERDB_EXPORT Env *NewMemEnv(Env *base_env);

} // namespace tagfilterdb

#endif // STORAGE_TAGFILTERDB_HELPERS_MEMENV_MEMENV_H_
