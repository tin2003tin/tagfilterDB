#include "kernel.hpp"

namespace tin_compiler
{
    Kernel::Kernel(int index)
        : index(index)
    {
    }
    Kernel::Kernel(int index, std::unordered_set<Item> items)
        : index(index), items(items), closure({items})
    {
    }

    Kernel::~Kernel()
    {
    }

    bool Kernel::operator==(const Kernel &that) const
    {
        if (items.size() != that.items.size())
        {
            return false;
        }
        for (auto &item : items)
        {
            if (!Unique::isElement(item, that.items))
            {
                return false;
            }
        }
       
        return true;
    }

    void Kernel::display()
    {
        std::cout << "(Kernel: " << index << ")\n";
        for (auto item : items)
        {
            item.display();
        }

        std::cout << "Closure: " << std::endl;
        for (auto closer : closure)
        {
            closer.display();
        }
        std::cout << "Goto: " << std::endl;
        for (auto go : gotos)
        {
            std::cout << go.first << ": " << go.second << std::endl;
        }
    }
}
