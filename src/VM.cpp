#include <VM.hpp>
#include <cmath>

namespace VWA::VM
{
    //TODO: remove dynamic memory allocation
    void VM::exec(instruction::ByteCodeElement *instruction, uint8_t *stackBase)
    {
        for (;;)
        {
            switch (instruction->byteCode)
            {
                using namespace instruction;
            case Exit:
                throw ExitException{mmu.stack.popVal<int32_t>()};
            case Push:
                mmu.stack.push(ReadInstructionArg<uint64_t>(instruction + 1));
                instruction += 1 + sizeof(uint64_t);
                continue;
            case Pop:
                mmu.stack.pop(ReadInstructionArg<uint64_t>(instruction + 1));
                instruction += 1 + sizeof(uint64_t);
                continue;
            case Dup:
            {
                auto offset = ReadInstructionArg<uint64_t>(instruction + 1);
                auto size = ReadInstructionArg<uint64_t>(instruction + 1 + sizeof(uint64_t));
                std::memcpy(mmu.stack.getData() + mmu.stack.getTop(), mmu.stack.getData() + mmu.stack.getTop() - offset, size);
                mmu.stack.push(size);
                instruction += 1 + 2 * sizeof(uint64_t);
                continue;
            }
            case PopMiddle:
            {
                auto size = ReadInstructionArg<uint64_t>(instruction + 1);
                auto length = ReadInstructionArg<uint64_t>(instruction + 1 + sizeof(uint64_t));
                std::memmove(mmu.stack.getData() + mmu.stack.getTop() - size - length, mmu.stack.getData() + mmu.stack.getTop() - size, size);
                mmu.stack.pop(length);
                instruction += 1 + 2 * sizeof(uint64_t);
                continue;
            }
            case PushConst8:
                mmu.stack.pushVal(ReadInstructionArg<uint8_t>(instruction + 1));
                instruction += 1 + sizeof(uint8_t);
                continue;
            case PushConst32:
                mmu.stack.pushVal(ReadInstructionArg<int32_t>(instruction + 1));
                instruction += 1 + sizeof(uint32_t);
                continue;
            case PushConst64:
                mmu.stack.pushVal(ReadInstructionArg<int64_t>(instruction + 1));
                instruction += 1 + sizeof(uint64_t);
                continue;
            case PushConstN:
            {
                auto size = ReadInstructionArg<uint64_t>(instruction + 1);
                instruction += 1 + sizeof(uint64_t);
                mmu.stack.PushN(size, &instruction->value);
                instruction += size;
                continue;
            }
            case Jump:
            {
                auto where = ReadInstructionArg<ByteCodeElement *>(instruction + 1);
                instruction = where;
                continue;
            }
            case JumpIfFalse:
            {
                auto where = ReadInstructionArg<ByteCodeElement *>(instruction + 1);
                if (!mmu.stack.popVal<bool>())
                    instruction = where;
                else
                    instruction += 1 + sizeof(ByteCodeElement *);
                continue;
            }
            case JumpIfTrue:
            {
                auto where = ReadInstructionArg<ByteCodeElement *>(instruction + 1);
                if (mmu.stack.popVal<bool>())
                    instruction = where;
                else
                    instruction += 1 + sizeof(ByteCodeElement *);
                continue;
            }
            case JumpFunc:
            {
                auto where = ReadInstructionArg<ByteCodeElement *>(instruction + 1);
                auto argSize = ReadInstructionArg<uint64_t>(instruction + 1 + sizeof(ByteCodeElement *));
                auto argBegin = mmu.stack.getData() + mmu.stack.getTop() - argSize;
                mmu.stack.push(2 * sizeof(uint8_t *));
                std::memmove(argBegin + 2 * sizeof(void *), argBegin, argSize);
                *reinterpret_cast<uint8_t **>(argBegin) = stackBase;
                stackBase = argBegin;
                *reinterpret_cast<ByteCodeElement **>(argBegin + sizeof(uint8_t *)) = instruction + 1 + sizeof(ByteCodeElement *) + sizeof(uint64_t);
                instruction = where;
                continue;
            }
            case FCall:
                throw std::runtime_error("Unlinked symbol");
            case Return:
            {
                auto argSize = ReadInstructionArg<uint64_t>(instruction + 1);
                instruction = *reinterpret_cast<ByteCodeElement **>(stackBase + sizeof(uint8_t *));
                auto argBegin = mmu.stack.getData() + mmu.stack.getTop() - argSize;
                mmu.stack.pop(mmu.stack.getData() + mmu.stack.getTop() - stackBase - argSize);
                auto oldBase = stackBase;
                stackBase = *reinterpret_cast<uint8_t **>(stackBase);
                std::memmove(oldBase, argBegin, argSize);
                if (instruction == nullptr)
                    return;
                continue;
            }
            case JumpFFI:
            {
                auto where = ReadInstructionArg<FFIFunc>(instruction + 1);
                auto argSize = ReadInstructionArg<uint64_t>(instruction + 1 + sizeof(FFIFunc));
                where(&mmu.stack, this);
                instruction += 1 + sizeof(FFIFunc) + sizeof(uint64_t);
                continue;
            }
            case ReadLocal:
            {
                auto size = ReadInstructionArg<uint64_t>(instruction + 1);
                auto addr = ReadInstructionArg<uint64_t>(instruction + 1 + sizeof(uint64_t));
                mmu.stack.PushN(size, stackBase + addr);
                instruction += 1 + 2 * sizeof(uint64_t);
                continue;
            }
            case WriteLocal:
            {
                auto size = ReadInstructionArg<uint64_t>(instruction + 1);
                auto addr = ReadInstructionArg<uint64_t>(instruction + 1 + sizeof(uint64_t));
                std::memcpy(stackBase + addr, mmu.stack.getData() + mmu.stack.getTop() - size, size);
                mmu.stack.pop(size);
                instruction += 1 + 2 * sizeof(uint64_t);
                continue;
            }
            case ReadGlobal:
            {
                auto size = ReadInstructionArg<uint64_t>(instruction + 1);
                mmu.stack.PushN(size, mmu.stack.popVal<uint8_t *>());
                instruction += 1 + sizeof(uint64_t);
                continue;
            }
            case WriteGlobal:
            {
                auto size = ReadInstructionArg<uint64_t>(instruction + 1);
                auto src = mmu.stack.getData() + mmu.stack.getTop() - size;
                std::memcpy(mmu.stack.popVal<uint8_t *>(), src, size);
                mmu.stack.pop(size);
                instruction += 1 + sizeof(uint64_t);
                continue;
            }
            case FtoD:
                mmu.stack.pushVal<double>(mmu.stack.popVal<float>());
                instruction++;
                continue;
            case FtoI:
                mmu.stack.pushVal<int32_t>(mmu.stack.popVal<float>());
                instruction++;
                continue;
            case FtoL:
                mmu.stack.pushVal<int64_t>(mmu.stack.popVal<float>());
                instruction++;
                continue;
            case FtoC:
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<float>());
                instruction++;
                continue;
            case DtoF:
                mmu.stack.pushVal<float>(mmu.stack.popVal<double>());
                instruction++;
                continue;
            case DtoI:
                mmu.stack.pushVal<int32_t>(mmu.stack.popVal<double>());
                instruction++;
                continue;
            case DtoL:
                mmu.stack.pushVal<int64_t>(mmu.stack.popVal<double>());
                instruction++;
                continue;
            case DtoC:
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<double>());
                instruction++;
                continue;
            case ItoF:
                mmu.stack.pushVal<float>(mmu.stack.popVal<int32_t>());
                instruction++;
                continue;
            case ItoD:
                mmu.stack.pushVal<double>(mmu.stack.popVal<int32_t>());
                instruction++;
                continue;
            case ItoL:
                mmu.stack.pushVal<int64_t>(mmu.stack.popVal<int32_t>());
                instruction++;
                continue;
            case ItoC:
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<int32_t>());
                instruction++;
                continue;
            case LtoF:
                mmu.stack.pushVal<float>(mmu.stack.popVal<int64_t>());
                instruction++;
                continue;
            case LtoD:
                mmu.stack.pushVal<double>(mmu.stack.popVal<int64_t>());
                instruction++;
                continue;
            case LtoI:
                mmu.stack.pushVal<int32_t>(mmu.stack.popVal<int64_t>());
                instruction++;
                continue;
            case LtoC:
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<int64_t>());
                instruction++;
                continue;
            case CtoF:
                mmu.stack.pushVal<float>(mmu.stack.popVal<uint8_t>());
                instruction++;
                continue;
            case CtoD:
                mmu.stack.pushVal<double>(mmu.stack.popVal<uint8_t>());
                instruction++;
                continue;
            case CtoI:
                mmu.stack.pushVal<int32_t>(mmu.stack.popVal<uint8_t>());
                instruction++;
                continue;
            case CtoL:
                mmu.stack.pushVal<int64_t>(mmu.stack.popVal<uint8_t>());
                instruction++;
                continue;
            case AddF:
            {
                auto rhs = mmu.stack.popVal<float>();
                mmu.stack.pushVal<float>(mmu.stack.popVal<float>() + rhs);
                instruction++;
                continue;
            }
            case AddD:
            {
                auto rhs = mmu.stack.popVal<double>();
                mmu.stack.pushVal<double>(mmu.stack.popVal<double>() + rhs);
                instruction++;
                continue;
            }
            case AddI:
            {
                auto rhs = mmu.stack.popVal<int32_t>();
                mmu.stack.pushVal<int32_t>(mmu.stack.popVal<int32_t>() + rhs);
                instruction++;
                continue;
            }
            case AddL:
            {
                auto rhs = mmu.stack.popVal<int64_t>();
                mmu.stack.pushVal<int64_t>(mmu.stack.popVal<int64_t>() + rhs);
                instruction++;
                continue;
            }
            case SubtractF:
            {
                auto rhs = mmu.stack.popVal<float>();
                mmu.stack.pushVal<float>(mmu.stack.popVal<float>() - rhs);
                instruction++;
                continue;
            }
            case SubtractD:
            {
                auto rhs = mmu.stack.popVal<double>();
                mmu.stack.pushVal<double>(mmu.stack.popVal<double>() - rhs);
                instruction++;
                continue;
            }
            case SubtractI:
            {
                auto rhs = mmu.stack.popVal<int32_t>();
                mmu.stack.pushVal<int32_t>(mmu.stack.popVal<int32_t>() - rhs);
                instruction++;
                continue;
            }
            case SubtractL:
            {
                auto rhs = mmu.stack.popVal<int64_t>();
                mmu.stack.pushVal<int64_t>(mmu.stack.popVal<int64_t>() - rhs);
                instruction++;
                continue;
            }
            case MultiplyF:
            {
                auto rhs = mmu.stack.popVal<float>();
                mmu.stack.pushVal<float>(mmu.stack.popVal<float>() * rhs);
                instruction++;
                continue;
            }
            case MultiplyD:
            {
                auto rhs = mmu.stack.popVal<double>();
                mmu.stack.pushVal<double>(mmu.stack.popVal<double>() * rhs);
                instruction++;
                continue;
            }
            case MultiplyI:
            {
                auto rhs = mmu.stack.popVal<int32_t>();
                mmu.stack.pushVal<int32_t>(mmu.stack.popVal<int32_t>() * rhs);
                instruction++;
                continue;
            }
            case MultiplyL:
            {
                auto rhs = mmu.stack.popVal<int64_t>();
                mmu.stack.pushVal<int64_t>(mmu.stack.popVal<int64_t>() * rhs);
                instruction++;
                continue;
            }
            case DivideF:
            {
                auto rhs = mmu.stack.popVal<float>();
                mmu.stack.pushVal<float>(mmu.stack.popVal<float>() / rhs);
                instruction++;
                continue;
            }
            case DivideD:
            {
                auto rhs = mmu.stack.popVal<double>();
                mmu.stack.pushVal<double>(mmu.stack.popVal<double>() / rhs);
                instruction++;
                continue;
            }
            case DivideI:
            {
                auto rhs = mmu.stack.popVal<int32_t>();
                mmu.stack.pushVal<int32_t>(mmu.stack.popVal<int32_t>() / rhs);
                instruction++;
                continue;
            }
            case DivideL:
            {
                auto rhs = mmu.stack.popVal<int64_t>();
                mmu.stack.pushVal<int64_t>(mmu.stack.popVal<int64_t>() / rhs);
                instruction++;
                continue;
            }
            case ModuloI:
            {
                auto rhs = mmu.stack.popVal<int32_t>();
                mmu.stack.pushVal<int32_t>(mmu.stack.popVal<int32_t>() % rhs);
                instruction++;
                continue;
            }
            case ModuloL:
            {
                auto rhs = mmu.stack.popVal<int64_t>();
                mmu.stack.pushVal<int64_t>(mmu.stack.popVal<int64_t>() % rhs);
                instruction++;
                continue;
            }
            case PowerF:
            {
                auto rhs = mmu.stack.popVal<float>();
                mmu.stack.pushVal<float>(powf(mmu.stack.popVal<float>(), rhs));
                instruction++;
                continue;
            }
            case PowerD:
            {
                auto rhs = mmu.stack.popVal<double>();
                mmu.stack.pushVal<double>(pow(mmu.stack.popVal<double>(), rhs));
                instruction++;
            }
            case PowerI:
            {
                auto rhs = mmu.stack.popVal<int32_t>();
                auto lhs = mmu.stack.popVal<int32_t>();
                for (int32_t i = 0; i < rhs; i++)
                    lhs *= lhs;
                mmu.stack.pushVal<int32_t>(lhs);
                instruction++;
                continue;
            }
            case PowerL:
            {
                auto rhs = mmu.stack.popVal<int64_t>();
                auto lhs = mmu.stack.popVal<int64_t>();
                for (int64_t i = 0; i < rhs; i++)
                    lhs *= lhs;
                mmu.stack.pushVal<int64_t>(lhs);
                instruction++;
                continue;
            }
            case And:
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<uint8_t>() && mmu.stack.popVal<uint8_t>());
                instruction++;
                continue;
            case Or:
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<uint8_t>() || mmu.stack.popVal<uint8_t>());
                instruction++;
                continue;
            case Not:
                mmu.stack.pushVal<uint8_t>(!mmu.stack.popVal<uint8_t>());
                instruction++;
                continue;
            case EqualF:
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<float>() == mmu.stack.popVal<float>());
                instruction++;
                continue;
            case EqualD:
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<double>() == mmu.stack.popVal<double>());
                instruction++;
                continue;
            case EqualI:
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<int32_t>() == mmu.stack.popVal<int32_t>());
                instruction++;
                continue;
            case EqualL:
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<int64_t>() == mmu.stack.popVal<int64_t>());
                instruction++;
                continue;
            case NotEqualF:
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<float>() != mmu.stack.popVal<float>());
                instruction++;
                continue;
            case NotEqualD:
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<double>() != mmu.stack.popVal<double>());
                instruction++;
                continue;
            case NotEqualI:
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<int32_t>() != mmu.stack.popVal<int32_t>());
                instruction++;
                continue;
            case NotEqualL:
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<int64_t>() != mmu.stack.popVal<int64_t>());
                instruction++;
                continue;
            case LessThanF:
            {
                auto rhs = mmu.stack.popVal<float>();
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<float>() < rhs);
                instruction++;
                continue;
            }
            case LessThanD:
            {
                auto rhs = mmu.stack.popVal<double>();
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<double>() < rhs);
                instruction++;
                continue;
            }
            case LessThanI:
            {
                auto rhs = mmu.stack.popVal<int32_t>();
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<int32_t>() < rhs);
                instruction++;
                continue;
            }
            case LessThanL:
            {
                auto rhs = mmu.stack.popVal<int64_t>();
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<int64_t>() < rhs);
                instruction++;
                continue;
            }
            case GreaterThanF:
            {
                auto rhs = mmu.stack.popVal<float>();
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<float>() > rhs);
                instruction++;
                continue;
            }
            case GreaterThanD:
            {
                auto rhs = mmu.stack.popVal<double>();
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<double>() > rhs);
                instruction++;
                continue;
            }
            case GreaterThanI:
            {
                auto rhs = mmu.stack.popVal<int32_t>();
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<int32_t>() > rhs);
                instruction++;
                continue;
            }
            case GreaterThanL:
            {
                auto rhs = mmu.stack.popVal<int64_t>();
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<int64_t>() > rhs);
                instruction++;
                continue;
            }
            case LessThanOrEqualF:
            {
                auto rhs = mmu.stack.popVal<float>();
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<float>() <= rhs);
                instruction++;
                continue;
            }
            case LessThanOrEqualD:
            {
                auto rhs = mmu.stack.popVal<double>();
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<double>() <= rhs);
                instruction++;
                continue;
            }
            case LessThanOrEqualI:
            {
                auto rhs = mmu.stack.popVal<int32_t>();
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<int32_t>() <= rhs);
                instruction++;
                continue;
            }
            case LessThanOrEqualL:
            {
                auto rhs = mmu.stack.popVal<int64_t>();
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<int64_t>() <= rhs);
                instruction++;
                continue;
            }
            case GreaterThanOrEqualF:
            {
                auto rhs = mmu.stack.popVal<float>();
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<float>() >= rhs);
                instruction++;
                continue;
            }
            case GreaterThanOrEqualD:
            {
                auto rhs = mmu.stack.popVal<double>();
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<double>() >= rhs);
                instruction++;
                continue;
            }
            case GreaterThanOrEqualI:
            {
                auto rhs = mmu.stack.popVal<int32_t>();
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<int32_t>() >= rhs);
                instruction++;
                continue;
            }
            case GreaterThanOrEqualL:
            {
                auto rhs = mmu.stack.popVal<int64_t>();
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<int64_t>() >= rhs);
                instruction++;
                continue;
            }
            }
        }
    }
}