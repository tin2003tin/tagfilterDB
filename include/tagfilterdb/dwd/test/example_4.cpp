#include "../LRcomplier.hpp"

int main()
{
    std::string grammarRule = R"(
        S' -> #Section                                          ## start
        #Section -> #SectionName ( #ProductList ) #Section      ## section
        #Section -> ε                               
        #SectionName -> #ID                                     ## setSectionName 
        #ProductList -> #Product , #ProductList                 ## addProduct
        #ProductList -> #Product                                ## setProduct   
        #ProductList -> ε                               
        #Product -> #Name #Amount #Price                        ## product
        #Name -> #ID                                            ## setName
        #Amount ->  [ #ID ]                                     ## setAmount
        #Amount ->  ε                                           
        #Price -> = #ID                                         ## setPrice
        #Price -> ε
    )";
    tin_compiler::LRCompiler compiler(grammarRule);
    compiler.details(true, true, true);
    std::string input = R"(
                        drink(
                            milk = 10,
                            botteOfWater[1] , 
                            )
                        food(
                            salad[10] = 500,
                            apple,
                            firedRice = 20, 
                        )
                         )";
    auto tokens = compiler.getLexer().setInput(input).tokenize();
    tin_compiler::Token::display(tokens);
    compiler.getParser().setLog(true);
    try
    {
        auto ast = compiler.getParser().setTokens(tokens).parse();
        tin_compiler::ASTNode::display(ast, false);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
}