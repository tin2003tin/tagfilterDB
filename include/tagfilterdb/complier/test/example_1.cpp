#include "../LRcomplier.hpp"

int main()
{
    std::string compilerText = R"(
        S' -> S                            ## start
        S -> INSERT INTO T VALUE V         ## insert
        S -> SELECT C FROM T W F           ## select
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
    tin_compiler::LRCompiler compiler(compilerText);
    compiler.details();
    std::string input1 = "SELECT (id,name,email) FROM user$";
    std::string input2 = "SELECT * FROM user WHERE id =5 $";
    std::string input3 = "SELECT id, name FROM employee JOIN salary ON id = employeId $";
    std::string input4 = "SELECT ((   id,name,email)) FROM user WHERE salary= 10000 JOIN salary ON id =employeId   $";
    auto tokens1 = compiler.getLexer().setInput(input1).tokenize();
    auto tokens2 = compiler.getLexer().setInput(input2).tokenize();
    auto tokens3 = compiler.getLexer().setInput(input3).tokenize();
    auto tokens4 = compiler.getLexer().setInput(input4).tokenize();
    tin_compiler::Token::display(tokens4);
    try
    {
        compiler.getParser().setTokens(tokens1).parse();
        std::cout << "passed 1" << std::endl;
        compiler.getParser().setTokens(tokens2).parse();
        std::cout << "passed 2" << std::endl;
        compiler.getParser().setTokens(tokens3).parse();
        std::cout << "passed 3" << std::endl;
        auto ast = compiler.getParser().setTokens(tokens4).parse();
        tin_compiler::ASTNode::display(ast);
        std::cout << "passed 4" << std::endl;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Parsing error: " << error.what() << std::endl;
    }
    return 0;
}