#ifndef COMPILER_LR_PARSER_HPP
#define COMPILER_LR_PARSER_HPP

#include <vector>
#include <string>
#include <unordered_map>
#include <stack>
#include <memory>
#include "stackable.hpp"
#include "ast.hpp"
#include "../internal/strategy.hpp"
#include "../type/token.hpp"
#include "parser.hpp"

namespace tin_compiler
{
    class LRParser : public Parser
    {
    private:
        Strategy *strategy;
        std::stack<std::shared_ptr<Stackable>> stack;
        std::vector<Token> *tokens;
        int currIndex;
        bool isLog = false;

        void
        shift(const std::string &action, Token::Type tokenType);
        std::shared_ptr<InterASTNode> reduce(int ruleIndex);
        void goTo(int nextState, std::shared_ptr<InterASTNode> node);

        void printStack() const;

    public:
        LRParser(Strategy &s) : strategy(&s) {}

        Parser &setTokens(std::vector<Token> &tokens) override;
        std::shared_ptr<ASTNode> parse() override;
        void setLog(bool b) { isLog = b; }
    };

}

#include "LRparser.cpp"

#endif
