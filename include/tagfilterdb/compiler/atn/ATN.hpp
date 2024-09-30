#pragma once

#include "tagfilterdb/compiler/RuleContext.hpp"
#include "tagfilterdb/threadManager/synchronization.hpp"

namespace tagfilterdb::compiler::atn {
class LexerATNSimulator;
class ParserATNSimulator;
class ATNType;
class ATNState;
class DecisionState;
class RuleStartState;
class RuleStopState;
class LexerAction;

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

    std::vector<ATNState *> m_states;
    std::vector<DecisionState *> m_decisionToState;
    std::vector<RuleStartState *> m_ruleToStartState;
    std::vector<RuleStopState *> m_ruleToStopState;
    ATNType m_grammarType;
    std::vector<size_t> m_ruleToTokenType;
    std::vector<Ref<const LexerAction>> lexerActions;
};
} // namespace tagfilterdb::compiler::atn