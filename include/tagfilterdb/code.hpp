#ifndef TAGFILTERDB_INCLUDE_CODE_HPP_
#define TAGFILTERDB_INCLUDE_CODE_HPP_

#include <cstdint>
#include <string>

namespace tagfilterdb {
/// @brief Encodes a 32-bit integer into a 4-byte sequence in little-endian
/// format.
/// @param p_dst Pointer to the destination buffer where the encoded value will
/// be stored.
/// @param a_v The 32-bit integer value to encode.
inline void Encode32(char *p_dst, uint32_t a_v) {
    uint8_t *const p_buffer = (uint8_t *)(p_dst);

    p_buffer[0] = (uint8_t)(a_v);
    p_buffer[1] = (uint8_t)(a_v >> 8);
    p_buffer[2] = (uint8_t)(a_v >> 16);
    p_buffer[3] = (uint8_t)(a_v >> 24);
}

/// @brief Decodes a 4-byte sequence from little-endian format into a 32-bit
/// integer.
/// @param p Pointer to the source buffer containing the encoded value.
/// @return The decoded 32-bit integer value.
inline uint32_t Decode32(const char *p) {
    const uint8_t *const p_buffer = (const uint8_t *)(p);

    return ((uint32_t)p_buffer[0]) | ((uint32_t)(p_buffer[1]) << 8) |
           ((uint32_t)(p_buffer[2]) << 16) | ((uint32_t)(p_buffer[3]) << 24);
}

/// @brief Appends a 32-bit integer encoded in little-endian format to a string.
/// @param p_dst Pointer to the `std::string` object to which the encoded value
/// will be appended.
/// @param a_v The 32-bit integer value to encode and append.
inline void AppendEncode32(std::string *p_dst, uint32_t a_v) {
    char t_buf[sizeof(a_v)];
    Encode32(t_buf, a_v);
    p_dst->append(t_buf, sizeof(t_buf));
}
} // namespace tagfilterdb

#endif
