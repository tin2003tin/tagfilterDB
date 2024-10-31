#pragma once

#include "tagfilterdb/compiler/LRcomplier.hpp"

using namespace tin_compiler;

const std::string compilerText = R"(
        S' -> S                            ## start
        S -> INSERT INTO T VALUE V         ## insert
        S -> SELECT C FROM T F W           ## select
        W -> ε
        W -> WHERE Con                     ## where
    F -> ε
        F -> JOIN T ON Con                 ## join
        Con -> #ID = V                     ## condition
        C -> ( C )                         ## bucket
        C -> #ID , C                       ## expandColumn
        C -> #ID                           ## setColumn
        C -> *                             ## setAllColumn
        T -> #ID                           ## setTable
        V -> ( V )                         ## bucket
        V -> #ID , V                       ## expandValue 
        V -> #ID						   ## setValue
    )";

namespace service::banking {
    class BankingCompiler {
        LRCompiler compiler;
        public :
        BankingCompiler() :  compiler(compilerText) {}
        nlohmann::json toJson() {
            return compiler.toJson();
        }

        void details() {
            compiler.details();
        }

        void setInput(std::string input) {
            compiler.getLexer().setInput(input);
        }  

        std::vector<Token> Tokenize() {
            return compiler.getLexer().tokenize();
        }

        std::shared_ptr<ASTNode> Parse(std::vector<Token> tokens) {
            try
            {
                auto ast = compiler.getParser().setTokens(tokens).parse();
                return ast;
            }
            catch(const std::exception& e)
            {
              std::cerr << "Parsing error: " << e.what() << std::endl;
            }
          return nullptr;
        }
    };
}
