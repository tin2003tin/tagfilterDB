#pragma once

#include "tagfilterdb/compiler/internalCommon.hpp"

namespace tagfilterdb::compiler::atn {
enum class TransitionType : size_t {
    EPSILON = 1,
    RANGE = 2,
    RULE = 3,
    PREDICATE = 4, // e.g., {isType(input.LT(1))}?
    ATOM = 5,
    ACTION = 6,
    SET = 7, // ~(A|B) or ~atom, wildcard, which convert to next 2
    NOT_SET = 8,
    WILDCARD = 9,
    PRECEDENCE = 10,
};

std::string transitionTypeName(TransitionType transitionType) {
    switch (transitionType) {
    case TransitionType::EPSILON:
        return "EPSILON";
    case TransitionType::RANGE:
        return "RANGE";
    case TransitionType::RULE:
        return "RULE";
    case TransitionType::PREDICATE:
        return "PREDICATE";
    case TransitionType::ATOM:
        return "ATOM";
    case TransitionType::ACTION:
        return "ACTION";
    case TransitionType::SET:
        return "SET";
    case TransitionType::NOT_SET:
        return "NOT_SET";
    case TransitionType::WILDCARD:
        return "WILDCARD";
    case TransitionType::PRECEDENCE:
        return "PRECEDENCE";
    }
    return "UNKNOWN";
}
}