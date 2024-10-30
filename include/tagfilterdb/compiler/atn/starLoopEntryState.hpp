#pragma once

#include "decisionState.hpp"

namespace tagfilterdb::compiler::atn {
class StarLoopEntryState final : public DecisionState {
  public:
    static bool is(const ATNState &atnState) {
        return atnState.getStateType() == ATNStateType::STAR_LOOP_ENTRY;
    }

    static bool is(const ATNState *atnState) {
        return atnState != nullptr && is(*atnState);
    }

    bool isPrecedenceDecision = false;

    StarLoopbackState *loopBackState = nullptr;

    StarLoopEntryState() : DecisionState(ATNStateType::STAR_LOOP_ENTRY) {}
};
} // namespace tagfilterdb::compiler::atn