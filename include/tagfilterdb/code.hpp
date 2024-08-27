#ifndef TAGFILTERDB_INCLUDE_CODE_HPP_
#define TAGFILTERDB_INCLUDE_CODE_HPP_

#include <string>
#include <cstdint>

namespace tagfilterdb
{
    inline void Encode32(char *dst, uint32_t v)
    {
        uint8_t *const buffer = (uint8_t *)(dst);

        buffer[0] = (uint8_t)(v);
        buffer[1] = (uint8_t)(v >> 8);
        buffer[2] = (uint8_t)(v >> 16);
        buffer[3] = (uint8_t)(v >> 24);
    }
    inline uint32_t Decode32(const char *p)
    {
        const uint8_t *const buffer = (const uint8_t *)(p);

        return ((uint32_t)buffer[0]) |
               ((uint32_t)(buffer[1]) << 8) |
               ((uint32_t)(buffer[2]) << 16) |
               ((uint32_t)(buffer[3]) << 24);
    }
    void AppendEncode32(std::string *dst, uint32_t v)
    {
        char buf[sizeof(v)];
        Encode32(buf, v);
        dst->append(buf, sizeof(buf));
    }
}

#endif