#include <Tokens.hpp>
namespace VWA
{
    std::string Token::toString() const
    {
        //TODO: reverse escape sequences
        switch (type)
        {
        case TokenType::lparen:
            return "(";
        case TokenType::rparen:
            return ")";
        case TokenType::lbracket:
            return "[";
        case TokenType::rbracket:
            return "]";
        case TokenType::lbrace:
            return "{";
        case TokenType::rbrace:
            return "}";
        case TokenType::comma:
            return ",";
        case TokenType::dot:
            return ".";
        case TokenType::semicolon:
            return ";";
        case TokenType::colon:
            return ":";
        case TokenType::double_colon:
            return "::";
        case TokenType::plus:
            return "+";
        case TokenType::minus:
            return "-";
        case TokenType::star:
            return "*";
        case TokenType::divide:
            return "/";
        case TokenType::mod:
            return "%";
        case TokenType::ampersand:
            return "&";
        case TokenType::and_:
            return "&&";
        case TokenType::or_:
            return "||";
        case TokenType::not_:
            return "!";
        case TokenType::eq:
            return "==";
        case TokenType::neq:
            return "!=";
        case TokenType::lt:
            return "<";
        case TokenType::gt:
            return ">";
        case TokenType::leq:
            return "<=";
        case TokenType::geq:
            return ">=";
        case TokenType::assign:
            return "=";
        case TokenType::if_:
            return "if";
        case TokenType::else_:
            return "else";
        case TokenType::while_:
            return "while";
        case TokenType::for_:
            return "for";
        case TokenType::return_:
            return "return";
        case TokenType::break_:
            return "break";
        case TokenType::continue_:
            return "continue";
        case TokenType::struct_definition:
            return "struct";
        case TokenType::function_definition:
            return "func";
        case TokenType::variable_declaration:
            return "let";
        case TokenType::mutable_:
            return "mut";
        case TokenType::arrow_operator:
            return "->";
        case TokenType::size_of:
            return "sizeof";
        case TokenType::identifier:
            return std::get<std::string>(value);
        case TokenType::int_:
            return std::to_string(std::get<int32_t>(value));
        case TokenType::float_:
            return std::to_string(std::get<float>(value))+'f';
        case TokenType::long_:
            return std::to_string(std::get<int64_t>(value))+'l';
        case TokenType::double_:
            return std::to_string(std::get<double>(value));
        case TokenType::string_literal:
            return '"' + std::get<std::string>(value) + '"';
        case TokenType::char_:
            return std::string("'") + std::get<char>(value) + "'";
        case TokenType::bool_:
            return std::get<bool>(value) ? "true" : "false";
        }
    }
}