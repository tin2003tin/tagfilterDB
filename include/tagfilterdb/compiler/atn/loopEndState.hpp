#pragma once

#include "ATNState.hpp"

namespace tagfilterdb::compiler::atn {
class LoopEndState final : public ATNState {
  public:
    static bool is(const ATNState &atnState) {
        return atnState.getStateType() == ATNStateType::LOOP_END;
    }

    static bool is(const ATNState *atnState) {
        return atnState != nullptr && is(*atnState);
    }

    ATNState *loopBackState = nullptr;

    LoopEndState() : ATNState(ATNStateType::LOOP_END) {}
};
} // namespace tagfilterdb::compiler::atn