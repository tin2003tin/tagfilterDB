#include "item.hpp"
#include <algorithm>
#include <queue>

namespace tin_compiler
{

    Item::Item(Rule *rule, int dotIndex)
        : rule(rule), dotIndex(dotIndex)
    {
        if (dotIndex > rule->development.size())
        {
            this->dotIndex = rule->development.size();
        }
        if (rule->index == 0)
        {
            lookAheads.insert(END);
        }
    }

    Item::Item(Rule *rule, int dotIndex, std::unordered_set<std::string> lookAheads)
        : rule(rule), dotIndex(dotIndex), lookAheads(lookAheads)
    {
        if (dotIndex > rule->development.size())
        {
            this->dotIndex = rule->development.size();
        }
        if (rule->index == 0)
        {
            lookAheads.insert(END);
        }
    }

    bool Item::operator==(const Item &other) const
    {
        if (rule->development.size() != other.rule->development.size())
        {
            return false;
        }

        if (dotIndex != other.dotIndex && rule->nonTerminal != other.rule->nonTerminal)
        {
            return false;
        }

        for (size_t i = 0; i < rule->development.size(); ++i)
        {
            if (rule->development[i] != other.rule->development[i])
            {
                return false;
            }
        }
        return true;
    }

    std::unordered_set<Item> Item::newItemAfterShift()
    {
        std::unordered_set<Item> newItems;
        if (dotIndex < rule->development.size() && rule->development[dotIndex] != EPSILON)
        {
            Item item = Item(rule, dotIndex + 1);
            item.lookAheads = lookAheads;
            newItems.insert(item);
        }
        return newItems;
    }

    std::unordered_set<Item> Item::newItemsFromSymbolAfterDot(Grammar *grammar)
    {
        std::unordered_set<Item> newItems;
        if (dotIndex >= rule->development.size())
        {
            return newItems;
        }
        std::unordered_set<Rule *> closers = grammar->getRulesForNonterminal(rule->development[dotIndex]);
        std::queue<Rule *> queue;
        for (auto &closer : closers)
        {
            queue.push(closer);
        }
        Rule *top;
        while (!queue.empty())
        {
            top = queue.front();
            queue.pop();
            std::unordered_set<Rule *> newRules = grammar->getRulesForNonterminal(top->development[0]);
            for (auto &newRule : newRules)
            {
                if (!Unique::isElement(newRule, closers))
                {
                    Unique::addUnique(newRule, closers);
                    queue.push(newRule);
                }
            }
        }

        for (auto &closer : closers)
        {
            newItems.insert(Item(closer, 0, grammar->follows[closer->nonTerminal]));
        }

        return newItems;
    }

    void Item::display()
    {
        std::string out;
        for (int i = 0; i < rule->development.size(); i++)
        {
            if (i == dotIndex)
            {
                out.append(".");
            }
            out.append(rule->development[i] + " ");
        }
        if (dotIndex == rule->development.size())
        {
            out.append(".");
        }
        out.append(" | ");
        for (auto &look : lookAheads)
        {
            out.append(look + " ");
        }
        std::cout << rule->nonTerminal << ": " << out << std::endl;
    }
}
