#include "tincompiler/LRcomplier.hpp"
#include "tagfilterdb/memtable.h"

HANDLER(Builder)
{
    int dimension_;
    std::string name_;
    bool isRtree_ = false;
    std::vector<std::pair<std::string,std::string>> axis_;

    DECLARE(
        REGISTER(Builder::createTable),
        REGISTER(Builder::useRTree),
        REGISTER(Builder::loadDimension),
        REGISTER(Builder::loadAxis), );

    FUNC(createTable)
    {
       name_ = input[0];
    }

    FUNC(useRTree)
    {
       isRtree_ = true;
    }

    FUNC(loadDimension)
    {
        dimension_ = std::stoi(input[0]);
    }

    FUNC(loadAxis)
    {
        axis_.push_back({input[0],input[1]});
    }
    public :
    void Print() {
        std::cout << "Name: " << name_ << std::endl;
        std::cout << "Dimension: " << dimension_ << std::endl;
        std::cout << "IsUseRTree: " << isRtree_ << std::endl;
        for (int i = 0 ; i < axis_.size(); i++) {
            std::cout << "D" + std::to_string(i + 1) << ": [" << axis_[i].first << ", "
            << axis_[i].second << "]" << std::endl;
        }
    }
};

void Complie(std::string input, Builder& builder) {
    std::string grammarRule = R"(
        S' -> #CreateTable                                 
        #CreateTable -> Create #ID #Use         ## Builder::createTable
        #Use -> ε
        #Use -> Use #RTree #Use                              
        #RTree -> RTree ( #RTreeDetails )       ## Builder::useRTree  
        #RTreeDetails -> #Dimension , #Axis             
        #Dimension -> #ID                       ## Builder::loadDimension
        #Axis -> ε
        #Axis -> [ #ID , #ID ]                  ## Builder::loadAxis
        #Axis -> [ #ID , #ID ] , #Axis          ## Builder::loadAxis 
    )";

    tin_compiler::LRCompiler compiler(grammarRule, std::make_shared<Builder>(builder));

    auto tokens = compiler.getLexer().setInput(input).tokenize();
    tin_compiler::Token::display(tokens);
    compiler.getParser().setLog(true);
    try
    {
        auto ast = compiler.getParser().setTokens(tokens).parse();
        tin_compiler::ASTNode::display(ast);
        compiler.getHandlerControl().setAST(ast).execute();
    }
    catch (const std::exception &error)
    {
        std::cerr << "Parsing error: " << error.what() << std::endl;
    }
}

int main()
{
    Builder builder;
    std::string schema = "Create Location Use RTree(2,[x_min,x_max],[y_min,y_max])";
    Complie(schema,builder);
    builder.Print();
}
