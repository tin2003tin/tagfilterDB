#ifndef TAGFILTERDB_CODING_H
#define TAGFILTERDB_CODING_H

#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>

#include "tagfilterdb/dataView.h"

namespace tagfilterdb {
inline void EncodeFixed32(char *dst, uint32_t value) {
    uint8_t *const buffer = reinterpret_cast<uint8_t *>(dst);

    // Recent clang and gcc optimize this to a single mov / str instruction.
    buffer[0] = static_cast<uint8_t>(value);
    buffer[1] = static_cast<uint8_t>(value >> 8);
    buffer[2] = static_cast<uint8_t>(value >> 16);
    buffer[3] = static_cast<uint8_t>(value >> 24);
}

inline void EncodeFixed64(char *dst, uint64_t value) {
    uint8_t *const buffer = reinterpret_cast<uint8_t *>(dst);

    // Recent clang and gcc optimize this to a single mov / str instruction.
    buffer[0] = static_cast<uint8_t>(value);
    buffer[1] = static_cast<uint8_t>(value >> 8);
    buffer[2] = static_cast<uint8_t>(value >> 16);
    buffer[3] = static_cast<uint8_t>(value >> 24);
    buffer[4] = static_cast<uint8_t>(value >> 32);
    buffer[5] = static_cast<uint8_t>(value >> 40);
    buffer[6] = static_cast<uint8_t>(value >> 48);
    buffer[7] = static_cast<uint8_t>(value >> 56);
}

void PutFixed64(std::string *dst, uint64_t value) {
    char buf[sizeof(value)];
    EncodeFixed64(buf, value);
    dst->append(buf, sizeof(buf));
}

void PutFixed32(std::string *dst, uint32_t value) {
    char buf[sizeof(value)];
    EncodeFixed32(buf, value);
    dst->append(buf, sizeof(buf));
}

inline uint32_t DecodeFixed32(const char *ptr) {
    const uint8_t *const buffer = reinterpret_cast<const uint8_t *>(ptr);

    // Recent clang and gcc optimize this to a single mov / ldr instruction.
    return (static_cast<uint32_t>(buffer[0])) |
           (static_cast<uint32_t>(buffer[1]) << 8) |
           (static_cast<uint32_t>(buffer[2]) << 16) |
           (static_cast<uint32_t>(buffer[3]) << 24);
}

inline uint64_t DecodeFixed64(const char *ptr) {
    const uint8_t *const buffer = reinterpret_cast<const uint8_t *>(ptr);

    // Recent clang and gcc optimize this to a single mov / ldr instruction.
    return (static_cast<uint64_t>(buffer[0])) |
           (static_cast<uint64_t>(buffer[1]) << 8) |
           (static_cast<uint64_t>(buffer[2]) << 16) |
           (static_cast<uint64_t>(buffer[3]) << 24) |
           (static_cast<uint64_t>(buffer[4]) << 32) |
           (static_cast<uint64_t>(buffer[5]) << 40) |
           (static_cast<uint64_t>(buffer[6]) << 48) |
           (static_cast<uint64_t>(buffer[7]) << 56);
}

inline uint64_t DecodeFixed64FromString(const std::string &str) {
    return DecodeFixed64(str.data());
}

char *EncodeVarint64(char *dst, uint64_t v) {
    static const int B = 128;
    uint8_t *ptr = reinterpret_cast<uint8_t *>(dst);
    while (v >= B) {
        *(ptr++) = v | B;
        v >>= 7;
    }
    *(ptr++) = static_cast<uint8_t>(v);
    return reinterpret_cast<char *>(ptr);
}

void PutVarint64(std::string *dst, uint64_t v) {
    char buf[10];
    char *ptr = EncodeVarint64(buf, v);
    dst->append(buf, ptr - buf);
}

const char *GetVarint64Ptr(const char *p, const char *limit, uint64_t *value) {
    uint64_t result = 0;
    for (uint32_t shift = 0; shift <= 63 && p < limit; shift += 7) {
        uint64_t byte = *(reinterpret_cast<const uint8_t *>(p));
        p++;
        if (byte & 128) {
            // More bytes are present
            result |= ((byte & 127) << shift);
        } else {
            result |= (byte << shift);
            *value = result;
            return reinterpret_cast<const char *>(p);
        }
    }
    return nullptr;
}

bool GetVarint64(DataView *input, uint64_t *value) {
    const char *p = input->data();
    const char *limit = p + input->size();
    const char *q = GetVarint64Ptr(p, limit, value);
    if (q == nullptr) {
        return false;
    } else {
        *input = DataView(q, limit - q);
        return true;
    }
}
} // namespace tagfilterdb

#endif
