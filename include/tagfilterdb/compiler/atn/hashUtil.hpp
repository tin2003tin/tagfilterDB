#pragma once

#include <cstddef>

namespace tagfilterdb::compiler::atn {
inline bool cachedHashCodeEqual(size_t lhs, size_t rhs) {
    return lhs == rhs || lhs == 0 || rhs == 0;
}
}