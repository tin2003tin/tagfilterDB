#include "string.hpp"
#include <algorithm>
#include <sstream>

namespace tin_compiler
{

    void String::trim(std::string &str)
    {
        str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch)
                                            { return !std::isspace(ch); }));
        str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch)
                               { return !std::isspace(ch); })
                      .base(),
                  str.end());
    }

    void String::split(const std::string &text, const std::string &delimiter, std::vector<std::string> &tokens)
    {
        size_t start = 0;
        size_t end = text.find(delimiter);
        while (end != std::string::npos)
        {
            tokens.push_back(text.substr(start, end - start));
            start = end + delimiter.length();
            end = text.find(delimiter, start);
        }
        tokens.push_back(text.substr(start, end));
    }
}