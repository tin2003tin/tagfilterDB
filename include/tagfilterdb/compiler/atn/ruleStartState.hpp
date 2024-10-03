
#pragma once

#include "ATNState.hpp"

namespace tagfilterdb::compiler::atn {
class RuleStartState final : public ATNState {
  public:
    static bool is(const ATNState &atnState) {
        return atnState.getStateType() == ATNStateType::RULE_START;
    }

    static bool is(const ATNState *atnState) {
        return atnState != nullptr && is(*atnState);
    }

    RuleStopState *stopState = nullptr;
    bool isLeftRecursiveRule = false;

    RuleStartState() : ATNState(ATNStateType::RULE_START) {}
};
} // namespace tagfilterdb::compiler::atn