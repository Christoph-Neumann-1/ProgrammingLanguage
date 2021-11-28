#pragma once
#include <Imports.hpp>
#include <Stack.hpp>

//TODO: implement
extern "C" std::vector<std::pair<std::string, VWA::Imports::ImportedFileData::FuncDef>> *imports;
//TODO: fix for the argument order, right now the args are passed in the exact opposite order
#define WRAP_FUNC(func) \
    [](VWA::Stack *stack, VWA::VM::VM *vm) { WrapFunc(stack, func); }

#define WRAP_FUNC_RAW_DATA(func) \
    [](VWA::Stack *stack, VWA::VM::VM *vm) { WrapFunc(stack, vm, func); }

namespace VWA
{
    template <typename R, typename... Args>
    requires(!std::is_void_v<R>) void WrapFunc(Stack *stack, R (*func)(Args...))
    {
        stack->pushVal(func(stack->popVal<Args>()...));
    }

    template <typename R, typename... Args>
    requires std::is_void_v<R>
    void WrapFunc(Stack *stack, R (*func)(Args...))
    {
        func(stack->popVal<Args>()...);
    }

    template <typename R, typename... Args>
    requires(!std::is_void_v<R>) void WrapFunc(Stack *stack, VM::VM *vm, R (*func)(Stack *, VM::VM *, Args...))
    {
        stack->pushVal(func(stack, vm, stack->popVal<Args>()...));
    }

    template <typename R, typename... Args>
    requires std::is_void_v<R>
    void WrapFunc(Stack *stack, VM::VM *vm, R (*func)(Stack *, VM::VM *, Args...))
    {
        func(stack, vm, stack->popVal<Args>()...);
    }
}

//TODO: structs
//This header contains the definitions for writing a module in c++ and making it available to the compiler.

// extern "C" VWA::Imports::ImportedFileData *MODULE_ENTRY_POINT(VWA::Imports::ImportManager *manager)
// {
// }