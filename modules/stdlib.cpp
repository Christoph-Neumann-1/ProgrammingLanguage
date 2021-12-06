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

void mwrite(long addr, char val)
{
    *(char *)addr = val;
}

char mread(long addr)
{
    return *(char *)addr;
}
template <typename T>
concept FFIType = requires(T t)
{
    {
        T::typeName()
        } -> std::same_as<std::string_view>;
    {
        T::size()
        } -> std::same_as<size_t>;
    {t.get()};
};

struct S
{
    constexpr static std::string_view foo()
    {
        return "foo";
    }
    constexpr static size_t size()
    {
        return sizeof(int);
    }
    constexpr int get()
    {
        return 42;
        //TODO: is this a good idea? Or should I just require that the struct can be reinterpreted as the respective type?
    }
};

template <typename T>
struct typeName;

template <>
struct typeName<int>
{
    static constexpr std::string_view value = "int";
};

template <>
struct typeName<char>
{
    static constexpr std::string_view value = "char";
};

template <>
struct typeName<float>
{
    static constexpr std::string_view value = "float";
};

template <>
struct typeName<double>
{
    static constexpr std::string_view value = "double";
};

template <>
struct typeName<long>
{
    static constexpr std::string_view value = "long";
};

template <>
struct typeName<void>
{
    static constexpr std::string_view value = "void";
};

//TODO: structs
template <typename R, typename... Args>
VWA::Imports::ImportedFileData::FuncDef exportDef(std::string_view name, R (*)(Args...))
{
    return {.name = std::string{name}, .returnType = std::string{typeName<R>::value}, .parameters = {{std::string{typeName<Args>::value}, true}...}, .isC = true};
}
//TODO: make fully compile time
//TODO: remove hack involving passing in function pointer, the template system should be able to do this
//TODO: make this run outside of a function, allowing it to be used next to the definition, ideally as part of the definition
#define EXPORT_F(f)                                                         \
    data.exportedFunctions.emplace(#f, (                                    \
                                           {                                \
                                               auto tmp = exportDef(#f, f); \
                                               tmp.func = WRAP_FUNC(f);     \
                                               tmp;                         \
                                           }));

//How do I get rid of the warning about linkage?
extern "C" VWA::Imports::ImportedFileData
MODULE_ENTRY_POINT(VWA::Imports::ImportManager *manager)
{
    //TODO: make a wrapper that auto detects parameter types and generates appropriate boiler plate code.
    //This might require wrapping everything in a struct which then returns the correct type.
    VWA::Imports::ImportedFileData data;
    EXPORT_F(add);
    EXPORT_F(printN);
    EXPORT_F(printC);
    EXPORT_F(getC);
    EXPORT_F(mwrite);
    EXPORT_F(mread);
    // data.exportedFunctions.emplace("add", VWA::Imports::ImportedFileData::FuncDef{.name = "add", .returnType = "int", .parameters = {{"int", true}, {"int", true}}, .func = WRAP_FUNC(addUsingStruct), .isC = true});
    // data.exportedFunctions.emplace("printN", VWA::Imports::ImportedFileData::FuncDef{.name = "printN", .returnType = "void", .parameters = {{"int", true}}, .func = WRAP_FUNC(printN), .isC = true});
    // data.exportedFunctions.emplace("printC", VWA::Imports::ImportedFileData::FuncDef{.name = "printC", .returnType = "void", .parameters = {{"char", true}}, .func = WRAP_FUNC(printC), .isC = true});
    // data.exportedFunctions.emplace("getC", VWA::Imports::ImportedFileData::FuncDef{.name = "getC", .returnType = "char", .func = WRAP_FUNC(getC), .isC = true});
    // data.exportedFunctions.emplace("mwrite", VWA::Imports::ImportedFileData::FuncDef{.name = "mwrite", .returnType = "void", .parameters = {{"long", true}, {"char", true}}, .func = WRAP_FUNC(mwrite), .isC = true});
    data.exportedFunctions.emplace("malloc", VWA::Imports::ImportedFileData::FuncDef{.name = "malloc", .returnType = "long", .parameters = {{"long", true}}, .func = WRAP_FUNC(malloc), .isC = true});
    data.exportedFunctions.emplace("free", VWA::Imports::ImportedFileData::FuncDef{.name = "free", .returnType = "void", .parameters = {{"long", true}}, .func = WRAP_FUNC(free), .isC = true});
    // data.exportedFunctions.emplace("mread", VWA::Imports::ImportedFileData::FuncDef{.name = "mread", .returnType = "char", .parameters = {{"long", true}}, .func = WRAP_FUNC(mread), .isC = true});

    return data;
}
