#pragma once

#include "ATNState.hpp"

namespace tagfilterdb::compiler::atn {
class StarLoopbackState final : public ATNState {
  public:
    static bool is(const ATNState &atnState) {
        return atnState.getStateType() == ATNStateType::STAR_LOOP_BACK;
    }

    static bool is(const ATNState *atnState) {
        return atnState != nullptr && is(*atnState);
    }

    StarLoopbackState() : ATNState(ATNStateType::STAR_LOOP_BACK) {}

    StarLoopEntryState *getLoopEntryState() const {
        if (transitions[0]->target != nullptr &&
            transitions[0]->target->getStateType() ==
                ATNStateType::STAR_LOOP_ENTRY) {
            return (StarLoopEntryState *)(transitions[0]->target);
        }
        return nullptr;
    }
};
} // namespace tagfilterdb::compiler::atn