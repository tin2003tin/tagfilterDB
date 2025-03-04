// Copyright (c) 2025-present, tin2003tin, User
//   This source code is part of [TagfilterDB]
//   (https://github.com/tin2003tin/tagfilterDB)
//
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "util/testutil.h"

#include <string>

#include "util/random.h"

namespace tagfilterdb {
namespace test {

DataView RandomString(Random *rnd, int len, std::string *dst) {
    dst->resize(len);
    for (int i = 0; i < len; i++) {
        (*dst)[i] = static_cast<char>(' ' + rnd->Uniform(95)); // ' ' .. '~'
    }
    return DataView(*dst);
}

std::string RandomKey(Random *rnd, int len) {
    // Make sure to generate a wide variety of characters so we
    // test the boundary conditions for short-key optimizations.
    static const char kTestChars[] = {'\0', '\1', 'a',    'b',    'c',
                                      'd',  'e',  '\xfd', '\xfe', '\xff'};
    std::string result;
    for (int i = 0; i < len; i++) {
        result += kTestChars[rnd->Uniform(sizeof(kTestChars))];
    }
    return result;
}

DataView CompressibleString(Random *rnd, double compressed_fraction, size_t len,
                            std::string *dst) {
    int raw = static_cast<int>(len * compressed_fraction);
    if (raw < 1)
        raw = 1;
    std::string raw_data;
    RandomString(rnd, raw, &raw_data);

    // Duplicate the random data until we have filled "len" bytes
    dst->clear();
    while (dst->size() < len) {
        dst->append(raw_data);
    }
    dst->resize(len);
    return DataView(*dst);
}

} // namespace test
} // namespace tagfilterdb
