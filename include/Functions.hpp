#pragma once
#include <string>
#include <Types.hpp>
namespace VWA{
    struct FunctionInfo{
        std::string name;
        VarType returnType;
        CustomTypeInfo *rtypeInfo;
        std::vector<std::pair<VarType, CustomTypeInfo *>> args;
    };
}