#pragma once
#include <AST.hpp>
#include <Imports.hpp>
#include <stdexcept>
#include <Error.hpp>

namespace VWA
{
    class Compiler
    {

        //TODO: args and state
        Imports::ImportedFileData data;
        std::vector<instruction::ByteCodeElement> bytecode;
        const TypeInfo compileNode(const ASTNode &node, const FunctionData &func, const Scope *scope);
        void compileFunction(const FunctionData &func);
        template <typename T>
        void writeBytes(T value)
        {
            uint8_t *ptr = (uint8_t *)&value;
            for (int i = 0; i < sizeof(T); i++)
            {
                bytecode.push_back({ptr[i]});
            }
        }
        uint64_t getVarOffset(const Variable &var, const Scope &mainScope)
        {
            const Scope *scope = &mainScope;
            uint64_t offset = sizeof(instruction::ByteCodeElement *) + sizeof(uint8_t *);
            for (int i = 0; i < var.offsets.size(); ++i)
            {
                offset += scope->stackSize;
                scope = &scope->children[var.offsets[i]];
            }
            if (var.scopeIndex)
                for (auto i = var.scopeIndex - 1; i + 1 > 0; --i)
                {
                    offset += getSizeOfType(scope->variables[i].second);
                }
            return offset;
        }

        std::pair<uint64_t, TypeInfo> getStructMemberOffset(const std::vector<ASTNode> &list, const Scope &scope, uint currentIndex = 0, StructInfo *structInfo = nullptr)
        {
            if (!currentIndex)
            {
                auto &var = std::get<Variable>(list.begin()->data);
                auto res = getStructMemberOffset(list, scope, 1, var.type.structInfo);
                res.first += getVarOffset(var, scope);
                return res;
            }
            auto previousSize = 0;
            for (auto i = std::get<long>(list[currentIndex].data) - 1; i + 1 > 0; --i)
            {
                previousSize += getSizeOfType(structInfo->fields[i].type);
            }
            if (currentIndex + 1 < list.size())
            {
                auto next = getStructMemberOffset(list, scope, currentIndex + 1, structInfo);
                return {previousSize + next.first, next.second};
            }
            return {previousSize, structInfo->fields[std::get<long>(list[currentIndex].data)].type};
        }

        void CastToType(const TypeInfo &from, const TypeInfo &to)
        {
            if (from.type == VarType::STRUCT || to.type == VarType::STRUCT)
                throw std::runtime_error("Cannot cast structs");
            if (from.type == VarType::STRING || to.type == VarType::STRING)
                throw std::runtime_error("Cannot cast strings");
            if (from.type == VarType::VOID || to.type == VarType::VOID)
                throw std::runtime_error("Cannot cast void");
            switch (from.type)
            {
            case VarType::CHAR:
            case VarType::BOOL:
                switch (to.type)
                {
                case VarType::BOOL:
                case VarType::CHAR:
                    break;
                case VarType::INT:
                    bytecode.push_back({instruction::CtoI});
                    break;
                case VarType::FLOAT:
                    bytecode.push_back({instruction::CtoF});
                    break;
                case VarType::LONG:
                    bytecode.push_back({instruction::CtoL});
                    break;
                case VarType::DOUBLE:
                    bytecode.push_back({instruction::CtoD});
                    break;
                default:
                    break;
                }
                break;
            case VarType::INT:
                switch (to.type)
                {
                case VarType::BOOL:
                case VarType::CHAR:
                    bytecode.push_back({instruction::ItoC});
                    break;
                case VarType::INT:
                    break;
                case VarType::FLOAT:
                    bytecode.push_back({instruction::ItoF});
                    break;
                case VarType::LONG:
                    bytecode.push_back({instruction::ItoL});
                    break;
                case VarType::DOUBLE:
                    bytecode.push_back({instruction::ItoD});
                    break;
                default:
                    break;
                }
                break;
            case VarType::FLOAT:
                switch (to.type)
                {
                case VarType::BOOL:
                case VarType::CHAR:
                    bytecode.push_back({instruction::FtoC});
                    break;
                case VarType::INT:
                    bytecode.push_back({instruction::FtoI});
                    break;
                case VarType::FLOAT:
                    break;
                case VarType::LONG:
                    bytecode.push_back({instruction::FtoL});
                    break;
                case VarType::DOUBLE:
                    bytecode.push_back({instruction::FtoD});
                    break;
                default:
                    break;
                }
                break;
            case VarType::LONG:
                switch (to.type)
                {
                case VarType::BOOL:
                case VarType::CHAR:
                    bytecode.push_back({instruction::LtoC});
                    break;
                case VarType::INT:
                    bytecode.push_back({instruction::LtoI});
                    break;
                case VarType::FLOAT:
                    bytecode.push_back({instruction::LtoF});
                    break;
                case VarType::LONG:
                    break;
                case VarType::DOUBLE:
                    bytecode.push_back({instruction::LtoD});
                    break;
                default:
                    break;
                }
                break;
            case VarType::DOUBLE:
                switch (to.type)
                {
                case VarType::BOOL:
                case VarType::CHAR:
                    bytecode.push_back({instruction::DtoC});
                    break;
                case VarType::INT:
                    bytecode.push_back({instruction::DtoI});
                    break;
                case VarType::FLOAT:
                    bytecode.push_back({instruction::DtoF});
                    break;
                case VarType::LONG:
                    bytecode.push_back({instruction::DtoL});
                    break;
                case VarType::DOUBLE:
                    break;
                default:
                    break;
                }
                break;
            default:
                break;
            }
        }

        //This function assumes that a value of type is already on the stack. If its importance is lower than otherType it will be cast.
        //This only works on primitive types.
        //This should be done before invoking the compiler
        const TypeInfo PerformTypePromotion(const TypeInfo &type, const TypeInfo &otherType)
        {
            switch (type.type)
            {
            case VarType::BOOL:
            case VarType::CHAR:
                switch (otherType.type)
                {
                case VarType::BOOL:
                case VarType::CHAR:
                    return type;
                default:
                    break;
                }

            case VarType::INT:
                switch (otherType.type)
                {
                case VarType::BOOL:
                case VarType::CHAR:
                case VarType::INT:
                    return type;
                default:
                    break;
                }
            case VarType::FLOAT:
                switch (otherType.type)
                {
                case VarType::BOOL:
                case VarType::CHAR:
                case VarType::INT:
                case VarType::FLOAT:
                case VarType::LONG:
                    return type;
                default:
                    break;
                }
            case VarType::LONG:
                switch (otherType.type)
                {
                case VarType::BOOL:
                case VarType::CHAR:
                case VarType::INT:
                case VarType::LONG:
                    return type;
                default:
                    break;
                }
            default:
                break;
            }
            CastToType(type, otherType);
            return otherType;
        }

    public:
        Compiler() = default;
        Compiler(const Compiler &) = delete;
        Compiler(Compiler &&) = delete;
        Compiler &operator=(const Compiler &) = delete;
        Compiler &operator=(Compiler &&) = delete;
        ~Compiler() = default;
        Imports::ImportedFileData compile(const AST &ast);
    };
}