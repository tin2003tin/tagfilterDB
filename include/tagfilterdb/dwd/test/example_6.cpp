#include "../LRcomplier.hpp"

int main()
{
    std::string grammarRule = R"(
        S' -> #SQL
        #SQL -> #SELECT ; #SQL
        #SQL -> #INSERT ; #SQL
        #SQL -> ε
        #SELECT -> SELECT #Colunm FROM #Table
        #INSERT -> INSERT INTO #Table VALUES #Value
        #Colunm -> ( #Colunm )
        #Colunm -> #ID , #Colunm
        #Colunm -> #ID
        #Colunm -> #AllColunm
        #AllColunm -> *
        #Value -> ( #Value )
        #Value -> #ID , #Value
        #Value -> #ID                           
        #Table -> #ID
    )";
    tin_compiler::LRCompiler compiler(grammarRule);
    compiler.details();
    std::string input = R"(
        SELECT (((id,fullname,nickname,age,email))) FROM employee;
        SELECT * FROM salary;
        INSERT INTO employee VALUES (5,"Siriwid Thongon","Tin",20,"tinsiriwid@gmail.com");
    )";
    auto tokens = compiler.getLexer().setInput(input).tokenize();
    tin_compiler::Token::display(tokens);
    compiler.getParser().setLog(true);
    try
    {
        auto ast = compiler.getParser().setTokens(tokens).parse();
        tin_compiler::ASTNode::display(ast);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
}