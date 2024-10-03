#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <tuple>

#include "tagfilterdb/common.hpp"

namespace tagfilterdb::support {

class Unicode final {
  public:
    static constexpr char32_t REPLACEMENT_CHARACTER = 0xfffd;

    static constexpr bool isValid(char32_t codePoint) {
        return codePoint < 0xd800 ||
               (codePoint > 0xdfff && codePoint <= 0x10ffff);
    }

  private:
    Unicode() = delete;
    Unicode(const Unicode &) = delete;
    Unicode(Unicode &&) = delete;
    Unicode &operator=(const Unicode &) = delete;
    Unicode &operator=(Unicode &&) = delete;
};

class Utf8 final {
  public:
    static std::pair<char32_t, size_t> decode(std::string_view input);
    static std::optional<std::u32string> strictDecode(std::string_view input);
    static std::u32string lenientDecode(std::string_view input);
    static std::string &encode(std::string *buffer, char32_t codePoint);
    static std::optional<std::string> strictEncode(std::u32string_view input);
    static std::string lenientEncode(std::u32string_view input);

  private:
    Utf8() = delete;
    Utf8(const Utf8 &) = delete;
    Utf8(Utf8 &&) = delete;
    Utf8 &operator=(const Utf8 &) = delete;
    Utf8 &operator=(Utf8 &&) = delete;
};
} // namespace tagfilterdb::support

#include "utf8.cpp"