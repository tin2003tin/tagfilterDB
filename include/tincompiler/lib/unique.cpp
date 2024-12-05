#include "unique.hpp"
#include <algorithm>
#include <sstream>

namespace tin_compiler
{

    template <typename Element>
    bool Unique::addUnique(const Element &element, std::unordered_set<Element> &set)
    {
        return set.insert(element).second;
    }

    template <typename Key, typename Value>
    bool Unique::addUnique(const Key &key, const Value &value, std::unordered_map<Key, Value> &map)
    {
        return map.emplace(key, value).second;
    }

    template <typename Element>
    bool Unique::addUnique(const Element &element, std::vector<Element> &vec)
    {
        if (!isElement(element, vec))
        {
            vec.push_back(element);
            return true;
        }
        return false;
    }

    template <typename Key, typename Value>
    bool Unique::appendMap(const Key &key, const Value &value, std::unordered_map<Key, std::unordered_set<Value>> &map)
    {
        auto &set = map[key];
        if (std::find(set.begin(), set.end(), value) == set.end())
        {
            set.insert(value);
            return true;
        }
        return false;
    }

    template <typename Element>
    bool Unique::isElement(const Element &element, const std::unordered_set<Element> &set)
    {
        return set.find(element) != set.end();
    }

    template <typename Key, typename Value>
    bool Unique::isElement(const Key &key, std::unordered_map<Key, Value> &map)
    {
        return map.find(key) != map.end();
    }
    template <typename Element>
    bool Unique::isElement(const Element &element, const std::vector<Element> &vec)
    {
        return std::find(vec.begin(), vec.end(), element) != vec.end();
    }
}