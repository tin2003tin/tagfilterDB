#ifndef GRAMMAR_HPP
#define GRAMMAR_HPP

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <iostream>
#include "rule.hpp"

namespace tin_compiler
{
    const std::string EPSILON = "ε";
    const std::string END = "$";

    class Grammar
    {
    private:
        void initializeRulesAndAlphabetAndNonterminals(const std::string &text);
        void initializeAlphabetAndTerminals();
        void initializeFirsts();
        void initializeFollows();
        bool collectDevelopmentFirsts(const Rule &rule);

    public:
        std::unordered_set<std::string> alphabet;
        std::unordered_set<std::string> nonTerminals;
        std::unordered_set<std::string> terminals;
        std::vector<Rule> rules;
        std::unordered_map<std::string, std::unordered_set<std::string>> firsts;
        std::unordered_map<std::string, std::unordered_set<std::string>> follows;
        std::string axiom;

        Grammar() {}
        Grammar(const std::string &text);
        ~Grammar();

        void setGrammar(const std::string &text);
        std::vector<std::string> getSquenceFirsts(std::vector<std::string>);
        std::unordered_set<Rule *> getRulesForNonterminal(std::string nonterminal);
        void displayRules() const;
        void detail() const;
    };
}

#include "grammar.cpp"

#endif // GRAMMAR_HPP
