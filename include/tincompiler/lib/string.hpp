#ifndef TIN_COMPILER_LIB_STRING_HPP
#define TIN_COMPILER_LIB_STRING_HPP

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace tin_compiler {

class String {
  public:
    static void trim(std::string &str);
    static void split(const std::string &text, const std::string &delimiter,
                      std::vector<std::string> &tokens);
    static std::vector<std::string> split(const std::string &text,
                                          const std::string &delimiter);
};
} // namespace tin_compiler

#include "string.cpp"

#endif // LIB_HPP
