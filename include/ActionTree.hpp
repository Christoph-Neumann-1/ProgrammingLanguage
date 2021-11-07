#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <variant>
#include <Node.hpp>
#include <optional>

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
        STRING,
        BOOL,
        STRUCT
    };
    struct StructInfo;
    struct StructData;
    struct TypeInfo
    {
        VarType type;
        bool isMutable;
        const StructInfo *structInfo;
    };
    struct VarData
    {
        VarType type;
        std::variant<int, long, float, double, char, std::string, bool, std::unique_ptr<VarData>> data;
    };
    struct VarRef
    {
        VarData *data;
    };
    struct StructInfo
    {
        bool initialized = false;
        std::string name;
        struct Field
        {
            const std::string name;
            const TypeInfo type;
        };
        std::vector<Field> fields;
    };
    struct StructData
    {
        const StructInfo &info;
        std::vector<VarData> data;
    };

    enum class NodeType
    {
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
        VALUE,
        VARIABLE,
        ADD,
        SUB,
        MUL,
        DIV,
        MOD,
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
    struct ActionTreeNode
    {
        NodeType type;
        //TODO: DATA
    };

    struct FunctionData
    {
        bool initialized = false;
        std::string name;
        struct Argument
        {
            const std::string name;
            const TypeInfo type;
        };
        std::vector<Argument> arguments;
        TypeInfo returnType;
        ActionTreeNode body;
    };

    class ActionTree
    {
        std::unordered_map<std::string, StructInfo> structs;
        std::unordered_map<std::string, FunctionData> functions;
        FunctionData *main = nullptr;

        void processStruct(const ASTNode &_struct);

        void processFunc(const ASTNode &_func);

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
            return TypeInfo{VarType::STRUCT, false, &structs[_type]};
        }

    public:
        ActionTree(const ASTNode &root);
    };

}