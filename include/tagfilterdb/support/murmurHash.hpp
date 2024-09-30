#pragma once

#include <cstdint>
#include <type_traits>

#include "tagfilterdb/common.hpp"
#include "tagfilterdb/export.hpp"

namespace tagfilterdb::support {
#define HASH_VERSION X32
uint32_t Hash(const char *data, size_t n, uint32_t seed);
} // namespace tagfilterdb

#include "tagfilterdb/support/murmurHash.cpp"