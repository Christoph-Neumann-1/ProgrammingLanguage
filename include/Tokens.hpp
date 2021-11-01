#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <variant>

namespace VWA
{
    //TODO: reorder
    enum class TokenType
    {
        //brackets
        lparen,
        rparen,
        lbrace,
        rbrace,
        lbracket,
        rbracket,
        //separators
        semicolon,
        colon,
        double_colon,
        comma,
        dot,

        //arithmetic
        plus,
        minus,
        star,
        divide,
        mod,
        ampersand,
        //logical
        and_,
        or_,
        not_,
        //comparison
        eq,
        neq,
        lt,
        gt,
        leq,
        geq,
        // =
        assign,
        //Branches
        if_,
        else_,
        while_,
        //literals
        int_,
        long_,
        bool_,
        float_,
        double_,
        char_,
        string_literal, //Comverto to array of chars?
        //function is just the keyword, identifier is used for everything else like type name, variables structs etc.
        function_definition,
        struct_definition,
        identifier,
        variable_definition,
        //other keywords
        mutable_,
        return_,
        arrow_operator,
        size_of,
    };
    struct Token
    {
        TokenType type;
        std::variant<
            int32_t,
            bool,
            float,
            double,
            uint8_t,
            int64_t,
            std::string,
            >
            value;
        //For debugging
        //Maybe use a reference to the line instead?
        std::weak_ptr<std::string> file;
        uint32_t line;
        //TODO: use the same map used to find the tokens in the first place. For values simply use std::to_string(value).
        std::string toString() const;
    };
}