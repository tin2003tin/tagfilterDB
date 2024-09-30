#ifndef TAGFILERDB_DB_CACHE_HPP
#define TAGFILERDB_DB_CACHE_HPP

#include "tagfilterdb/common.hpp"
#include "tagfilterdb/export.hpp"

namespace tagfilter {
class TAGFILTERDB_EXPORT Cache;

TAGFILTERDB_EXPORT Cache *NewLRUCache(size_t cap);

class TAGFILTERDB_EXPORT Cache {
  public:
    Cache() = default;
    Cache(const Cache &) = delete;
    Cache &operator=(const Cache &) = delete;
    virtual ~Cache();
    struct Handle {};
    virtual Handle* Insert(std::string_view key
};
} // namespace tagfilter

#endif