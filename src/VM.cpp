#include <VM.hpp>

namespace VWA::VM
{
    void VM::exec(uint64_t programCounter)
    {
        for (;;)
        {
            switch (code[programCounter].byteCode)
            {
                using namespace instruction;
                case Exit:
                    throw ExitException{readValFromCode<int32_t>(programCounter + 1)};
                case InstructionAddr:
                    uint8_t reg=readValFromCode<uint8_t>(programCounter + 1);
                    programCounter+=sizeof(uint8_t)+sizeof(instruction::instruction);
                    
            }
        }
    }
}