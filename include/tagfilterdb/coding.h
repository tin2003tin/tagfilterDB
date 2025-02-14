#ifndef TAGFILTERDB_CODING_H
#define TAGFILTERDB_CODING_H

#include <cstdio>
#include <memory>

namespace tagfilterdb {
    inline void EncodeFixed64(char* dst, uint64_t value) {
        uint8_t* const buffer = reinterpret_cast<uint8_t*>(dst);
        
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

    std::string EncodeFixed64ToString(uint64_t value) {
        char buffer[8];
        EncodeFixed64(buffer, value);
        return std::string(buffer, 8);
    }

    void PutFixed64(std::string* dst, uint64_t value) {
        char buf[sizeof(value)];
        EncodeFixed64(buf, value);
        dst->append(buf, sizeof(buf));
    }

    inline uint64_t DecodeFixed64(const char* ptr) {
        const uint8_t* const buffer = reinterpret_cast<const uint8_t*>(ptr);
      
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

    inline uint64_t DecodeFixed64FromString(const std::string& str) {
        if (str.size() < 8) {
            throw std::runtime_error("DecodeFixed64FromString: input string too short");
        }
        return DecodeFixed64(str.data());
    }
}

#endif
