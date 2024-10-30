#pragma once

#include "tagfilterdb/compiler/internalCommon.hpp"

namespace tagfilterdb::compiler::atn {
enum class PredictionContextType : size_t {
    SINGLETON = 1,
    ARRAY = 2,
};
}