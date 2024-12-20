#ifndef LR_COMPILER_HPP
#define LR_COMPILER_HPP

#include "compiler.hpp"
#include "internal/LRstrategy.hpp"
#include "parser/LRparser.hpp"
#include "lexer/LRlexer.hpp"

namespace tin_compiler
{
    class LRCompiler : public Compiler
    {
    private:
        LRStrategy strategy;
        LRLexer lexer;
        LRParser parser;
        HandlerControl handlerControl;

    public:
        LRCompiler(const std::string &grammarRule, std::shared_ptr<CompilerHandler> handler = std::make_shared<NullHandler>())
            : strategy(LRStrategy(grammarRule)),
              lexer(LRLexer(Token::TOKENSYMBOL, Token::STRING_SYMBOL, strategy)),
              parser(LRParser(strategy)),
              handlerControl(HandlerControl(handler))
        {
            strategy.buildState();
        }
        Strategy &getStrategy() override
        {
            return strategy;
        }
        Parser &getParser() override
        {
            return parser;
        }
        Lexer &getLexer() override
        {
            return lexer;
        }
        HandlerControl &getHandlerControl() override
        {
            return handlerControl;
        }

        std::string toString() const {
            return strategy.toString();
        }

        void details() const override {
            std::cout << toString();
        }
        nlohmann::json toJson() const override { 
            return strategy.toJson();
        }
    };
}
#endif
