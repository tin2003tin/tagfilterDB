#ifndef KERNEL_HPP
#define KERNEL_HPP

#include "item.hpp"
#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm>

namespace tin_compiler
{
    class Kernel
    {
    public:
        int index;

        std::unordered_set<Item> items;
        std::unordered_set<Item> closure;
        std::unordered_map<std::string, int> gotos;
        std::vector<std::string> keys;

        Kernel(int index);
        explicit Kernel(int index, std::unordered_set<Item> items);
        ~Kernel();


        bool operator==(const Kernel &that) const;
        void display();
    };
}

#include "kernel.cpp"

#endif
