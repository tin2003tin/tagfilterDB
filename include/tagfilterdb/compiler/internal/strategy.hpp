#ifndef COMPILER_STRETEGY_HPP
#define COMPILER_STRETEGY_HPP

#include "grammar.hpp"
#include "state.hpp"
#include "json.hpp"

namespace tin_compiler
{
    class Strategy
    {

    public:
        Grammar grammar;
        std::vector<State> states;
        std::vector<std::string> handlerName;

        virtual void buildState() = 0;
        virtual std::string toString() const = 0;
        virtual nlohmann::json toJson() const = 0;

        virtual ~Strategy() = default;
    };
}

#endif