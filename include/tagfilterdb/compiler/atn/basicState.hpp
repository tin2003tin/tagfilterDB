
#pragma once

#include "decisionState.hpp"

namespace tagfilterdb::compiler::atn {
class BasicState final : public ATNState {
  public:
    static bool is(const ATNState &atnState) {
        return atnState.getStateType() == ATNStateType::BASIC;
    }

    static bool is(const ATNState *atnState) {
        return atnState != nullptr && is(*atnState);
    }

    BasicState() : ATNState(ATNStateType::BASIC) {}
};
}