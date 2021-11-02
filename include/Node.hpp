#pragma once
#include <vector>
#include <Tokens.hpp>
namespace VWA
{
    //I will try to store just the token and handle precedence
    struct ASTNode
    {
        enum class Type
        {
            Program,
            Function,
            FunctionCall,
            Type,
            Value,
            Variable,
            Declaration,
            Assignment,
            BinaryOperator,
            UnaryOperator,
            Return,
            Branch, //Covers for, while, if, and else
            Break,
            Continue,
            Body,
        };
        Type type;
        std::variant<std::monostate, Token, std::tuple<VarType, CustomTypeInfo *, bool>> value;
        std::vector<ASTNode> children;
    };
}
