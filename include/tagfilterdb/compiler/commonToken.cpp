#include "commonToken.hpp"

#include "sourceStream.hpp"
#include "vocabulary.hpp"

#include "tagfilterdb/support/range.hpp"
#include "tagfilterdb/support/util.hpp"

using namespace tagfilterdb::compiler;

const SourceStream CommonToken::EMPTY_SOURCE;

CommonToken::CommonToken(size_t type) {
    InitializeInstanceFields();
    _type = type;
}
CommonToken::CommonToken(SourceStream source, size_t type, size_t channel,
                         size_t start, size_t stop) {
    InitializeInstanceFields();
    _source = source;
    _type = type;
    _channel = channel;
    _start = start;
    _stop = stop;
    if (_source.tokenSource != nullptr) {
        _line = static_cast<int>(source.tokenSource->getLine());
        _charPositionInLine = source.tokenSource->getCharPositionInLine();
    }
}

CommonToken::CommonToken(size_t type, const std::string &text) {
    InitializeInstanceFields();
    _type = type;
    _channel = DEFAULT_CHANNEL;
    _text = text;
    _source = EMPTY_SOURCE;
}

CommonToken::CommonToken(Token *oldToken) {
    InitializeInstanceFields();
    _type = oldToken->getType();
    _line = oldToken->getLine();
    _index = oldToken->getTokenIndex();
    _charPositionInLine = oldToken->getCharPositionInLine();
    _channel = oldToken->getChannel();
    _start = oldToken->getStartIndex();
    _stop = oldToken->getStopIndex();

    if (support::is<CommonToken *>(oldToken)) {
        _text = (static_cast<CommonToken *>(oldToken))->_text;
        _source = (static_cast<CommonToken *>(oldToken))->_source;
    } else {
        _text = oldToken->getText();
        _source = SourceStream(
            {oldToken->getTokenSource(), oldToken->getInputStream()});
    }
}

size_t CommonToken::getType() const { return _type; }

void CommonToken::setLine(size_t line) { _line = line; }

std::string CommonToken::getText() const {
    if (!_text.empty()) {
        return _text;
    }
    CharStream *input = getInputStream();
    if (input == nullptr) {
        return "";
    }
    size_t n = input->size();
    if (_start < n && _stop < n) {
        return input->getText(support::Range(_start, _stop));
    } else {
        return "<EOF>";
    }
}
void CommonToken::setText(const std::string &text) { _text = text; }

size_t CommonToken::getLine() const { return _line; }

size_t CommonToken::getCharPositionInLine() const {
    return _charPositionInLine;
}

void CommonToken::setCharPositionInLine(size_t charPositionInLine) {
    _charPositionInLine = charPositionInLine;
}

size_t CommonToken::getChannel() const { return _channel; }

void CommonToken::setChannel(size_t channel) { _channel = channel; }

void CommonToken::setType(size_t type) { _type = type; }

size_t CommonToken::getStartIndex() const { return _start; }

void CommonToken::setStartIndex(size_t start) { _start = start; }

size_t CommonToken::getStopIndex() const { return _stop; }

void CommonToken::setStopIndex(size_t stop) { _stop = stop; }

size_t CommonToken::getTokenIndex() const { return _index; }

void CommonToken::setTokenIndex(size_t index) { _index = index; }

TokenSource *CommonToken::getTokenSource() const { return _source.tokenSource; }

CharStream *CommonToken::getInputStream() const { return _source.charStream; }

std::string CommonToken::toString() const { return ""; }

// TODO: (CommonToken) INVALID_INDEX
void CommonToken::InitializeInstanceFields() {
    _type = 0;
    _line = 0;
    _charPositionInLine = 0;
    _channel = DEFAULT_CHANNEL;
    _index = 0;
    _start = 0;
    _stop = 0;
    _source = EMPTY_SOURCE;
}
