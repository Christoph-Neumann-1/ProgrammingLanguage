#include <Compiler.hpp>
#include <cstring>
//TODO: panic mode

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
    void Compiler::compile(const AST &ast)
    {
        data.internalStart = data.importedFunctions.size();
        for (auto &func : ast.functions)
        {
            //TODO: args
            data.importedFunctions.push_back({func.first, Imports::ImportedFileData::FuncDef{.name = func.first, .returnType = AST::typeAsString(func.second.returnType), .isC = false}});
        }
        for (auto &func : ast.functions)
        {
            compileFunction(func.second);
        }
        data.bc = std::make_unique<instruction::ByteCodeElement[]>(bytecode.size());
        std::memcpy(data.bc.get(), bytecode.data(), bytecode.size() * sizeof(instruction::ByteCodeElement));
        data.bcSize = bytecode.size();
        if (auto it = std::find_if(data.importedFunctions.begin() + data.internalStart, data.importedFunctions.end(),
                                   [](const std::pair<std::string, VWA::Imports::ImportedFileData::FuncDef> &val)
                                   { return val.first == "main"; });
            it != data.importedFunctions.end())
        {
            data.main = it->second.bc;
            data.hasMain = true;
        }
        else
        {
            data.main = 0;
            data.hasMain = false;
        }
        data.staticLink();
        manager.makeModuleAvailable("main", std::move(data));
    }

    void Compiler::compileFunction(const FunctionData &func)
    {
        auto addr = bytecode.size();
        compileNode(func.body, func, &func.scopes);
        if (func.name == "main")
        {
            bytecode.push_back({instruction::PushConst32});
            writeBytes(0);
            bytecode.push_back({instruction::Return});
            writeBytes((uint64_t)4);
        }
        auto f = std::find_if(data.importedFunctions.begin() + data.internalStart, data.importedFunctions.end(),
                              [func](const std::pair<std::string, VWA::Imports::ImportedFileData::FuncDef> &val)
                              { return val.first == func.name; });
        f->second.bc = (instruction::ByteCodeElement *)addr;
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
            uint64_t tSize, tOffset;
            TypeInfo tType;
            if (std::holds_alternative<Variable>(vec[0].data))
            {
                auto &var = std::get<Variable>(vec[0].data);
                tSize = getSizeOfType(var.type);
                tOffset = getVarOffset(var, func.scopes);
                tType = var.type;
            }
            else
            {
                auto [tOffset_, tType_] = getStructMemberOffset(std::get<std::vector<ASTNode>>(vec[0].data), func.scopes);
                tSize = getSizeOfType(tType_);
                tOffset = tOffset_;
                tType = tType_;
            }
            auto exprType = compileNode(vec[1], func, scope);
            if (exprType.type != tType.type)
                CastToType(exprType, tType);
            bytecode.push_back({instruction::WriteLocal});
            writeBytes(tSize);
            writeBytes(tOffset);
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
        case NodeType::DOT:
        {
            auto pos = getStructMemberOffset(std::get<std::vector<ASTNode>>(node.data), func.scopes);
            bytecode.push_back({instruction::ReadLocal});
            writeBytes(getSizeOfType(pos.second));
            writeBytes(pos.first);
            return pos.second;
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
        case NodeType::BOOL_Val:
        case NodeType::CHAR_Val:
        {
            bytecode.push_back({instruction::PushConst8});
            writeBytes<char>(std::holds_alternative<bool>(node.data) ? std::get<bool>(node.data) : std::get<char>(node.data));
            return {VarType::CHAR, false, nullptr};
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
        case NodeType::FUNC_CALL:
        {
            //TODO: remove JumpInternal resolve names during linking stage
            auto &vec = std::get<std::vector<ASTNode>>(node.data);
            auto funcName = std::get<std::string>(vec[0].data);

            Imports::ImportedFileData::FuncDef *definition;
            auto f = std::find_if(data.importedFunctions.begin(), data.importedFunctions.end(),
                                  [&funcName](const auto &f)
                                  { return f.first == funcName; });
            if (f == data.importedFunctions.end())
            {
                auto stdlib = manager.getModule("stdlib");
                auto sym = stdlib->exportedFunctions.find(funcName);
                if (sym == stdlib->exportedFunctions.end())
                    throw std::runtime_error("Function not found");
                definition = &sym->second;
            }
            else
            {
                definition = &f->second;
            }
            TypeInfo rtype;
            if (definition->returnType != "int")
                throw std::runtime_error("Function return type not supported");
            rtype = {VarType::INT, false, nullptr};
            for (int i = 1; i < vec.size(); i++)
            {
                auto &arg = vec[i];
                auto argType = compileNode(arg, func, scope);
                if (argType.type != rtype.type)
                    CastToType(argType, rtype);
            }
            auto argSize = sizeof(int) * (vec.size() - 1);
            if (definition->isC)
            {
                bytecode.push_back({instruction::JumpFFI});
                writeBytes(definition->func);
                writeBytes<uint64_t>(argSize);
            }
            else
            {
                bytecode.push_back({instruction::FCall});
                writeBytes(std::distance(data.importedFunctions.begin(), f));
                writeBytes<uint64_t>(argSize);
            }
            return rtype;
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
        case NodeType::NOT:
        {

            if (auto t = compileNode(std::get<std::vector<ASTNode>>(node.data)[0], func, scope); t.type != VarType::BOOL)
                CastToType(t, {VarType::BOOL});
            bytecode.push_back({instruction::Not});
            return {VarType::BOOL, false, nullptr};
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
            compileNode(vec[0], func, nextScope);
            auto condAddr = bytecode.size();
            compileNode(vec[1], func, nextScope);
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
        case NodeType::WHILE:
        {
            auto &vec = std::get<std::vector<ASTNode>>(node.data);
            auto condAddr = bytecode.size();
            compileNode(vec[0], func, scope);
            bytecode.push_back({instruction::JumpIfFalse});
            auto addr1 = bytecode.size();
            writeBytes(uint64_t(0));
            compileNode(vec[1], func, scope);
            bytecode.push_back({instruction::Jump});
            writeBytes(condAddr);
            auto end = bytecode.size();
            *(uint64_t *)&bytecode[addr1] = end;
            break;
        }
        case NodeType::NOOP:
            break;
        }
        return {VarType::VOID, false, nullptr};
    }

}
#undef BinaryMathOp