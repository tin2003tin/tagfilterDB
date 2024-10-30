#ifndef TIN_COMPILER_STATE_HPP
#define TIN_COMPILER_STATE_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include "json.hpp"

class State
{
public:
    int index;
    std::unordered_map<std::string, std::string> lrAction;
    std::unordered_map<std::string, int> lrGoto;

    State() {}
    explicit State(int index) : index(index) {}

     std::string toString() const {
        std::ostringstream oss;

        oss << "State " << index << ":\n";
        oss << "Actions:\n";
        for (const auto &action : lrAction) {
            oss << "  " << action.first << " -> " << action.second << "\n";
        }
        oss << "Gotos:\n";
        for (const auto &go : lrGoto) {
            oss << "  " << go.first << " -> " << go.second << "\n";
        }

        return oss.str();
    }

     nlohmann::json toJson() const {
        nlohmann::json j;

        j["index"] = index;

        for (const auto &action : lrAction) {
            j["actions"][action.first] = action.second; 
        }

        // Add gotos to the JSON
        for (const auto &go : lrGoto) {
            j["gotos"][go.first] = go.second; 
        }

        return j;
    }
};

#endif