#include <Parser.hpp>
#include <sstream>

//Instead of trying to find the end first, just pass the current token into the function and return both a node and set the current token, passed
// by reference, to the next token.
//Struct parsing can be combined into main parser
//TODO: disallow assignment within expressions
//TODO: move inside class
//TODO: add backtrace
//TODO: catch more errors here
//TODO: replace push back with emplace_back where possible
//TODO: save line number in Nodes
namespace VWA
{
#pragma region Expression Parsing

    [[nodiscard]] ErrorOr<ParseTreeNode> parseExpression(const std::vector<Token> &tokens, size_t &start);
    [[nodiscard]] ErrorOr<ParseTreeNode> parseComparison(const std::vector<Token> &tokens, size_t &start);
    [[nodiscard]] ErrorOr<ParseTreeNode> parseAddition(const std::vector<Token> &tokens, size_t &start);
    [[nodiscard]] ErrorOr<ParseTreeNode> parseMultiplication(const std::vector<Token> &tokens, size_t &start);
    [[nodiscard]] ErrorOr<ParseTreeNode> parseUnary(const std::vector<Token> &tokens, size_t &start);
    [[nodiscard]] ErrorOr<ParseTreeNode> parsePower(const std::vector<Token> &tokens, size_t &start);
    [[nodiscard]] ErrorOr<ParseTreeNode> parsePrimary(const std::vector<Token> &tokens, size_t &start);
    [[nodiscard]] ErrorOr<ParseTreeNode> parseDot(const std::vector<Token> &tokens, size_t &start);
    [[nodiscard]] ErrorOr<ParseTreeNode> parseFuncCall(const std::vector<Token> &tokens, size_t &start);

    //TODO: This should parse the type of a variable of function return, checking for arrays and such.
    //To do so, I need to change the way I store variable types.
    [[nodiscard]] ErrorOr<ParseTreeNode> parseType(const std::vector<Token> &tokens, size_t &start)
    {
        throw std::runtime_error("Not implemented");
    }

    //TODO: move the bracket pairing in here and return the end as well
    [[nodiscard]] ErrorOr<ParseTreeNode> parseExpression(const std::vector<Token> &tokens, size_t &start)
    {
        auto node = TRY(parseComparison(tokens, start));
        while (1)
            switch (tokens[start].type)
            {
            case TokenType::and_:
            case TokenType::or_:
            {
                node = ParseTreeNode{tokens[start], {std::move(node), TRY(parseComparison(tokens, ++start))}};
                break;
            }
            default: //TODO: better way to define end of expression
                return node;
            }
    }

    [[nodiscard]] ErrorOr<ParseTreeNode> parseComparison(const std::vector<Token> &tokens, size_t &start)
    {
        auto node = TRY(parseAddition(tokens, start));
        //Should I allow chained comparisons?
        while (1)
            switch (tokens[start].type)
            {
            case TokenType::lt:
            case TokenType::gt:
            case TokenType::leq:
            case TokenType::geq:
            case TokenType::eq:
            case TokenType::neq:
            {
                node = ParseTreeNode{tokens[start], {std::move(node), TRY(parseAddition(tokens, ++start))}};
                break;
            }
            default:
                return node;
            }
    }

    [[nodiscard]] ErrorOr<ParseTreeNode> parseAddition(const std::vector<Token> &tokens, size_t &start)
    {
        auto node = TRY(parseMultiplication(tokens, start));
        while (1)
            switch (tokens[start].type)
            {
            case TokenType::plus:
            case TokenType::minus:
            {
                node = ParseTreeNode{tokens[start], {std::move(node), TRY(parseMultiplication(tokens, ++start))}};
                break;
            }
            default:
                return node;
            }
    }

    [[nodiscard]] ErrorOr<ParseTreeNode> parseMultiplication(const std::vector<Token> &tokens, size_t &start)
    {
        auto node = TRY(parseUnary(tokens, start));
        while (1)
            switch (tokens[start].type)
            {
            case TokenType::star:
            case TokenType::divide:
            case TokenType::mod:
            {
                node = ParseTreeNode{tokens[start], {std::move(node), TRY(parseUnary(tokens, ++start))}};
                break;
            }
            default:
                return node;
            }
    }

    //TODO: should the priority of this be higher or lower than parsePower?
    [[nodiscard]] ErrorOr<ParseTreeNode> parseUnary(const std::vector<Token> &tokens, size_t &start)
    {
        switch (tokens[start].type)
        {
        case TokenType::plus: //This should be ignored.
        case TokenType::minus:
        case TokenType::not_:
        {
            return ParseTreeNode{tokens[start], {TRY(parseUnary(tokens, ++start))}};
        }
        default:
            return parsePower(tokens, start);
        }
    }

    [[nodiscard]] ErrorOr<ParseTreeNode> parsePower(const std::vector<Token> &tokens, size_t &start)
    {
        auto lhs = TRY(parsePrimary(tokens, start));
        if (tokens[start].type != TokenType::power)
        {
            return lhs;
        }
        return ParseTreeNode{{TokenType::power}, {std::move(lhs), TRY(parsePower(tokens, ++start))}};
    }

    //TODO: sizeof
    [[nodiscard]] ErrorOr<ParseTreeNode> parsePrimary(const std::vector<Token> &tokens, size_t &start)
    {
        switch (tokens[start].type)
        {
        case TokenType::int_:
        case TokenType::long_:
        case TokenType::float_:
        case TokenType::double_:
        case TokenType::char_:
        case TokenType::bool_:
        {
            return {tokens[start++]};
        }
        case TokenType::lparen:
        {
            ++start;
            auto result = TRY(parseExpression(tokens, start));
            if (tokens[start++].type != TokenType::rparen)
            {
                return Error{.message = "Unexpected token, Expected ')' found " + tokens[start - 1].toString()};
            }
            return result;
        }
        case TokenType::identifier:
        {
            //An identifier can mean three things here:
            //1. A variable name
            //2. A function call
            //3. a variable name with the dot operator
            switch (tokens[start + 1].type)
            {
            //This is a function call. Inside the parentheses may be 0 or more comma seperated expressions(the arguments)
            //These arguments are stored as children of the node
            case TokenType::lparen:
            {
                return parseFuncCall(tokens, start);
            }
            case TokenType::dot:
            {
                return parseDot(tokens, start);
            }
            default:
            {
                return {tokens[start++]};
            }
            }
        default:
        {
            return Error{.message = "Unexpected token, expected identifier, function call, parentheses or value. Found" + tokens[start].toString()};
        }
        }
        }
    }

    [[nodiscard]] ErrorOr<ParseTreeNode> parseFuncCall(const std::vector<Token> &tokens, size_t &start)
    {
        ParseTreeNode node{{TokenType::function_call, tokens[start].value}};
        start += 2;
        while (tokens[start].type != TokenType::rparen)
        {
            node.children.emplace_back(TRY(parseExpression(tokens, start)));
            if (tokens[start].type != TokenType::rparen)
            {
                if (tokens[start].type != TokenType::comma)
                {
                    return Error{.message = "Unexpected token, expected ',' or ')' found " + tokens[start].toString()};
                }
                ++start;
            }
        }
        ++start;
        return node;
    }

    //Returns either a tree of dot operators or just and identifier
    [[nodiscard]] ErrorOr<ParseTreeNode> parseDot(const std::vector<Token> &tokens, size_t &start)
    {
        //I have decided on this format for the dot operator:
        //dot is the root node, the children are the identifiers in the correct order

        ParseTreeNode root{{TokenType::dot}, {{tokens[start++]}}};

        for (;;)
        {
            if (tokens[start].type == TokenType::dot)
            {
                if (tokens[++start].type != TokenType::identifier)
                {
                    return Error{.message = "Unexpected token, expected identifier. Found " + tokens[start].toString()};
                }
                root.children.emplace_back(tokens[start++]);
                continue;
            }
            return root;
        }
    }

#pragma endregion Expression Parsing

    //TODO: why does this exist?
    // ParseTreeNode parseVariableDeclaration(const std::vector<Token> &tokens, size_t &start)
    // {
    //     //Format let (mut) x:int; or with initializer let (mut) x:int = 5;
    //     if (tokens[start++].type != TokenType::variable_declaration)
    //     {
    //         throw std::runtime_error("Expected variable declaration");
    //     }
    //     ParseTreeNode node{Token{TokenType::variable_declaration}};
    //     switch (tokens[start].type)
    //     {
    //     case TokenType::mutable_:
    //         node.children.push_back(ParseTreeNode{Token{TokenType::mutable_}}); //TODO: better way to store this
    //         if (tokens[++start].type != TokenType::identifier)
    //         {
    //             throw std::runtime_error("Expected identifier");
    //         }
    //         node.children.push_back({tokens[start++]});
    //         break;
    //     case TokenType::identifier:
    //         node.children.push_back(ParseTreeNode{tokens[start++]});
    //         break;
    //     default:
    //         throw std::runtime_error("Expected identifier");
    //     }
    //     if (tokens[start++].type != TokenType::colon)
    //     {
    //         throw std::runtime_error("Expected colon after variable name");
    //     }
    //     //TODO: refactor for easy extraction into assignment statement
    //     node.children.push_back(ParseTreeNode{tokens[start++]}); //type
    //     switch (tokens[start].type)
    //     {
    //     case TokenType::semicolon:
    //         break;
    //     case TokenType::assign:
    //         node.children.push_back(parseExpression(tokens, ++start));
    //         if (tokens[start].type != TokenType::semicolon)
    //         {
    //             throw std::runtime_error("Expected semicolon");
    //         }
    //         break;
    //     default:
    //         throw std::runtime_error("Expected semicolon");
    //     }
    //     ++start;
    //     return node;
    // }

    //TODO: default values
    [[nodiscard]] ErrorOr<ParseTreeNode> parseParameterDeclaration(const std::vector<Token> &tokens, size_t &start)
    {
        ParseTreeNode node{Token{TokenType::variable_declaration}};
        switch (tokens[start].type)
        {
        case TokenType::mutable_:
            node.children.emplace_back(Token{TokenType::mutable_}); //TODO: better way to store this
            if (tokens[++start].type != TokenType::identifier)
            {
                return Error{.message = "Expected identifier found " + tokens[start].toString()};
            }
            node.children.emplace_back(tokens[start++]);
            break;
        case TokenType::identifier:
            node.children.emplace_back(tokens[start++]);
            break;
        default:
            return Error{.message = "Expected identifier found " + tokens[start].toString()};
        }
        if (tokens[start++].type != TokenType::colon)
        {
            return Error{.message = "Expected colon after variable name"};
        }
        //TODO: refactor for easy extraction into assignment statement
        node.children.emplace_back(tokens[start++]); //type
        return node;
    }
    [[nodiscard]] ErrorOr<ParseTreeNode> parseStatement(const std::vector<Token> &tokens, size_t &pos);

#pragma region Statement Parsing
    [[nodiscard]] ErrorOr<ParseTreeNode> parseStatement(const std::vector<Token> &tokens, size_t &pos);
    [[nodiscard]] ErrorOr<ParseTreeNode> parseBlock(const std::vector<Token> &tokens, size_t &pos);
    [[nodiscard]] ErrorOr<ParseTreeNode> parseIf(const std::vector<Token> &tokens, size_t &pos);
    [[nodiscard]] ErrorOr<ParseTreeNode> parseWhile(const std::vector<Token> &tokens, size_t &pos);
    [[nodiscard]] ErrorOr<ParseTreeNode> parseFor(const std::vector<Token> &tokens, size_t &pos);
    [[nodiscard]] ErrorOr<ParseTreeNode> parseDecl(const std::vector<Token> &tokens, size_t &pos);
    [[nodiscard]] ErrorOr<ParseTreeNode> parseAssign(const std::vector<Token> &tokens, size_t &pos);

    [[nodiscard]] ErrorOr<ParseTreeNode> parseStatement(const std::vector<Token> &tokens, size_t &pos)
    {
        switch (tokens[pos].type)
        {
        case TokenType::lbrace:
            return parseBlock(tokens, pos);

        case TokenType::variable_declaration:
            return parseDecl(tokens, pos);
        case TokenType::identifier:
            if (tokens[pos + 1].type == TokenType::assign)
            {
                return parseAssign(tokens, pos);
            }
            else if (tokens[pos + 1].type == TokenType::lparen)
            {
                auto node = TRY(parseFuncCall(tokens, pos));
                if (tokens[pos++].type != TokenType::semicolon)
                {
                    return Error{.message = "Expected semicolon after function call"};
                }
                return node;
            }
            else if (tokens[pos + 1].type == TokenType::dot)
            {
                auto dot = TRY(parseDot(tokens, pos));
                if (tokens[pos].type != TokenType::assign)
                {
                    return Error{.message = "Expected assignment after dot operator"};
                }
                ParseTreeNode node{{TokenType::assign}, {std::move(dot), TRY(parseExpression(tokens, pos))}};
                return node;
            }
            else
            {
                return Error{.message = "Expected assignment or parentheses after identifier"};
            }
        case TokenType::if_:
            return parseIf(tokens, pos);
        case TokenType::while_:
            return parseWhile(tokens, pos);
        case TokenType::for_:
            return parseFor(tokens, pos);
        case TokenType::semicolon:
            return ParseTreeNode{tokens[pos++]};
        case TokenType::return_:
        { //TODO: rewrite this, because evaluation order is unspecified
            ParseTreeNode node{tokens[pos++], {tokens[pos].type == TokenType::semicolon ? std::vector<ParseTreeNode>{} : std::vector<ParseTreeNode>{TRY(parseExpression(tokens, pos))}}};
            if (tokens[pos++].type != TokenType::semicolon)
            {
                return Error{.message = "Expected semicolon after return"};
            }
            return node;
        }
        case TokenType::break_:
        {
            ParseTreeNode node{tokens[pos++]};
            if (tokens[pos++].type != TokenType::semicolon)
            {
                return Error{.message = "Expected semicolon after break"};
            }
            return node;
        }
        case TokenType::continue_:
        {
            ParseTreeNode node{tokens[pos++]};
            if (tokens[pos++].type != TokenType::semicolon)
            {
                return Error{.message = "Expected semicolon after continue"};
            }
            return node;
        }
        //These two need to wait until I implement pointers
        case TokenType::new_:
        case TokenType::delete_:
            return ParseTreeNode{tokens[pos++], {TRY(parseType(tokens, pos))}};
        default:
            return Error{.message = "Expected statement found " + tokens[pos].toString()};
        }
    }

    [[nodiscard]] ErrorOr<ParseTreeNode> parseBlock(const std::vector<Token> &tokens, size_t &pos)
    {
        ParseTreeNode node{Token{TokenType::compound}};
        if (tokens[pos++].type != TokenType::lbrace)
        {
            throw std::runtime_error("Expected left brace");
        }
        while (tokens[pos].type != TokenType::rbrace)
        {
            node.children.emplace_back(TRY(parseStatement(tokens, pos)));
        }
        ++pos;
        return node;
    }

    [[nodiscard]] ErrorOr<ParseTreeNode> parseDecl(const std::vector<Token> &tokens, size_t &pos)
    {
        ParseTreeNode node{Token{TokenType::variable_declaration}};
        if (tokens[++pos].type == TokenType::mutable_)
        {
            node.children.emplace_back(Token{TokenType::mutable_});
            ++pos;
        }
        node.children.emplace_back(tokens[pos++]);
        if (tokens[pos++].type != TokenType::colon)
        {
            return Error{.message = "Expected colon after variable declaration"};
        }
        node.children.emplace_back(tokens[pos++]);
        if (tokens[pos].type == TokenType::assign)
        {
            node.children.emplace_back(TRY(parseExpression(tokens, ++pos)));
            if (tokens[pos++].type != TokenType::semicolon)
            {
                return Error{.message = "Expected semicolon after variable declaration"};
            }
            return node;
        }
        else if (tokens[pos++].type == TokenType::semicolon)
        {
            return node;
        }
        else
        {
            return Error{.message = "Expected assignment or semicolon after declaration"};
        }
    }

    [[nodiscard]] ErrorOr<ParseTreeNode> parseAssign(const std::vector<Token> &tokens, size_t &pos)
    {
        ParseTreeNode node{{TokenType::assign}, {{tokens[pos++]}, {TRY(parseExpression(tokens, ++pos))}}};
        if (tokens[pos++].type != TokenType::semicolon)
        {
            return Error{.message = "Expected semicolon after assignment"};
        }
        return node;
    }

    [[nodiscard]] ErrorOr<ParseTreeNode> parseIf(const std::vector<Token> &tokens, size_t &pos)
    {
        ParseTreeNode node{Token{TokenType::if_}};
        if (tokens[pos++].type != TokenType::if_)
        {
            return Error{.message = "Expected if"};
        }
        if (tokens[pos++].type != TokenType::lparen)
        {
            return Error{.message = "Expected left parenthesis after if"};
        }
        node.children.emplace_back(TRY(parseExpression(tokens, pos)));
        if (tokens[pos++].type != TokenType::rparen)
        {
            return Error{.message = "Expected right parenthesis after condition"};
        }
        node.children.emplace_back(TRY(parseStatement(tokens, pos)));
        if (tokens[pos].type == TokenType::else_)
        {
            node.children.emplace_back(TRY(parseStatement(tokens, ++pos)));
        }
        return node;
    }

    [[nodiscard]] ErrorOr<ParseTreeNode> parseWhile(const std::vector<Token> &tokens, size_t &pos)
    {
        ParseTreeNode node{Token{TokenType::while_}};
        if (tokens[pos++].type != TokenType::while_)
        {
            return Error{.message = "Expected while"};
        }
        if (tokens[pos++].type != TokenType::lparen)
        {
            return Error{.message = "Expected left parenthesis after while"};
        }
        node.children.emplace_back(TRY(parseExpression(tokens, pos)));
        if (tokens[pos++].type != TokenType::rparen)
        {
            return Error{.message = "Expected right parenthesis after condition"};
        }
        node.children.emplace_back(TRY(parseStatement(tokens, pos)));
        return node;
    }

    [[nodiscard]] ErrorOr<ParseTreeNode> parseFor(const std::vector<Token> &tokens, size_t &pos)
    {
        ParseTreeNode node{Token{TokenType::for_}};
        if (tokens[pos++].type != TokenType::for_)
        {
            //TODO: this and similar should be exceptions. This method should never be called if there isn't a for. Alternatively remove this check,
            //and assume the caller knows what he's doing.
            return Error{.message = "Expected for"};
        }
        if (tokens[pos++].type != TokenType::lparen)
        {
            return Error{.message = "Expected left parenthesis after for"};
        }
        auto init = TRY(parseStatement(tokens, pos));
        auto cond = TRY(parseExpression(tokens, pos));
        if (tokens[pos++].type != TokenType::semicolon)
        {
            return Error{.message = "Expected semicolon after condition"};
        }
        auto inc = TRY(parseStatement(tokens, pos));
        if (tokens[pos++].type != TokenType::rparen)
        {
            return Error{.message = "Expected right parenthesis after for"};
        }
        node.children.emplace_back(std::move(init));
        node.children.emplace_back(std::move(cond));
        node.children.emplace_back(std::move(inc));
        node.children.emplace_back(TRY(parseStatement(tokens, pos)));
        return node;
    }
#pragma endregion Statement Parsing

    [[nodiscard]] ErrorOr<ParseTreeNode> parseFunction(const std::vector<Token> &tokens, size_t &pos)
    {
        //A consists of the func token, the name(identifier), the return type, the body, and the args are at the end,  so we can get their number easily
        ParseTreeNode root{Token{TokenType::function_definition}};
        if (tokens[++pos].type != TokenType::identifier)
        {
            throw std::runtime_error("Expected identifier after function");
        }
        root.value.value = tokens[pos++].value; //Save the name of the function
        if (tokens[pos++].type != TokenType::lparen)
        {
            throw std::runtime_error("Expected ( after function name");
        }
        std::vector<ParseTreeNode> args;
        while (tokens[pos].type != TokenType::rparen)
        {
            args.push_back(TRY(parseParameterDeclaration(tokens, pos)));
            if (tokens[pos].type != TokenType::rparen)
            {
                if (tokens[pos].type != TokenType::comma)
                {
                    throw std::runtime_error("Expected , after argument type");
                }
                ++pos;
            }
        }
        root.children.reserve(args.size() + 2);
        if (tokens[++pos].type == TokenType::arrow_operator)
        {
            if (tokens[++pos].type != TokenType::identifier)
            {
                throw std::runtime_error("Expected identifier after ->");
            }
            root.children.push_back({tokens[pos++]});
        }
        else
        {
            root.children.emplace_back(Token{TokenType::identifier, "void"});
        }
        root.children.emplace_back(TRY(parseStatement(tokens, pos)));
        root.children.insert(root.children.end(), std::make_move_iterator(args.begin()), std::make_move_iterator(args.end())); //Pushed to the end for easier access to other elements
        return root;
    }

    [[nodiscard]] ErrorOr<ParseTreeNode> parseStruct(const std::vector<Token> &tokens, size_t &pos)
    {
        ++pos;
        if (tokens[pos].type != TokenType::identifier)
        {
            throw std::runtime_error("Expected identifier after struct");
        }
        ParseTreeNode root{Token{TokenType::struct_definition, tokens[pos++].value}};
        if (tokens[pos++].type != TokenType::lbrace)
        {
            throw std::runtime_error("Expected { after struct");
        }
        while (tokens[pos].type != TokenType::rbrace)
        {
            root.children.push_back(TRY(parseParameterDeclaration(tokens, pos)));
            if (tokens[pos++].type != TokenType::semicolon)
            {
                throw std::runtime_error("Expected ; after struct member");
            }
        }
        ++pos;
        return root;
    }

    [[nodiscard]] ErrorOr<ParseTreeNode> generateParseTree(const std::vector<VWA::Token> &tokens)
    {
        //There are two types of top level definitions: functions and structs
        //Depending on the type call the appropriate function
        size_t pos = 0;
        ParseTreeNode root{Token{TokenType::null}};

        while (tokens[pos].type != TokenType::eof)
        {
            switch (tokens[pos].type)
            {
            case TokenType::function_definition:
                root.children.emplace_back(TRY(parseFunction(tokens, pos)));
                break;
            case TokenType::struct_definition:
                root.children.emplace_back(TRY(parseStruct(tokens, pos)));
                break;
            default:
                //TODO: file and line number
                //TODO: add backtrace if appropriate
                return Error{.message = "Expected function or struct at top level, found " + tokens[pos].toString(), .suggestion = "Did you close a function to early?"};
            }
        }

        return root;
    }

    template <typename T>
    [[nodiscard]] std::string getTokenValueAsString(const T &val)
    {
        return std::to_string(val);
    }
    template <>
    [[nodiscard]] std::string getTokenValueAsString<std::string>(const std::string &val)
    {
        return val;
    }
    template <>
    [[nodiscard]] std::string getTokenValueAsString<char>(const char &val)
    {
        return std::string(1, val);
    }
    template <>
    [[nodiscard]] std::string getTokenValueAsString<std::monostate>(const std::monostate &val)
    {
        return "";
    }

    template <class... Ts>
    struct overloaded : Ts...
    {
        using Ts::operator()...;
    };
    template <class... Ts>
    overloaded(Ts...) -> overloaded<Ts...>;

    [[nodiscard]] std::string TreeToString(const ParseTreeNode &root, int indent)
    {
        std::stringstream ss;
        ss << std::string(indent, ' ') << root.value.toString() << " " << std::visit(overloaded{[](const char c)
                                                                                                { return std::string(1, c); },
                                                                                                [](const std::monostate)
                                                                                                { return std::string(); },
                                                                                                [](const std::string &str)
                                                                                                { return str; },
                                                                                                [](const auto val)
                                                                                                { return std::to_string(val); }},
                                                                                     root.value.value)
           << std::endl;
        for (const auto &child : root.children)
        {
            ss << TreeToString(child, indent + 4);
        }
        return ss.str();
    }

}
