
#pragma once

#include "decisionState.hpp"

namespace tagfilterdb::compiler::atn {
class BlockEndState final : public ATNState {
  public:
    static bool is(const ATNState &atnState) {
        return atnState.getStateType() == ATNStateType::BLOCK_END;
    }

    static bool is(const ATNState *atnState) {
        return atnState != nullptr && is(*atnState);
    }

    BlockStartState *startState = nullptr;

    BlockEndState() : ATNState(ATNStateType::BLOCK_END) {}
};
}