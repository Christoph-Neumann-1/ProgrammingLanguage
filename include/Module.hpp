#pragma once
#include <Imports.hpp>
#include <Stack.hpp>

//TODO: fix for the argument order, right now the args are passed in the exact opposite order
#define WRAP_FUNC(func) \
    [](VWA::Stack *stack, VWA::VM::VM *vm) { VWA::WrapFunc(stack, func); }

#define WRAP_FUNC_RAW_DATA(func) \
    [](VWA::Stack *stack, VWA::VM::VM *vm) { VWA::WrapFunc(stack, vm, func); }

#define FFI_STRUCT struct __attribute__((__packed__))

//TODO: return a pointer instead, try modifiying the map of files, so that a pointer will work, so as to avoid unnecessary copying
//This should ideally be outside of a namespace
#define MODULE_IMPL                                                                                                  \
    VWA::Imports::ImportedFileData VWA::fileData;                                                                    \
    std::vector<void (*)()> VWA::LoadCallbacks;                                                                      \
    std::vector<void (*)()> VWA::LinkCallbacks;                                                                      \
    extern "C" VWA::Imports::ImportedFileData                                                                        \
    MODULE_ENTRY_POINT(VWA::Imports::ImportManager *manager)                                                         \
    {                                                                                                                \
        for (auto &cb : VWA::LoadCallbacks)                                                                          \
            cb();                                                                                                    \
        return std::move(VWA::fileData);                                                                             \
    }                                                                                                                \
    /*The purpose of this function is to initialize the functions pointers used for communication with other modules \
    data refers to the data of this module, it will be removed once I remove the need to move from fileData*/        \
    extern "C" void MODULE_LINK(VWA::Imports::ImportedFileData &data)                                                \
    {                                                                                                                \
        for (auto &cb : VWA::LinkCallbacks)                                                                          \
            cb();                                                                                                    \
    }

#define ADD_LOAD_CALLBACK(cb) \
    VWA::Autorun INTERNAL_UNIQUE_NAME(registerLoadCb){[]() { VWA::LoadCallbacks.push_back(cb); }};

#define ADD_LINK_CALLBACK(cb) \
    VWA::Autorun INTERNAL_UNIQUE_NAME(registerLinkCb){[]() { VWA::LinkCallbacks.push_back(cb); }};

//TODO: make fully compile time
//TODO: I should be able to use it like this: EXPORT_FUNC(void, func, int, int)
//TODO: remove hack involving passing in function pointer, the template system should be able to do this
//TODO: make this run outside of a function, allowing it to be used next to the definition, ideally as part of the definition
//TODO: change name
#define EXPORT_FUNC(ret, name, args...) \
    ret name(args);                     \
    EXPORT_F(name)                      \
    ret name(args)

#define EXPORT_FUNC_WITH_NAME(exportedName, ret, name, args...) \
    ret name(args);                                             \
    EXPORT_F_WITH_NAME(name, exportedName)                      \
    ret name(args)

#define EXPORT_F(f) \
    EXPORT_F_WITH_NAME(f, f)

#define EXPORT_F_WITH_NAME(f, name)                                                                                 \
    namespace                                                                                                       \
    {                                                                                                               \
        VWA::Autorun INTERNAL_UNIQUE_NAME(runner){                                                                  \
            []() {                                                                                                  \
                VWA::fileData.exportedFunctions.emplace(#name, (                                                    \
                                                                   {                                                \
                                                                       auto tmp = VWA::getFuncDefFromPtr(#name, f); \
                                                                       tmp.func = WRAP_FUNC(f);                     \
                                                                       tmp;                                         \
                                                                   }));                                             \
            }};                                                                                                     \
    }
//TODO:
#define IMPORT_FUNC(module, ret, name, args...) \
    ret (*name)(args){[]() { return nullptr; }()};

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)

#define INTERNAL_UNIQUE_NAME(name) CONCAT(name, __COUNTER__)

#define STRUCT_NAME(name) \
    static constexpr std::string_view typeName = #name;

#define EXPORT_STRUCT(name, members...) \
    EXPORT_STRUCT_WITH_NAME(name, name, members)

#define EXPORT_STRUCT_WITH_NAME(internalName, exportedName, members...)                                                                       \
    namespace                                                                                                                                 \
    {                                                                                                                                         \
        VWA::Autorun INTERNAL_UNIQUE_NAME(internal_struct_autorun){                                                                           \
            []() {                                                                                                                            \
                VWA::fileData.exportedStructs.emplace(VWA::typeName<internalName>, VWA::getStructDefFromMemberList<internalName, members>()); \
            }};                                                                                                                               \
    }

namespace VWA
{

    extern Imports::ImportedFileData fileData;
    extern std::vector<void (*)()> LoadCallbacks;
    extern std::vector<void (*)()> LinkCallbacks;

    //TODO: make this easier to use
    struct Autorun
    {
        Autorun(auto &&func)
        {
            func();
        }
    };
    namespace
    {
        template <typename T>
        T WrapGetNextArg(uint8_t *&current)
        {
            T value = *(T *)current;
            current += sizeof(T);
            return value;
        }
    };
    template <size_t prev, typename T = void, typename... Args>
    FFI_STRUCT ParameterOffset;
    template <size_t prev>
    FFI_STRUCT ParameterOffset<prev, void>{
        template <size_t idx>
        static void get(uint8_t *){}};
    template <size_t prev, typename T, typename... Args>
    FFI_STRUCT ParameterOffset
    {
        static constexpr size_t offset = prev;
        using next = ParameterOffset<prev + sizeof(T), Args...>;
        using type = T;
        static T read(uint8_t * start)
        {
            return *(T *)(start + offset);
        }
        template <size_t idx, typename next, typename>
        struct Get
        {
            static constexpr auto value = Get<idx - 1, typename next::next, typename next::type>::value;
            using type = typename next::Get<idx - 1, typename next::next, typename next::type>::type;
        };

        template <typename next, typename U>
        struct Get<0, next, U>
        {
            static constexpr auto value = offset;
            using type = U;
        };

        template <size_t idx>
        static auto get(uint8_t * start)
        {
            using T2 = Get<idx, next, T>;
            return *(typename T2::type *)(start + T2::value);
        }
    };

    template <typename... Args>
    struct ParameterReader
    {
        using offsets = ParameterOffset<0, Args...>;
        using sequence = std::make_integer_sequence<size_t, sizeof...(Args)>;
    };

    //TODO: require structs to be trivial
    template <typename R, typename... Args>
    requires(!std::is_void_v<R>) void WrapFunc(Stack *stack, R (*func)(Args...))
    {
        using reader = ParameterReader<Args...>;
        auto expander = [&]<size_t... I>(uint8_t * start, std::integer_sequence<size_t, I...>)
        {
            return func(reader::offsets::template get<I>(start)...);
        };
        stack->pop((sizeof(Args) + ... + 0));
        auto argBegin = stack->getData() + stack->getTop();
        stack->pushVal(expander(argBegin, typename reader::sequence{}));
    }

    template <typename R, typename... Args>
    requires std::is_void_v<R>
    void WrapFunc(Stack *stack, R (*func)(Args...))
    {
        using reader = ParameterReader<Args...>;
        auto expander = [&]<size_t... I>(uint8_t * start, std::integer_sequence<size_t, I...>)
        {
            // func(reader::offsets::read<I>(start)...);
            func(reader::offsets::template get<I>(start)...);
        };
        stack->pop((sizeof(Args) + ... + 0));
        auto argBegin = stack->getData() + stack->getTop();
        // func(WrapGetNextArg<Args>(argBegin)...);
        expander(argBegin, typename reader::sequence{});
    }

    template <typename R, typename... Args>
    requires(!std::is_void_v<R>) void WrapFunc(Stack *stack, VM::VM *vm, R (*func)(Stack *, VM::VM *, Args...))
    {
        using reader = ParameterReader<Args...>;
        auto expander = [&]<size_t... I>(uint8_t * start, std::integer_sequence<size_t, I...>)
        {
            return func(stack, vm, reader::offsets::template get<I>(start)...);
        };
        stack->pop((sizeof(Args) + ... + 0));
        auto argBegin = stack->getData() + stack->getTop();
        stack->pushVal(expander(argBegin, typename reader::sequence{}));
    }

    template <typename R, typename... Args>
    requires std::is_void_v<R>
    void WrapFunc(Stack *stack, VM::VM *vm, R (*func)(Stack *, VM::VM *, Args...))
    {
        using reader = ParameterReader<Args...>;
        auto expander = [&]<size_t... I>(uint8_t * start, std::integer_sequence<size_t, I...>)
        {
            // func(reader::offsets::read<I>(start)...);
            func(stack, vm, reader::offsets::template get<I>(start)...);
        };
        stack->pop((sizeof(Args) + ... + 0));
        auto argBegin = stack->getData() + stack->getTop();
        // func(WrapGetNextArg<Args>(argBegin)...);
        expander(argBegin, typename reader::sequence{});
    }
    //TODO: support structs requiring conversion
    //TODO: extend to work with builtin types
    template <typename T>
    concept FFIType = std::same_as<decltype(T::typeName), const std::string_view>;

    template <typename T>
    constexpr std::string_view typeName = T::typeName;

    template <>
    constexpr std::string_view typeName<int> = "int";

    template <>
    constexpr std::string_view typeName<float> = "float";

    template <>
    constexpr std::string_view typeName<double> = "double";

    template <>
    constexpr std::string_view typeName<long> = "long";

    template <>
    constexpr std::string_view typeName<char> = "char";

    template <>
    constexpr std::string_view typeName<bool> = "bool";

    template <>
    constexpr std::string_view typeName<void> = "void";

    //TODO: structs
    template <typename R, typename... Args>
    VWA::Imports::ImportedFileData::FuncDef getFuncDefFromPtr(std::string_view name, R (*)(Args...))
    {
        return {.name = std::string{name}, .returnType = std::string{typeName<R>}, .parameters = {{std::string{typeName<Args>}, true}...}, .isC = true};
    }

    template <FFIType S, typename... Args>
    VWA::Imports::ImportedFileData::StructDef getStructDefFromMemberList(std::string_view exportedName = typeName<S>)
    {
        //TODO: consider using requires instead of static_assert
        static_assert(sizeof...(Args) > 0, "Structs must not be empty");
        static_assert(sizeof(S) == (sizeof(Args) + ...), "Member list must be same size as struct");
        return {.name = std::string{exportedName}, .fields = {{std::string{typeName<Args>}, true}...}};
    }

    //Note: You need to declare all structs yourself, you can't import them. Maybe I'll make a code generator for this
    //You also need to manually specify the structs you export and import. It is your responsibility to make sure they match the structs in your code.
}

//TODO: structs
//This header contains the definitions for writing a module in c++ and making it available to the compiler.

// extern "C" VWA::Imports::ImportedFileData *MODULE_ENTRY_POINT(VWA::Imports::ImportManager *manager)
// {
// }