
#pragma once

#include "decisionState.hpp"

namespace tagfilterdb::compiler::atn {
class TokensStartState final : public DecisionState {
  public:
    static bool is(const ATNState &atnState) {
        return atnState.getStateType() == ATNStateType::TOKEN_START;
    }

    static bool is(const ATNState *atnState) {
        return atnState != nullptr && is(*atnState);
    }

    TokensStartState() : DecisionState(ATNStateType::TOKEN_START) {}
};
} // namespace tagfilterdb::compiler::atn