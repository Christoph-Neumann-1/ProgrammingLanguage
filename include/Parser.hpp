#pragma once
#include <Tokens.hpp>
#include <Node.hpp>
#include <Error.hpp>

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
    */

    [[nodiscard]] std::string TreeToString(const ParseTreeNode &node, int indent = 0);
    [[nodiscard]] ErrorOr<ParseTreeNode> generateParseTree(const std::vector<VWA::Token> &tokens);

}