#pragma once

#include "ATNState.hpp"

namespace tagfilterdb::compiler::atn {
class DecisionState : public ATNState {
  public:
    static bool is(const ATNState &atnState) {
        const auto stateType = atnState.getStateType();
        return (stateType >= ATNStateType::BLOCK_START &&
                stateType <= ATNStateType::TOKEN_START) ||
               stateType == ATNStateType::PLUS_LOOP_BACK ||
               stateType == ATNStateType::STAR_LOOP_ENTRY;
    }
    static bool is(const ATNState *atnState) {
        return atnState != nullptr && is(*atnState);
    }

    int decision = -1;
    bool nonGreedy = false;

    virtual std::string toString() const override {
        return ATNState::toString();
    }

  protected:
    using ATNState::ATNState;
};
} // namespace tagfilerdb::compiler::atn