#pragma once
#include <cstdint>
namespace VWA
{
    namespace instruction
    {
        enum instruction : uint8_t
        {
            //Special control instructions
            Exit,         //Stop execution. Args: 32bit int (exit code)
            AddF,
            SubtractF,
            MultiplyF,
            DivideF,
            PowerF,
            AddD,
            SubtractD,
            MultiplyD,
            DivideD,
            PowerD,
            AddI,
            SubtractI,
            MultiplyI,
            DivideI,
            PowerI,
            AddL,
            SubtractL,
            MultiplyL,
            DivideL,
            PowerL,
            ModuloI,
            ModuloL,
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
            GreaterThanF,
            LessThanF,
            GreaterThanOrEqualF,
            LessThanOrEqualF,
            EqualF,
            NotEqualF,
            GreaterThanD,
            LessThanD,
            GreaterThanOrEqualD,
            LessThanOrEqualD,
            EqualD,
            NotEqualD,
            GreaterThanI,
            LessThanI,
            GreaterThanOrEqualI,
            LessThanOrEqualI,
            EqualI,
            NotEqualI,
            GreaterThanL,
            LessThanL,
            GreaterThanOrEqualL,
            LessThanOrEqualL,
            EqualL,
            NotEqualL,
            //stack manipulation instructions. Btw the stack pointer should be in a global somewhere.
            
            ReadLocal,//Reads data at an offset from the base pointer.
            WriteLocal, 
            ReadGlobal,//These instructions read data from a pointer, provided on the stack.
            WriteGlobal,
            //TODO: direct copy
            Push,//Increment the stack pointer. This does not allow for intialization of the value, use read or ConstPush for that.
            Pop,
            PushConst8,//Push a constant value onto the stack.
            PushConst32,
            PushConst64,
            PushConstN,//This instruction takes the number of bytes as the first argument.
            //Control flow
            //The addresses are indices to an array containing the instructions.
            //Later, when I add support for multiple dynamically linkes files, the instruction tables will just be appended.
            Jump,        //continue with the instruction at the given address. Args: 64bit uint (address) Example: function call
            JumpIfFalse, //continue with the instruction at the given address if the top of the stack is false. Args: 64bit uint (address)
            JumpIfTrue,  //continue with the instruction at the given address if the top of the stack is true. Args: 64bit uint (address)
            JumpFunc,//Jump to the address, while setting the base pointer and return address. Args: 64bit uint (address), 64bit uint (nBytes for args)
            JumpFFI,//Jump to the ffi function at the given address.
            Return,//Return from a function and restore base pointer and stack. Args: 64bit uint (nBytes for rval
            
        };

        union ByteCodeElement
        {
            uint8_t value;
            instruction byteCode;
        };
    }

}