#ifndef TAGFILTERDB_INCLUDE_CODE_HPP_
#define TAGFILTERDB_INCLUDE_CODE_HPP_

#include <string>
#include <cstdint>

namespace tagfilterdb
{
    inline void Encode32(char *p_dst, uint32_t a_v)
    {
        uint8_t *const p_buffer = (uint8_t *)(p_dst);

        p_buffer[0] = (uint8_t)(a_v);
        p_buffer[1] = (uint8_t)(a_v >> 8);
        p_buffer[2] = (uint8_t)(a_v >> 16);
        p_buffer[3] = (uint8_t)(a_v >> 24);
    }
    inline uint32_t Decode32(const char *p)
    {
        const uint8_t *const p_buffer = (const uint8_t *)(p);

        return ((uint32_t)p_buffer[0]) |
               ((uint32_t)(p_buffer[1]) << 8) |
               ((uint32_t)(p_buffer[2]) << 16) |
               ((uint32_t)(p_buffer[3]) << 24);
    }
    void AppendEncode32(std::string *p_dst, uint32_t a_v)
    {
        char t_buf[sizeof(a_v)];
        Encode32(t_buf, a_v);
        p_dst->append(t_buf, sizeof(t_buf));
    }
}

#endif