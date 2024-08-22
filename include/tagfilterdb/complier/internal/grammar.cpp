#include "grammar.hpp"
#include "../lib/string.hpp"
#include "../lib/unique.hpp"

namespace tin_compiler
{

    Grammar::Grammar(const std::string &text)
    {
        setGrammar(text);
    }

    Grammar::~Grammar()
    {
        // Destructor logic if needed
    }

    void Grammar::setGrammar(const std::string &text)
    {
        initializeRulesAndAlphabetAndNonterminals(text);
        initializeAlphabetAndTerminals();
        initializeFirsts();
        initializeFollows();
    }

    void Grammar::initializeRulesAndAlphabetAndNonterminals(const std::string &text)
    {
        std::vector<std::string> lines;
        String::split(text, "\n", lines);

        for (auto &line : lines)
        {
            String::trim(line);
            if (!line.empty())
            {
                Rule rule(rules.size(), line);

                rules.push_back(rule);

                if (axiom.empty())
                {
                    axiom = rule.nonTerminal;
                }

                Unique::addUnique(rule.nonTerminal, alphabet);
                Unique::addUnique(rule.nonTerminal, nonTerminals);
            }
        }
    }

    void Grammar::initializeAlphabetAndTerminals()
    {
        for (const auto &rule : rules)
        {
            for (const auto &symbol : rule.development)
            {
                if (symbol != EPSILON && !Unique::isElement(symbol, nonTerminals))
                {
                    Unique::addUnique(symbol, alphabet);
                    Unique::addUnique(symbol, terminals);
                }
            }
        }
    }

    void Grammar::initializeFirsts()
    {
        bool notDone;
        while (true)
        {
            notDone = false;
            for (auto &rule : rules)
            {
                if (rule.development.size() == 1 && rule.development[0] == EPSILON)
                {
                    notDone = Unique::appendMap(rule.nonTerminal, EPSILON, firsts);
                }
                else
                {
                    notDone = collectDevelopmentFirsts(rule) || notDone;
                }
            }
            if (!notDone)
            {
                break;
            }
        }
    }

    void Grammar::initializeFollows()
    {
        bool notDone = true;
        while (notDone)
        {
            notDone = false;
            for (int i = 0; i < rules.size(); i++)
            {
                Rule rule = rules[i];
                if (i == 0)
                {
                    notDone = Unique::appendMap(rule.nonTerminal, END, follows);
                }
                for (int j = 0; j < rule.development.size(); j++)
                {
                    std::string symbol = rule.development[j];
                    if (Unique::isElement(symbol, nonTerminals))
                    {
                        std::vector<std::string> development;
                        for (int d = j + 1; d < rule.development.size(); d++)
                        {
                            development.push_back(rule.development[d]);
                        }
                        std::vector<std::string> afterSymbolFirsts = getSquenceFirsts(development);
                        for (auto &first : afterSymbolFirsts)
                        {
                            if (first == EPSILON)
                            {
                                std::unordered_set<std::string> nonTerminalFollows = follows[rule.nonTerminal];
                                for (auto autoFollow : nonTerminalFollows)
                                {
                                    notDone = Unique::appendMap(symbol, autoFollow, follows);
                                }
                            }
                            else
                            {
                                notDone = Unique::appendMap(symbol, first, follows);
                            }
                        }
                    }
                }
            }
        }
    }

    bool Grammar::collectDevelopmentFirsts(const Rule &rule)
    {
        bool result = false;
        bool epsilonInSymbolFirsts = true;

        for (const auto &symbol : rule.development)
        {
            epsilonInSymbolFirsts = false;
            if (Unique::isElement(symbol, terminals))
            {
                result = Unique::appendMap(rule.nonTerminal, symbol, firsts);
                break;
            }
            if (firsts.find(symbol) != firsts.end())
            {
                for (const auto &first : firsts[symbol])
                {
                    epsilonInSymbolFirsts = epsilonInSymbolFirsts || first == EPSILON;
                    result = Unique::appendMap(rule.nonTerminal, first, firsts) || result;
                }
            }
            if (!epsilonInSymbolFirsts)
            {
                break;
            }
        }
        if (epsilonInSymbolFirsts)
        {
            result = Unique::appendMap(rule.nonTerminal, EPSILON, firsts) || result;
        }
        return result;
    }

    std::unordered_set<Rule *> Grammar::getRulesForNonterminal(std::string nonterminal)
    {
        std::unordered_set<Rule *> result;
        for (auto &rule : rules)
        {
            if (nonterminal == rule.nonTerminal)
            {
                result.insert(&rule);
            }
        }
        return result;
    }

    std::vector<std::string> Grammar::getSquenceFirsts(std::vector<std::string> sequence)
    {
        std::vector<std::string> result;
        bool epsilonInSymbolFirsts = true;
        for (auto &symbol : sequence)
        {
            epsilonInSymbolFirsts = false;
            if (Unique::isElement(symbol, terminals))
            {
                Unique::addUnique(symbol, result);
                break;
            }
            for (auto &first : firsts[symbol])
            {
                epsilonInSymbolFirsts = epsilonInSymbolFirsts || first == EPSILON;
                Unique::addUnique(first, result);
            }
            epsilonInSymbolFirsts = epsilonInSymbolFirsts || firsts[symbol].size() == 0;
            if (!epsilonInSymbolFirsts)
            {
                break;
            }
        }
        if (epsilonInSymbolFirsts)
        {
            Unique::addUnique(EPSILON, result);
        }
        return result;
    }

    void Grammar::displayRules() const
    {
        for (const auto &rule : rules)
        {
            rule.display();
        }
    }

    void Grammar::detail() const
    {
        std::cout << "Axiom: " << axiom << std::endl;

        std::cout << "Alphabet: ";
        for (const auto &symbol : alphabet)
        {
            std::cout << symbol << " ";
        }
        std::cout << std::endl;

        std::cout << "NonTerminals: ";
        for (const auto &nonTerminal : nonTerminals)
        {
            std::cout << nonTerminal << " ";
        }
        std::cout << std::endl;

        std::cout << "Terminals: ";
        for (const auto &terminal : terminals)
        {
            std::cout << terminal << " ";
        }
        std::cout << std::endl;

        std::cout << "Firsts: ";
        for (const auto &first : firsts)
        {
            std::cout << first.first << ": ";
            for (const auto &v : first.second)
            {
                std::cout << v << " ";
            }
        }
        std::cout << std::endl;

        std::cout << "Follows: ";
        for (const auto &follow : follows)
        {
            std::cout << follow.first << ": ";
            for (const auto &v : follow.second)
            {
                std::cout << v << " ";
            }
        }
        std::cout << std::endl;

        displayRules();
    }

}
