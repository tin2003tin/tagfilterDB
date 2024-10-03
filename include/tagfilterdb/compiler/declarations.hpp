#pragma once

#include "utility"

namespace tagfilterdb {
namespace compiler {
class IntStream;
class Token;
class TokenSource;
class CommonToken;
class CharStream;
class Recognizer;
class RuleContext;
class ParserRuleContext;
class Parser;
class Vocabulary;
class SourceStream;

template <typename Symbol> class TokenFactory;

class ProxyErrorListener;
class ErrorListener;

namespace atn {
class ATN;
class ATNDataView;
enum class ATNTpye;
class ATNState;
class BasicBlockStartState;
class BasicState;
class BlockEndState;
class BlockStartState;
class DecisionState;
class LoopEndState;
class PlusBlockStartState;
class PlusLoopbackState;
class RuleStartState;
class RuleStopState;
class StarBlockStartState;
class StarLoopEntryState;
class StarLoopbackState;
class TokensStartState;
} // namespace atn

} // namespace compiler

namespace support {
class Range;
class RangeSet;
} // namespace support
} // namespace tagfilterdb