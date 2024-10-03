#pragma once

#include "intStream.hpp"

namespace tagfilterdb::compiler {
class Token {
  public:
    static constexpr size_t INVALID_TYPE = 0;
    static constexpr size_t EPSILON = std::numeric_limits<size_t>::max() - 1;
    static constexpr size_t MIN_USER_TOKEN_TYPE = 1;
    static constexpr size_t EOF = IntStream::EOF;

    virtual ~Token() = default;

    static constexpr size_t DEFAULT_CHANNEL = 0;
    static constexpr size_t HIDDEN_CHANNEL = 1;
    static constexpr size_t MIN_USER_CHANNEL_VALUE = 2;

    virtual std::string getText() const = 0;
    virtual size_t getType() const = 0;
    virtual size_t getLine() const = 0;
    virtual size_t getCharPositionInLine() const = 0;
    virtual size_t getChannel() const = 0;
    virtual size_t getTokenIndex() const = 0;
    virtual size_t getStartIndex() const = 0;
    virtual size_t getStopIndex() const = 0;
    virtual TokenSource *getTokenSource() const = 0;
    virtual CharStream *getInputStream() const = 0;
    virtual std::string toString() const = 0;
};
} // namespace tagfilterdb::compiler