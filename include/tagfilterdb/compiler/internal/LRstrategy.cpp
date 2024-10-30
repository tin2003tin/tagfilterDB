#include "LRstrategy.hpp"
#include <iostream>
#include <string>

namespace tin_compiler
{

    void LRStrategy::init(const std::string &text)
    {
        std::string grammarText;
        std::vector<std::string> handlername;
        std::vector<std::string> lines;
        String::split(text, "\n", lines);

        for (auto &line : lines)
        {
            String::trim(line);
            if (line.empty())
            {
                continue;
            }
            if (line.find("##") != std::string::npos)
            {
                std::vector<std::string> parts;
                String::split(line, "##", parts);
                grammarText += parts[0] + "\n";
                String::trim(parts[1]);
                handlerName.push_back(parts[1]);
            }
            else
            {
                grammarText += line + "\n";
                handlerName.push_back("");
            }
        }

        this->grammar = Grammar(grammarText);
        handlerName = handlerName;
    }

    void LRStrategy::buildState()
    {
        KernelGraph graph(grammar);
        for (auto &kernel : graph.kernels)
        {
            State state(states.size());
            for (auto &key : kernel.keys)
            {
                int nextStateIndex = kernel.gotos[key];
                if (Unique::isElement(key, grammar.terminals))
                {
                    std::string text = "s" + std::to_string(nextStateIndex);
                    state.lrAction[key] = text;
                }
                else
                {
                    state.lrGoto[key] = nextStateIndex;
                }
            }
            for (auto &item : kernel.closure)
            {
                if (item.dotIndex == item.rule->development.size() || item.rule->development[0] == EPSILON)
                {
                    for (auto &lookAehad : item.lookAheads)
                    {
                        std::string text = "r" + std::to_string(item.rule->index);
                        if (text == "r0")
                        {
                            text = "accept";
                        }
                        state.lrAction[lookAehad] = text;
                    }
                }
            }
            states.push_back(state);
        }
    }

    std::string LRStrategy::toString() const {
        std::ostringstream oss;

        oss << "==Grammar==\n";
        oss << grammar.toString(); // Assuming grammar has a toString method
        oss << "\n";

        oss << "==State==\n";
        for (const auto &state : states) {
            oss << state.toString() << "\n"; // Assuming State has a toString method
        }
        oss << "\n";

        oss << "==Handler==\n";
        for (const auto &handler : handlerName) {
            oss << handler << "\n";
        }

        return oss.str(); // Convert the stream to string
    }

     nlohmann::json LRStrategy::toJson() const {
        nlohmann::json j;

        // Add grammar to JSON
        j["grammar"] = grammar.toJson(); // Assuming Grammar has a toJson method

        // Add states to JSON
        for (const auto &state : states) {
            j["states"].push_back(state.toJson()); // Assuming State has a toJson method
        }

        // Add handlers to JSON
        j["handlers"] = handlerName;

        return j; // Return the constructed JSON object
    }
}
