#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <variant>
#include <Node.hpp>
#include <optional>
#include <sstream>

//TODO: implicit return in void functions

/*
Idea for AST:
Breaks and continue are handled at compile time and just cause a jump to another node. Data is stored on a stack and the pointer is just moved.
As for returning from functions, the return address should probably be stored on the stack as well. There are no destructors, so everything should be simple enough.
Heap allocation needs to be handled by the user anyways, so it's not my problem. I need to be careful with cyclic references in structs though. I don't want an infinite loop.
Maybe a compile time check and a maximun recursion depth for types?
Now I also need to handle references to existing values. I will do that by storing the address of the value. Whether something is a reference should be known at compile time.
At first I will try to pass all parameters by value, later I will add referneces to that and to local variables. I don't know if I should allow references in structs.
Maybe pointers aren't needed after all. new and delete could just use references.
In order for the stack to work, the size of the types should be known, while there are ways to circumvent that, they are incredibly stupid. 
I can just calculate the size recursivelly.
Also I should remember to add arrays in the future.
Ideally there won't be any recursive functions modelling function calls, just simple jumps to other nodes and later instructions.
TODO: handle reference types (let mut x&:int=y;)First the keyword, then the usual modifiers, then the name. In case of a refernence,
this is followed by a &. The type remains int and it can be used exactly like a normal variable, but internally it is no longer a simple offset,
instead it is a index into the stack, but this index is independent of the current scope. There is no reference to a reference.
References may be reassigned, but they may not point to a constant or expression.
If you do something like let mut a:int; let mut b&:int=a; let mut c&:int=b; The variable c will point to a as well.
The type of references must match. Mut may be omitted for a constant reference, but it may not be added if the value refered to is not mutable.
*/

//TODO: lookup tables instead of map of strings

namespace VWA
{
    //The idea for the action tree is to bring everything into a more useful format, as well as to resolve symbols and therefore allow for
    //Type checking.
    enum class VarType : uint64_t
    {
        VOID,
        INT,
        LONG,
        FLOAT,
        DOUBLE,
        CHAR,
        BOOL,
        STRING,
        STRUCT
    };
    struct StructInfo;
    struct TypeInfo
    {
        VarType type;
        bool isMutable;
        StructInfo *structInfo;
    };
    struct Scope
    {
        //TODO: precompute variable offsets from base pointer
        std::vector<std::pair<std::string, TypeInfo>> variables;
        uint64_t stackSize = 0;
        std::vector<Scope> children;
        Scope *parent = nullptr;
        uint64_t index;
    };
    //A variable refers to a previously declared variable. It is then used by the compiler and the optimizer
    //TODO: consider reverting to offset from stack pointer
    //TODO: add reference to scope
    struct Variable
    {
        TypeInfo type;
        std::vector<uint64_t> offsets; //The indices refer to the children in the scopes. An empty vector means it is a argument.
        uint64_t scopeIndex;           //The index of the variable in the scope.
    };
    struct StructInfo
    {
        std::string name;
        struct Field
        {
            const std::string name;
            const TypeInfo type;
        };
        std::vector<Field> fields;
        size_t size = 0; //This can't be calculated immediately, because the structs it depends on may not have been processed yet. Therefore it is calculated in a seperate pass.
        //0 means it is not calculated yet. The minimus size is therefore 1.
    };

    enum class NodeType
    {
        NOOP,
        BLOCK,
        IF,
        WHILE,
        FOR,
        DECL,
        ASSIGN,
        RETURN,
        BREAK,
        CONTINUE,
        FUNC_CALL,
        STR_Val,
        INT_Val,
        LONG_Val,
        FLOAT_Val,
        DOUBLE_Val,
        CHAR_Val,
        BOOL_Val,
        VARIABLE,
        DOT,
        ADD,
        SUB,
        MUL,
        DIV,
        MOD,
        POW,
        EQ,
        NEQ,
        GT,
        LT,
        GE,
        LE,
        AND,
        OR,
        NOT,
        SIZEOF,
    };

    //Would subclasses help with evaluation?
    struct ASTNode
    {
        NodeType type;
        std::variant<std::monostate, std::string, int, long, float, double, char, bool, std::vector<ASTNode>, Variable> data;
    };

    struct FunctionData
    {
        std::string name;
        struct Argument
        {
            const std::string name;
            const TypeInfo type;
        };
        std::vector<Argument> arguments;
        TypeInfo returnType;
        ASTNode body;
        Scope scopes;
        const ParseTreeNode *bodyNode;
    };

    inline size_t getSizeOfType(const TypeInfo &type)
    {
        switch (type.type)
        {
        case VarType::VOID:
            throw std::runtime_error("Can't get size of void type");
        case VarType::INT:
            return sizeof(int);
        case VarType::LONG:
            return sizeof(long);
        case VarType::FLOAT:
            return sizeof(float);
        case VarType::DOUBLE:
            return sizeof(double);
        case VarType::CHAR:
            return sizeof(char);
        case VarType::STRING:
            throw std::runtime_error("Not implemented");
        case VarType::BOOL:
            return sizeof(bool);
        case VarType::STRUCT:
            return type.structInfo->size;
        }
    }

    struct AST
    {
        std::unordered_map<std::string, StructInfo> structs;
        std::unordered_map<std::string, FunctionData> functions;

        size_t ComputeTypeSize(StructInfo &info, size_t depth = 0)
        {
            //TODO: references
            const size_t maxDepth = 1000;
            if (depth >= maxDepth)
            {
                throw std::runtime_error("Recursion depth exceeded");
            }
            if (info.size)
                return info.size;
            size_t counter = 0;
            if (info.fields.size() == 0)
            {
                info.size = 1;
                return info.size;
            }
            for (auto &field : info.fields)
            {
                counter += field.type.type == VarType::STRUCT ? ComputeTypeSize(*field.type.structInfo, depth + 1) : getSizeOfType(field.type);
            }
            info.size = counter;
            return info.size;
        }

        void processStruct(std::pair<StructInfo &, const ParseTreeNode &> _struct);

        void processFunc(const ParseTreeNode &_func);

        void processFuncBody(FunctionData &func);

        ASTNode processDot(const ParseTreeNode &node, const Scope &scope);

        ASTNode processExpression(const ParseTreeNode &expression, const Scope &scope);

        ASTNode processNode(const ParseTreeNode &oldNode, Scope &scope);

        std::pair<StructInfo &, const ParseTreeNode &> readStructDecl(const ParseTreeNode &_struct);

        TypeInfo getType(const std::string &_type)
        {
            if (_type == "void")
                return TypeInfo{VarType::VOID, false, nullptr};
            if (_type == "int")
                return TypeInfo{VarType::INT, false, nullptr};
            if (_type == "long")
                return TypeInfo{VarType::LONG, false, nullptr};
            if (_type == "float")
                return TypeInfo{VarType::FLOAT, false, nullptr};
            if (_type == "double")
                return TypeInfo{VarType::DOUBLE, false, nullptr};
            if (_type == "char")
                return TypeInfo{VarType::CHAR, false, nullptr};
            if (_type == "string")
                return TypeInfo{VarType::STRING, false, nullptr};
            if (_type == "bool")
                return TypeInfo{VarType::BOOL, false, nullptr};
            auto info = structs.find(_type);
            if (info == structs.end())
                throw std::runtime_error("Unknown type: " + _type);
            return TypeInfo{VarType::STRUCT, false, &info->second};
        }

        std::string typeAsString(const TypeInfo &type) const
        {
            switch (type.type)
            {
            case VarType::VOID:
                return "void";
            case VarType::INT:
                return "int";
            case VarType::LONG:
                return "long";
            case VarType::FLOAT:
                return "float";
            case VarType::DOUBLE:
                return "double";
            case VarType::CHAR:
                return "char";
            case VarType::STRING:
                return "string";
            case VarType::BOOL:
                return "bool";
            case VarType::STRUCT:
                return type.structInfo->name;
            }
        }

        std::string NodeTypeToString(NodeType type) const
        {
            switch (type)
            {
            case NodeType::BLOCK:
                return "BLOCK";
            case NodeType::IF:
                return "IF";
            case NodeType::WHILE:
                return "WHILE";
            case NodeType::FOR:
                return "FOR";
            case NodeType::DECL:
                return "DECL";
            case NodeType::ASSIGN:
                return "ASSIGN";
            case NodeType::RETURN:
                return "RETURN";
            case NodeType::BREAK:
                return "BREAK";
            case NodeType::CONTINUE:
                return "CONTINUE";
            case NodeType::FUNC_CALL:
                return "FUNC_CALL";
            case NodeType::STR_Val:
                return "STR_Val";
            case NodeType::INT_Val:
                return "INT_Val";
            case NodeType::LONG_Val:
                return "LONG_Val";
            case NodeType::FLOAT_Val:
                return "FLOAT_Val";
            case NodeType::DOUBLE_Val:
                return "DOUBLE_Val";
            case NodeType::CHAR_Val:
                return "CHAR_Val";
            case NodeType::BOOL_Val:
                return "BOOL_Val";
            case NodeType::VARIABLE:
                return "VARIABLE";
            case NodeType::DOT:
                return "DOT";
            case NodeType::ADD:
                return "ADD";
            case NodeType::SUB:
                return "SUB";
            case NodeType::MUL:
                return "MUL";
            case NodeType::DIV:
                return "DIV";
            case NodeType::MOD:
                return "MOD";
            case NodeType::POW:
                return "POW";
            case NodeType::EQ:
                return "EQ";
            case NodeType::NEQ:
                return "NEQ";
            case NodeType::GT:
                return "GT";
            case NodeType::LT:
                return "LT";
            case NodeType::GE:
                return "GE";
            case NodeType::LE:
                return "LE";
            case NodeType::AND:
                return "AND";
            case NodeType::OR:
                return "OR";
            case NodeType::NOT:
                return "NOT";
            case NodeType::SIZEOF:
                return "SIZEOF";
            }
        }
        template <class... Ts>
        struct overloaded : Ts...
        {
            using Ts::operator()...;
        };
        template <class... Ts>
        overloaded(Ts...) -> overloaded<Ts...>;

        std::string NodeToString(const ASTNode &node, int depth = 0) const
        {
            std::string result(1, '\n');
            result += std::string(depth * 4, ' ');
            result += NodeTypeToString(node.type);
            result += "    ";
            result += std::visit(overloaded{[](const std::string &str) -> std::string
                                            { return str; },
                                            [](const float f) -> std::string
                                            { return std::to_string(f); },
                                            [](const double d) -> std::string
                                            { return std::to_string(d); },
                                            [](const int i) -> std::string
                                            { return std::to_string(i); },
                                            [](const long l) -> std::string
                                            { return std::to_string(l); },
                                            [](const char c) -> std::string
                                            { return std::string(1, c); },
                                            [](const bool b) -> std::string
                                            { return b ? "true" : "false"; },
                                            [&](const std::vector<ASTNode> &v) -> std::string
                                            {
                                                std::string res;
                                                for (auto &n : v)
                                                {
                                                    res += NodeToString(n, depth + 1);
                                                }
                                                return res;
                                            },
                                            [&](const Variable &var) -> std::string
                                            {
                                                std::string res;
                                                for (auto &offset : var.offsets)
                                                {
                                                    res += std::to_string(offset);
                                                    res += ".";
                                                }
                                                res += std::to_string(var.scopeIndex);
                                                return res;
                                            },
                                            [](std::monostate) -> std::string
                                            { return ""; }},
                                 node.data);
            return result;
        }

        TypeInfo expressionType(const ASTNode &node);
        void ValidateNode(const ASTNode &node);

        FunctionData *main = nullptr;
        AST(const ParseTreeNode &root);
        std::string toString() const
        {
            std::stringstream ss;
            ss << "Structs: \n";
            for (auto &[name, info] : structs)
            {
                ss << "\n  " << name << ": " << info.size << '\n';
                for (auto &field : info.fields)
                {
                    ss << "    " << (field.type.isMutable ? "mut " : "") << field.name << ": " << typeAsString(field.type) << '\n';
                }
            }
            ss << "\n\nFunctions: \n";
            for (auto &[name, func] : functions)
            {
                ss << "\n  " << name << '(';
                for (auto &arg : func.arguments)
                {
                    ss << (arg.type.isMutable ? "mut " : "") << arg.name << ": " << typeAsString(arg.type) << ", ";
                }
                ss << ") -> " << typeAsString(func.returnType);
                ss << NodeToString(func.body, 1);
            }
            return ss.str();
        }
    };
}