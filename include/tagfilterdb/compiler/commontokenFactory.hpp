#pragma once

#include "tokenFactory.hpp"

namespace tagfilterdb::compiler {
class CommonTokenFactory : public TokenFactory<CommonToken> {
  public:
    static const Unique<TokenFactory<CommonToken>> DEFAULT;

  protected:
    const bool copyText;

  public:
    CommonTokenFactory(bool copyText);
    CommonTokenFactory();

    virtual Unique<CommonToken> create(SourceStream source, size_t type,
                                       const std::string &text, size_t channel,
                                       size_t start, size_t stop, size_t line,
                                       size_t charPositionInLine) override;
    virtual Unique<CommonToken> create(size_t type,
                                       const std::string &text) override;
};
} // namespace tagfilterdb::compiler

#include "commontokenFactory.cpp"