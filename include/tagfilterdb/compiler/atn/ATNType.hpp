#pragma once

#include "tagfilterdb/compiler/internalCommon.hpp"

namespace tagfilterdb::compiler::atn {
enum class ATNType {
    LEXER = 0,
    PARSER = 1,
};
}