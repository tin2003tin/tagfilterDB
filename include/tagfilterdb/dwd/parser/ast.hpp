#ifndef COMPILER_AST_HPP
#define COMPILER_AST_HPP

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include "stackable.hpp"

namespace tin_compiler
{
    static const std::string IDENTITIER = "#ID";

    class ASTNode : public Stackable
    {
    private:
        static void printNode(std::shared_ptr<ASTNode> node, int num, bool isPrint, bool isHandler);

    public:
        virtual ~ASTNode() = default;
        static void display(std::shared_ptr<ASTNode> root, bool isHandler = false)
        {
            const std::string yellow = "\033[33m";
            const std::string reset = "\033[0m";
            std::cout << yellow + "==AbstractTree==" + reset;
            printNode(root, 0, true, isHandler);
            std::cout << std::endl;
        }
    };

    class InterASTNode : public ASTNode
    {
    public:
        std::string type;
        std::vector<std::shared_ptr<ASTNode>> children;
        std::string handler;

        InterASTNode(std::string type) : type(type) {}
        void addChild(std::shared_ptr<ASTNode> child)
        {
            children.push_back(child);
        }
    };

    class ValueASTNode : public ASTNode
    {
    public:
        std::string value;
        ValueASTNode(std::string value)
            : value(value) {}
    };
}

#include "ast.cpp"

#endif