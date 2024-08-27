#ifndef TAGFILTERDB_INCLUDE_CODE_HPP_
#define TAGFILTERDB_INCLUDE_CODE_HPP_

#include <cstddef>

#include "tagfilterdb/export.hpp"

namespace tagfilterdb
{
    enum CompressionType
    {
        kNoCompression = 0x0,
        kSnappyCompression = 0x1,
        kZstdCompression = 0x2,
    };

    struct TAGFILTERDB_EXPORT Options
    {
    };
    struct TAGFILTERDB_EXPORT ReadOptions
    {
    };
    struct TAGFILTERDB_EXPORT WriteOptions
    {
    };
}

#endif