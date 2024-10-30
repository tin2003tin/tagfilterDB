#include "handler.hpp"

namespace tin_compiler
{
    void HandlerControl::execute()
    {
        outputNode(root);
    }

    std::vector<std::string> HandlerControl::outputNode(std::shared_ptr<ASTNode> node)
    {
        if (node == nullptr)
        {
            return {};
        }
        if (std::shared_ptr<ValueASTNode> van = std::dynamic_pointer_cast<ValueASTNode>(node))
        {
            return {van.get()->value};
        }
        if (std::shared_ptr<InterASTNode> ian = std::dynamic_pointer_cast<InterASTNode>(node))
        {
            std::vector<std::string> input;
            std::vector<std::string> output;
            for (auto &child : ian.get()->children)
            {
                auto outputChild = outputNode(child);

                for (auto &o : outputChild)
                {
                    input.push_back(o);
                }
            }

            // std::cout << "## " << ian.get()->handler << std::endl;
            if (ian.get()->handler != "" && mappingHandler.find(ian.get()->handler) != mappingHandler.end())
            {
                auto a = mappingHandler[ian.get()->handler];
                (handler.get()->*a)(input, output);
            }
            return output;
        }
        return {};
    }
}
