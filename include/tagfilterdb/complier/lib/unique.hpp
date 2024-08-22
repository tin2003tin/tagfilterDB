#ifndef TIN_COMPILER_LIB_UNIQUE_HPP
#define TIN_COMPILER_LIB_UNIQUE_HPP

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace tin_compiler
{

    class Unique
    {
    public:
        template <typename Element>
        static bool addUnique(const Element &element, std::unordered_set<Element> &set);
        template <typename Element>
        static bool addUnique(const Element &element, std::vector<Element> &vec);
        template <typename Key, typename Value>
        static bool addUnique(const Key &value, const Value &key, std::unordered_map<Key, Value> &map);
        template <typename Key, typename Value>
        static bool appendMap(const Key &value, const Value &key, std::unordered_map<Key, std::unordered_set<Value>> &map);
        template <typename Element>
        static bool isElement(const Element &element, const std::unordered_set<Element> &set);
        template <typename Key, typename Value>
        static bool isElement(const Key &value, std::unordered_map<Key, Value> &map);
        template <typename Element>
        static bool isElement(const Element &element, const std::vector<Element> &vec);
    };
}

#include "unique.cpp"

#endif // LIB_HPP
