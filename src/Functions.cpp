#include <Functions.hpp>

namespace VWA
{
    std::string FunctionInfo::toString()
    {
        std::string ret;
        ret += "Function: ";
        ret += name;
        ret += "\n";
        ret += "Arguments: ";
        for (auto arg : args)
        {
            ret += arg.name;
            if(arg.isMutable)
                ret += "mut ";
            ret += typeToString(arg.type,arg.typeInfo);
            ret += " ";
        }
        ret += "\n";
        ret += "Returns: ";
        ret += typeToString(returnType,rtypeInfo);
        ret += "\n";
        return ret;
    }
}