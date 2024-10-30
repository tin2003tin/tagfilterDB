#pragma once

#include "tagfilterdb/compiler/internalCommon.hpp"

namespace tagfilterdb::compiler {
class Vocabulary final {
  public:
    [[deprecated("Use the default constructor of Vocabulary "
                 "instead.")]] static const Vocabulary EMPTY_VOCABULARY;
    Vocabulary() {}
    Vocabulary(const Vocabulary &) = default;
    Vocabulary(std::vector<std::string> literalNames,
               std::vector<std::string> symbolicNames);
    Vocabulary(std::vector<std::string> literalNames,
               std::vector<std::string> symbolicNames,
               std::vector<std::string> displayNames);

    constexpr size_t getMaxTokenType() const { return _maxTokenType; }
    std::string_view getLiteralName(size_t tokenType) const;
    std::string_view getSymbolicName(size_t tokenType) const;
    std::string getDisplayName(size_t tokenType) const;

  private:
    std::vector<std::string> const _literalNames;
    std::vector<std::string> const _symbolicNames;
    std::vector<std::string> const _displayNames;
    const size_t _maxTokenType = 0;
};
} // namespace tagfilerdb::compiler

#include "vocabulary.cpp"