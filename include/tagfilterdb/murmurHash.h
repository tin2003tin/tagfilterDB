#ifndef TAGFILTERDB_MURMURHASH_H
#define TAGFILTERDB_MURMURHASH_H

// A variation of the MurmurHash3 implementation
// (https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp) Here
// we unrolled the loop used there into individual calls to update(), as we
// usually hash object fields instead of entire buffers.

// Platform-specific functions and macros

// Microsoft Visual Studio

#include <cstdint>
#include <string>

namespace tagfilterdb::support {

/// @brief Encodes a 32-bit integer into a 4-byte sequence in little-endian format.
/// @param p_dst Pointer to the destination buffer where the encoded value will be stored.
/// @param a_v The 32-bit integer value to encode.
inline void Encode32(char *p_dst, uint32_t a_v) {
    uint8_t *const p_buffer = reinterpret_cast<uint8_t *>(p_dst);

    p_buffer[0] = static_cast<uint8_t>(a_v);
    p_buffer[1] = static_cast<uint8_t>(a_v >> 8);
    p_buffer[2] = static_cast<uint8_t>(a_v >> 16);
    p_buffer[3] = static_cast<uint8_t>(a_v >> 24);
}

/// @brief Decodes a 4-byte sequence from little-endian format into a 32-bit integer.
/// @param p Pointer to the source buffer containing the encoded value.
/// @return The decoded 32-bit integer value.
inline uint32_t Decode32(const char *p) {
    const uint8_t *const p_buffer = reinterpret_cast<const uint8_t *>(p);

    return static_cast<uint32_t>(p_buffer[0]) |
           (static_cast<uint32_t>(p_buffer[1]) << 8) |
           (static_cast<uint32_t>(p_buffer[2]) << 16) |
           (static_cast<uint32_t>(p_buffer[3]) << 24);
}

/// @brief Appends a 32-bit integer encoded in little-endian format to a string.
/// @param p_dst Pointer to the `std::string` object to which the encoded value will be appended.
/// @param a_v The 32-bit integer value to encode and append.
inline void AppendEncode32(std::string *p_dst, uint32_t a_v) {
    char t_buf[sizeof(a_v)];
    Encode32(t_buf, a_v);
    p_dst->append(t_buf, sizeof(t_buf));
}

#define HASH_VERSION X32

class MurmurHash {
public:
    /// @brief Computes a hash using a variant of the Murmur hash algorithm.
    /// @param data Pointer to the data to hash.
    /// @param n The size of the data in bytes.
    /// @param seed An initial seed value for the hash function.
    /// @return The computed 32-bit hash value.
    static uint32_t Hash(const char *data, size_t n, uint32_t seed);
};

#ifndef FALLTHROUGH_INTENDED
#define FALLTHROUGH_INTENDED do {} while (0)
#endif

#if HASH_VERSION == X32

/// @brief Computes a hash using the 32-bit Murmur hash variant.
/// @param data Pointer to the data to hash.
/// @param n Size of the data in bytes.
/// @param seed Initial seed value for the hash function.
/// @return The computed 32-bit hash value.
inline uint32_t MurmurHash::Hash(const char *data, size_t n, uint32_t seed) {
    const uint32_t m = 0xc6a4a793;
    const uint32_t r = 24;
    const char *limit = data + n;
    uint32_t h = seed ^ (n * m);

    // Process 4 bytes at a time
    while (data + 4 <= limit) {
        uint32_t w = Decode32(data);
        data += 4;
        h += w;
        h *= m;
        h ^= (h >> 16);
    }

    // Handle remaining bytes
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

} // namespace tagfilterdb::support


#endif