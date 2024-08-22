#ifndef TIN_COMPILER_LIB_STRING_HPP
#define TIN_COMPILER_LIB_STRING_HPP

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace tin_compiler
{

    class String
    {
    public:
        static void trim(std::string &str);
        static void split(const std::string &text, const std::string &delimiter, std::vector<std::string> &tokens);
    };
}

#include "string.cpp"

#endif // LIB_HPP
