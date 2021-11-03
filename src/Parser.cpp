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

    //TODO: move the bracket pairing in here and return the end as well
    ASTNode parseExpression(const std::vector<Token> &tokens, size_t start, size_t end)
    {
    }

    ASTNode parseVariableDeclaration(const std::vector<Token> &tokens, size_t& start)
    {
        //let (optional mut) name : type;
        //If there is any other token, it is an error
        //It is assumed that the first token is let. Otherwise this function should not be called
        bool isMutable = tokens[start + 1].type == TokenType::mutable_;
        if(tokens[start + 1 + isMutable].type != TokenType::identifier)
        {
            throw std::runtime_error("Expected identifier");
        }
        Token name = tokens[start + 1 + isMutable];
        if(tokens[start + 2 + isMutable].type != TokenType::colon)
        {
            throw std::runtime_error("Expected colon after var name");
        }
        if(tokens[start + 3 + isMutable].type != TokenType::identifier)
        {
            throw std::runtime_error("Expected type name");
        }
        Token type = tokens[start + 3 + isMutable];
        if(tokens[start + 4 + isMutable].type != TokenType::semicolon)
        {
            throw std::runtime_error("Expected semicolon");
        }
        ASTNode node{.type=ASTNode::Type::Declaration};
        //The layout is: name, type, isMutable
        node.children.push_back(ASTNode{.type=ASTNode::Type::Identifier, .value=name});
        node.children.push_back(ASTNode{.type=ASTNode::Type::Identifier, .value=type});
        node.children.push_back(ASTNode{.type=ASTNode::Type::Value, .value=Token{.type=TokenType::bool_, .value=isMutable}});
        start += 5 + isMutable;
        return node;
    }

    ASTNode parseStatement(const std::vector<Token> &tokens, size_t pos, size_t end)
    {
        // ASTNode root{.type = ASTNode::Type::Body};

        // //Step one: identify the type of statement
        // for (; pos < end; ++pos)
        // {
        //     auto type = tokens[pos].type;
        //     switch (type)
        //     {
        //     case TokenType::if_:
        //     { //Find condition
        //         if (tokens[pos + 1].type != TokenType::lparen)
        //         {
        //             throw std::runtime_error("Expected ( after if");
        //         }
        //         size_t condition_end = findClosingBrace(tokens, pos + 1, TokenType::lparen);
        //         auto condition = parseExpression(tokens, pos + 2, condition_end);
        //         //Find body
        //         if (tokens[condition_end + 1].type != TokenType::lbrace)
        //         {
        //             throw std::runtime_error("Expected { after if condition");
        //         }
        //         size_t body_end = findClosingBrace(tokens, condition_end + 1, TokenType::lbrace);
        //         auto body = parseStatements(tokens, condition_end + 2, body_end);
        //         ASTNode node{.type = ASTNode::Type::Branch, .value = Token{TokenType::if_}};
        //         node.children.push_back(condition);
        //         node.children.push_back(body);
        //         //Check for else
        //         if (tokens[body_end + 1].type == TokenType::else_)
        //         {
        //             if (tokens[body_end + 2].type != TokenType::lbrace)
        //             {
        //                 throw std::runtime_error("Expected { after else");
        //             }
        //             size_t else_end = findClosingBrace(tokens, body_end + 2, TokenType::lbrace);
        //             auto else_body = parseStatements(tokens, body_end + 3, else_end);
        //             //Else just adds a second body node
        //             node.children.push_back(else_body);
        //             pos = else_end;
        //         }
        //         else
        //         {
        //             pos = body_end;
        //         }
        //         root.children.push_back(node);
        //         break;
        //     }
        //     case TokenType::for_:
        //     {
        //         //TODO: remove requirement for last semicolon
        //         ASTNode node{.type = ASTNode::Type::Branch, .value = Token{TokenType::for_}};
        //         if (tokens[pos + 1].type != TokenType::lparen)
        //         {
        //             throw std::runtime_error("Expected ( after for");
        //         }
        //         size_t condition_end = findClosingBrace(tokens, pos + 1, TokenType::lparen);
        //         //A for statement has 2 expressions
        //         size_t firsttk = pos + 2;
        //         size_t semicolon = pos + 1;
        //         for (auto i = 0; i < 3; ++i)
        //         {
        //             for (++semicolon; semicolon < condition_end; ++semicolon)
        //             {
        //                 if (tokens[semicolon].type == TokenType::semicolon)
        //                 {
        //                     node.children.push_back(parseExpression(tokens, firsttk, semicolon));
        //                     firsttk = semicolon + 1;
        //                     break;
        //                 }
        //             }
        //         }
        //         if (tokens[condition_end + 1].type != TokenType::lbrace)
        //         {
        //             throw std::runtime_error("Expected { after for condition");
        //         }
        //         size_t body_end = findClosingBrace(tokens, condition_end + 1, TokenType::lbrace);
        //         auto body = parseStatements(tokens, condition_end + 2, body_end);
        //         node.children.push_back(body);
        //         root.children.push_back(node);
        //         pos = body_end;
        //         break;
        //     }
        //     case TokenType::while_:
        //     {
        //         ASTNode node{.type = ASTNode::Type::Branch, .value = Token{TokenType::while_}};
        //         if (tokens[pos + 1].type != TokenType::lparen)
        //         {
        //             throw std::runtime_error("Expected ( after while");
        //         }
        //         size_t condition_end = findClosingBrace(tokens, pos + 1, TokenType::lparen);
        //         //A for statement has 2 expressions
        //         node.children.push_back(parseExpression(tokens, pos + 2, condition_end));
        //         if (tokens[condition_end + 1].type != TokenType::lbrace)
        //         {
        //             throw std::runtime_error("Expected { after for condition");
        //         }
        //         size_t body_end = findClosingBrace(tokens, condition_end + 1, TokenType::lbrace);
        //         auto body = parseStatements(tokens, condition_end + 2, body_end);
        //         node.children.push_back(body);
        //         root.children.push_back(node);
        //         pos = body_end;
        //         break;
        //     }
        //     case TokenType::return_:
        //     {
        //         ASTNode node{.type = ASTNode::Type::Branch, .value = Token{TokenType::return_}};
        //         break;
        //     }
        //     default:
        //         throw std::runtime_error("Unexpected token");
        //     }
        // }
        // return root;
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
    }

    ASTNode parseStruct(const std::vector<Token> &tokens, size_t &pos)
    {
        ++pos;
        ASTNode root{.type = ASTNode::Type::Struct};

        if (tokens[pos].type != TokenType::identifier)
        {
            throw std::runtime_error("Expected identifier after struct");
        }
        root.children.push_back({.type = ASTNode::Type::Identifier, .value = tokens[pos++]});//The name of the struct. I will check for 
        if(tokens[pos++].type != TokenType::lbrace)
        {
            throw std::runtime_error("Expected { after struct name");
        }
        //duplicates later
        while(tokens[pos].type != TokenType::rbrace)
        {
            if(tokens[pos].type != TokenType::variable_declaration)
            {
                throw std::runtime_error("Expected identifier in struct");
            }
            root.children.push_back(parseVariableDeclaration(tokens, pos));
        }
        ++pos;
        return root;
    }

    ASTNode generateParseTree(const std::vector<VWA::Token> &tokens)
    {
        //There are two types of top level definitions: funcitions and structs
        //Depending on the type call the appropriate function
        size_t pos = 0; //Passed to all functions
        ASTNode root{.type = ASTNode::Type::Root};
        while (tokens[pos].type != TokenType::eof)
        {
            switch (tokens[pos].type)
            {
            case TokenType::function_definition:
            {
                root.children.push_back(parseFunction(tokens, pos));
                break;
            }
            case TokenType::struct_definition:
            {
                root.children.push_back(parseStruct(tokens, pos));
                break;
            }
            default:
                throw std::runtime_error("Unexpected token " + tokens[pos].toString() +". Only struct and func is allowed at top level.");
            }
        }
        //TODO: if type is executable, then it should contain a main function. The main function should be first. Just check the name
        //of every function in the previous step and if it is main push it on the front of the vector
        return root;
    }
}
