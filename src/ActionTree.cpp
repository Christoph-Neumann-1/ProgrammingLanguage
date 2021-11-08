#include <ActionTree.hpp>

namespace VWA
{
    ActionTree::ActionTree(const ASTNode &root)
    {
        std::vector<std::pair<StructInfo&,const ASTNode&>> structDeclarations;
        for (auto &child : root.children)
        {
            switch (child.value.type)
            {
            case TokenType::struct_definition:
                structDeclarations.push_back(readStructDecl(child));
                break;
            case TokenType::function_definition:
            break;
            default:
                throw std::runtime_error("Invalid ast, only functions and structs are allowed at file level.");
            }
        }
        for(auto &struct_:structDeclarations)
        {
            processStruct(struct_);
        }
        for(auto&child:root.children)
        {
            switch (child.value.type)
            {
            case TokenType::struct_definition:
                break;
            case TokenType::function_definition:
                processFunc(child);
                break;
            default:
                throw std::runtime_error("How?");
            }
        }
        for(auto &struct_:structs)
        {
            ComputeTypeSize(struct_.second);
        }
    }

    //TODO: remove return and just iterate over nodes again
    std::pair<StructInfo &, const ASTNode &> ActionTree::readStructDecl(const ASTNode &_struct)
    {
        auto info=structs.find(std::get<std::string>(_struct.value.value));
        if(info!=structs.end())
        {
            throw std::runtime_error("Struct "+std::get<std::string>(_struct.value.value)+" already defined");
        }
        auto &structInfo=structs[std::get<std::string>(_struct.value.value)];
        structInfo.name=std::get<std::string>(_struct.value.value);
        return {structInfo,_struct};
    }

    void ActionTree::processStruct(std::pair<StructInfo &, const ASTNode &> _struct)
    {
        auto &[info,structNode]=_struct;
        for (auto &child : structNode.children)
        {
            bool is_Mutable = child.children[0].value.type == TokenType::mutable_;
            auto name = std::get<std::string>(child.children[0 + is_Mutable].value.value);
            auto typeName = std::get<std::string>(child.children[1 + is_Mutable].value.value);
            auto type = getType(typeName);
            type.isMutable = is_Mutable;
            info.fields.push_back({name, type});
        }
    }
    void ActionTree::processFunc(const ASTNode &func)
    {
        auto &info = functions[std::get<std::string>(func.value.value)];
        info.name = std::get<std::string>(func.value.value);
        info.returnType = getType(std::get<std::string>(func.children[0].value.value));
        auto &body = func.children[1]; //TODO: parse
        for (size_t i = 2; i < func.children.size(); i++)
        {
            auto &child = func.children[i];
            bool is_Mutable = child.children[0].value.type == TokenType::mutable_;
            auto name = std::get<std::string>(child.children[0 + is_Mutable].value.value);
            auto typeName = std::get<std::string>(child.children[1 + is_Mutable].value.value);
            auto type = getType(typeName);
            type.isMutable = is_Mutable;
            info.arguments.push_back({name, type});
        }
        if (info.name == "main")
        {
            if (info.returnType.type != VarType::VOID && info.returnType.type != VarType::INT)
            {
                throw std::runtime_error("main function must return int or void");
            }
            if (info.arguments.size() != 0)
            {
                throw std::runtime_error("main may not have any arguments as of now.");
            }
            main = &info;
        }
    }

};