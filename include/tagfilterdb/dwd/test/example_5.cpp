#include "../LRcomplier.hpp"

HANDLER(OrderHandler)
{
    DECLARE(
        REGISTER(OrderHandler::setAmount),
        REGISTER(OrderHandler::setPrice),
        REGISTER(OrderHandler::setName),
        REGISTER(OrderHandler::setFood), );

    FUNC(setAmount)
    {
        int number = std::stoi(input[0]);
        output.push_back(input[0]);
    }
    FUNC(setPrice)
    {
        int number = std::stoi(input[0]);
        output.push_back(input[0]);
    }
    FUNC(setName)
    {
        output.push_back(input[0]);
    }
    FUNC(setFood)
    {
        std::cout << "Name: " << input[0];
        std::cout << ", Amount: " << input[1];
        std::cout << ", Price: " << input[2] << std::endl;
    }
};

int main()
{
    std::string grammarRule = R"(
        S -> #Order                       ## OrderHandler::end
        #Order -> #Food and #Order        ## OrderHandler::appendOrder
        #Order -> #Food                   ## OrderHandler::setOrder
        #Food -> #Name #Amount #Price     ## OrderHandler::setFood 
        #Name -> #ID                      ## OrderHandler::setName
        #Price -> = #ID                   ## OrderHandler::setPrice
        #Amount -> [ #ID ]                ## OrderHandler::setAmount
    )";
    tin_compiler::LRCompiler complier(grammarRule, std::make_shared<OrderHandler>());
    // complier.details(true, true, true);
    std::string input = "apple[5] = 300 and salad[1] = 279 and banana[20] = 500 and pen[2] = 100";
    auto tokens = complier.getLexer().setInput(input).tokenize();
    tin_compiler::Token::display(tokens);
    complier.getParser().setLog(true);
    try
    {
        auto ast = complier.getParser().setTokens(tokens).parse();
        tin_compiler::ASTNode::display(ast);
        complier.getHandlerControl().setAST(ast).execute();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
}