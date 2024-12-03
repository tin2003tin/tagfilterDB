#ifndef ITEM_HPP
#define ITEM_HPP

#include <vector>
#include <string>

#include "rule.hpp"
#include "grammar.hpp"

namespace tin_compiler
{
    class Item
    {

    public:
        Rule *rule;
        int dotIndex;
        std::unordered_set<std::string> lookAheads;

        Item(Rule *rule, int dotIndex);
        explicit Item(Rule *rule, int dotIndex, std::unordered_set<std::string> lookAheads);

        bool operator==(const Item &other) const;
        std::unordered_set<Item> newItemAfterShift();
        std::unordered_set<Item> newItemsFromSymbolAfterDot(Grammar *grammar);
        void display();
    };
}

namespace std
{
    template <>
    struct hash<tin_compiler::Item>
    {
        size_t operator()(const tin_compiler::Item &item) const
        {
            size_t hashValue = std::hash<std::string>()(item.rule->nonTerminal);
            hashValue ^= std::hash<int>()(item.dotIndex) + 0x9e3779b9 + (hashValue << 6) + (hashValue >> 2);
            for (const auto &dev : item.rule->development)
            {
                hashValue ^= std::hash<std::string>()(dev) + 0x9e3779b9 + (hashValue << 6) + (hashValue >> 2);
            }
            return hashValue;
        }
    };
}

#include "item.cpp"

#endif // ITEM_HPP
