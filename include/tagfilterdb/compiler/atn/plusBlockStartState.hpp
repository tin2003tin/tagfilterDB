#pragma once

#include "blockStartState.hpp"

namespace tagfilterdb::compiler::atn {
class PlusBlockStartState final : public BlockStartState {
  public:
    static bool is(const ATNState &atnState) {
        return atnState.getStateType() == ATNStateType::PLUS_BLOCK_START;
    }

    static bool is(const ATNState *atnState) {
        return atnState != nullptr && is(*atnState);
    }

    PlusLoopbackState *loopBackState = nullptr;

    PlusBlockStartState() : BlockStartState(ATNStateType::PLUS_BLOCK_START) {}
};
}