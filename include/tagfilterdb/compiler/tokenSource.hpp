#pragma once

#include "tokenFactory.hpp"

namespace tagfilterdb::compiler {
class TokenSource {
  public:
    virtual ~TokenSource();
    virtual Unique<Token> nextToken() = 0;
    virtual size_t getLine() const = 0;
    virtual size_t getCharPositionInLine() = 0;
    virtual CharStream *getInputStream() = 0;
    virtual std::string getSourceName() = 0;
    template <typename T1>
    void setTokenFactory(TokenFactory<T1> * /*factory*/) {}
    virtual TokenFactory<CommonToken> *getTokenFactory() = 0;
};
} // namespace tagfilterdb::compiler