#ifndef TAGFILTERDB_INCLUDE_CODE_HPP_
#define TAGFILTERDB_INCLUDE_CODE_HPP_

#include <cstddef>

#include "tagfilterdb/export.hpp"

namespace tagfilterdb {
enum CompressionType {
    e_NoCompression = 0x0,
    e_SnappyCompression = 0x1,
    e_ZstdCompression = 0x2,
};

struct TAGFILTERDB_EXPORT Options {};
struct TAGFILTERDB_EXPORT ReadOptions {};
struct TAGFILTERDB_EXPORT WriteOptions {};

} // namespace tagfilterdb

#endif