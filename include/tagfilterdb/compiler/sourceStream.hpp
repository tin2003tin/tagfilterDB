#pragma once

#include "charStream.hpp"
#include "internalCommon.hpp"
#include "tokenSource.hpp"

namespace tagfilterdb::compiler {
class SourceStream {
  public:
    TokenSource *tokenSource;
    CharStream *charStream;

    SourceStream() : tokenSource(nullptr), charStream(nullptr) {}
    SourceStream(TokenSource *t, CharStream *c)
        : tokenSource(t), charStream(c) {}
};
} // namespace tagfilterdb::compiler
