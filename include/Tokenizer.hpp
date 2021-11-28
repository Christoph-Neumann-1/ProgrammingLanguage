#pragma once
#include <Tokens.hpp>
#include <vector>
#include <File.hpp>
#include <Logger.hpp>
namespace VWA
{
    std::vector<Token> Tokenizer(File, ILogger &);

}