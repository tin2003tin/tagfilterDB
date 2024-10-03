#pragma once

#include "ATNStateType.hpp"
#include "tagfilterdb/support/rangeSet.hpp"
#include "transition.hpp"

namespace tagfilterdb::compiler::atn {
class ATNState {
  public:
    static constexpr size_t INITIAL_NUM_TRANSITIONS = 4;
    static constexpr size_t INVALID_STATE_NUMBER =
        std::numeric_limits<size_t>::max();

    size_t stateNumber = INVALID_STATE_NUMBER;
    size_t ruleIndex = 0; // at runtime, we don't have Rule objects
    bool epsilonOnlyTransitions = false;

    std::vector<ConstTransitionPtr> transitions;

    ATNState() = delete;

    ATNState(ATNState const &) = delete;

    ATNState(ATNState &&) = delete;

    virtual ~ATNState() = default;

    ATNState &operator=(ATNState const &) = delete;

    ATNState &operator=(ATNState &&) = delete;

    void addTransition(ConstTransitionPtr e);
    void addTransition(size_t index, ConstTransitionPtr e);
    ConstTransitionPtr removeTransition(size_t index);

    virtual size_t hashCode() const;
    virtual bool equals(const ATNState &other) const;

    virtual bool isNonGreedyExitState() const;
    virtual std::string toString() const;

    ATNStateType getStateType() const { return _stateType; }

  protected:
    explicit ATNState(ATNStateType stateType) : _stateType(stateType) {}

  private:
    /// Used to cache lookahead during parsing, not used during construction.

    support::RangeSet _nextTokenWithinRule;
    std::atomic<bool> _nextTokenUpdated{false};

    const ATNStateType _stateType;

    friend class ATN;
};

inline bool operator==(const ATNState &lhs, const ATNState &rhs) {
    return lhs.equals(rhs);
}

inline bool operator!=(const ATNState &lhs, const ATNState &rhs) {
    return !operator==(lhs, rhs);
}
} // namespace tagfilterdb::compiler::atn

#include "ATNState.cpp"