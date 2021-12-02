#include <cstdint>
#include <Stack.hpp>
#include <ByteCode.hpp>
#include <unordered_map>
#include <list>
#include <vector>
#include <memory>

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
    using FFIFunc = void (*)(Stack *, VM *);

    class VM
    {
    public: //TODO: make private again
        MemoryManager mmu;

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
        void exec(instruction::ByteCodeElement *instruction, uint8_t *stackBase);
        int run(instruction::ByteCodeElement *main)
        {
            try
            {
                mmu.stack.pushVal<uint8_t *>(nullptr);
                mmu.stack.pushVal<instruction::ByteCodeElement *>(nullptr);
                exec(main, mmu.stack.getData() + mmu.stack.getTop() - sizeof(uint8_t *) - sizeof(instruction::ByteCodeElement *));
            }
            catch (ExitException e)
            {
                return e.exitCode;
            }
            return mmu.stack.popVal<int32_t>();
        }
    };
}