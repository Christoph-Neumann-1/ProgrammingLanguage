#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <variant>
#include <Node.hpp>
#include <optional>

//TODO: implicit return in void functions

/*
Idea for ActionTree:
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
*/

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
        StructInfo *structInfo;
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

    class ActionTree
    {
        std::unordered_map<std::string, StructInfo> structs;
        std::unordered_map<std::string, FunctionData> functions;
        FunctionData *main = nullptr;

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

        void processStruct(std::pair<StructInfo &, const ASTNode &> _struct);

        void processFunc(const ASTNode &_func);

        std::pair<StructInfo &, const ASTNode &> readStructDecl(const ASTNode &_struct);

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

    public:
        ActionTree(const ASTNode &root);
    };

}