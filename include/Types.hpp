#pragma once
#include <string>
#include <variant>
#include <vector>

namespace VWA
{
    struct CustomTypeData;
    using Variable=std::variant<int32_t,int64_t,float,double,char,bool,CustomTypeData>;

    enum class VarType
    {
        Void,
        Int,
        Long,
        Float,
        Double,
        Char,
        Bool,
        Custom
    };

    //TODO: stored as a map of string to a unique ptr. If the entry does not exist, create an empty one and fill in the data once the definition is found.

    struct CustomTypeInfo
    {
        bool isInitialized=false;
        std::string name;
        struct Field
        {
            std::string name;
            bool isMutable;
            VarType type;
            CustomTypeInfo* typeInfo;
        };     
        std::vector<Field> fields;   
    };
    struct CustomTypeData
    {
        CustomTypeInfo &info;
        std::vector<Variable> data;
    };
}
