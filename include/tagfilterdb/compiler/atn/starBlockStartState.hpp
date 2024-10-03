#pragma once

#include "blockStartState.hpp"

namespace tagfilterdb::compiler::atn {
class  StarBlockStartState final : public BlockStartState {
  public:
    static bool is(const ATNState &atnState) {
        return atnState.getStateType() == ATNStateType::STAR_BLOCK_START;
    }

    static bool is(const ATNState *atnState) {
        return atnState != nullptr && is(*atnState);
    }

    StarBlockStartState() : BlockStartState(ATNStateType::STAR_BLOCK_START) {}
};
}