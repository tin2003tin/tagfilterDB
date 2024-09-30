#include "vocabulary.hpp"
#include "token.hpp"

using namespace tagfilterdb::compiler;

const Vocabulary Vocabulary::EMPTY_VOCABULARY;

Vocabulary::Vocabulary(std::vector<std::string> literalNames,
                       std::vector<std::string> symbolicNames)
    : Vocabulary(std::move(literalNames), std::move(symbolicNames), {}) {}

Vocabulary::Vocabulary(std::vector<std::string> literalNames,
                       std::vector<std::string> symbolicNames,
                       std::vector<std::string> displayNames)
    : _literalNames(std::move(literalNames)),
      _symbolicNames(std::move(symbolicNames)),
      _displayNames(std::move(displayNames)),
      _maxTokenType(
          std::max(_displayNames.size(),
                   std::max(_literalNames.size(), _symbolicNames.size())) -
          1) {}

std::string_view Vocabulary::getLiteralName(size_t tokenType) const {
    if (tokenType < _literalNames.size()) {
        return _literalNames[tokenType];
    }

    return "";
}

std::string_view Vocabulary::getSymbolicName(size_t tokenType) const {
    if (tokenType == Token::EOF) {
        return "EOF";
    }

    if (tokenType < _symbolicNames.size()) {
        return _symbolicNames[tokenType];
    }

    return "";
}

std::string Vocabulary::getDisplayName(size_t tokenType) const {
    if (tokenType < _displayNames.size()) {
        std::string_view displayName = _displayNames[tokenType];
        if (!displayName.empty()) {
            return std::string(displayName);
        }
    }
    std::string_view literalName = getLiteralName(tokenType);
    if (!literalName.empty()) {
        return std::string(literalName);
    }

    std::string_view symbolicName = getSymbolicName(tokenType);
    if (!symbolicName.empty()) {
        return std::string(symbolicName);
    }

    return std::to_string(tokenType);
}