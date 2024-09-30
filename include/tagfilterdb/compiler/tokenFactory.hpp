#pragma once

#include "tagfilterdb/compiler/internalCommon.hpp"

namespace tagfilterdb::compiler {
template <typename Symbol> class TokenFactory {
  public:
    virtual ~TokenFactory() {}
    virtual Unique<Symbol> create(SourceStream source, size_t type,
                                  const std::string &text, size_t channel,
                                  size_t start, size_t stop, size_t line,
                                  size_t charPositionInLine) = 0;
    virtual Unique<Symbol> create(size_t type, const std::string &text) = 0;
};
} // namespace tagfilterdb::compiler