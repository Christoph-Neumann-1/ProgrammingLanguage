#include <Module.hpp>
#include <functional>

MODULE_IMPL

static int64_t begin;

int add(int a, int b)
{
    return a + b;
}
EXPORT_F(add)

void printN(int n)
{
    printf("%i\n", n);
}
EXPORT_F(printN)

int getN()
{
    int n;
    [[maybe_unused]] int _ = scanf("%i", &n);
    return n;
}
EXPORT_F(getN)

void printC(char c)
{
    printf("%c", c);
}
EXPORT_F(printC)

char getC()
{
    char c;
    scanf("%c", &c);
    return c;
}
EXPORT_F(getC)

void mwrite(long addr, char val)
{
    *(char *)addr = val;
}
EXPORT_F(mwrite)

char mread(long addr)
{
    return *(char *)addr;
}
EXPORT_F(mread)

long now()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count() - begin;
}
EXPORT_F(now)

void printL(long n)
{
    printf("%li\n", n);
}
EXPORT_F(printL)

template <typename T>
void stuff(T t)
{
    printf("%i\n", t);
}

//How do I get rid of the warning about linkage?
//TODO: automate, with option for manually implementing if necessary
//TODO: add macros for supporting custom types, so that I don't have to manually implement them,
//... or I could just force the user to add a layer of abstraction
//TODO: return a pointer instead, try modifiying the map of files, so that a pointer will work, so as to avoid unnecessary copying
extern "C" VWA::Imports::ImportedFileData
MODULE_ENTRY_POINT(VWA::Imports::ImportManager *manager)
{

    VWA::fileData.exportedFunctions.emplace("malloc", VWA::Imports::ImportedFileData::FuncDef{.name = "malloc", .returnType = "long", .parameters = {{"long", true}}, .func = WRAP_FUNC(malloc), .isC = true});
    VWA::fileData.exportedFunctions.emplace("free", VWA::Imports::ImportedFileData::FuncDef{.name = "free", .returnType = "void", .parameters = {{"long", true}}, .func = WRAP_FUNC(free), .isC = true});
    return std::move(VWA::fileData);
}
