#include "ast.hpp"

namespace tin_compiler
{
    void ASTNode::printNode(std::shared_ptr<ASTNode> node, int num, bool isPrint, bool isHandler)
    {
        if (node == nullptr)
        {
            return;
        }
        if (std::shared_ptr<ValueASTNode> van = std::dynamic_pointer_cast<ValueASTNode>(node))
        {
            if (isPrint)
            {
                std::cout << van.get()->value << " ";
            }
        }
        if (std::shared_ptr<InterASTNode> ian = std::dynamic_pointer_cast<InterASTNode>(node))
        {
            if (isPrint)
            {
                std::cout << std::endl;
                for (int i = 0; i < num; i++)
                {
                    std::cout << "  ";
                }
                if (num != 0)
                {
                    std::cout << "-";
                }

                if (ian.get()->type[0] == '#')
                {
                    std::cout << ian.get()->type.substr(1) << " ";
                }
                else
                {
                    std::cout << ian.get()->type << " ";
                }
                if (isHandler)
                {
                    std::cout << "(" << ian.get()->handler << ") ";
                }
            }
            std::reverse(ian.get()->children.begin(), ian.get()->children.end());
            for (auto &n : ian.get()->children)
            {
                if (std::shared_ptr<ValueASTNode> vcn = std::dynamic_pointer_cast<ValueASTNode>(n))
                {
                    printNode(vcn, num + 1, true, isHandler);
                }
                if (std::shared_ptr<InterASTNode> icn = std::dynamic_pointer_cast<InterASTNode>(n))
                {
                    if (icn.get()->type == ian.get()->type)
                    {
                        printNode(icn, num, false, isHandler);
                    }
                    else
                    {
                        printNode(icn, num + 1, true, isHandler);
                    }
                }
            }
        }
    }
}