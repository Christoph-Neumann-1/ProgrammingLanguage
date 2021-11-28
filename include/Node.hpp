#pragma once
#include <vector>
#include <Tokens.hpp>
namespace VWA
{
    //I will try to store just the token and handle precedence
    struct ParseTreeNode
    {
        Token value;
        std::vector<ParseTreeNode> children;
        ParseTreeNode() : value(Token{.type = TokenType::null}) {}
        ParseTreeNode(const Token &value) : value(value) {}
        ParseTreeNode(const Token &value, const std::vector<ParseTreeNode> &children) : value(value), children(children) {}
        ParseTreeNode(const Token &value, std::vector<ParseTreeNode> &&children) : value(value), children(std::move(children)) {}
        ParseTreeNode(Token &&value, std::vector<ParseTreeNode> &&children) : value(std::move(value)), children(std::move(children)) {}
        ParseTreeNode(Token &&value, const std::vector<ParseTreeNode> &children) : value(std::move(value)), children(children) {}
        ParseTreeNode(Token &&value) : value(std::move(value)) {}
        //TODO: do I need this, or can the compiler do it for me?
        ParseTreeNode(const ParseTreeNode &other) : value(other.value), children(other.children) {}
        ParseTreeNode(ParseTreeNode &&other) : value(std::move(other.value)), children(std::move(other.children)) {}
        ParseTreeNode &operator=(const ParseTreeNode &other)
        {
            value = other.value;
            children = other.children;
            return *this;
        }
        ParseTreeNode &operator=(ParseTreeNode &&other)
        {
            value = std::move(other.value);
            children = std::move(other.children);
            return *this;
        }
    };
}
