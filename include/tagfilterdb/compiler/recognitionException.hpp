#pragma once

#include "tagfilterdb/compiler/internalCommon.hpp"
#include "tagfilterdb/exceptions.hpp"

namespace tagfilterdb::compiler {
class RecognitionException : public RuntimeException {
  private:
    Recognizer *_recognizer;
    IntStream *_input;
    // ParserRuleContext *_ctx;

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

  public:
    // support::RangeSet getExpectedTokens() const;
    virtual RuleContext *getCtx() const;
    virtual IntStream *getInputStream() const;
    virtual Token *getOffendingToken() const;
    virtual Recognizer *getRecognizer() const;

  private:
    void InitializeInstanceFields();
};
} // namespace tagfilterdb::compiler

// #include "recognitionException.cpp"