#include <Parser.hpp>
namespace VWA
{
    std::pair<VarType, CustomTypeInfo *> GetType(const std::string &type, pass1_result &result)
    {
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
                it.fields.push_back({name,isMutable,type_pair.first,type_pair.second});
                end += 3 + isMutable;
                continue;
            }
            throw std::runtime_error("Unexpected token " + tokens[end].toString());
        }
        pos=end;
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
            default:
                continue;
            }
        }

        return result;
    }
}
