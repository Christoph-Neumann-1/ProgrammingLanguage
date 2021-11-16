#pragma once
#include <vector>
#include <Stack.hpp>
#include <cstdio>
#include <ByteCode.hpp>
#include <stdexcept>
namespace VWA
{
    class Interpreter
    {
        struct InterpreterExit
        {
            int code;
            InterpreterExit(int code) : code(code) {}
        };

        //Interpreter pointer is usually not needed, but in case you want to temporarily continue executing something else, you can.
        //The only problem here is returning. Right now returning means jumping to another instruction. As this does not make sense here,
        //You may give nullptr as return address. This will tell the interpreter, that you called this function from the outside and
        //Do not want the interpreter to continue. It will return with code 0, Yielding execution.
        //The exit statement still has a purpose, as it will cause the entire interpreter(including nested interpreters) to stop.
        //It also allows to pass return values.
        //The exit condition is implemented with an exception, since that offers a clean way to back up the call stack.
        //This is just a hack until I achieve c compatibility.
        std::vector<void (*)(Stack *, Interpreter *)> externFunctions = {[](Stack *stck, Interpreter *interpreter)
                                                                         {
                                                                            //  char val = *stck->readVal<char>(stck->getTop() - 1);
                                                                            //  printf("called%c.", val);
                                                                         }};

    public:
        union CodeElement
        {
            instruction::instruction instr;
            uint8_t byte;
            CodeElement(instruction::instruction instr_) : instr(instr_) {}
            CodeElement(uint8_t byte_) : byte(byte_) {}
        };

    private:
        std::vector<CodeElement> code;
        std::vector<uint8_t> constants;

    public:
        Stack stack;
        Interpreter(std::vector<CodeElement> code_, std::vector<uint8_t> constants_) : code(code_), constants(constants_) {}
        void exec(uint64_t pos);
        int run()
        {
            try
            {
                exec(0);
            }
            catch (InterpreterExit &e)
            {
                return e.code;
            }
            throw std::runtime_error("Interpreter exited without returning.");
        }
    };
}