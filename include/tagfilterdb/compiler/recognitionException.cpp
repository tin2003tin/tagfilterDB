#include "recognitionException.hpp"
#include "atn/ATN.hpp"
#include "recognizer.hpp"

namespace tagfilterdb::compiler {
RecognitionException::RecognitionException(Recognizer *recognizer,
                                           IntStream *input,
                                           ParserRuleContext *ctx,
                                           Token *offendingToken)
    : RecognitionException("", recognizer, input, ctx, offendingToken) {}

RecognitionException::RecognitionException(const std::string &message,
                                           Recognizer *recognizer,
                                           IntStream *input,
                                           ParserRuleContext *ctx,
                                           Token *offendingToken)
    : RuntimeException(message), _recognizer(recognizer), _input(input),
      _offendingToken(offendingToken) {
    InitializeInstanceFields();
    if (recognizer != nullptr) {
        _offendingState = recognizer->getState();
    }
}
RecognitionException::~RecognitionException() {}

size_t RecognitionException::getOffendingState() const {
    return _offendingState;
}

void RecognitionException::setOffendingState(size_t offendingState) {
    _offendingState = offendingState;
}

// support::RangeSet RecognitionException::getExpectedTokens() const {
//     if (_recognizer) {
//         return _recognizer->getATN().getExpectedTokens(_offendingState,
//         _ctx);
//     }
//     return support::RangeSet::EMPTY_SET;
// }

// RuleContext *RecognitionException::getCtx() const { return _ctx; }

IntStream *RecognitionException::getInputStream() const { return _input; }

Token *RecognitionException::getOffendingToken() const {
    return _offendingToken;
}

Recognizer *RecognitionException::getRecognizer() const { return _recognizer; }

void RecognitionException::InitializeInstanceFields() { _offendingState = -1; }
} // namespace tagfilterdb::compiler
