#include "../LRcomplier.hpp"

int main()
{
    std::string compilerText = R"(
        S' -> #Program                                

        #Program -> #IncludeDirective #Program
        #Program -> #StructDeclaration #Program
        #Program -> #FunctionDeclaration #Program
        #Program -> ε

        #IncludeDirective -> # include < #ID >
        #StructDeclaration -> struct #StructName { #StructMembers }
        #StructName -> #ID
        #StructMembers -> #MemberDeclaration ; #StructMembers
        #StructMembers -> ε
        #MemberDeclaration->#Type #Identifier


        #FunctionDeclaration -> func #FunctionName ( #Parameters ) #ReturnType { #Body }  
        #ReturnType -> #Type
        #FunctionName -> #ID
        #Parameters -> #Paramemter , #Parameters
        #Parameters -> #Paramemter
        #Parameters -> ε
        #Paramemter -> #Type #Identifier 
        #Body -> #VariableDeclaration ; #Body
        #Body -> #Assignment ; #Body
        #Body -> #FunctionCall ; #Body
        #Body -> #ReturnStatement ; 
        #Body -> ε
        #VariableDeclaration -> #Type #Identifier
        #VariableDeclaration -> #Type #Identifier = #Literal
        #Assignment -> #Identifier = #Literal
        #Assignment -> #MemberAccess = #Literal
        #MemberAccess -> #Identifier . #Member
        #Member -> #ID
        #Assignment -> ε   
        #FunctionCall -> ε
        #ReturnStatement -> return #Literal
      
        #Type -> #ID #ArraySize
        #ArraySize -> [ #ID ]
        #ArraySize -> ε
        #Identifier -> #ID
        #Literal -> #ID
    )";

    tin_compiler::LRCompiler compiler(compilerText);
    // compiler.details();
    std::string input = R"(
    #include <math> 
    #include <iostream>

    struct Employee
    {
        int id;
        char[50] name;
        string email;
    }

    struct Salary
    {
        int id; 
        int employeeId;
        int salary;
    } 

    func sum(int x, int y) int {
        return sum ;
    }

    func main() int {
        User user;
        user.name = tin;
        int x;
        int y = 10;
        x = 5;
        return 0;
    }
     $)";
    auto tokens = compiler.getLexer().setInput(input).tokenize();
    tin_compiler::Token::display(tokens);
    compiler.getParser().setLog(true);
    try
    {
        auto ast = compiler.getParser().setTokens(tokens).parse();
        tin_compiler::ASTNode::display(ast);
    }
    catch (const std::exception &error)
    {
        std::cerr << "Parsing error: " << error.what() << std::endl;
    }
    return 0;
}