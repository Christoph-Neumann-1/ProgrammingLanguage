#pragma once
#include <Tokens.hpp>
#include <Types.hpp>
#include <Functions.hpp>
#include <Node.hpp>

namespace VWA
{
    /*
    Multiple passes. First find and store function, struct and, if allowed, variable definitions
    Then validate all structs.
    After that begin parsing the functions. 
    This too takes multiple passes.
    First create a simple tree of all the operations.
    This tree only contains ids for variables. Functions are already defined and can be looked up.
    Then fill in the variable references and perform type checking.
    Finally perform one or more optimization passes.
    Then the code can be executed.
    TODO: figure out best way to store and pass vars.
    TODO: better representation of data.
    TODO: make use of stack.
    TODO: eliminate redundant work
    TODO: consider switching to unique ids if pointers cause problems.
    TODO: consider let x:int instead of let int x
    */

    struct pass1_result
    {
        std::unordered_map<std::string, CustomTypeInfo> structs;
        //Functions are parsed later, so their tokens need to be stored.
        //TODO: avoid copies of the tokens
        std::unordered_map<std::string, std::pair<FunctionInfo, std::vector<Token>>> functions;
    };

    pass1_result findDefinitions(std::vector<Token> tokens);

    ASTNode generateParseTree(const std::vector<VWA::Token> &tokens);

}