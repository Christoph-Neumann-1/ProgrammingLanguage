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
        case TokenType::range_operator:
            return "..";
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
            return "ID";
        case TokenType::int_:
            return "int";
        case TokenType::float_:
            return "float";
        case TokenType::long_:
            return "long";
        case TokenType::double_:
            return "double";
        case TokenType::string_literal:
            return "string";
        case TokenType::char_:
            return "char";
        case TokenType::bool_:
            return "bool";
        case TokenType::new_:
            return "new";
        case TokenType::delete_:
            return "delete";
        case TokenType::power:
            return "**";
        case TokenType::constexpr_:
            return "constexpr_";
        case TokenType::export_:
            return "export";
        case TokenType::tailrec:
            return "tailrec";
        case TokenType::import_:
            return "import";
        default:
            return "empty";
        }
    }
}