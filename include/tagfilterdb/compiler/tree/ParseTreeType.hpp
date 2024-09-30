#pragma once

#include "tagfilterdb/common.hpp"

namespace tagfilterdb::compiler::tree {
enum class ParseTreeType : size_t {
    TERMINAL = 1,
    ERROR = 2,
    RULE = 3,
};
}