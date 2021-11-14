#include <ByteCode.hpp>
#include <stdexcept>
namespace VWA
{
    uint64_t getInstructionSize(instruction::instruction instruction)
    {
        using namespace instruction;
        switch (instruction)
        {
        case Exit:
            return 5;
        case LocalAddr:
        case ExternSymbol:
        case FFISymbol:
            throw std::runtime_error("Instruction does not have a defined size. It exists only for the linker.");
        case Call:
            return 9;
        case Addf:
        case Subtractf:
        case Multiplyf:
        case Dividef:
        case Powerf:
        case Addi:
        case Subtracti:
        case Multiplyi:
        case Dividei:
        case Poweri:
        case Moduloi:
        case Addd:
        case Subtractd:
        case Multiplyd:
        case Divided:
        case Powerd:
        case Addl:
        case Subtractl:
        case Multiplyl:
        case Dividel:
        case Powerl:
        case Modulol:
        case FtoD:
        case FtoI:
        case FtoL:
        case FtoC:
        case ItoF:
        case ItoD:
        case ItoL:
        case ItoC:
        case DtoF:
        case DtoI:
        case DtoL:
        case DtoC:
        case LtoF:
        case LtoD:
        case LtoI:
        case LtoC:
        case CtoF:
        case CtoD:
        case CtoL:
        case CtoI:
        case And:
        case Or:
        case Not:
        case GreaterThanf:
        case GreaterThanOrEqualf:
        case LessThanf:
        case LessThanOrEqualf:
        case GreaterThani:
        case GreaterThanOrEquali:
        case LessThani:
        case LessThanOrEquali:
        case GreaterThand:
        case GreaterThanOrEquald:
        case LessThand:
        case LessThanOrEquald:
        case GreaterThanl:
        case GreaterThanOrEquall:
        case LessThanl:
        case LessThanOrEquall:
        case Equalf:
        case Equali:
        case Equall:
        case Equald:
        case NotEqualf:
        case NotEquali:
        case NotEquall:
        case NotEquald:
        case Read:
        case Write:
        case StackPush:
        case StackPop:
        case StackTop:
        case InstructionAddr:
        case JumpFromStack:
        case Duplicate:
            return 9;
        case Jump:
        case JumpIfFalse:
        case JumpIfTrue:
            return 9;
        case TrueCheck64:
        return 9;
        case TrueCheck32:
        return 5;
        case TrueCheck16:
        return 3;
        case TrueCheck8:
        return 2;
        }
    }
}