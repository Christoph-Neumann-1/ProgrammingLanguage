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
        ParseTreeNode(const Token &value_) : value(value_) {}
        ParseTreeNode(const Token &value_, const std::vector<ParseTreeNode> &children_) : value(value_), children(children_) {}
        ParseTreeNode(const Token &value_, std::vector<ParseTreeNode> &&children_) : value(value_), children(std::move(children_)) {}
        ParseTreeNode(Token &&value_, std::vector<ParseTreeNode> &&children_) : value(std::move(value_)), children(std::move(children_)) {}
        ParseTreeNode(Token &&value_, const std::vector<ParseTreeNode> &children_) : value(std::move(value_)), children(children_) {}
        ParseTreeNode(Token &&value_) : value(std::move(value_)) {}
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
