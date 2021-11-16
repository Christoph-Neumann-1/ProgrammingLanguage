#include <cstdint>
#include <Stack.hpp>
#include <ByteCode.hpp>

namespace VWA::VM
{
    class MemoryManager
    {
        Stack stack;
        //TODO:heap,constants,code? Memory map.

        friend class VM; //Vm needs the ability to access the stack and add new regions.
        //Returns a pointer the region of memory. Size is used to assert that it is within bounds
    public:
        template <typename T>
        const T *Read(uint64_t addr) const
        {
            return stack.readVal<T>(addr);
        }
        const uint8_t *Read(uint64_t addr, uint64_t size) const
        {
            return stack.readBytes(addr, size);
        }
        template <typename T>
        void Write(uint64_t addr, T value)
        {
            return stack.writeVal(addr, value);
        }
        void Write(uint64_t addr, uint64_t size, const uint8_t *source)
        {
            stack.writeBytes(addr, size, source);
        }
        //TODO: read n bytes
    };

    class VM
    {

        MemoryManager mmu;
        //TODO:relocatable code
        instruction::ByteCodeElement *code{nullptr};
        struct ExitException
        {
            int32_t exitCode;
        };

        template <typename T>
        T ReadInstructionArg(instruction::ByteCodeElement *begin)
        {
            return *reinterpret_cast<T *>(begin);
        }

    public:
        VM(instruction::ByteCodeElement *code_, size_t size) : code(new instruction::ByteCodeElement[size])
        {
            std::memcpy(code, code_, size);
        }
        ~VM()
        {
            delete[] code;
        }
        void exec(instruction::ByteCodeElement *instruction, uint8_t *stackBase);
        int run()
        {
            try
            {
                mmu.stack.pushVal<uint8_t *>(nullptr);
                mmu.stack.pushVal<instruction::ByteCodeElement *>(nullptr);
                exec(code, mmu.stack.getData() + mmu.stack.getTop() - sizeof(uint8_t *) - sizeof(instruction::ByteCodeElement *)); //TODO: args
            }
            catch (ExitException e)
            {
                return e.exitCode;
            }
            return mmu.stack.popVal<int32_t>();
        }
    };
}