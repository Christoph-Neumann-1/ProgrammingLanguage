#include <cstdint>
#include <Stack.hpp>
#include <ByteCode.hpp>

namespace VWA::VM
{
    template <uint64_t size> //Size in bits
    struct Register
    {
        uint8_t data[size / 8];
        template <typename T>
        requires(sizeof(T) <= size) void set(T value)
        {
            *reinterpret_cast<T *>(data) = value;
        }
        template <typename T>
        requires(sizeof(T) <= size)
            T get()
        const
        {
            return *reinterpret_cast<T *>(data);
        }
    };

    class MemoryManager
    {
        Stack stack;
        //TODO:heap,constants,code? Memory map.

        friend class VM; //Vm needs the ability to access the stack and add new regions.
        //Returns a pointer the region of memory. Size is used to assert that it is within bounds
    public:
        const uint8_t *Read(uint64_t addr, uint64_t size) const
        {
            return stack.readBytes(addr, size);
        }
        void Write(uint64_t addr, uint64_t size, const uint8_t *source)
        {
            stack.writeBytes(addr, size, source);
        }
    };

    class VM
    {
        Register<64> stack_base;
        Register<64> r64a, r64b, r64c, r64d, r64e, r64f, r64g;
        Register<32> r32a, r32b, r32c, r32d;
        Register<8> rba, rbb, rbc;
        MemoryManager mmu;
        union ByteCodeElement
        {
            instruction::instruction byteCode;
            uint8_t value;
        };
        //TODO:relocatable code
        ByteCodeElement *code{nullptr};
        struct ExitException
        {
            int32_t exitCode;
        };
        template <typename T>
        T readValFromCode(uint64_t addr)
        {
            return *reinterpret_cast<T *>(code + addr);
        }
        template <typename T>
        void RegisterStore(uint8_t reg, T value)
        {
         //TODO: size comparison, read method, enum for access;   
        }

    public:
        void exec(uint64_t programCounter);
        int run()
        {
            try
            {
                exec(0); //0 is expected to contain main, or the jump to main
            }
            catch (ExitException e)
            {
                return e.exitCode;
            }
            throw std::runtime_error("VM exited without an exit code");
        }
    };
}