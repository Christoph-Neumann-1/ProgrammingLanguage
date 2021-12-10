#pragma once
#include <Imports.hpp>
#include <Stack.hpp>

//TODO: implement
extern "C" std::vector<std::pair<std::string, VWA::Imports::ImportedFileData::FuncDef>> *imports;
//TODO: fix for the argument order, right now the args are passed in the exact opposite order
#define WRAP_FUNC(func) \
    [](VWA::Stack *stack, VWA::VM::VM *vm) { VWA::WrapFunc(stack, func); }

#define WRAP_FUNC_RAW_DATA(func) \
    [](VWA::Stack *stack, VWA::VM::VM *vm) { VWA::WrapFunc(stack, vm, func); }

#define FFI_STRUCT struct __attribute__((__packed__))

//This should ideally be outside of a namespace
#define MODULE_IMPL \
    VWA::Imports::ImportedFileData VWA::fileData;

//TODO: make fully compile time
//TODO: remove hack involving passing in function pointer, the template system should be able to do this
//TODO: make this run outside of a function, allowing it to be used next to the definition, ideally as part of the definition
#define EXPORT_F(f) \
    EXPORT_F_WITH_NAME(f, f)

#define EXPORT_F_WITH_NAME(f, name)                                                                         \
    namespace                                                                                               \
    {                                                                                                       \
        VWA::Autorun INTERNAL_UNIQUE_NAME(runner){                                                          \
            []() {                                                                                          \
                VWA::fileData.exportedFunctions.emplace(#name, (                                            \
                                                                   {                                        \
                                                                       auto tmp = VWA::exportDef(#name, f); \
                                                                       tmp.func = WRAP_FUNC(f);             \
                                                                       tmp;                                 \
                                                                   }));                                     \
            }};                                                                                             \
    }

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)

#define INTERNAL_UNIQUE_NAME(name) CONCAT(name, __COUNTER__)

namespace VWA
{

    extern Imports::ImportedFileData fileData;
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

    // template <size_t idx, typename offset>
    // auto readParameter(uint8_t *start)
    // {
    //     return readParameter<idx - 1, offset::next>(start);
    // }
    // template <typename offset>
    // auto readParameter<0, offset>(uint8_t *start)
    // {
    //     return offset::read(start);
    // }

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
        //TODO: this originally caused a warning about unsequenced modification, this seems to make the warning go away, but as far as I know, it is still unspecified behavior
        //Idea: create a metaprogram that sums up the sizes of all previous arguments, this can then be used to access the arguments, evaluation order can therefore be ignored.
        //Problem: How to split the args at a certain point? Or another way to do this, maybe a variadic macro?
        //TODO: ask on forum
        // stack->pushVal(func(WrapGetNextArg<Args>(argBegin)...));
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
        stack->pop((sizeof(Args) + ... + 0));
        auto argBegin = stack->getData() + stack->getTop();
        stack->pushVal(func(stack, vm, (*(Args *)((argBegin += sizeof(Args)) - sizeof(Args)))...));
    }

    template <typename R, typename... Args>
    requires std::is_void_v<R>
    void WrapFunc(Stack *stack, VM::VM *vm, R (*func)(Stack *, VM::VM *, Args...))
    {
        stack->pop((sizeof(Args) + ... + 0));
        auto argBegin = stack->getData() + stack->getTop();
        func(stack, vm, (*(Args *)((argBegin += sizeof(Args)) - sizeof(Args)))...);
    }
    //TODO: support structs requiring conversion
    template <typename T>
    concept FFIType = requires
    {
        {
            T::typeName()
            } -> std::same_as<std::string_view>;
    };

    template <typename T>
    struct typeName;

    template <typename T>
    requires FFIType<T>
    struct typeName<T>
    {
        static constexpr std::string_view value = T::typeName();
    };

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

    //Note: You need to declare all structs yourself, you can't import them. Maybe I'll make a code generator for this
    //You also need to manually specify the structs you export and import. It is your responsibility to make sure they match the structs in your code.
}

//TODO: structs
//This header contains the definitions for writing a module in c++ and making it available to the compiler.

// extern "C" VWA::Imports::ImportedFileData *MODULE_ENTRY_POINT(VWA::Imports::ImportManager *manager)
// {
// }