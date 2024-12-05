#pragma once

#include "../LRcomplier.hpp"

HANDLER(Handler)
{
    DECLARE(
        REGISTER(Handler::setIdentifier),
        REGISTER(Handler::setLiteral),
        REGISTER(Handler::setMember),
        REGISTER(Handler::assignment1),
        REGISTER(Handler::assignment2),
        REGISTER(Handler::assignment3), );

    FUNC(setIdentifier)
    {
        output.push_back(input[0]);
    }

    FUNC(setLiteral)
    {
        output.push_back(input[0]);
    }

    FUNC(setMember)
    {
        output.push_back(input[0]);
    }

    FUNC(assignment1)
    {
        std::cout << "a1 ";
        for (auto &a : input)
        {
            std::cout << a << " ";
        }
        std::cout << std::endl;
    }

    FUNC(assignment2)
    {
        std::cout << "a2 ";
        for (auto &a : input)
        {
            std::cout << a << " ";
        }
        std::cout << std::endl;
    }

    FUNC(assignment3)
    {
        std::cout << "a3 ";
        for (auto &a : input)
        {
            std::cout << a << " ";
        }
        std::cout << std::endl;
    }
};

int compiler_example_2()
{
    std::string grammarRule = R"(
        S' -> #Body                                 
        #Body -> #Assignment ; #Body
        #Body -> ε
        #Assignment -> #Identifier                  ## Handler::assignment1
        #Assignment -> #Identifier = #Literal       ## Handler::assignment2
        #Assignment -> #MemberAccess = #Literal     ## Handler::assignment3
        #MemberAccess -> #Identifier . #Member      
        #Member -> #ID                              ## Handler::setMember
        #Identifier -> #ID                          ## Handler::setIdentifier
        #Literal -> #ID                             ## Handler::setLiteral
    )";

    tin_compiler::LRCompiler compiler(grammarRule, std::make_shared<Handler>());

    std::string input = R"(
    name = tin ;
    id = 5 ;
    person.name = 4 ;
    email;
     $)";
    auto tokens = compiler.getLexer().setInput(input).tokenize();
    tin_compiler::Token::display(tokens);
    compiler.getParser().setLog(true);
    try
    {
        auto ast = compiler.getParser().setTokens(tokens).parse();
        compiler.getHandlerControl().setAST(ast).execute();
    }
    catch (const std::exception &error)
    {
        std::cerr << "Parsing error: " << error.what() << std::endl;
    }
}
