#include <Compiler.hpp>
#include <cstring>

#define BinaryMathOp(name)                                                                  \
    {                                                                                       \
        auto &lhs = compileNode(std::get<std::vector<ASTNode>>(node.data)[0], func, scope); \
        auto &rhs = compileNode(std::get<std::vector<ASTNode>>(node.data)[1], func, scope); \
        auto resType = PerformTypePromotion(rhs, lhs);                                      \
        auto rhSize = getSizeOfType(rhs);                                                   \
        auto lhSize = getSizeOfType(lhs);                                                   \
        if (resType.type != lhs.type)                                                       \
        {                                                                                   \
            bytecode.push_back({instruction::Dup});                                         \
            writeBytes(lhSize + rhSize);                                                    \
            writeBytes(lhSize);                                                             \
            CastToType(lhs, resType);                                                       \
            bytecode.push_back({instruction::Dup});                                         \
            writeBytes(2 * rhSize);                                                         \
            writeBytes(rhSize);                                                             \
        }                                                                                   \
        switch (resType.type)                                                               \
        {                                                                                   \
        case VarType::INT:                                                                  \
            bytecode.push_back({instruction::name##I});                                     \
            break;                                                                          \
        case VarType::FLOAT:                                                                \
            bytecode.push_back({instruction::name##F});                                     \
            break;                                                                          \
        case VarType::LONG:                                                                 \
            bytecode.push_back({instruction::name##L});                                     \
            break;                                                                          \
        case VarType::DOUBLE:                                                               \
            bytecode.push_back({instruction::name##D});                                     \
            break;                                                                          \
        default:                                                                            \
            throw std::runtime_error("Invalid type");                                       \
        }                                                                                   \
        if (lhs.type != resType.type)                                                       \
        {                                                                                   \
            bytecode.push_back({instruction::PopMiddle});                                   \
            writeBytes(rhSize);                                                             \
            writeBytes(lhSize + rhSize);                                                    \
        }                                                                                   \
        return resType;                                                                     \
    }
namespace VWA
{
    Imports::ImportedFileData Compiler::compile(const AST &ast)
    {
        for (auto &func : ast.functions)
        {
            compileFunction(func.second);
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
        if (func.name == "main")
        {
            bytecode.push_back({instruction::PushConst32});
            writeBytes(0);
            bytecode.push_back({instruction::Return});
            writeBytes((uint64_t)4);
        }
        functions[func.name] = (instruction::ByteCodeElement *)addr;
    }
    const TypeInfo Compiler::compileNode(const ASTNode &node, const FunctionData &func, const Scope *scope)
    {
        uint counter = 0;
        switch (node.type)
        {
        case NodeType::DECL:
        {
            if (std::get_if<std::vector<ASTNode>>(&node.data))
            {
                auto &exprType = compileNode(std::get<std::vector<ASTNode>>(node.data)[1], func, scope);
                auto &type = std::get<Variable>(std::get<std::vector<ASTNode>>(node.data)[0].data);
                if (exprType.type != type.type.type)
                    CastToType(exprType, type.type);
                bytecode.push_back({instruction::WriteLocal});
                writeBytes(getSizeOfType(type.type));
                auto addr = getVarOffset(type, func.scopes);
                writeBytes(addr);
            }
            break;
        }
        case NodeType::ASSIGN:
        {
            auto &vec = std::get<std::vector<ASTNode>>(node.data);
            auto &var = std::get<Variable>(vec[0].data);
            auto exprType = compileNode(vec[1], func, scope);
            if (exprType.type != var.type.type)
                CastToType(exprType, var.type);
            bytecode.push_back({instruction::WriteLocal});
            writeBytes(getSizeOfType(var.type));
            writeBytes(getVarOffset(var, func.scopes));
            break;
        }
        case NodeType::VARIABLE:
        {
            bytecode.push_back({instruction::ReadLocal});
            auto &type = std::get<Variable>(node.data);
            writeBytes(getSizeOfType(type.type));
            writeBytes(getVarOffset(type, func.scopes));
            return type.type;
        }
        case NodeType::INT_Val:
        {
            bytecode.push_back({instruction::PushConst32});
            writeBytes(std::get<int>(node.data));
            return {VarType::INT, false, nullptr};
        }
        case NodeType::FLOAT_Val:
        {
            bytecode.push_back({instruction::PushConst32});
            writeBytes(std::get<float>(node.data));
            return {VarType::FLOAT, false, nullptr};
        }
        case NodeType::LONG_Val:
        {
            bytecode.push_back({instruction::PushConst64});
            writeBytes(std::get<long>(node.data));
            return {VarType::LONG, false, nullptr};
        }
        case NodeType::DOUBLE_Val:
        {
            bytecode.push_back({instruction::PushConst64});
            writeBytes(std::get<double>(node.data));
            return {VarType::DOUBLE, false, nullptr};
        }
        case NodeType::ADD:
            BinaryMathOp(Add);
        case NodeType::SUB:
            BinaryMathOp(Subtract);
        case NodeType::MUL:
            BinaryMathOp(Multiply);
        case NodeType::DIV:
            BinaryMathOp(Divide);
        case NodeType::POW:
            BinaryMathOp(Power);
        case NodeType::MOD:
        {
            auto &lhs = compileNode(std::get<std::vector<ASTNode>>(node.data)[0], func, scope);
            auto &rhs = compileNode(std::get<std::vector<ASTNode>>(node.data)[1], func, scope);
            auto resType = PerformTypePromotion(rhs, lhs);
            auto rhSize = getSizeOfType(rhs);
            auto lhSize = getSizeOfType(lhs);
            if (resType.type != lhs.type)
            {
                bytecode.push_back({instruction::Dup});
                writeBytes(lhSize + rhSize);
                writeBytes(lhSize);
                CastToType(lhs, resType);
                bytecode.push_back({instruction::Dup});
                writeBytes(2 * rhSize);
                writeBytes(rhSize);
            }
            switch (resType.type)
            {
            case VarType::INT:
                bytecode.push_back({instruction::ModuloI});
                break;

            case VarType::LONG:
                bytecode.push_back({instruction::ModuloL});
                break;
            default:
                throw std::runtime_error("Invalid type");
            }
            if (lhs.type != resType.type)
            {
                bytecode.push_back({instruction::PopMiddle});
                writeBytes(rhSize);
                writeBytes(lhSize + rhSize);
            }
            return resType;
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
            auto &vec = std::get<std::vector<ASTNode>>(node.data);
            if (vec.size())
            {
                auto exprType = compileNode(vec[0], func, scope);
                if (exprType.type != func.returnType.type)
                    CastToType(exprType, func.returnType);
            }
            bytecode.push_back({instruction::Return});
            writeBytes(getSizeOfType(func.returnType));
            break;
        }
        case NodeType::LT:
        {
            compileNode(std::get<std::vector<ASTNode>>(node.data)[0], func, scope);
            compileNode(std::get<std::vector<ASTNode>>(node.data)[1], func, scope);
            bytecode.push_back({instruction::LessThanI});
            break;
        }
        case NodeType::IF:
        {
            auto &vec = std::get<std::vector<ASTNode>>(node.data);
            bool hasElse = vec.size() == 3;
            compileNode(vec[0], func, scope);
            bytecode.push_back({instruction::JumpIfFalse});
            auto addr1 = bytecode.size();
            writeBytes(uint64_t(0));
            compileNode(vec[1], func, scope);
            uint64_t addr2;
            if (hasElse)
            {
                bytecode.push_back({instruction::Jump});
                addr2 = bytecode.size();
                writeBytes(uint64_t(0));
            }
            auto endIf = bytecode.size();
            *(uint64_t *)&bytecode[addr1] = endIf;
            if (hasElse)
            {
                compileNode(vec[2], func, scope);
                *(uint64_t *)&bytecode[addr2] = bytecode.size();
            }
            break;
        }
        case NodeType::FOR:
        {
            auto &vec = std::get<std::vector<ASTNode>>(node.data);
            auto nextScope = &scope->children[counter++];
            bytecode.push_back({instruction::Push});
            writeBytes(nextScope->stackSize);
            compileNode(vec[0], func, scope);
            auto condAddr = bytecode.size();
            compileNode(vec[1], func, scope);
            bytecode.push_back({instruction::JumpIfFalse});
            auto addr1 = bytecode.size();
            writeBytes(uint64_t(0));
            compileNode(vec[3], func, nextScope);
            compileNode(vec[2], func, nextScope);
            bytecode.push_back({instruction::Jump});
            writeBytes(condAddr);
            auto endFor = bytecode.size();
            *(uint64_t *)&bytecode[addr1] = endFor;
            bytecode.push_back({instruction::Pop});
            writeBytes(nextScope->stackSize);
            break;
        }
        case NodeType::NOOP:
            break;
        }

        return {VarType::VOID, false, nullptr};
    }

}
#undef BinaryMathOp