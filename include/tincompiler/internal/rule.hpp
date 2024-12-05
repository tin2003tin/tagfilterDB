#ifndef TIN_COMPILER_RULE_HPP
#define TIN_COMPILER_RULE_HPP

#include <string>
#include <vector>
#include "../lib/string.hpp"
#include <iostream>
#include "json.hpp"

namespace tin_compiler
{
    class Rule
    {
    private:
        void createDevelopment(const std::string &text);

    public:
        int index;
        std::string nonTerminal;
        std::vector<std::string> development;

        Rule(int index, const std::string &nont, const std::vector<std::string> &dev);
        explicit Rule(int index, const std::string &text);
        std::string toString() const;
        nlohmann::json toJson() const;
        bool operator==(const Rule &other) const;
    };
}

#include "rule.cpp"

#endif // TIN_COMPILER_RULE_HPP
