#pragma once
#include <vector>
#include <Tokens.hpp>
namespace VWA
{
    //I will try to store just the token and handle precedence
    struct ASTNode
    {
        Token value;
        std::vector<ASTNode> children;
    };
}
