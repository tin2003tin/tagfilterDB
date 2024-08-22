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

    void LRStrategy::displayGrammar()
    {
        const std::string yellow = "\033[33m";
        const std::string reset = "\033[0m";
        std::cout << yellow + "==Grammar==" + reset << std::endl;
        grammar.detail();
    }

    void LRStrategy::displayState()
    {
        const std::string yellow = "\033[33m";
        const std::string reset = "\033[0m";
        std::cout << yellow + "==State==" + reset << std::endl;
        for (auto state : states)
        {
            state.display();
            printf("\n");
        }
    }
    void LRStrategy::displayHandler()
    {
        const std::string yellow = "\033[33m";
        const std::string reset = "\033[0m";
        std::cout << yellow + "==Handler==" + reset << std::endl;
        for (const auto &handler : handlerName)
        {
            std::cout << handler << "\n";
        }
    }
}
