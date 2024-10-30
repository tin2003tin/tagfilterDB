
#pragma once

#include "decisionState.hpp"

namespace tagfilterdb::compiler::atn {
class BlockStartState : public DecisionState {
  public:
    static bool is(const ATNState &atnState) {
        const auto stateType = atnState.getStateType();
        return stateType >= ATNStateType::BLOCK_START &&
               stateType <= ATNStateType::STAR_BLOCK_START;
    }

    static bool is(const ATNState *atnState) {
        return atnState != nullptr && is(*atnState);
    }

    BlockEndState *endState = nullptr;

  protected:
    using DecisionState::DecisionState;
};
}