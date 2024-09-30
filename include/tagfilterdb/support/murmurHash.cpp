#include "tagfilterdb/support/murmurHash.hpp"

#include "tagfilterdb/common.hpp"
#include "tagfilterdb/support/code.hpp"

// A variation of the MurmurHash3 implementation
// (https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp) Here
// we unrolled the loop used there into individual calls to update(), as we
// usually hash object fields instead of entire buffers.

// Platform-specific functions and macros

// Microsoft Visual Studio

namespace tagfilterdb::support {

#ifndef FALLTHROUGH_INTENDED
#define FALLTHROUGH_INTENDED                                                   \
    do {                                                                       \
    } while (0)
#endif

#if HASH_VERSION == X32

uint32_t Hash(const char *data, size_t n, uint32_t seed) {
    // Similar to murmur hash
    const uint32_t m = 0xc6a4a793;
    const uint32_t r = 24;
    const char *limit = data + n;
    uint32_t h = seed ^ (n * m);

    // Pick up four bytes at a time
    while (data + 4 <= limit) {
        uint32_t w = Decode32(data);
        data += 4;
        h += w;
        h *= m;
        h ^= (h >> 16);
    }

    // Pick up remaining bytes
    switch (limit - data) {
    case 3:
        h += static_cast<uint8_t>(data[2]) << 16;
        FALLTHROUGH_INTENDED;
    case 2:
        h += static_cast<uint8_t>(data[1]) << 8;
        FALLTHROUGH_INTENDED;
    case 1:
        h += static_cast<uint8_t>(data[0]);
        h *= m;
        h ^= (h >> r);
        break;
    }
    return h;
}
#else
#error "Expected sizeof(size_t) to be 4 or 8."
#endif
}