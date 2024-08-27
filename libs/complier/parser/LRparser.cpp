#include "LRparser.hpp"
#include <stdexcept>
#include <iostream>

namespace tin_compiler
{
    Parser &LRParser::setTokens(std::vector<Token> &tokens)
    {
        this->tokens = &tokens;
        return *this;
    }
    std::shared_ptr<ASTNode> LRParser::parse()
    {
        currIndex = 0;
        stack.push(std::make_shared<Goto>(0));
        if (tokens->empty())
        {
            throw std::runtime_error("Error: Token list is empty.");
        }
        if (isLog)
        {
            const std::string yellow = "\033[33m";
            const std::string reset = "\033[0m";
            std::cout << yellow + "==Parsing==" + reset << std::endl;
        }
        while (!stack.empty())
        {
            if (currIndex > tokens->size())
            {
                break;
            }
            if (isLog)
            {
                printStack();
            }

            int targetState = -1;
            Stackable *top = stack.top().get();
            if (Goto *gotoPtr = dynamic_cast<Goto *>(top))
            {
                targetState = gotoPtr->state;
            }

            if (targetState == -1)
            {
                throw std::runtime_error("Error: connot find goto.");
            }
            Token::Type tokenType = tokens[0][currIndex].type;
            std::string tokenKey;
            switch (tokenType)
            {
            case Token::IDENTIFIER:
                tokenKey = IDENTITIER;
                break;
            case Token::SYMBOL:
                tokenKey = tokens[0][currIndex].value;
                break;
            case Token::KEYWORD:
                tokenKey = tokens[0][currIndex].value;
                break;
            }

            std::string action;
            if (Unique::isElement(tokenKey, strategy->states[targetState].lrAction))
            {
                action = strategy->states[targetState].lrAction[tokenKey];
            }
            else
            {
                throw std::runtime_error("Error:cannot find action");
            }
            if (isLog)
            {
                std::cout << " | " << tokenKey << ":" << action << std::endl;
            }

            switch (action[0])
            {
            case 'a':
                stack.pop();
                return std::dynamic_pointer_cast<ASTNode>(stack.top());
            case 's':
                shift(action, tokenType);
                break;
            case 'r':
                int ruleIndex = std::stoi(action.substr(1));
                std::shared_ptr<InterASTNode> node = reduce(ruleIndex);
                Rule *rule = &strategy->grammar.rules[ruleIndex];
                std::string nonTerminal = rule->nonTerminal;
                Stackable *newTop = stack.top().get();
                Goto *topPtr = dynamic_cast<Goto *>(newTop);
                int gotoState = strategy->states[topPtr->state].lrGoto[nonTerminal];
                goTo(gotoState, node);
            }
        }
        return nullptr;
    }
    void LRParser::shift(const std::string &action, Token::Type tokenType)
    {
        int nextState = std::stoi(action.substr(1));
        std::string tokenValue = tokens[0][currIndex].value;

        switch (tokenType)
        {
        case Token::IDENTIFIER:
            stack.push(std::make_shared<ValueASTNode>(tokenValue));
            break;

        default:
            stack.push(std::make_shared<Terminal>(tokenValue));
            break;
        }
        stack.push(std::make_shared<Goto>(nextState));
        currIndex++;
    }
    std::shared_ptr<InterASTNode> LRParser::reduce(int ruleIndex)
    {
        Rule *rule = &strategy->grammar.rules[ruleIndex];
        if (rule->development.size() == 1 && rule->development[0] == EPSILON)
        {
            return nullptr;
        }

        std::shared_ptr<InterASTNode> root = std::make_shared<InterASTNode>(rule->nonTerminal);
        root.get()->handler = strategy->handlerName[ruleIndex];
        for (int i = 0; i < rule->development.size() * 2; i++)
        {
            if (i % 2 == 1)
            {
                auto topElement = stack.top();
                stack.pop();

                if (std::shared_ptr<ASTNode> van = std::dynamic_pointer_cast<ASTNode>(topElement))
                {
                    root->addChild(van);
                }
            }
            else
            {
                stack.pop();
            }
        }

        return root;
    }
    void LRParser::goTo(int nextState, std::shared_ptr<InterASTNode> node)
    {
        stack.push(node);
        stack.push(std::make_shared<Goto>(nextState));
    }

    void LRParser::printStack() const
    {
        std::stack<std::shared_ptr<Stackable>> tempStack = stack;
        std::deque<std::shared_ptr<Stackable>> elements;

        while (!tempStack.empty())
        {
            elements.push_front(tempStack.top());
            tempStack.pop();
        }

        for (const auto &elem : elements)
        {
            if (auto g = dynamic_cast<Goto *>(elem.get()))
            {
                std::cout << g->state << " ";
            }
            else if (auto van = dynamic_cast<ValueASTNode *>(elem.get()))
            {
                std::cout << "ID:" << van->value << " ";
            }
            else if (auto t = dynamic_cast<Terminal *>(elem.get()))
            {
                std::cout << t->value << " ";
            }
            else if (auto van = dynamic_cast<InterASTNode *>(elem.get()))
            {
                std::cout << van->type << " ";
            }
        }
    }

}
