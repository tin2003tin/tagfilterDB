#ifndef COMPILER_LEXER_HPP
#define COMPILER_LEXER_HPP

#include <string>
#include <vector>
#include "../type/token.hpp"

namespace tin_compiler
{
    class Lexer
    {
    public:
        virtual ~Lexer() = default;
        virtual Lexer &setInput(const std::string &text) = 0;
        virtual std::vector<Token> tokenize() = 0;
    };
}

#endif
