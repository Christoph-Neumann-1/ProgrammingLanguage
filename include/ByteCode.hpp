#pragma once
#include <cstdint>
namespace VWA
{
    namespace instruction
    {
        enum instruction : uint8_t
        {
            //Operands are takes from the top of the stack. The result is pushed back onto the stack, consuming the operands.
            //Special control instructions
            Exit, //Stop execution. Args: 32bit int (exit code)
            LocalAddr,//Tells the interpreter that the next command requires offsetting the adress.This is not executed
            ExternSymbol, //Tells the compiler that the next commands address is the index of an symbol, which needs to be resolved at runtime.
            FFISymbol, //Tells the compiler that the next commands address is the index of an symbol from the c api, which needs to loaded dynamically.
            Call, //Call a function, which is either intrinsic, or from another language(c interface).The interpreter will assume the function
            //to be of the type void(*)(Stack*) Args: 64bit pointer to function
            //General arithmetic instructions oprands are as follows: destinatin register
            //followed by the registers containing operands
            Addf,
            Subtractf,
            Multiplyf,
            Dividef,
            Powerf,
            Addd,
            Subtractd,
            Multiplyd,
            Divided,
            Powerd,
            Addi,
            Subtracti,
            Multiplyi,
            Dividei,
            Poweri,
            Addl,
            Subtractl,
            Multiplyl,
            Dividel,
            Powerl,
            Moduloi,
            Modulol,
            //Casting
            //Operands: dest,source
            FtoD,
            FtoI,
            FtoL,
            FtoC,
            ItoF,
            ItoD,
            ItoL,
            ItoC,
            LtoF,
            LtoD,
            LtoI,
            LtoC,
            DtoF,
            DtoI,
            DtoL,
            DtoC,
            CtoF,
            CtoI,
            CtoL,
            CtoD,
            //Booleans may be interpreted as characters. 
            //Boolean logic instructions
            And,
            Or,
            Not,
            //Comparison instructions
            GreaterThanf,
            LessThanf,
            GreaterThanOrEqualf,
            LessThanOrEqualf,
            Equalf,
            NotEqualf,
            GreaterThand,
            LessThand,
            GreaterThanOrEquald,
            LessThanOrEquald,
            Equald,
            NotEquald,
            GreaterThani,
            LessThani,
            GreaterThanOrEquali,
            LessThanOrEquali,
            Equali,
            NotEquali,
            GreaterThanl,
            LessThanl,
            GreaterThanOrEquall,
            LessThanOrEquall,
            Equall,
            NotEquall,
            //stack manipulation instructions. Btw the stack pointer should be in a global somewhere.
            StackPush, //Increment the stack pointer. Args: 64bit uint (amount to increment)
            StackPop,  //Decrement the stack pointer. Args: 64bit uint (amount to decrement)
            StackTop,  //Reads the current top of the stack and pushes it back onto the stack.
            Duplicate, //Dubplicates nbytes of the stack.
            //I need to support data copy without stack manipulation.
            Read,  //Read a value from an address and put it on the stack. Args: 64bit uint (address), 64bit uint (size)
            ReadConst,
            Write, //Write a value from the stack to an address. Args: 64bit uint (address), 64bit uint (size)
            //Control flow
            //The addresses are indices to an array containing the instructions.
            //Later, when I add support for multiple dynamically linkes files, the instruction tables will just be appended.
            Jump,          //continue with the instruction at the given address. Args: 64bit uint (address) Example: function call
            JumpIfFalse,   //continue with the instruction at the given address if the top of the stack is false. Args: 64bit uint (address)
            JumpIfTrue,    //continue with the instruction at the given address if the top of the stack is true. Args: 64bit uint (address)
            JumpFromStack, //continue with the instruction at the given address. Args: 64bit uint (address)(given on the stack)
            InstructionAddr,//Put the address of the instruction on the stack.
            //Use this for JumpFromStack
        };
    }

    //This function returns the total size used by this instruction including arguments.
    uint64_t getInstructionSize(instruction::instruction instruction);
}