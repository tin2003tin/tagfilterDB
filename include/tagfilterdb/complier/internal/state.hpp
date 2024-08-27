#ifndef TIN_COMPILER_STATE_HPP
#define TIN_COMPILER_STATE_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>

class State
{
public:
    int index;
    std::unordered_map<std::string, std::string> lrAction;
    std::unordered_map<std::string, int> lrGoto;

    State() {}
    explicit State(int index) : index(index) {}
    void display()
    {
        std::cout << "State " << index << ":\n";
        std::cout << "Actions:\n";
        for (auto &action : lrAction)
        {
            std::cout << "  " << action.first << " -> " << action.second << "\n";
        }
        std::cout << "Gotos:\n";
        for (auto &go : lrGoto)
        {
            std::cout << "  " << go.first << " -> " << go.second << "\n";
        }
    };
};

#endif