#include "rule.hpp"

namespace tin_compiler
{
    void Rule::createDevelopment(const std::string &text)
    {
        std::vector<std::string> split;
        String::split(text, "->", split);
        std::string nont = split[0];
        String::trim(nont);
        nonTerminal = nont;
        std::string development = split[1];
        String::trim(development);
        String::split(development, " ", this->development);
    }

    Rule::Rule(int index, const std::string &nont, const std::vector<std::string> &dev)
        : index(index), nonTerminal(nont), development(dev) {}

    Rule::Rule(int index, const std::string &text) : index(index)
    {
        createDevelopment(text);
    }

    bool Rule::operator==(const Rule &other) const
    {
        if (this->nonTerminal != other.nonTerminal)
        {
            return false;
        }
        if (this->development.size() != other.development.size())
        {
            return false;
        }
        for (int i = 0; i < this->development.size(); i++)
        {
            if (this->development[i] != other.development[i])
            {
                return false;
            }
        }
        return true;
    }

     std::string Rule::toString() const {
        std::ostringstream oss;
        oss << "Rule " << index << ": " << nonTerminal << " -> ";
        for (const auto &dev : development) {
            oss << dev << " ";
        }
        return oss.str();
    }

     nlohmann::json Rule::toJson() const {
        nlohmann::json j;
        j["index"] = index;                   
        j["non_terminal"] = nonTerminal;      // Add non-terminal
        j["development"] = development;       // Add development

        return j;                             // Return the constructed JSON object
    }
}
