#ifndef STORAGE_LEVELDB_INCLUDE_STATUS_H_
#define STORAGE_LEVELDB_INCLUDE_STATUS_H_

#include <algorithm>
#include <string>

#include "tagfilter/export.hpp"

namespace tagfilterdb
{
    class TAGFILTERDB_EXPORT Status
    {
    public:
    private:
        enum Code
        {
            kOk = 0,
            kNotFound = 1,
            kCorruption = 2,
            kNotSupported = 3,
            kInvalidArgument = 4,
            kIOError = 5
        };
    };
}

#endif