#include <Module.hpp>
#include <functional>

int add(int a, int b)
{
    return a + b;
}

void printN(int n)
{
    printf("%i\n", n);
}

//How do I get rid of the warning about linkage?
extern "C" VWA::Imports::ImportedFileData MODULE_ENTRY_POINT(VWA::Imports::ImportManager *manager)
{
    VWA::Imports::ImportedFileData data;
    data.exportedFunctions.emplace("add", VWA::Imports::ImportedFileData::FuncDef{.name = "add", .returnType = "int", .parameters = {{"int", true}, {"int", true}}, .func = WRAP_FUNC(add), .isC = true});
    data.exportedFunctions.emplace("printN", VWA::Imports::ImportedFileData::FuncDef{.name = "printN", .returnType = "void", .parameters = {{"int", true}}, .func = WRAP_FUNC(printN), .isC = true});
    return data;
}
