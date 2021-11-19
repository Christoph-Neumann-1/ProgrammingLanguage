#pragma once
#include <AST.hpp>
#include <Imports.hpp>

namespace VWA
{
    class Compiler
    {

        //TODO: args and state
        std::unordered_map<std::string, instruction::ByteCodeElement *> functions;
        std::vector<instruction::ByteCodeElement> bytecode;
        void compileNode(const ASTNode &node, const FunctionData &func);
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
                for (auto i = var.scopeIndex - 1; i >= 0; --i)
                {
                    offset += getSizeOfType(scope->variables[i].second);
                }
            return offset;
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
};