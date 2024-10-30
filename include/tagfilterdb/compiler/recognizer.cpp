#include "recognizer.hpp"

#include "atn/ATN.hpp"
#include "consoleErrorListener.hpp"
#include "recognitionException.hpp"
#include "tagfilterdb/support/string.hpp"
#include "tagfilterdb/support/util.hpp"
#include "vocabulary.hpp"

using namespace tagfilterdb;
using namespace tagfilterdb::compiler;
using namespace tagfilterdb::internal;

std::map<const Vocabulary *, std::map<std::string_view, size_t>>
    Recognizer::_tokenTypeMapCache;
std::map<std::vector<std::string>, std::map<std::string, size_t>>
    Recognizer::_ruleIndexMapCache;

Recognizer::Recognizer() {
    InitializeInstanceFields();
    _proxListener.addErrorListener(&ConsoleErrorListener::INSTANCE);
}

std::map<std::string_view, size_t> Recognizer::getTokenTypeMap() {
    const Vocabulary &vocabulary = getVocabulary();

    UniqueLock<Mutex> lck(_mutex);
    std::map<std::string_view, size_t> result;
    auto iterator = _tokenTypeMapCache.find(&vocabulary);
    if (iterator != _tokenTypeMapCache.end()) {
        result = iterator->second;
    } else {
        for (size_t i = 0; i <= getATN().maxTokenType; ++i) {
            std::string_view literalName = vocabulary.getLiteralName(i);
            if (!literalName.empty()) {
                result[literalName] = i;
            }

            std::string_view symbolicName = vocabulary.getSymbolicName(i);
            if (!symbolicName.empty()) {
                result[symbolicName] = i;
            }
        }
        result["EOF"] = EOF;
        _tokenTypeMapCache[&vocabulary] = result;
    }

    return result;
}

std::map<std::string, size_t> Recognizer::getRuleIndexMap() {
    const std::vector<std::string> &ruleNames = getRuleNames();
    if (ruleNames.empty()) {
        throw "The current recognizer does not provide a list of rule names.";
    }

    UniqueLock<Mutex> lck(_mutex);
    std::map<std::string, size_t> result;
    auto iterator = _ruleIndexMapCache.find(ruleNames);
    if (iterator != _ruleIndexMapCache.end()) {
        result = iterator->second;
    } else {
        result = support::toMap(ruleNames);
        _ruleIndexMapCache[ruleNames] = result;
    }
    return result;
}

size_t Recognizer::getTokenType(std::string_view tokenName) {
    const std::map<std::string_view, size_t> &map = getTokenTypeMap();
    auto iterator = map.find(tokenName);
    if (iterator == map.end())
        return Token::INVALID_TYPE;

    return iterator->second;
}

void Recognizer::setInterpreter(atn::ATNSimulator *interpreter) {
    // Usually the interpreter is set by the descendant (lexer or parser
    // (simulator), but can also be exchanged by the profiling ATN simulator.
    // delete _interpreter;
    _interpreter = interpreter;
}

std::string Recognizer::getErrorHeader(RecognitionException *e) {
    // We're having issues with cross header dependencies, these two classes
    // will need to be rewritten to remove that.
    size_t line = e->getOffendingToken()->getLine();
    size_t charPositionInLine = e->getOffendingToken()->getCharPositionInLine();
    return std::string("line ") + std::to_string(line) + ":" +
           std::to_string(charPositionInLine);
}

std::string Recognizer::getTokenErrorDisplay(Token *t) {
    if (t == nullptr) {
        return "<no Token>";
    }
    std::string s = t->getText();
    if (s == "") {
        if (t->getType() == EOF) {
            s = "<EOF>";
        } else {
            s = std::string("<") + std::to_string(t->getType()) +
                std::string(">");
        }
    }

    std::string result;
    result.reserve(s.size() + 2);
    result.push_back('\'');
    support::String::escapeWhitespace(result, s);
    result.push_back('\'');
    result.shrink_to_fit();
    return result;
}

void Recognizer::addErrorListener(ErrorListener *listener) {
    _proxListener.addErrorListener(listener);
}

void Recognizer::removeErrorListener(ErrorListener *listener) {
    _proxListener.removeErrorListener(listener);
}

void Recognizer::removeErrorListeners() {
    _proxListener.removeErrorListeners();
}

ProxyErrorListener &Recognizer::getErrorListenerDispatch() {
    return _proxListener;
}

Recognizer::~Recognizer() {}

void Recognizer::InitializeInstanceFields() {
    _stateNumber = ATNState::INVALID_STATE_NUMBER;
    _interpreter = nullptr;
}
