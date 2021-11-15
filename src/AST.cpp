#include <AST.hpp>

namespace VWA
{
    AST::AST(const ParseTreeNode &root)
    {
        std::vector<std::pair<StructInfo &, const ParseTreeNode &>> structDeclarations;
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
                throw std::runtime_error("Invalid parse tree, only functions and structs are allowed at file level.");
            }
        }
        for (auto &struct_ : structDeclarations)
        {
            processStruct(struct_);
        }
        for (auto &child : root.children)
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
        for (auto &struct_ : structs)
            ComputeTypeSize(struct_.second);
        for (auto &func : functions)
            processFuncBody(func.second);
    }

    //TODO: remove return and just iterate over nodes again
    std::pair<StructInfo &, const ParseTreeNode &> AST::readStructDecl(const ParseTreeNode &_struct)
    {
        auto info = structs.find(std::get<std::string>(_struct.value.value));
        if (info != structs.end())
        {
            throw std::runtime_error("Struct " + std::get<std::string>(_struct.value.value) + " already defined");
        }
        auto &structInfo = structs[std::get<std::string>(_struct.value.value)];
        structInfo.name = std::get<std::string>(_struct.value.value);
        return {structInfo, _struct};
    }

    void AST::processStruct(std::pair<StructInfo &, const ParseTreeNode &> _struct)
    {
        auto &[info, structNode] = _struct;
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
    void AST::processFunc(const ParseTreeNode &func)
    {
        auto it = functions.find(std::get<std::string>(func.children[0].value.value));
        if (it != functions.end())
        {
            throw std::runtime_error("Function " + std::get<std::string>(func.children[0].value.value) + " already defined");
        }
        auto &info = functions[std::get<std::string>(func.value.value)];
        info.name = std::get<std::string>(func.value.value);
        info.returnType = getType(std::get<std::string>(func.children[0].value.value));
        info.bodyNode = &func.children[1];
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

    std::vector<uint64_t> FindScopeOffset(const Scope *scope)
    {
        std::vector<uint64_t> indices;
        while (scope->parent)
        {
            indices.push_back(scope->index);
            scope = scope->parent;
        }
        std::reverse(indices.begin(), indices.end());
        return indices;
    }

    Variable resolveVariableName(const std::string &name, const Scope *scope)
    {
        while (scope != nullptr)
        {
            auto it = std::find_if(scope->variables.begin(), scope->variables.end(), [&](const std::pair<std::string, VWA::TypeInfo> &var)
                                   {
                                       if (var.first == name)
                                       {
                                           return true;
                                       }
                                       return false;
                                   });
            if (it != scope->variables.end())
                return {it->second, FindScopeOffset(scope), (uint64_t)std::distance(scope->variables.begin(), it)};
            scope = scope->parent;
        }
        throw std::runtime_error("Variable " + name + " not found");
    }

    ASTNode AST::processDot(const ParseTreeNode &node, const Scope &scope)
    {
        auto var = resolveVariableName(std::get<std::string>(node.children[0].value.value), &scope);
        if (var.type.type != VarType::STRUCT)
        {
            throw std::runtime_error("Only structs can be accessed with dot operator");
        }
        ASTNode root{.type = NodeType::DOT, .data = std::vector<ASTNode>{}};
        auto &vec = std::get<std::vector<ASTNode>>(root.data);
        vec.reserve(node.children.size() + 1);
        vec.push_back({.type = NodeType::VARIABLE, .data = var});
        auto currentStruct = var.type.structInfo;
        for (size_t i = 1; i < node.children.size(); i++)
        {
            auto it = std::find_if(currentStruct->fields.begin(), currentStruct->fields.end(), [&](const StructInfo::Field &field)
                                   {
                                       if (field.name == std::get<std::string>(node.children[i].value.value))
                                       {
                                           return true;
                                       }
                                       return false;
                                   });
            if (it == currentStruct->fields.end())
            {
                throw std::runtime_error("Field " + std::get<std::string>(node.children[i].value.value) + " not found in struct " + currentStruct->name);
            }
            auto offset = std::distance(currentStruct->fields.begin(), it);
            vec.push_back({.type = NodeType::LONG_Val, .data = offset});
            currentStruct = it->type.structInfo;
            if (!currentStruct)
            {
                throw std::runtime_error("Field " + std::get<std::string>(node.children[i].value.value) + " is not a struct");
            }
        }
        return root;
    }

    ASTNode AST::processExpression(const ParseTreeNode &oldNode, const Scope &scope)
    {
        switch (oldNode.value.type)
        {
        case TokenType::not_:
        {
            auto child = processExpression(oldNode.children[0], scope);
            return {NodeType::NOT, std::vector<ASTNode>{child}};
        }
        case TokenType::identifier:
            return {NodeType::VARIABLE, resolveVariableName(std::get<std::string>(oldNode.value.value), &scope)};
        case TokenType::function_call:
        {
            //TODO: external linking
            ASTNode node{NodeType::FUNC_CALL, std::vector<ASTNode>{}};
            auto &vec = std::get<std::vector<ASTNode>>(node.data);
            auto funcName = std::get<std::string>(oldNode.value.value);
            vec.push_back({NodeType::STR_Val, funcName});
            auto func = functions.find(funcName);
            if (func == functions.end())
            {
                throw std::runtime_error("Function " + funcName + " not found");
            }
            for (size_t i = 0; i < oldNode.children.size(); ++i)
            {
                auto &child = oldNode.children[i];
                vec.emplace_back(processExpression(child, scope));
            }
            return node;
        }
        case TokenType::int_:
            return {NodeType::INT_Val, std::get<int>(oldNode.value.value)};
        case TokenType::float_:
            return {NodeType::FLOAT_Val, std::get<float>(oldNode.value.value)};
        case TokenType::string_literal:
            return {NodeType::STR_Val, std::get<std::string>(oldNode.value.value)};
        case TokenType::bool_:
            return {NodeType::BOOL_Val, std::get<bool>(oldNode.value.value)};
        case TokenType::long_:
            return {NodeType::LONG_Val, std::get<long>(oldNode.value.value)};
        case TokenType::char_:
            return {NodeType::CHAR_Val, std::get<char>(oldNode.value.value)};
        case TokenType::double_:
            return {NodeType::DOUBLE_Val, std::get<double>(oldNode.value.value)};
        case TokenType::minus:
        {
            if (oldNode.children.size() == 1)
            {
                //TODO: make intrinsic
                auto operand = processExpression(oldNode.children[0], scope);
                return {NodeType::MUL, std::vector<ASTNode>{operand, {NodeType::INT_Val, -1}}};
            }
        }
        case TokenType::plus:
        case TokenType::star:
        case TokenType::divide:
        case TokenType::mod:
        case TokenType::power:
        case TokenType::lt:
        case TokenType::gt:
        case TokenType::leq:
        case TokenType::geq:
        case TokenType::eq:
        case TokenType::neq:
        case TokenType::and_:
        case TokenType::or_:
        {
            auto lhs = processExpression(oldNode.children[0], scope);
            auto rhs = processExpression(oldNode.children[1], scope);
            NodeType type;
            switch (oldNode.value.type)
            {
            case TokenType::plus:
                type = NodeType::ADD;
                break;
            case TokenType::minus:
                type = NodeType::SUB;
                break;
            case TokenType::star:
                type = NodeType::MUL;
                break;
            case TokenType::divide:
                type = NodeType::DIV;
                break;
            case TokenType::mod:
                type = NodeType::MOD;
                break;
            case TokenType::power:
                type = NodeType::POW;
                break;
            case TokenType::lt:
                type = NodeType::LT;
                break;
            case TokenType::gt:
                type = NodeType::GT;
                break;
            case TokenType::leq:
                type = NodeType::LE;
                break;
            case TokenType::geq:
                type = NodeType::GE;
                break;
            case TokenType::eq:
                type = NodeType::EQ;
                break;
            case TokenType::neq:
                type = NodeType::NEQ;
                break;
            case TokenType::and_:
                type = NodeType::AND;
                break;
            case TokenType::or_:
                type = NodeType::OR;
                break;
            default:
                throw std::runtime_error("Unknown operator");
            }
            return {type, std::vector<ASTNode>{lhs, rhs}};
        }
        case TokenType::dot:
        {
            return processDot(oldNode, scope);
        }
        default:
            throw std::runtime_error("Unknown node " + oldNode.value.toString());
        }
    }

    ASTNode AST::processNode(const ParseTreeNode &oldNode, Scope &scope)
    {
        ASTNode newNode;
        switch (oldNode.value.type)
        {
        case TokenType::compound:
        {
            auto inner = scope.children.emplace_back();
            inner.index = scope.children.size() - 1;
            inner.parent = &scope;
            newNode.type = NodeType::BLOCK;
            newNode.data = std::vector<ASTNode>{};
            auto &vec = std::get<std::vector<ASTNode>>(newNode.data);
            for (auto &child : oldNode.children)
            {
                vec.emplace_back(processNode(child, inner));
            }
            break;
        }
        case TokenType::variable_declaration:
        {
            newNode.type = NodeType::DECL;
            bool is_Mutable = oldNode.children[0].value.type == TokenType::mutable_;
            auto &name = std::get<std::string>(oldNode.children[0 + is_Mutable].value.value);
            auto &typeName = std::get<std::string>(oldNode.children[1 + is_Mutable].value.value);
            auto typeInfo = getType(typeName);
            typeInfo.isMutable = is_Mutable;
            scope.variables.emplace_back(name, typeInfo);
            if (oldNode.children.size() == 3 + is_Mutable)
            {
                Variable var{.type = typeInfo, .offsets = FindScopeOffset(&scope), .scopeIndex = scope.variables.size()};
                newNode.data = std::vector<ASTNode>{{NodeType::VARIABLE, var}, processExpression(oldNode.children[2 + is_Mutable], scope)};
            }
            break;
        }
        case TokenType::assign:
        {
            auto &name = std::get<std::string>(oldNode.children[0].value.value);
            auto var = resolveVariableName(name, &scope);
            auto value = processExpression(oldNode.children[1], scope);
            newNode.type = NodeType::ASSIGN;
            newNode.data = std::vector<ASTNode>{{NodeType::VARIABLE, var}, value};
            break;
        }
        case TokenType::if_:
        {
            newNode.type = NodeType::IF;
            newNode.data = std::vector<ASTNode>{processExpression(oldNode.children[0], scope), processNode(oldNode.children[1], scope)};
            if (oldNode.children.size() == 3)
            {
                std::get<std::vector<ASTNode>>(newNode.data).emplace_back(processNode(oldNode.children[2], scope));
            }
            break;
        }
        case TokenType::while_:
        {
            newNode.type = NodeType::WHILE;
            newNode.data = std::vector<ASTNode>{processExpression(oldNode.children[0], scope), processNode(oldNode.children[1], scope)};
            break;
        }
        case TokenType::for_:
        {
            newNode.type = NodeType::FOR;
            newNode.data = std::vector<ASTNode>{processNode(oldNode.children[0], scope), processExpression(oldNode.children[1], scope), processNode(oldNode.children[2], scope), processNode(oldNode.children[3], scope)};
            break;
        }
        case TokenType::return_:
        {
            newNode.type = NodeType::RETURN;
            newNode.data = oldNode.children.size() == 1 ? std::vector<ASTNode>{processExpression(oldNode.children[0], scope)} : std::vector<ASTNode>{};
            break;
        }
        //Break and continue get resolved later on.
        case TokenType::break_:
        {
            newNode.type = NodeType::BREAK;
            break;
        }
        case TokenType::continue_:
        {
            newNode.type = NodeType::CONTINUE;
            break;
        }
        default:
            return processExpression(oldNode, scope);
        }
        return newNode;
    }

    void AST::processFuncBody(FunctionData &info)
    {
        info.scopes.variables.reserve(info.arguments.size());
        for (auto &arg : info.arguments)
        {
            info.scopes.variables.emplace_back(arg.name, arg.type);
            info.scopes.stackSize += getSizeOfType(arg.type);
        }
        info.body = processNode(*info.bodyNode, info.bodyNode->value.type == TokenType::compound ? info.scopes : info.scopes.children.emplace_back());
    }

    bool TypeCastPossible(const TypeInfo &from, const TypeInfo &to)
    {
        if (from.type == VarType::VOID || to.type == VarType::VOID)
            return false;
        if (from.type == to.type && from.structInfo == to.structInfo)
            return true;
        if (from.type > VarType::VOID && to.type > VarType::VOID && from.type < VarType::STRING && to.type < VarType::STRING)
            return true;
    }

    //TODO: integrate directly with compile part
    TypeInfo AST::expressionType(const ASTNode &node)
    {
        switch (node.type)
        {
        case NodeType::FUNC_CALL:
        {
            auto &vec = std::get<std::vector<ASTNode>>(node.data);
            auto func = functions.find(std::get<std::string>(vec[0].data));
            if (func == functions.end())
                throw std::runtime_error("Unknown function " + std::get<std::string>(vec[0].data));
            if (vec.size() + 1 != func->second.arguments.size())
                throw std::runtime_error("Wrong number of arguments for function " + std::get<std::string>(vec[0].data));
            for (size_t i = 1; i < vec.size(); i++)
            {
                auto argType = expressionType(vec[i]);
                if (!TypeCastPossible(argType, func->second.arguments[i].type))
                    throw std::runtime_error("Wrong type of argument " + std::to_string(i) + " for function " + std::get<std::string>(vec[0].data));
            }
            return func->second.returnType;
        }
        case NodeType::VARIABLE:
        {
            auto &var = std::get<Variable>(node.data);
            return var.type;
        }
        case NodeType::INT_Val:
            return {VarType::INT, false, nullptr};
        case NodeType::FLOAT_Val:
            return {VarType::FLOAT, false, nullptr};
        case NodeType::STR_Val:
            return {VarType::STRING, false, nullptr};
        case NodeType::BOOL_Val:
            return {VarType::BOOL, false, nullptr};
        case NodeType::CHAR_Val:
            return {VarType::CHAR, false, nullptr};
        case NodeType::LONG_Val:
            return {VarType::LONG, false, nullptr};
        case NodeType::DOUBLE_Val:
            return {VarType::DOUBLE, false, nullptr};
        case NodeType::ADD:
        case NodeType::SUB:
        case NodeType::MUL:
        case NodeType::DIV:
        case NodeType::POW:
        {
            auto lhs = expressionType(std::get<std::vector<ASTNode>>(node.data)[0]);
            auto rhs = expressionType(std::get<std::vector<ASTNode>>(node.data)[1]);
            //TODO:cast in this order int->long->float->double
        }
        default:
            throw std::runtime_error("Unknown expression type");
        }
    }
    void AST::ValidateNode(const ASTNode &node) {}

};