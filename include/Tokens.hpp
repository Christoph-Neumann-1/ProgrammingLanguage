#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <unordered_map>

namespace VWA
{
    //TODO: reorder
    enum class TokenType
    {
        null,
        array,
        compound, //TODO: use for paramaters
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
        range_operator,

        //arithmetic
        plus,
        minus,
        star,
        divide,
        mod,
        ampersand,
        power,
        //logical
        //Only symbols? or words like and, or, not?
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
        for_,
        break_,
        continue_,
        //literals
        int_,
        long_,
        bool_,
        float_,
        double_,
        char_,
        string_literal, //Comverto to array of chars? example: "abc"=>['a','b','c']||{'a','b','c'} what seperator?
        //function is just the keyword, identifier is used for everything else like type name, variables structs etc.
        function_definition,
        function_call,
        struct_definition,
        identifier,
        variable_declaration,
        export_,
        import_,
        //other keywords
        constexpr_,
        tailrec,
        mutable_,
        return_,
        arrow_operator,
        size_of, //TODO:
        new_,
        delete_,
        eof, //Could be removed but it makes the parser easier
    };
    //TODO: replace with better data structure
    const std::unordered_map<std::string, TokenType> keywords{
        {"if", TokenType::if_},
        {"else", TokenType::else_},
        {"while", TokenType::while_},
        {"for", TokenType::for_},
        {"return", TokenType::return_},
        {"func", TokenType::function_definition},
        {"struct", TokenType::struct_definition},
        {"let", TokenType::variable_declaration},
        {"mut", TokenType::mutable_},
        {"sizeof", TokenType::size_of},
        {"true", TokenType::bool_},
        {"false", TokenType::bool_},
        {"break", TokenType::break_},
        {"continue", TokenType::continue_},
        {"new", TokenType::new_},
        {"delete", TokenType::delete_},
        {"export", TokenType::export_},
        {"constexpr", TokenType::constexpr_},
        {"tailrec", TokenType::tailrec},
        {"import", TokenType::import_},
    };
    struct Token
    {
        TokenType type;
        std::variant<
            std::monostate,
            int32_t,
            bool,
            float,
            double,
            char,
            int64_t,
            std::string>
            value;
        //For debugging
        //Maybe use a reference to the line instead?
        //Use optional?
        std::shared_ptr<const std::string> file;
        int line = -1;
        //TODO: use the same map used to find the tokens in the first place. For values simply use std::to_string(value).
        std::string toString() const;
    };
}