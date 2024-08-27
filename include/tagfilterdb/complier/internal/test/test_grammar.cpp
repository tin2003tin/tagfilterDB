#include "../grammar.hpp"
#include "../../lib/unique.hpp"
#include <iostream>
#include <sstream>
#include <cassert>

void test_grammarInit_1()
{
    std::string grammarText = R"(
        S -> A B
        A -> a
        A -> ε
        B -> b
    )";

    tin_compiler::Grammar grammar(grammarText);

    // Check the number of rules
    assert(grammar.rules.size() == 4);

    // Check the non-terminals
    std::vector<std::string> expectNonTerminal = {"S", "A", "B"};

    for (auto &exp : expectNonTerminal)
    {
        assert(tin_compiler::Unique::isElement(exp, grammar.nonTerminals));
    }

    // Check the terminals
    std::vector<std::string> expectTerminal = {"a", "b"};
    for (auto &exp : expectTerminal)
    {
        assert(tin_compiler::Unique::isElement(exp, grammar.terminals));
    }

    // Check the alphabet
    std::vector<std::string> expectAlphabet = {"S", "A", "B", "a", "b"};
    for (auto &exp : expectAlphabet)
    {
        assert(tin_compiler::Unique::isElement(exp, grammar.alphabet));
    }

    // Check Firsts
    std::unordered_map<std::string, std::vector<std::string>> expectFirsts;
    expectFirsts["S"] = {"a", "ε", "b"};
    expectFirsts["A"] = {"a", "ε"};
    expectFirsts["B"] = {"b"};
    for (auto &exp : expectFirsts)
    {
        auto g = grammar.firsts;
        assert(g[exp.first].size() == exp.second.size());
        for (auto &e : exp.second)
        {
            assert(std::find(g[exp.first].begin(), g[exp.first].end(), e) != g[exp.first].end());
        }
    }

    // Check Follows
    std::unordered_map<std::string, std::vector<std::string>> expectFollows;
    expectFollows["S"] = {"$"};
    expectFollows["A"] = {"b"};
    expectFollows["B"] = {"$"};
    for (auto &exp : expectFollows)
    {
        auto g = grammar.follows;
        assert(g[exp.first].size() == exp.second.size());
        for (auto &e : exp.second)
        {
            assert(std::find(g[exp.first].begin(), g[exp.first].end(), e) != g[exp.first].end());
        }
    }

    // Check the axiom
    assert(grammar.axiom == "S");
    std::cout << "test_grammerInit_1 passed!" << std::endl;
}

void test_grammarInit_2()
{
    std::string grammarText = R"(
        S' -> S
        S -> INSERT INTO T VALUE ( V )
        S -> SELECT C FROM T W F
        W -> ε
        W -> WHERE Con
        F -> ε
        F -> FROM T ON Con
        Con -> ID = V 
        C -> ( C )
        C -> ID , C 
        C -> ID
        C -> *
        T -> ID
        V -> ( V )
        V -> ID , V
        V -> ID								    
    )";

    tin_compiler::Grammar grammar(grammarText);

    // Check the number of rules
    assert(grammar.rules.size() == 16);

    // Check the non-terminals
    std::vector<std::string> expectNonTerminal = {
        "S'",
        "S",
        "C",
        "T",
        "V",
        "W",
        "F",
        "Con",
    };
    assert(grammar.nonTerminals.size() == expectNonTerminal.size());
    for (auto &exp : expectNonTerminal)
    {
        assert(tin_compiler::Unique::isElement(exp, grammar.nonTerminals));
    }

    // Check element that is not in the non-terminals
    std::vector<std::string> expectNotInTerminal = {"SELECT", "INSERT", "FROM", "ID", "HELLO WORLD"};
    for (auto &exp : expectNotInTerminal)
    {
        assert(!tin_compiler::Unique::isElement(exp, grammar.nonTerminals));
    }

    // Check the terminals
    std::vector<std::string> expectTerminal = {"SELECT", "INSERT", "FROM", "ID", "WHERE", "ON", "=", ",", "*", "(", ")", "VALUE", "INTO"};
    assert(grammar.terminals.size() == expectTerminal.size());
    for (auto &exp : expectTerminal)
    {
        assert(tin_compiler::Unique::isElement(exp, grammar.terminals));
    }

    // Check the alphabet
    assert(grammar.alphabet.size() == (expectTerminal.size() + expectNonTerminal.size()));

    // Check Firsts
    std::unordered_map<std::string, std::vector<std::string>> expectFirsts;
    expectFirsts["S'"] = {"INSERT", "SELECT"};
    expectFirsts["S"] = {"INSERT", "SELECT"};
    expectFirsts["W"] = {"WHERE", "ε"};
    expectFirsts["F"] = {"FROM", "ε"};
    expectFirsts["Con"] = {"ID"};
    expectFirsts["C"] = {"(", "ID", "*"};
    expectFirsts["T"] = {"ID"};
    expectFirsts["V"] = {"(", "ID"};
    assert(expectFirsts.size() == grammar.firsts.size());
    for (auto &exp : expectFirsts)
    {
        auto g = grammar.firsts;
        assert(g[exp.first].size() == exp.second.size());
        for (auto &e : exp.second)
        {
            assert(std::find(g[exp.first].begin(), g[exp.first].end(), e) != g[exp.first].end());
        }
    }

    std::unordered_map<std::string, std::vector<std::string>> expectFollows;
    expectFollows["S'"] = {"$"};
    expectFollows["S"] = {"$"};
    expectFollows["W"] = {"FROM", "$"};
    expectFollows["F"] = {"$"};
    expectFollows["Con"] = {"FROM", "$"};
    expectFollows["C"] = {"FROM", ")"};
    expectFollows["T"] = {
        "VALUE",
        "WHERE",
        "FROM",
        "ON",
        "$"};
    expectFollows["V"] = {")", "$", "FROM"};
    for (auto &exp : expectFollows)
    {
        auto g = grammar.follows;
        assert(g[exp.first].size() == exp.second.size());
        for (auto &e : exp.second)
        {
            assert(std::find(g[exp.first].begin(), g[exp.first].end(), e) != g[exp.first].end());
        }
    }

    // Check the axiom
    assert(grammar.axiom == "S'");
    std::cout << "test_grammerInit_2 passed!" << std::endl;
}

void test_grammarRule_1()
{
    std::string grammarText = R"(
        S' -> S           
        S -> INSERT INTO T VALUE ( V )    
        S -> SELECT C FROM T                       
        C -> ID , C                             
        C -> ID
        C -> *                                    
        T -> ID                                    
        V -> ID , V   						      
        V -> ID								    
    )";

    tin_compiler::Grammar grammar(grammarText);

    std::vector<std::string> expectedRules = {
        "S' -> S ",
        "S -> INSERT INTO T VALUE ( V ) ",
        "S -> SELECT C FROM T ",
        "C -> ID , C ",
        "C -> ID ",
        "C -> * ",
        "T -> ID ",
        "V -> ID , V ",
        "V -> ID "};

    size_t index = 0;
    for (const auto &rule : grammar.rules)
    {
        std::stringstream buffer;
        std::streambuf *old = std::cout.rdbuf(buffer.rdbuf());
        rule.display();
        std::cout.rdbuf(old);
        std::string output = buffer.str();
        std::string expectedOutput = "Rule " + std::to_string(index) + ": " + expectedRules[index] + "\n";
        assert(output == expectedOutput);
        ++index;
    }
    std::cout << "test_grammerRule_1 passed!" << std::endl;
}

int main()
{
    try
    {
        test_grammarInit_1();
        test_grammarInit_2();
        test_grammarRule_1();
        std::cout << "All tests passed!" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
