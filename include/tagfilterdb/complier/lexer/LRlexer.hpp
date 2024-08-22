#ifndef COMPILER_LR_LEXER_HPP
#define COMPILER_LR_LEXER_HPP

#include <string>
#include <tuple>
#include <unordered_set>
#include <unordered_map>
#include "../type/token.hpp"
#include "../internal/strategy.hpp"
#include "lexer.hpp"

namespace tin_compiler
{
    class LRLexer : public Lexer
    {
    public:
        LRLexer() {}
        LRLexer(const std::unordered_set<char> &symbols, const std::unordered_set<char> &stringSymbols, Strategy &s)
            : position(0), symbols(symbols), stringSymbols(stringSymbols), terminals(s.grammar.terminals)
        {
        }

        LRLexer(const std::unordered_set<char> &symbols, const std::unordered_set<char> &stringSymbol,
                const std::unordered_set<std::string> &terminals)
            : position(0), symbols(symbols), stringSymbols(stringSymbols), terminals(terminals)
        {
        }
        LRLexer &setInput(const std::string &text) override;
        std::vector<Token> tokenize() override;

    private:
        std::string input;
        size_t position;
        std::unordered_set<char> symbols;
        std::unordered_set<char> stringSymbols;
        std::unordered_set<std::string> terminals;

        std::string readWord();
        std::string readNumber();
        bool isSymbol(char c);
        bool isStringSymbol(char c);
        bool isTerminal(const std::string &word);
    };
}

#include "LRlexer.cpp"

#endif
