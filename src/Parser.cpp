#include <Parser.hpp>

//Instead of trying to find the end first, just pass the current token into the function and return both a node and set the current token, passed
// by reference, to the next token.
//Struct parsing can be combined into main parser
//TODO: dissallow assignment within expressions
namespace VWA
{
    std::pair<VarType, CustomTypeInfo *> GetType(const std::string &type, pass1_result &result)
    {
        if (type == "void")
            return std::make_pair(VarType::Void, nullptr);
        if (type == "int")
            return {VarType::Int, nullptr};
        if (type == "long")
            return {VarType::Long, nullptr};
        if (type == "float")
            return {VarType::Float, nullptr};
        if (type == "double")
            return {VarType::Double, nullptr};
        if (type == "bool")
            return {VarType::Bool, nullptr};
        if (type == "char")
            return {VarType::Char, nullptr};
        return {VarType::Custom, &result.structs[type]};
    }
    void ParseStructDefinition(pass1_result &result, const std::vector<Token> &tokens, size_t &pos)
    {
        std::string struct_name = std::get<std::string>(tokens[pos + 1].value); //if the user is stupid this could cause a segfault
        auto &it = result.structs[struct_name];
        if (it.isInitialized)
        {
            throw std::runtime_error("Struct " + struct_name + " is already defined");
        }
        it.isInitialized = true;
        it.name = struct_name;
        size_t end;
        uint counter = 0;
        for (end = pos + 2;; ++end)
        {
            if (end == tokens.size())
            {
                throw std::runtime_error("Unexpected end of file");
            }
            if (tokens[end].type == TokenType::lbrace)
            {
                ++counter;
                continue;
            }
            if (tokens[end].type == TokenType::rbrace)
            {
                --counter;
                if (counter == 0)
                {
                    break;
                }
                continue;
            }
            if (tokens[end].type == TokenType::variable_declaration)
            {
                bool isMutable = tokens[end + 1].type == TokenType::mutable_;
                std::string type = std::get<std::string>(tokens[end + 1 + isMutable].value);
                if (tokens[end + 2 + isMutable].type != TokenType::identifier)
                {
                    throw std::runtime_error("Expected identifier");
                }
                std::string name = std::get<std::string>(tokens[end + 2 + isMutable].value);
                if (tokens[end + 3 + isMutable].type != TokenType::semicolon)
                {
                    throw std::runtime_error("Expected semicolon");
                }
                auto type_pair = GetType(type, result);
                it.fields.push_back({name, isMutable, type_pair.first, type_pair.second});
                end += 3 + isMutable;
                continue;
            }
            throw std::runtime_error("Unexpected token " + tokens[end].toString());
        }
        pos = end;
    }

    void ParseFunctionDefinition(pass1_result &result, const std::vector<Token> &tokens, size_t &pos)
    {
        auto name = std::get<std::string>(tokens[pos + 1].value);
        if (auto it = result.functions.find(name); it != result.functions.end())
        {
            throw std::runtime_error("Function " + name + " is already defined");
        }
        auto &it = result.functions[name];
        it.first.name = name;
        size_t counter = 0;
        for (pos += 2;; ++pos)
        {
            if (pos == tokens.size())
            {
                throw std::runtime_error("Unexpected end of file");
            }
            if (tokens[pos].type == TokenType::lparen)
            {
                ++counter;
                continue;
            }
            if (tokens[pos].type == TokenType::rparen)
            {
                --counter;
                if (counter == 0)
                {
                    break;
                }
                continue;
            }
            if (tokens[pos].type == TokenType::variable_declaration)
            {
                bool isMutable = tokens[pos + 1].type == TokenType::mutable_;
                std::string type = std::get<std::string>(tokens[pos + 1 + isMutable].value);
                if (tokens[pos + 2 + isMutable].type != TokenType::identifier)
                {
                    throw std::runtime_error("Expected identifier");
                }
                std::string _name = std::get<std::string>(tokens[pos + 2 + isMutable].value);
                if (tokens[pos + 3 + isMutable].type != TokenType::comma && tokens[pos + 3 + isMutable].type != TokenType::rparen)
                {
                    throw std::runtime_error("Expected comma or parentheses");
                }
                auto type_pair = GetType(type, result);
                it.first.args.push_back({_name, type_pair.first, isMutable, type_pair.second});
                pos += 2 + isMutable;
                continue;
            }
            throw std::runtime_error("Unexpected token " + tokens[pos].toString());
        }
        if (tokens[pos + 1].type == TokenType::arrow_operator)
        {
            if (tokens[pos + 2].type != TokenType::identifier)
            {
                throw std::runtime_error("Expected identifier");
            }
            auto rtype = GetType(std::get<std::string>(tokens[pos + 2].value), result);
            it.first.returnType = rtype.first;
            it.first.rtypeInfo = rtype.second;
            pos += 2;
        }
        else
        {
            it.first.returnType = VarType::Void;
            it.first.rtypeInfo = nullptr;
        }
        size_t body_start = pos + 1;
        for (pos += 1;; ++pos)
        {
            if (pos == tokens.size())
            {
                throw std::runtime_error("Unexpected end of file");
            }
            if (tokens[pos].type == TokenType::lbrace)
            {
                ++counter;
                continue;
            }
            if (tokens[pos].type == TokenType::rbrace)
            {
                --counter;
                if (counter == 0)
                {
                    break;
                }
                continue;
            }
        }
        it.second = {tokens.begin() + body_start + 1, tokens.begin() + pos};
    }

    ASTNode parseExpression(const std::vector<Token> &tokens, size_t &start);

    //Returns either a tree of dot operators or just and identifier
    ASTNode parseDotOperator(const std::vector<Token> &tokens, size_t &start)
    {
        //I have decided on this format for the dot operator:
        //dot is the root node, the children are the identifiers in the correct order

        ASTNode root{{TokenType::dot}, {{tokens[start++]}}};

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

    ASTNode parseFactor(const std::vector<Token> &tokens, size_t &pos)
    {
        switch (tokens[pos].type)
        {
        case TokenType::int_:
        case TokenType::long_:
        case TokenType::float_:
        case TokenType::double_:
        {
            return {tokens[pos++]};
        }
        case TokenType::lparen:
        {
            ++pos;
            auto result = parseExpression(tokens, pos);
            if (tokens[pos].type != TokenType::rparen)
            {
                throw std::runtime_error("Expected ')'");
            }
            ++pos;
            return result;
        }
        case TokenType::minus:
        {
            ++pos;
            auto result = parseFactor(tokens, pos);
            switch (result.value.type)
            {
            case TokenType::int_:
                return {{TokenType::int_, -std::get<int>(result.value.value)}};
            case TokenType::long_:
                return {{TokenType::long_, -std::get<long>(result.value.value)}};
            case TokenType::float_:
                return {{TokenType::float_, -std::get<float>(result.value.value)}};
            case TokenType::double_:
                return {{TokenType::double_, -std::get<double>(result.value.value)}};
            default:
                return {.value = {TokenType::minus}, .children{result}};
            }
        }
        case TokenType::identifier:
        {
            //An identifier can mean three things here:
            //1. A variable name
            //2. A function call
            //3. a variable name with the dot operator
            switch (tokens[pos + 1].type)
            {
            //This is a function call. Inside the parantheses may be 0 or more comma seperated expressions(the arguments)
            //These arguments are stored as children of the node
            case TokenType::lparen:
            {
                ASTNode node{{TokenType::function_call, tokens[pos].value}};
                pos += 2;
                while (tokens[pos].type != TokenType::rparen)
                {
                    node.children.push_back(parseExpression(tokens, pos));
                    if (tokens[pos].type != TokenType::rparen)
                    {
                        if (tokens[pos].type != TokenType::comma)
                        {
                            throw std::runtime_error("Expected ',' or ')'");
                        }
                        ++pos;
                    }
                }
                ++pos;
                return node;
            }
            case TokenType::dot:
            {
                return parseDotOperator(tokens, pos);
            }
            default:
            {
                return {tokens[pos++]};
            }
            }
        default:
        {
            throw std::runtime_error("Unexpected token " + tokens[pos].toString());
        }
        }
        }
    }

    ASTNode parsePower(const std::vector<Token> &tokens, size_t &pos)
    {
        auto lhs = parseFactor(tokens, pos);
        if (tokens[pos].type != TokenType::power)
        {
            return lhs;
        }
        ++pos;
        auto rhs = parsePower(tokens, pos);
        return {.value{TokenType::power}, .children{lhs, rhs}};
    }

    ASTNode parseTerm(const std::vector<Token> &tokens, size_t &pos)
    {
        ASTNode node = parsePower(tokens, pos);
        while (1)
        {
            switch (tokens[pos].type)
            {
            case TokenType::star:
            case TokenType::divide:
                node = ASTNode{.value = tokens[pos], .children{node, parsePower(tokens, ++pos)}};
                break;
            default:
                return node;
            }
        }
    }

    //TODO: move the bracket pairing in here and return the end as well
    ASTNode parseExpression(const std::vector<Token> &tokens, size_t &start)
    {
        ASTNode node = parseTerm(tokens, start);
        while (1)
            switch (tokens[start].type)
            {
            case TokenType::plus:
            case TokenType::minus:
                node = ASTNode{.value = tokens[start], .children = {node, parseTerm(tokens, ++start)}};
                break;
            default: //TODO: better way to define end of expression
                return node;
            }
    }

    ASTNode parseVariableDeclaration(const std::vector<Token> &tokens, size_t &start)
    {
        //Format let (mut) x:int; or with initializer let (mut) x:int = 5;
        if (tokens[start++].type != TokenType::variable_declaration)
        {
            throw std::runtime_error("Expected variable declaration");
        }
        ASTNode node{Token{TokenType::variable_declaration}};
        switch (tokens[start].type)
        {
        case TokenType::mutable_:
            node.children.push_back(ASTNode{Token{TokenType::mutable_}}); //TODO: better way to store this
            if (tokens[++start].type != TokenType::identifier)
            {
                throw std::runtime_error("Expected identifier");
            }
            node.children.push_back({tokens[start++]});
            break;
        case TokenType::identifier:
            node.children.push_back(ASTNode{tokens[start++]});
            break;
        default:
            throw std::runtime_error("Expected identifier");
        }
        if (tokens[start++].type != TokenType::colon)
        {
            throw std::runtime_error("Expected colon after variable name");
        }
        //TODO: refactor for easy extraction into assignment statement
        node.children.push_back(ASTNode{tokens[start++]}); //type
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
    ASTNode parseParameterDeclaration(const std::vector<Token> &tokens, size_t &start)
    {
        ASTNode node{Token{TokenType::variable_declaration}};
        switch (tokens[start].type)
        {
        case TokenType::mutable_:
            node.children.push_back(ASTNode{Token{TokenType::mutable_}}); //TODO: better way to store this
            if (tokens[++start].type != TokenType::identifier)
            {
                throw std::runtime_error("Expected identifier");
            }
            node.children.push_back({tokens[start++]});
            break;
        case TokenType::identifier:
            node.children.push_back(ASTNode{tokens[start++]});
            break;
        default:
            throw std::runtime_error("Expected identifier");
        }
        if (tokens[start++].type != TokenType::colon)
        {
            throw std::runtime_error("Expected colon after variable name. Type inference is not supported");
        }
        //TODO: refactor for easy extraction into assignment statement
        node.children.push_back(ASTNode{tokens[start++]}); //type
        return node;
    }
    ASTNode parseStatement(const std::vector<Token> &tokens, size_t &pos);
    ASTNode parseCompound(const std::vector<Token> &tokens, size_t &start)
    {
        ASTNode node{Token{TokenType::compound}};
        if (tokens[start++].type != TokenType::lbrace)
        {
            throw std::runtime_error("Expected left brace");
        }
        while (tokens[start].type != TokenType::rbrace)
        {
            node.children.push_back(parseStatement(tokens, start));
        }
        ++start;
        return node;
    }

    ASTNode parseStatement(const std::vector<Token> &tokens, size_t &pos)
    {
        switch (tokens[pos].type)
        {
        case TokenType::lbrace:
            return parseCompound(tokens, pos);

        case TokenType::variable_declaration:
            return parseVariableDeclaration(tokens, pos);
        default:
            throw std::runtime_error("Unexpected token " + tokens[pos].toString());
        }
    }

    ASTNode parseFunction(const std::vector<Token> &tokens, size_t &pos)
    {
        // auto &[info, tokens] = function;
        // ASTNode root{.type = ASTNode::Type::Function};
        // root.children.push_back({.type = ASTNode::Type::Value, .value = Token{TokenType::identifier, info.name}});
        // root.children.push_back({.type = ASTNode::Type::Type, .value = std::tuple<VarType, CustomTypeInfo *, bool>{info.returnType, info.rtypeInfo, true}});
        // root.children.push_back(parseStatements(tokens, 0, tokens.size()));
        // for (auto &arg : info.args)
        // {
        //     root.children.push_back({.type = ASTNode::Type::Type, .value = std::tuple<VarType, CustomTypeInfo *, bool>{arg.type, arg.typeInfo, arg.isMutable}});
        // }
        // return root;
        //A consists of the func token, the name(identifier), the return type, the body, and the args args are at the end,  so we can get their number easily
        ASTNode root{Token{TokenType::function_definition}};
        if (tokens[++pos].type != TokenType::identifier)
        {
            throw std::runtime_error("Expected identifier after function");
        }
        root.value.value = tokens[pos++].value; //Save the name of the function
        if (tokens[pos++].type != TokenType::lparen)
        {
            throw std::runtime_error("Expected ( after function name");
        }
        std::vector<ASTNode> args;
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

    ASTNode parseStruct(const std::vector<Token> &tokens, size_t &pos)
    {
        ++pos;
        if (tokens[pos].type != TokenType::identifier)
        {
            throw std::runtime_error("Expected identifier after struct");
        }
        ASTNode root{.value = Token{TokenType::struct_definition, tokens[pos++].value}};
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

    ASTNode generateParseTree(const std::vector<VWA::Token> &tokens)
    {
        //There are two types of top level definitions: funcitions and structs
        //Depending on the type call the appropriate function
        size_t pos = 0;
        // ASTNode root{.value = Token{TokenType::null}};

        // while (tokens[pos].type != TokenType::eof)
        // {
        //     switch (tokens[pos].type)
        //     {
        //     case TokenType::function_definition:
        //         root.children.push_back(parseFunction(tokens, pos));
        //         break;
        //     case TokenType::struct_definition:
        //         root.children.push_back(parseStruct(tokens, pos));
        //         break;
        //     default:
        //         throw std::runtime_error("Unexpected top level token token " + tokens[pos].toString());
        //     }
        // }

        //TODO: if type is executable, then it should contain a main function. The main function should be first. Just check the name
        //Main should be stored in the root node
        // return root;
        return parseExpression(tokens, pos);
    }
}
