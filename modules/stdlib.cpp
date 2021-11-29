#include <Module.hpp>
#include <functional>

FFI_STRUCT inp
{
    int a, b;
};

int add(int a, int b)
{
    return a + b;
}

int addUsingStruct(inp in)
{
    return in.a + in.b;
}

void printN(int n)
{
    printf("%i\n", n);
}

void printC(char c)
{
    printf("%c", c);
}

char getC()
{
    char c;
    scanf("%c", &c);
    return c;
}

//How do I get rid of the warning about linkage?
extern "C" VWA::Imports::ImportedFileData MODULE_ENTRY_POINT(VWA::Imports::ImportManager *manager)
{
    VWA::Imports::ImportedFileData data;
    data.exportedFunctions.emplace("add", VWA::Imports::ImportedFileData::FuncDef{.name = "add", .returnType = "int", .parameters = {{"int", true}, {"int", true}}, .func = WRAP_FUNC(addUsingStruct), .isC = true});
    data.exportedFunctions.emplace("printN", VWA::Imports::ImportedFileData::FuncDef{.name = "printN", .returnType = "void", .parameters = {{"int", true}}, .func = WRAP_FUNC(printN), .isC = true});
    data.exportedFunctions.emplace("printC", VWA::Imports::ImportedFileData::FuncDef{.name = "printC", .returnType = "void", .parameters = {{"char", true}}, .func = WRAP_FUNC(printC), .isC = true});
    data.exportedFunctions.emplace("getC", VWA::Imports::ImportedFileData::FuncDef{.name = "getC", .returnType = "char", .func = WRAP_FUNC(getC), .isC = true});
    return data;
}
