#include <Parser.hpp>
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

    pass1_result findDefinitions(std::vector<Token> tokens)
    {
        pass1_result result;
        for (size_t i = 0; i < tokens.size(); i++)
        {
            switch (tokens[i].type)
            {
            case TokenType::struct_definition:
                ParseStructDefinition(result, tokens, i);
                continue;
            case TokenType::function_definition:
                ParseFunctionDefinition(result, tokens, i);
                continue;
            default:
                continue;
            }
        }

        //Assert that all structs have been defined
        for (auto &[_, value] : result.structs)
        {
            if (!value.isInitialized)
            {
                throw std::runtime_error("Struct " + value.name + " is not defined");
            }
        }

        //Assert that main is defined
        if (auto main = result.functions.find("main"); main != result.functions.end())
        {
            //Assert that main has no arguments and returns int
            if (main->second.first.args.size() != 0)
            {
                throw std::runtime_error("main must have no arguments");
            }
            if (main->second.first.returnType != VarType::Int)
            {
                throw std::runtime_error("main must return int");
            }
        }
        else
        {
            throw std::runtime_error("main is not defined");
        }

        return result;
    }

    size_t findClosingBrace(const std::vector<Token> &tokens, size_t opening, TokenType openingType)
    {
        TokenType closingType;
        switch (openingType)
        {
        case TokenType::lbrace:
            closingType = TokenType::rbrace;
            break;
        case TokenType::lparen:
            closingType = TokenType::rparen;
            break;
        case TokenType::lbracket:
            closingType = TokenType::rbracket;
            break;
        default:
            throw std::runtime_error("Not a brace");
        }
        size_t counter = 1;
        for (size_t i = opening + 1; i < tokens.size(); i++)
        {
            if (tokens[i].type == openingType)
            {
                ++counter;
            }
            else if (tokens[i].type == closingType)
            {
                --counter;
                if (counter == 0)
                {
                    return i;
                }
            }
        }
        throw std::runtime_error("Unclosed ");
    }

    ASTNode parseExpression(const std::vector<Token> &tokens, size_t start, size_t end)
    {
        return {};
    }

    ASTNode parseStatements(const std::vector<Token> &tokens, size_t pos, size_t end)
    {
        //TODO: bounds checking
        ASTNode root{.type = ASTNode::Type::Body};
        {
            //Step one: identify the type of statement
            for (; pos < end; ++pos)
            {
                auto type = tokens[pos].type;
                switch (type)
                {
                case TokenType::if_:
                { //Find condition
                    if (tokens[pos + 1].type != TokenType::lparen)
                    {
                        throw std::runtime_error("Expected ( after if");
                    }
                    size_t condition_end = findClosingBrace(tokens, pos + 1, TokenType::lparen);
                    auto condition = parseExpression(tokens, pos + 2, condition_end);
                    //Find body
                    if (tokens[condition_end + 1].type != TokenType::lbrace)
                    {
                        throw std::runtime_error("Expected { after if condition");
                    }
                    size_t body_end = findClosingBrace(tokens, condition_end + 1, TokenType::lbrace);
                    auto body = parseStatements(tokens, condition_end + 2, body_end);
                    ASTNode node{.type = ASTNode::Type::Branch, .value = Token{TokenType::if_}};
                    node.children.push_back(condition);
                    node.children.push_back(body);
                    //Check for else
                    if (tokens[body_end + 1].type == TokenType::else_)
                    {
                        if (tokens[body_end + 2].type != TokenType::lbrace)
                        {
                            throw std::runtime_error("Expected { after else");
                        }
                        size_t else_end = findClosingBrace(tokens, body_end + 2, TokenType::lbrace);
                        auto else_body = parseStatements(tokens, body_end + 3, else_end);
                        //Else just adds a second body node
                        node.children.push_back(else_body);
                        pos = else_end;
                    }
                    else
                    {
                        pos = body_end;
                    }
                    break;
                }
                default:
                    throw std::runtime_error("Unexpected token");
                }
            }
        }
        return root;
    }

    ASTNode parseFunction(std::pair<VWA::FunctionInfo, std::vector<VWA::Token>> &function)
    {
        auto &[info, tokens] = function;
        ASTNode root{.type = ASTNode::Type::Function};
        root.children.push_back({.type = ASTNode::Type::Value, .value = Token{TokenType::identifier, info.name}});
        root.children.push_back({.type = ASTNode::Type::Type, .value = std::tuple<VarType, CustomTypeInfo *, bool>{info.returnType, info.rtypeInfo, true}});
        root.children.push_back(parseStatements(tokens, 0, tokens.size()));
        for (auto &arg : info.args)
        {
            root.children.push_back({.type = ASTNode::Type::Type, .value = std::tuple<VarType, CustomTypeInfo *, bool>{arg.type, arg.typeInfo, arg.isMutable}});
        }
        return root;
    }

    ASTNode generateParseTree(pass1_result pass1)
    {
        ASTNode root{.type = ASTNode::Type::Program};
        {
            //Main needs to be at index 0
            auto main = pass1.functions.find("main");
            root.children.push_back(parseFunction(main->second));
            pass1.functions.erase(main);
        }
        for (auto &[_, function] : pass1.functions)
        {
            root.children.push_back(parseFunction(function));
        }
        return root;
    }

}
