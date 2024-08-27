#ifndef COMPILER_HANDLER_CONTROL_HPP
#define COMPILER_HANDLER_CONTROL_HPP

#include "handler.hpp"
#include "../parser/ast.hpp"
#include <vector>
#include <memory>

class NullHandler : public CompilerHandler
{
public:
    NullHandler() = default;

    std::unordered_map<std::string, HandlerMethod> getFunctionNames() override
    {
        return {};
    }
};

namespace tin_compiler
{
    class HandlerControl
    {
    private:
        std::vector<std::string> outputNode(std::shared_ptr<ASTNode> node);

    public:
        std::shared_ptr<CompilerHandler> handler;
        std::unordered_map<std::string, HandlerMethod> mappingHandler;
        std::shared_ptr<ASTNode> root;
        HandlerControl(std::shared_ptr<CompilerHandler> handler) : handler(handler)
        {
            mappingHandler = handler->getFunctionNames();
        }
        HandlerControl &setAST(std::shared_ptr<ASTNode> root)
        {
            this->root = root;
            return *this;
        }
        void detail()
        {
            ASTNode::display(root, true);
        }

        void execute();
    };
}

#include "handlerControl.cpp"

#endif