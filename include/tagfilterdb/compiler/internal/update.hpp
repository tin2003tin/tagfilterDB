#ifndef COMPILER_UPDATE_HPP
#define COMPILER_UPDATE_HPP

#include "grammar.hpp"
#include "kernel.hpp"
#include <vector>

namespace tin_compiler
{
    class KernelGraph
    {
    private:
        Grammar *grammar;

        void initialize();
        void updateClosure(int kernelIndex);
        bool addGotos(int kernelIndex);

    public:
        std::vector<Kernel> kernels;
        KernelGraph(Grammar &g);
        void display();
    };
}

#include "update.cpp"

#endif // COMPILER_UPDATE_HPP
