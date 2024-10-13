#pragma once

#include <atomic>

#include "ATNsimulator.hpp"

namespace tagfilterdb::compiler::atn {
class LexerATNSimulator : public ATNSimulator {
  protected:
    struct SimState final {};

  public:
    static constexpr size_t MIN_DFA_EDGE = 0;
    static constexpr size_t MAX_DFA_EDGE = 127;

    Lexer *const _recog;
    size_t _startIndex;
    size_t _line;
    size_t _charPositionInLine;

  protected:
    size_t _mode;

  public:
    // dfa::DFA &getDFA(size_t mode);
    virtual size_t match(CharStream *input, size_t mode);
    /// Get the text matched so far for the current token.
    virtual std::string getText(CharStream *input);
    virtual size_t getLine() const;
    virtual void setLine(size_t line);
    virtual size_t getCharPositionInLine();
    virtual void setCharPositionInLine(size_t charPositionInLine);
    virtual void consume(CharStream *input);
    virtual std::string getTokenName(size_t t);

  private:
    void InitializeInstanceFields();
};
} // namespace tagfilterdb::compiler::atn