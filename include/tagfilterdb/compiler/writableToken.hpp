#pragma once

#include "token.hpp"

namespace tagfilterdb::compiler {
class WritableToken : public Token {
  public:
    virtual ~WritableToken() = default;
    virtual void setText(const std::string &text) = 0;
    virtual void setType(size_t ttype) = 0;
    virtual void setLine(size_t line) = 0;
    virtual void setCharPositionInLine(size_t pos) = 0;
    virtual void setChannel(size_t channel) = 0;
    virtual void setTokenIndex(size_t index) = 0;
};
} // namespace tagfilterdb::compiler