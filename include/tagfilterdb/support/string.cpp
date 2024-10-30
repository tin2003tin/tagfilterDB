#include "string.hpp"

namespace tagfilterdb::support {

void String::trim(std::string &str) {
    str.erase(str.begin(),
              std::find_if(str.begin(), str.end(),
                           [](unsigned char ch) { return !std::isspace(ch); }));
    str.erase(std::find_if(str.rbegin(), str.rend(),
                           [](unsigned char ch) { return !std::isspace(ch); })
                  .base(),
              str.end());
}

void String::split(const std::string &text, const std::string &delimiter,
                   std::vector<std::string> &tokens) {
    size_t start = 0;
    size_t end = text.find(delimiter);
    while (end != std::string::npos) {
        tokens.push_back(text.substr(start, end - start));
        start = end + delimiter.length();
        end = text.find(delimiter, start);
    }
    tokens.push_back(text.substr(start, end));
}

std::vector<std::string> String::split(const std::string &text,
                                       const std::string &delimiter) {

    std::vector<std::string> vec;
    split(text, delimiter, vec);
    return vec;
}

std::string String::join(const std::vector<std::string> &vec,
                         const std::string &separator) {
    std::string str;
    bool firstString = true;
    for (const auto &s : vec) {
        if (!firstString) {
            str.append(separator);
        }
        firstString = false;
        str.append(s);
    }
    return str;
}

std::string String::toHexString(const int t) {
    std::stringstream stream;
    stream << std::uppercase << std::hex << t;
    return stream.str();
}

std::string String::arrayToString(const std::vector<std::string> &data) {
    std::string answer;
    size_t toReserve = 0;
    for (const auto &sub : data) {
        toReserve += sub.size();
    }
    answer.reserve(toReserve);
    for (const auto &sub : data) {
        answer.append(sub);
    }
    return answer;
}

std::string String::escapeWhitespace(std::string_view in) {
    std::string out;
    escapeWhitespace(out, in);
    out.shrink_to_fit();
    return out;
}

std::string &String::escapeWhitespace(std::string &out, std::string_view in) {
    out.reserve(in.size()); // Best case, no escaping.
    for (const auto &c : in) {
        switch (c) {
        case '\t':
            out.append("\\t");
            break;
        case '\r':
            out.append("\\r");
            break;
        case '\n':
            out.append("\\n");
            break;
        default:
            out.push_back(c);
            break;
        }
    }
    return out;
}

} // namespace tagfilterdb