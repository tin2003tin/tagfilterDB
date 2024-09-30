#pragma once

#include "exceptions.hpp"
#include "tagfilterdb/compiler/internalCommon.hpp"

namespace tagfilterdb::compiler {
class RecognitionException : public RuntimeException {
  private:
    Recognizer *_recognier;
    IntStream *_input;
    ParserRuleContext *_ctx;
    Token *_offendingToken;
    size_t _offendingState;

  public:
    RecognitionException(Recognizer *recognizer, IntStream *input,
                         ParserRuleContext *ctx,
                         Token *offendingToken = nullptr);
    RecognitionException(const std::string &message, Recognizer *recognizer,
                         IntStream *input, ParserRuleContext *ctx,
                         Token *offendingToken = nullptr);
    RecognitionException(RecognitionException const &) = default;
    ~RecognitionException();
    RecognitionException &operator=(RecognitionException const &) = default;

    virtual size_t getOffendingState() const;

  protected:
    void setOffendingState(size_t offendingState);

    // TODO: (RecognitionException) misc::IntervalSet getExpectedTokens() const;
};
} // namespace tagfilterdb::compiler