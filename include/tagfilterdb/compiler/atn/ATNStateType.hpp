#pragma once

#include "tagfilterdb/compiler/internalCommon.hpp"

namespace tagfilterdb::compiler::atn {
enum class ATNStateType : size_t {
    INVALID = 0,
    BASIC = 1,
    RULE_START = 2,
    BLOCK_START = 3,
    PLUS_BLOCK_START = 4,
    STAR_BLOCK_START = 5,
    TOKEN_START = 6,
    RULE_STOP = 7,
    BLOCK_END = 8,
    STAR_LOOP_BACK = 9,
    STAR_LOOP_ENTRY = 10,
    PLUS_LOOP_BACK = 11,
    LOOP_END = 12,
};
std::string atnStateTypeName(ATNStateType atnStateType) {
    switch (atnStateType) {
    case ATNStateType::INVALID:
        return "INVALID";
    case ATNStateType::BASIC:
        return "BASIC";
    case ATNStateType::RULE_START:
        return "RULE_START";
    case ATNStateType::BLOCK_START:
        return "BLOCK_START";
    case ATNStateType::PLUS_BLOCK_START:
        return "PLUS_BLOCK_START";
    case ATNStateType::STAR_BLOCK_START:
        return "STAR_BLOCK_START";
    case ATNStateType::TOKEN_START:
        return "TOKEN_START";
    case ATNStateType::RULE_STOP:
        return "RULE_STOP";
    case ATNStateType::BLOCK_END:
        return "BLOCK_END";
    case ATNStateType::STAR_LOOP_BACK:
        return "STAR_LOOP_BACK";
    case ATNStateType::STAR_LOOP_ENTRY:
        return "STAR_LOOP_ENTRY";
    case ATNStateType::PLUS_LOOP_BACK:
        return "PLUS_LOOP_BACK";
    case ATNStateType::LOOP_END:
        return "LOOP_END";
    }
    return "UNKNOWN";
}
} // namespace tagfilterdb::compiler::atn