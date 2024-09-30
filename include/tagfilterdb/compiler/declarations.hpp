#pragma once

namespace tagfilterdb::compiler {
class IntStream;
class Token;
class TokenSource;
class CommonToken;
class CharStream;
class Recognizer;
class ParserRuleContext;

using SourceStream = std::pair<TokenSource *, CharStream *>;
} // namespace tagfilterdb::compiler