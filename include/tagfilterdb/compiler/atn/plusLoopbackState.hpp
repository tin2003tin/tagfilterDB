#pragma once

#include "blockStartState.hpp"

namespace tagfilterdb::compiler::atn {
class PlusLoopbackState final : public DecisionState {
  public:
    static bool is(const ATNState &atnState) {
        return atnState.getStateType() == ATNStateType::PLUS_LOOP_BACK;
    }

    static bool is(const ATNState *atnState) {
        return atnState != nullptr && is(*atnState);
    }

    PlusLoopbackState() : DecisionState(ATNStateType::PLUS_LOOP_BACK) {}
};
} // namespace tagfilterdb::compiler::atn