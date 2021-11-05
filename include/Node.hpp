#pragma once
#include <vector>
#include <Tokens.hpp>
namespace VWA
{
    //I will try to store just the token and handle precedence
    struct ASTNode
    {
        //TODO: try removing this enum and just using the token for all data
        // enum class Type
        // {
        //     Root,
        //     Function,
        //     Struct,
        //     FunctionCall,
        //     Value,//primitive values or initializer for struct
        //     Identifier,//Variables, types, and names nested within other nodes
        //     Declaration,//Create a new variable
        //     Operator,//Type is stored as token
        //     ControlFlow,//All branches, loops, and return/break/continue
        //     Compound,//Multiple statements enclosed in {}It may be used everywhere a statement is allowed
        // };
        // Type type;
        Token value;
        std::vector<ASTNode> children;
    };
}
