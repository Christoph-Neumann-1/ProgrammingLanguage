#include <Parser.hpp>
#include <sstream>

//Instead of trying to find the end first, just pass the current token into the function and return both a node and set the current token, passed
// by reference, to the next token.
//Struct parsing can be combined into main parser
//TODO: dissallow assignment within expressions
namespace VWA
{
#pragma region Expression Parsing

    ParseTreeNode parseExpression(const std::vector<Token> &tokens, size_t &start);
    ParseTreeNode parseComparison(const std::vector<Token> &tokens, size_t &start);
    ParseTreeNode parseAddition(const std::vector<Token> &tokens, size_t &start);
    ParseTreeNode parseMultiplication(const std::vector<Token> &tokens, size_t &start);
    ParseTreeNode parseUnary(const std::vector<Token> &tokens, size_t &start);
    ParseTreeNode parsePower(const std::vector<Token> &tokens, size_t &start);
    ParseTreeNode parsePrimary(const std::vector<Token> &tokens, size_t &start);
    ParseTreeNode parseDot(const std::vector<Token> &tokens, size_t &start);
    ParseTreeNode parseFuncCall(const std::vector<Token> &tokens, size_t &start);

    //TODO: move the bracket pairing in here and return the end as well
    ParseTreeNode parseExpression(const std::vector<Token> &tokens, size_t &start)
    {
        ParseTreeNode node = parseComparison(tokens, start);
        while (1)
            switch (tokens[start].type)
            {
            case TokenType::and_:
            case TokenType::or_:
                node = ParseTreeNode{.value = tokens[start], .children = {node, parseComparison(tokens, ++start)}};
                break;
            default: //TODO: better way to define end of expression
                return node;
            }
    }

    ParseTreeNode parseComparison(const std::vector<Token> &tokens, size_t &start)
    {
        ParseTreeNode node = parseAddition(tokens, start);
        //Should i allow chained comparisons?
        while (1)
            switch (tokens[start].type)
            {
            case TokenType::lt:
            case TokenType::gt:
            case TokenType::leq:
            case TokenType::geq:
            case TokenType::eq:
            case TokenType::neq:
                node = ParseTreeNode{.value = tokens[start], .children = {node, parseAddition(tokens, ++start)}};
                break;
            default:
                return node;
            }
    }

    ParseTreeNode parseAddition(const std::vector<Token> &tokens, size_t &start)
    {
        ParseTreeNode node = parseMultiplication(tokens, start);
        while (1)
            switch (tokens[start].type)
            {
            case TokenType::plus:
            case TokenType::minus:
                node = ParseTreeNode{.value = tokens[start], .children = {node, parseMultiplication(tokens, ++start)}};
                break;
            default:
                return node;
            }
    }

    ParseTreeNode parseMultiplication(const std::vector<Token> &tokens, size_t &start)
    {
        ParseTreeNode node = parseUnary(tokens, start);
        while (1)
            switch (tokens[start].type)
            {
            case TokenType::star:
            case TokenType::divide:
            case TokenType::mod:
                node = ParseTreeNode{.value = tokens[start], .children = {node, parseUnary(tokens, ++start)}};
                break;
            default:
                return node;
            }
    }

    ParseTreeNode parseUnary(const std::vector<Token> &tokens, size_t &start)
    {
        switch (tokens[start].type)
        {
        case TokenType::plus:
        case TokenType::minus:
        case TokenType::not_:
            return {tokens[start], {parseUnary(tokens, ++start)}};
        default:
            return parsePower(tokens, start);
        }
    }

    ParseTreeNode parsePower(const std::vector<Token> &tokens, size_t &start)
    {
        ParseTreeNode lhs = parsePrimary(tokens, start);
        if (tokens[start].type != TokenType::power)
        {
            return lhs;
        }
        ++start;
        ParseTreeNode rhs = parsePower(tokens, start);
        return {.value = {TokenType::power}, .children = {lhs, rhs}};
    }

    //TODO: sizeof
    ParseTreeNode parsePrimary(const std::vector<Token> &tokens, size_t &start)
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
            auto result = parseExpression(tokens, start);
            if (tokens[start++].type != TokenType::rparen)
            {
                throw std::runtime_error("Expected ')'");
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
            //This is a function call. Inside the parantheses may be 0 or more comma seperated expressions(the arguments)
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
            throw std::runtime_error("Unexpected token " + tokens[start].toString());
        }
        }
        }
    }

    ParseTreeNode parseFuncCall(const std::vector<Token> &tokens, size_t &start)
    {
        ParseTreeNode node{{TokenType::function_call, tokens[start].value}};
        start += 2;
        while (tokens[start].type != TokenType::rparen)
        {
            node.children.push_back(parseExpression(tokens, start));
            if (tokens[start].type != TokenType::rparen)
            {
                if (tokens[start].type != TokenType::comma)
                {
                    throw std::runtime_error("Expected ',' or ')'");
                }
                ++start;
            }
        }
        ++start;
        return node;
    }

    //Returns either a tree of dot operators or just and identifier
    ParseTreeNode parseDot(const std::vector<Token> &tokens, size_t &start)
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
                    throw std::runtime_error("Expected identifier");
                }
                root.children.push_back({tokens[start++]});
                continue;
            }
            return root;
        }
    }

#pragma endregion Expression Parsing

    ParseTreeNode parseVariableDeclaration(const std::vector<Token> &tokens, size_t &start)
    {
        //Format let (mut) x:int; or with initializer let (mut) x:int = 5;
        if (tokens[start++].type != TokenType::variable_declaration)
        {
            throw std::runtime_error("Expected variable declaration");
        }
        ParseTreeNode node{Token{TokenType::variable_declaration}};
        switch (tokens[start].type)
        {
        case TokenType::mutable_:
            node.children.push_back(ParseTreeNode{Token{TokenType::mutable_}}); //TODO: better way to store this
            if (tokens[++start].type != TokenType::identifier)
            {
                throw std::runtime_error("Expected identifier");
            }
            node.children.push_back({tokens[start++]});
            break;
        case TokenType::identifier:
            node.children.push_back(ParseTreeNode{tokens[start++]});
            break;
        default:
            throw std::runtime_error("Expected identifier");
        }
        if (tokens[start++].type != TokenType::colon)
        {
            throw std::runtime_error("Expected colon after variable name");
        }
        //TODO: refactor for easy extraction into assignment statement
        node.children.push_back(ParseTreeNode{tokens[start++]}); //type
        switch (tokens[start].type)
        {
        case TokenType::semicolon:
            break;
        case TokenType::assign:
            node.children.push_back(parseExpression(tokens, ++start));
            if (tokens[start].type != TokenType::semicolon)
            {
                throw std::runtime_error("Expected semicolon");
            }
            break;
        default:
            throw std::runtime_error("Expected semicolon");
        }
        ++start;
        return node;
    }

    //TODO: default values
    ParseTreeNode parseParameterDeclaration(const std::vector<Token> &tokens, size_t &start)
    {
        ParseTreeNode node{Token{TokenType::variable_declaration}};
        switch (tokens[start].type)
        {
        case TokenType::mutable_:
            node.children.push_back(ParseTreeNode{Token{TokenType::mutable_}}); //TODO: better way to store this
            if (tokens[++start].type != TokenType::identifier)
            {
                throw std::runtime_error("Expected identifier");
            }
            node.children.push_back({tokens[start++]});
            break;
        case TokenType::identifier:
            node.children.push_back(ParseTreeNode{tokens[start++]});
            break;
        default:
            throw std::runtime_error("Expected identifier");
        }
        if (tokens[start++].type != TokenType::colon)
        {
            throw std::runtime_error("Expected colon after variable name. Type inference is not supported");
        }
        //TODO: refactor for easy extraction into assignment statement
        node.children.push_back(ParseTreeNode{tokens[start++]}); //type
        return node;
    }
    ParseTreeNode parseStatement(const std::vector<Token> &tokens, size_t &pos);

#pragma region Statement Parsing
    ParseTreeNode parseStatement(const std::vector<Token> &tokens, size_t &pos);
    ParseTreeNode parseBlock(const std::vector<Token> &tokens, size_t &pos);
    ParseTreeNode parseIf(const std::vector<Token> &tokens, size_t &pos);
    ParseTreeNode parseWhile(const std::vector<Token> &tokens, size_t &pos);
    ParseTreeNode parseFor(const std::vector<Token> &tokens, size_t &pos);
    ParseTreeNode parseDecl(const std::vector<Token> &tokens, size_t &pos);
    ParseTreeNode parseAssign(const std::vector<Token> &tokens, size_t &pos);

    ParseTreeNode parseStatement(const std::vector<Token> &tokens, size_t &pos)
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
                // ParseTreeNode node{{TokenType::assign}, {{tokens[pos++], {parseExpression(tokens, ++pos)}}}};
                // if (tokens[pos++].type != TokenType::semicolon)
                // {
                //     throw std::runtime_error("Expected semicolon");
                // }
                // return node;
                return parseAssign(tokens, pos);
            }
            else if (tokens[pos + 1].type == TokenType::lparen)
            {
                ParseTreeNode node = parseFuncCall(tokens, pos);
                if (tokens[pos++].type != TokenType::semicolon)
                {
                    throw std::runtime_error("Expected semicolon");
                }
                return node;
            }
            else if (tokens[pos + 1].type == TokenType::dot)
            {
                auto dot = parseDot(tokens, pos);
                if (tokens[pos].type != TokenType::assign)
                {
                    throw std::runtime_error("Expected assignment");
                }
                ParseTreeNode node;
                node.value.type = TokenType::assign;
                node.children.push_back(dot);
                node.children.push_back(parseExpression(tokens, ++pos));
                return node;
            }
            else
            {
                throw std::runtime_error("Expected assignment or call");
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
        {
            ParseTreeNode node{tokens[pos++], {parseExpression(tokens, pos)}};
            if (tokens[pos++].type != TokenType::semicolon)
            {
                throw std::runtime_error("Expected semicolon");
            }
            return node;
        }
        case TokenType::break_:
        {
            ParseTreeNode node{tokens[pos++]};
            if (tokens[pos++].type != TokenType::semicolon)
            {
                throw std::runtime_error("Expected semicolon");
            }
            return node;
        }
        case TokenType::continue_:
        {
            ParseTreeNode node{tokens[pos++]};
            if (tokens[pos++].type != TokenType::semicolon)
            {
                throw std::runtime_error("Expected semicolon");
            }
            return node;
        }
        //These two need to wait until I implement pointers
        case TokenType::new_:
        case TokenType::delete_:
            throw std::runtime_error("Token not implemented");
        default:
            throw std::runtime_error("Unexpected token " + tokens[pos].toString());
        }
    }

    ParseTreeNode parseBlock(const std::vector<Token> &tokens, size_t &pos)
    {
        ParseTreeNode node{Token{TokenType::compound}};
        if (tokens[pos++].type != TokenType::lbrace)
        {
            throw std::runtime_error("Expected left brace");
        }
        while (tokens[pos].type != TokenType::rbrace)
        {
            node.children.push_back(parseStatement(tokens, pos));
        }
        ++pos;
        return node;
    }

    ParseTreeNode parseDecl(const std::vector<Token> &tokens, size_t &pos)
    {
        ParseTreeNode node{Token{TokenType::variable_declaration}};
        if (tokens[++pos].type == TokenType::mutable_)
        {
            node.children.push_back({{TokenType::mutable_}});
            ++pos;
        }
        node.children.push_back({tokens[pos++]});
        if (tokens[pos++].type != TokenType::colon)
        {
            throw std::runtime_error("Expected colon");
        }
        node.children.push_back({tokens[pos++]});
        if (tokens[pos].type == TokenType::assign)
        {
            node.children.push_back(parseExpression(tokens, ++pos));
            if (tokens[pos++].type != TokenType::semicolon)
            {
                throw std::runtime_error("Expected semicolon");
            }
            return node;
        }
        else if (tokens[pos++].type == TokenType::semicolon)
        {
            return node;
        }
        else
        {
            throw std::runtime_error("Expected assignment or semicolon");
        }
    }

    ParseTreeNode parseAssign(const std::vector<Token> &tokens, size_t &pos)
    {
        ParseTreeNode node{.value = {TokenType::assign}, .children = {{tokens[pos++]}, {parseExpression(tokens, ++pos)}}};
        if (tokens[pos++].type != TokenType::semicolon)
        {
            throw std::runtime_error("Expected semicolon");
        }
        return node;
    }

    ParseTreeNode parseIf(const std::vector<Token> &tokens, size_t &pos)
    {
        ParseTreeNode node{Token{TokenType::if_}};
        if (tokens[pos++].type != TokenType::if_)
        {
            throw std::runtime_error("Expected if");
        }
        if (tokens[pos++].type != TokenType::lparen)
        {
            throw std::runtime_error("Expected left parenthesis");
        }
        node.children.push_back(parseExpression(tokens, pos));
        if (tokens[pos++].type != TokenType::rparen)
        {
            throw std::runtime_error("Expected right parenthesis");
        }
        node.children.push_back(parseStatement(tokens, pos));
        if (tokens[pos].type == TokenType::else_)
        {
            node.children.push_back(parseStatement(tokens, ++pos));
        }
        return node;
    }

    ParseTreeNode parseWhile(const std::vector<Token> &tokens, size_t &pos)
    {
        ParseTreeNode node{Token{TokenType::while_}};
        if (tokens[pos++].type != TokenType::while_)
        {
            throw std::runtime_error("Expected while");
        }
        if (tokens[pos++].type != TokenType::lparen)
        {
            throw std::runtime_error("Expected left parenthesis");
        }
        node.children.push_back(parseExpression(tokens, pos));
        if (tokens[pos++].type != TokenType::rparen)
        {
            throw std::runtime_error("Expected right parenthesis");
        }
        node.children.push_back(parseStatement(tokens, pos));
        return node;
    }

    ParseTreeNode parseFor(const std::vector<Token> &tokens, size_t &pos)
    {
        ParseTreeNode node{Token{TokenType::for_}};
        if (tokens[pos++].type != TokenType::for_)
        {
            throw std::runtime_error("Expected for");
        }
        if (tokens[pos++].type != TokenType::lparen)
        {
            throw std::runtime_error("Expected left parenthesis");
        }
        auto init = parseStatement(tokens, pos);
        auto cond = parseExpression(tokens, pos);
        if (tokens[pos++].type != TokenType::semicolon)
        {
            throw std::runtime_error("Expected semicolon");
        }
        auto inc = parseStatement(tokens, pos);
        if (tokens[pos++].type != TokenType::rparen)
        {
            throw std::runtime_error("Expected right parenthesis");
        }
        node.children.push_back(init);
        node.children.push_back(cond);
        node.children.push_back(inc);
        node.children.push_back(parseStatement(tokens, pos));
        return node;
    }
#pragma endregion Statement Parsing

    ParseTreeNode parseFunction(const std::vector<Token> &tokens, size_t &pos)
    {
        //A consists of the func token, the name(identifier), the return type, the body, and the args args are at the end,  so we can get their number easily
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
            args.push_back(parseParameterDeclaration(tokens, pos));
            if (tokens[pos].type != TokenType::rparen)
            {
                if (tokens[pos].type != TokenType::comma)
                {
                    throw std::runtime_error("Expected , after argument type");
                }
                ++pos;
            }
        }
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
            root.children.push_back({{TokenType::identifier, "void"}});
        }
        root.children.push_back(parseStatement(tokens, pos));
        root.children.insert(root.children.end(), args.begin(), args.end()); //Pushed to the end for easier access to other elements
        return root;
    }

    ParseTreeNode parseStruct(const std::vector<Token> &tokens, size_t &pos)
    {
        ++pos;
        if (tokens[pos].type != TokenType::identifier)
        {
            throw std::runtime_error("Expected identifier after struct");
        }
        ParseTreeNode root{.value = Token{TokenType::struct_definition, tokens[pos++].value}};
        if (tokens[pos++].type != TokenType::lbrace)
        {
            throw std::runtime_error("Expected { after struct");
        }
        while (tokens[pos].type != TokenType::rbrace)
        {
            root.children.push_back(parseParameterDeclaration(tokens, pos));
            if (tokens[pos++].type != TokenType::semicolon)
            {
                throw std::runtime_error("Expected ; after struct member");
            }
        }
        ++pos;
        return root;
    }

    ParseTreeNode generateParseTree(const std::vector<VWA::Token> &tokens)
    {
        //There are two types of top level definitions: funcitions and structs
        //Depending on the type call the appropriate function
        size_t pos = 0;
        ParseTreeNode root{.value = Token{TokenType::null}};

        while (tokens[pos].type != TokenType::eof)
        {
            switch (tokens[pos].type)
            {
            case TokenType::function_definition:
                root.children.push_back(parseFunction(tokens, pos));
                break;
            case TokenType::struct_definition:
                root.children.push_back(parseStruct(tokens, pos));
                break;
            default:
                throw std::runtime_error("Unexpected top level token token " + tokens[pos].toString());
            }
        }

        //TODO: if type is executable, then it should contain a main function. The main function should be first. Just check the name
        //Main should be stored in the root node
        return root;
    }

    template <typename T>
    std::string getTokenValueAsString(const T &val)
    {
        return std::to_string(val);
    }
    template <>
    std::string getTokenValueAsString<std::string>(const std::string &val)
    {
        return val;
    }
    template <>
    std::string getTokenValueAsString<char>(const char &val)
    {
        return std::string(1, val);
    }
    template <>
    std::string getTokenValueAsString<std::monostate>(const std::monostate &val)
    {
        return "";
    }

    //I originally wanted to use the overload pattern for lambdas, but the gcc compiler decided to segfault in completely unrelated
    //places when adding it. Since it got the code from cppreference, it is not my fault at least. Seems like gcc does not support that part of
    //the standard.

    template <class... Ts>
    struct overloaded : Ts...
    {
        using Ts::operator()...;
    };
    template <class... Ts>
    overloaded(Ts...) -> overloaded<Ts...>;

    std::string TreeToString(const ParseTreeNode &root, int indent)
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
