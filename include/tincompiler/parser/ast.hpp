#ifndef COMPILER_AST_HPP
#define COMPILER_AST_HPP

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include "stackable.hpp"
#include "json.hpp"

using json = nlohmann::json;

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

        static json toJson(std::shared_ptr<tin_compiler::ASTNode> root);
        virtual json toJson() const { 
        return json();
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
        json toJson() const override {
            json j;
            j["type"] = type;             // Add node type
            j["handler"] = handler;       // Add handler if applicable
            j["children"] = json::array(); // Prepare to hold children JSON

            for (const auto& child : children) {
                j["children"].push_back(child->toJson()); // Recursively add child nodes
            }

            return j;
        }
    };

    class ValueASTNode : public ASTNode
    {
    public:
        std::string value;
        ValueASTNode(std::string value)
            : value(value) {}
        json toJson() const override {
            json j;
            j["type"] = "ValueASTNode"; // Indicate node type
            j["value"] = value;          // Add value
            return j;
        }
    };
}

#include "ast.cpp"

#endif