#ifndef COMPILER_STRETEGY_HPP
#define COMPILER_STRETEGY_HPP

#include "grammar.hpp"
#include "state.hpp"

namespace tin_compiler
{
    class Strategy
    {

    public:
        Grammar grammar;
        std::vector<State> states;
        std::vector<std::string> handlerName;

        virtual void buildState() = 0;
        virtual void displayState() = 0;
        virtual void displayHandler() = 0;
        virtual void displayGrammar() = 0;

        virtual ~Strategy() = default;
    };
}

#endif