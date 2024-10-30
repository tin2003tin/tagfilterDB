#ifndef COMPILER_LRSTRETEGY_HPP
#define COMPILER_LRSTRETEGY_HPP

#include <vector>
#include <string>
#include <unordered_map>

#include "strategy.hpp"
#include "update.hpp"

namespace tin_compiler
{

    class LRStrategy : public Strategy
    {
    private:
        void init(const std::string &text);

    public:
        void buildState() override;

        LRStrategy(const std::string &compilerText)
        {
            init(compilerText);
        }
        void displayState() override;
        void displayHandler() override;
        void displayGrammar() override;
    };

}

#include "LRstrategy.cpp"

#endif
