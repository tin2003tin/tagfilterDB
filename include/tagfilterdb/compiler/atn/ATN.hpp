#pragma once

#include "ATNType.hpp"
#include "tagfilterdb/threadManager/synchronization.hpp"

namespace tagfilterdb::compiler::atn {
class LexerATNSimulator;
class ParserATNSimulator;

class ATN {
  public:
    static constexpr size_t INVALID_ALT_NUMBER = 0;
    ATN();

    ATN(ATNType grammarType, size_t maxTokenType);

    ATN(const ATN &) = delete;

    ATN(ATN &&) = delete;

    ~ATN();

    ATN &operator=(const ATN &) = delete;

    ATN &operator=(ATN &&) = delete;

    std::vector<ATNState *> states;

    std::vector<DecisionState *> decisionToState;

    std::vector<RuleStartState *> ruleToStartState;

    std::vector<RuleStopState *> ruleToStopState;

    ATNType grammarType;

    size_t maxTokenType;

    std::vector<size_t> ruleToTokenType;

    // std::vector<Ref<const LexerAction>> lexerActions;
    //   misc::IntervalSet nextTokens(ATNState *s, RuleContext *ctx) const;
    support::RangeSet const &nextTokens(ATNState *s) const;

    void addState(ATNState *state);

    void removeState(ATNState *state);

    int defineDecisionState(DecisionState *s);

    DecisionState *getDecisionState(size_t decision) const;

    size_t getNumberOfDecisions() const;

    //   misc::IntervalSet getExpectedTokens(size_t stateNumber, RuleContext
    //   *context) const;

    std::string toString() const;

  private:
    friend class LexerATNSimulator;
    friend class ParserATNSimulator;

    mutable internal::Mutex _mutex;
    mutable internal::SharedMutex _stateMutex;
    mutable internal::SharedMutex _edgeMutex;
};
} // namespace tagfilterdb::compiler::atn

#include "ATN.cpp"