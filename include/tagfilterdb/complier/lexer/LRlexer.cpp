#include <iostream>
#include <cctype>
#include <algorithm>
#include "LRlexer.hpp"

namespace tin_compiler
{
    LRLexer &LRLexer::setInput(const std::string &text)
    {
        input = text;
        position = 0;
        return *this;
    }

    std::vector<Token> LRLexer::tokenize()
    {
        bool isOpenString = false;
        char openString;
        std::vector<Token> tokens;
        while (position < input.length())
        {
            char current_char = input[position];
            if (isOpenString)
            {
                if (current_char == openString)
                {
                    isOpenString = false;
                    ++position;
                }
                else
                {
                    tokens.back().value += current_char;
                    ++position;
                }
            }
            else if (isalpha(current_char))
            {
                std::string word = readWord();
                tin_compiler::Token::Type type = isTerminal(word) ? tin_compiler::Token::KEYWORD : tin_compiler::Token::IDENTIFIER;
                tokens.push_back(Token(type, word));
            }
            else if (isdigit(current_char))
            {
                std::string number = readNumber();
                tokens.push_back(Token(tin_compiler::Token::IDENTIFIER, number));
            }
            else if (isspace(current_char))
            {
                ++position;
            }
            else if (isStringSymbol(current_char))
            {
                isOpenString = true;
                openString = current_char;
                tokens.push_back(Token(tin_compiler::Token::IDENTIFIER, ""));
                ++position;
            }
            else if (isSymbol(current_char))
            {
                std::string symbol(1, current_char);
                tokens.push_back(Token(tin_compiler::Token::SYMBOL, symbol));
                ++position;
            }
            else
            {
                std::cerr << "Unexpected character: " << current_char << std::endl;
                ++position;
            }
        }
        if (!tokens.empty() && tokens.back().value != "$")
        {
            tokens.push_back(Token(Token::SYMBOL, "$"));
        }
        return tokens;
    }

    std::string LRLexer::readWord()
    {
        size_t start = position;
        while (position < input.length() && isalnum(input[position]))
        {
            ++position;
        }
        return input.substr(start, position - start);
    }

    std::string LRLexer::readNumber()
    {
        size_t start = position;
        while (position < input.length() && isdigit(input[position]))
        {
            ++position;
        }
        return input.substr(start, position - start);
    }

    bool LRLexer::isStringSymbol(char c)
    {
        return std::find(stringSymbols.begin(), stringSymbols.end(), c) != stringSymbols.end();
    }

    bool LRLexer::isSymbol(char c)
    {
        return std::find(symbols.begin(), symbols.end(), c) != symbols.end();
    }

    bool LRLexer::isTerminal(const std::string &word)
    {
        return std::find(terminals.begin(), terminals.end(), word) != terminals.end();
    }
}