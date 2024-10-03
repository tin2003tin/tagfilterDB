#pragma once

#include "blockStartState.hpp"
#include "tagfilterdb/compiler/internalCommon.hpp"

namespace tagfilterdb::compiler::atn {
class BasicBlockStartState final : public BlockStartState {
  public:
    static bool is(const ATNState &atnState) {
        return atnState.getStateType() == ATNStateType::BLOCK_START;
    }

    static bool is(const ATNState *atnState) {
        return atnState != nullptr && is(*atnState);
    }

    BasicBlockStartState() : BlockStartState(ATNStateType::BLOCK_START) {}
};
} // namespace tagfilterdb::compiler::atn