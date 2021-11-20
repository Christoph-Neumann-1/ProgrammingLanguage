#include <Compiler.hpp>
#include <cstring>

namespace VWA
{
    Imports::ImportedFileData Compiler::compile(const AST &ast)
    {
        for (auto &func : ast.functions)
        {
            compileFunction(func.second);
            if (func.first == "main")
            {
                bytecode.push_back({instruction::PushConst32});
                for (int i = 0; i < 4; ++i)
                    bytecode.push_back({0});
                bytecode.push_back({instruction::Return});
            }
        }
        Imports::ImportedFileData fileData;
        fileData.bc = std::make_unique<instruction::ByteCodeElement[]>(bytecode.size());
        std::memcpy(fileData.bc.get(), bytecode.data(), bytecode.size() * sizeof(instruction::ByteCodeElement));
        fileData.bcSize = bytecode.size();
        if (auto it = functions.find("main"); it != functions.end())
        {
            fileData.main = it->second;
            fileData.hasMain = true;
        }
        else
        {
            fileData.main = 0;
            fileData.hasMain = false;
        }
        return fileData;
    }

    void Compiler::compileFunction(const FunctionData &func)
    {
        auto addr = bytecode.size();
        // bytecode.push_back({instruction::PushConst32});
        // bytecode.push_back({4});
        // for (int i = 0; i < 3; i++)
        // {
        //     bytecode.push_back({0});
        // }
        // bytecode.push_back({instruction::Return});
        // bytecode.push_back({4});
        // for (int i = 0; i < 7; i++)
        // {
        //     bytecode.push_back({0});
        // }
        compileNode(func.body, func, &func.scopes);
        functions[func.name] = (instruction::ByteCodeElement *)addr;
    }
    void Compiler::compileNode(const ASTNode &node, const FunctionData &func, const Scope *scope)
    {
        uint counter = 0;
        switch (node.type)
        {
        case NodeType::DECL:
        {
            compileNode(std::get<std::vector<ASTNode>>(node.data)[1], func, scope);
            bytecode.push_back({instruction::WriteLocal});
            auto &type = std::get<Variable>(std::get<std::vector<ASTNode>>(node.data)[0].data);
            writeBytes(getSizeOfType(type.type));
            auto addr = getVarOffset(type, func.scopes);
            writeBytes(addr);
            break;
        }
        case NodeType::INT_Val:
        {
            bytecode.push_back({instruction::PushConst32});
            writeBytes(std::get<int>(node.data));
            break;
        }
        case NodeType::ADD:
        {
            compileNode(std::get<std::vector<ASTNode>>(node.data)[0], func, scope);
            compileNode(std::get<std::vector<ASTNode>>(node.data)[1], func, scope);
            bytecode.push_back({instruction::AddI});
            break;
        }
        case NodeType::BLOCK:
        {
            auto nextScope = &scope->children[counter++];
            bytecode.push_back({instruction::Push});
            writeBytes(nextScope->stackSize);
            for (auto &child : std::get<std::vector<ASTNode>>(node.data))
            {
                compileNode(child, func, nextScope);
            }
            bytecode.push_back({instruction::Pop});
            writeBytes(nextScope->stackSize);
            break;
        }
        case NodeType::RETURN:
        {
            auto vec = std::get<std::vector<ASTNode>>(node.data);
            if (vec.size())
                compileNode(vec[0], func, scope);
            bytecode.push_back({instruction::Return});
            writeBytes(getSizeOfType(func.returnType));
            break;
        }
        }
    }

}