#include <iostream>

#include "tagfilterdb/complier/LRcomplier.hpp"
#include "tagfilterdb/rStarTree/box.hpp"

#include "gtest/gtest.h"

namespace DBTesting
{
    TEST(TEST_R_STAR_TREE, BOX)
    {
        tagfilterdb::Box<2> box1({{1, 10}, {1, 10}});
        tagfilterdb::Box<2> box2({{2, 5}, {2, 5}});

        ASSERT_EQ(box1.toString(), "[(1, 10), (1, 10)]");
        ASSERT_EQ(box2.toString(), "[(2, 5), (2, 5)]");
        ASSERT_TRUE(box1.isOverlap(box2));
        ASSERT_EQ(box1.area(), 81);
        ASSERT_EQ(box2.area(), 9);
        ASSERT_EQ(box1.overlap(box2), 9);
        tagfilterdb::Box<2> u = tagfilterdb::Box<2>::Universe();
        ASSERT_EQ(box1.overlap(u), 81);
        ASSERT_EQ(box2.overlap(u), 9);
    }

    TEST(TEST_COMPILER, EX6)
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
        // compiler.details();
        std::string input = R"(
        SELECT (((id,fullname,nickname,age,email))) FROM employee;
        SELECT * FROM salary;
        INSERT INTO employee VALUES (5,"Siriwid Thongon","Tin",20,"tinsiriwid@gmail.com");
    )";
        auto tokens = compiler.getLexer().setInput(input).tokenize();
        // tin_compiler::Token::display(tokens);
        compiler.getParser().setLog(false);
        try
        {
            auto ast = compiler.getParser().setTokens(tokens).parse();
            // tin_compiler::ASTNode::display(ast);
            ASSERT_EQ(1, 1);
        }
        catch (const std::exception &e)
        {
            ASSERT_EQ(1, 0);
        }
    }
}