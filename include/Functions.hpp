#pragma once
#include <string>
#include <Types.hpp>
namespace VWA{
    struct FunctionInfo{
        std::string name;
        VarType returnType;
        CustomTypeInfo *rtypeInfo;
        struct param{
            std::string name;
            VarType type;
            bool isMutable;
            CustomTypeInfo *typeInfo;
        };
        std::vector<param> args;
        std::string toString();
    };
}