#ifndef COMPILER_HPP
#define COMPILER_HPP

#include <vector>
#include <string>
#include "internal/strategy.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "handlerControl/handlerControl.hpp"

namespace tin_compiler
{
    class Compiler
    {
    public:
        ~Compiler() = default;

        virtual Strategy &getStrategy() = 0;
        virtual Parser &getParser() = 0;
        virtual Lexer &getLexer() = 0;
        virtual HandlerControl &getHandlerControl() = 0;

        virtual void
        details(bool g = true, bool h = false, bool s = false)
        {
        }
    };
}

#endif
