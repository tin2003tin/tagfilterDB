#ifndef COMPILER_TOKEN_HPP
#define COMPILER_TOKEN_HPP

#include <unordered_set>
#include <string>
#include <vector>
#include <iostream>

namespace tin_compiler
{
    class Token
    {
    public:
        static const std::unordered_set<char> TOKENSYMBOL;
        static const std::unordered_set<char> STRING_SYMBOL;
        enum Type
        {
            IDENTIFIER,
            KEYWORD,
            SYMBOL,
            STRING
        };

        Type type;
        std::string value;
        Token(Type type, std::string value) : type(type), value(value) {}
        static void display(const std::vector<Token> &tokens)
        {
            const std::string yellow = "\033[33m";
            const std::string reset = "\033[0m";
            std::cout << yellow + "==Tokens==" + reset << std::endl;
            for (const auto &token : tokens)
            {
                std::cout << "Type: " << token.type << ", Value: " << token.value << std::endl;
            }
        }
    };

    const std::unordered_set<char> Token::TOKENSYMBOL = {
        '(',
        ')',
        '{',
        '}',
        ',',
        '=',
        '!',
        '-',
        '+',
        ';',
        '$',
        '<',
        '>',
        '#',
        '[',
        ']',
        '.',
        '*',
    };
    const std::unordered_set<char> Token::STRING_SYMBOL = {
        '\'',
        '"'};
}

#endif
