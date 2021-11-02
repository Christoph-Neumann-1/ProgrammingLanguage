#include <Types.hpp>

namespace VWA
{
    std::string typeToString(VarType type, CustomTypeInfo *info)
    {
        switch (type)
        {
        case VarType::Void:
            return "void";
        case VarType::Int:
            return "int";
        case VarType::Long:
            return "long";
        case VarType::Float:
            return "float";
        case VarType::Double:
            return "double";
        case VarType::Char:
            return "char";
        case VarType::Bool:
            return "bool";
        case VarType::Custom:
            return info->name;
        }
    }
}