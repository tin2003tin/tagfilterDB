#ifndef TIN_COMPILER_LIB_STRING_HPP
#define TIN_COMPILER_LIB_STRING_HPP

#include "tagfilterdb/common.hpp"

namespace tagfilterdb::support {

class String {
  public:
    static void trim(std::string &str);
    static void split(const std::string &text, const std::string &delimiter,
                      std::vector<std::string> &tokens);
    static std::vector<std::string> split(const std::string &text,
                                          const std::string &delimiter);
    static std::string join(const std::vector<std::string> &vec,
                            const std::string &separator);
    static std::string toHexString(const int t);
    static std::string arrayToString(const std::vector<std::string> &data);
    static std::string escapeWhitespace(std::string_view in);
    static std::string &escapeWhitespace(std::string &out, std::string_view in);
};
} // namespace tagfilterdb::support

#include "string.cpp"

#endif // LIB_HPP
