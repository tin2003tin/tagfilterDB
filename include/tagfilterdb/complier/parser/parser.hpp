#ifndef COMPILER_PARSER_HPP
#define COMPILER_PARSER_HPP

#include <vector>
#include <string>
#include <memory>

#include "ast.hpp"
#include "../internal/strategy.hpp"
#include "../type/token.hpp"

namespace tin_compiler
{
    class Parser
    {
    public:
        virtual Parser &setTokens(std::vector<Token> &tokens) = 0;
        virtual std::shared_ptr<ASTNode> parse() = 0;
        virtual void setLog(bool b) = 0;
        virtual ~Parser() = default;
    };

}

#endif
