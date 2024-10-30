#include "update.hpp"
#include <queue>

namespace tin_compiler
{

    KernelGraph::KernelGraph(Grammar &g) : grammar(&g)
    {
        initialize();
    }
    void KernelGraph::initialize()
    {
        int kernelIndex = 0;
        Kernel startKernel(0, {Item(&grammar->rules[0], 0)});
        kernels.push_back(startKernel);

        while (kernelIndex < kernels.size())
        {
            updateClosure(kernelIndex);
            if (!addGotos(kernelIndex))
            {
                kernelIndex++;
            }
        }
    }
    void KernelGraph::updateClosure(int kernelIndex)
    {
        for (auto item : kernels[kernelIndex].closure)
        {
            auto newItemsFromSymbolAfterDot = item.newItemsFromSymbolAfterDot(grammar);
            for (auto newItem : newItemsFromSymbolAfterDot)
            {
                Unique::addUnique(newItem, kernels[kernelIndex].closure);
            }
        }
    }
    bool KernelGraph::addGotos(int kernelIndex)
    {
        bool lookAheadsPropagated = false;
        std::unordered_map<std::string, std::unordered_set<Item>> mappingKeyItems;
        for (auto item : kernels[kernelIndex].closure)
        {
            auto shiftedItems = item.newItemAfterShift();
            for (auto &shiftedItem : shiftedItems)
            {
                auto symbolAfterDot = item.rule->development[item.dotIndex];
                Unique::addUnique(symbolAfterDot, kernels[kernelIndex].keys);
                Unique::appendMap(symbolAfterDot, shiftedItem, mappingKeyItems);
            }
        }
        std::vector<std::string> keys = kernels[kernelIndex].keys;
        for (auto key : keys)
        {
            Kernel newKernel(kernels.size(), {mappingKeyItems[key]});
            int targetKernelIndex = -1;

            for (int index = 0; index < kernels.size(); index++)
            {
                if (kernels[index] == newKernel)
                {
                    targetKernelIndex = index;
                    break;
                }
            }

            if (targetKernelIndex < 0)
            {
                targetKernelIndex = newKernel.index;
                kernels.push_back(newKernel);
            }
            else
            {
                for (auto &item : newKernel.items)
                {
                    lookAheadsPropagated = Unique::addUnique(item, kernels[targetKernelIndex].items);
                }
            }
            kernels[kernelIndex].gotos[key] = targetKernelIndex;
        }
        return lookAheadsPropagated;
    }
    void KernelGraph::display()
    {
        for (auto &kernel : kernels)
        {
            kernel.display();
            printf("\n");
        }
    }
}