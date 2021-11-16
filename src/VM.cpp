#include <VM.hpp>
#include <memory>
#include <cmath>

namespace VWA::VM
{
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
                break;
            case Pop:
                mmu.stack.pop(ReadInstructionArg<uint64_t>(instruction + 1));
                instruction += 1 + sizeof(uint64_t);
                break;
            case PushConst8:
                mmu.stack.pushVal(ReadInstructionArg<uint8_t>(instruction + 1));
                instruction += 1 + sizeof(uint8_t);
                break;
            case PushConst32:
                mmu.stack.pushVal(ReadInstructionArg<int32_t>(instruction + 1));
                instruction += 1 + sizeof(uint32_t);
                break;
            case PushConst64:
                mmu.stack.pushVal(ReadInstructionArg<int64_t>(instruction + 1));
                instruction += 1 + sizeof(uint64_t);
                break;
            case PushConstN:
            {
                auto size = ReadInstructionArg<uint64_t>(instruction + 1);
                instruction += 1 + sizeof(uint64_t);
                mmu.stack.PushN(size, &instruction->value);
                instruction += size;
                break;
            }
            case Jump:
            {
                auto where = ReadInstructionArg<ByteCodeElement *>(instruction + 1);
                instruction = where;
                break;
            }
            case JumpIfFalse:
            {
                auto where = ReadInstructionArg<ByteCodeElement *>(instruction + 1);
                if (!mmu.stack.popVal<bool>())
                    instruction = where;
                else
                    instruction += 1 + sizeof(ByteCodeElement *);
                break;
            }
            case JumpIfTrue:
            {
                auto where = ReadInstructionArg<ByteCodeElement *>(instruction + 1);
                if (mmu.stack.popVal<bool>())
                    instruction = where;
                else
                    instruction += 1 + sizeof(ByteCodeElement *);
                break;
            }
            case JumpFunc:
            {
                auto where = ReadInstructionArg<ByteCodeElement *>(instruction + 1);
                auto argSize = ReadInstructionArg<uint64_t>(instruction + 1 + sizeof(ByteCodeElement *));
                std::unique_ptr<uint8_t[]> args{new uint8_t[argSize]};
                std::memcpy(args.get(), mmu.stack.getData() + mmu.stack.getTop() - argSize, argSize);
                mmu.stack.pop(argSize);
                mmu.stack.pushVal<uint8_t *>(stackBase);
                stackBase = mmu.stack.getData() + mmu.stack.getTop() - sizeof(uint8_t *);
                mmu.stack.pushVal<ByteCodeElement *>(instruction + 1 + sizeof(ByteCodeElement *) + sizeof(uint64_t) + argSize);
                mmu.stack.PushN(argSize, args.get());
                instruction = where;
                break;
            }
            case Return:
            {
                auto argSize = ReadInstructionArg<uint64_t>(instruction + 1);
                instruction = *reinterpret_cast<ByteCodeElement **>(stackBase + sizeof(uint8_t *));
                std::unique_ptr<uint8_t[]> buffer{new uint8_t[argSize]};
                std::memcpy(buffer.get(), mmu.stack.getData() + mmu.stack.getTop() - argSize, argSize);
                mmu.stack.pop(mmu.stack.getData() + mmu.stack.getTop() - stackBase);
                mmu.stack.PushN(argSize, buffer.get());
                stackBase = *reinterpret_cast<uint8_t **>(stackBase);
                if (instruction == nullptr)
                    return;
                break;
            }
            case JumpFFI:
                throw std::runtime_error("JumpFFI not implemented");
            case ReadLocal:
            {
                auto size = ReadInstructionArg<uint64_t>(instruction + 1);
                auto addr = ReadInstructionArg<uint64_t>(instruction + 1 + sizeof(uint64_t));
                mmu.stack.PushN(size, stackBase + addr);
                instruction += 1 + 2 * sizeof(uint64_t);
                break;
            }
            case WriteLocal:
            {
                auto size = ReadInstructionArg<uint64_t>(instruction + 1);
                auto addr = ReadInstructionArg<uint64_t>(instruction + 1 + sizeof(uint64_t));
                std::memcpy(stackBase + addr, mmu.stack.getData() + mmu.stack.getTop() - size, size);
                mmu.stack.pop(size);
                instruction += 1 + 2 * sizeof(uint64_t);
                break;
            }
            case ReadGlobal:
            {
                auto size = ReadInstructionArg<uint64_t>(instruction + 1);
                auto addr = ReadInstructionArg<uint8_t *>(instruction + 1 + sizeof(uint64_t));
                mmu.stack.PushN(size, addr);
                instruction += 1 + sizeof(uint64_t) + sizeof(uint8_t *);
                break;
            }
            case WriteGlobal:
            {
                auto size = ReadInstructionArg<uint64_t>(instruction + 1);
                auto addr = ReadInstructionArg<uint8_t *>(instruction + 1 + sizeof(uint64_t));
                auto src = mmu.stack.getData() + mmu.stack.getTop() - size;
                std::memcpy(addr, src, size);
                mmu.stack.pop(size);
                instruction += 1 + sizeof(uint64_t) + sizeof(uint8_t *);
                break;
            }
            case FtoD:
                mmu.stack.pushVal<double>(mmu.stack.popVal<float>());
                instruction++;
                break;
            case FtoI:
                mmu.stack.pushVal<int32_t>(mmu.stack.popVal<float>());
                instruction++;
                break;
            case FtoL:
                mmu.stack.pushVal<int64_t>(mmu.stack.popVal<float>());
                instruction++;
                break;
            case FtoC:
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<float>());
                instruction++;
                break;
            case DtoF:
                mmu.stack.pushVal<float>(mmu.stack.popVal<double>());
                instruction++;
                break;
            case DtoI:
                mmu.stack.pushVal<int32_t>(mmu.stack.popVal<double>());
                instruction++;
                break;
            case DtoL:
                mmu.stack.pushVal<int64_t>(mmu.stack.popVal<double>());
                instruction++;
                break;
            case DtoC:
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<double>());
                instruction++;
                break;
            case ItoF:
                mmu.stack.pushVal<float>(mmu.stack.popVal<int32_t>());
                instruction++;
                break;
            case ItoD:
                mmu.stack.pushVal<double>(mmu.stack.popVal<int32_t>());
                instruction++;
                break;
            case ItoL:
                mmu.stack.pushVal<int64_t>(mmu.stack.popVal<int32_t>());
                instruction++;
                break;
            case ItoC:
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<int32_t>());
                instruction++;
                break;
            case LtoF:
                mmu.stack.pushVal<float>(mmu.stack.popVal<int64_t>());
                instruction++;
                break;
            case LtoD:
                mmu.stack.pushVal<double>(mmu.stack.popVal<int64_t>());
                instruction++;
                break;
            case LtoI:
                mmu.stack.pushVal<int32_t>(mmu.stack.popVal<int64_t>());
                instruction++;
                break;
            case LtoC:
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<int64_t>());
                instruction++;
                break;
            case CtoF:
                mmu.stack.pushVal<float>(mmu.stack.popVal<uint8_t>());
                instruction++;
                break;
            case CtoD:
                mmu.stack.pushVal<double>(mmu.stack.popVal<uint8_t>());
                instruction++;
                break;
            case CtoI:
                mmu.stack.pushVal<int32_t>(mmu.stack.popVal<uint8_t>());
                instruction++;
                break;
            case CtoL:
                mmu.stack.pushVal<int64_t>(mmu.stack.popVal<uint8_t>());
                instruction++;
                break;
            case AddF:
            {
                auto rhs = mmu.stack.popVal<float>();
                mmu.stack.pushVal<float>(mmu.stack.popVal<float>() + rhs);
                instruction++;
                break;
            }
            case AddD:
            {
                auto rhs = mmu.stack.popVal<double>();
                mmu.stack.pushVal<double>(mmu.stack.popVal<double>() + rhs);
                instruction++;
                break;
            }
            case AddI:
            {
                auto rhs = mmu.stack.popVal<int32_t>();
                mmu.stack.pushVal<int32_t>(mmu.stack.popVal<int32_t>() + rhs);
                instruction++;
                break;
            }
            case AddL:
            {
                auto rhs = mmu.stack.popVal<int64_t>();
                mmu.stack.pushVal<int64_t>(mmu.stack.popVal<int64_t>() + rhs);
                instruction++;
                break;
            }
            case SubtractF:
            {
                auto rhs = mmu.stack.popVal<float>();
                mmu.stack.pushVal<float>(mmu.stack.popVal<float>() - rhs);
                instruction++;
                break;
            }
            case SubtractD:
            {
                auto rhs = mmu.stack.popVal<double>();
                mmu.stack.pushVal<double>(mmu.stack.popVal<double>() - rhs);
                instruction++;
                break;
            }
            case SubtractI:
            {
                auto rhs = mmu.stack.popVal<int32_t>();
                mmu.stack.pushVal<int32_t>(mmu.stack.popVal<int32_t>() - rhs);
                instruction++;
                break;
            }
            case SubtractL:
            {
                auto rhs = mmu.stack.popVal<int64_t>();
                mmu.stack.pushVal<int64_t>(mmu.stack.popVal<int64_t>() - rhs);
                instruction++;
                break;
            }
            case MultiplyF:
            {
                auto rhs = mmu.stack.popVal<float>();
                mmu.stack.pushVal<float>(mmu.stack.popVal<float>() * rhs);
                instruction++;
                break;
            }
            case MultiplyD:
            {
                auto rhs = mmu.stack.popVal<double>();
                mmu.stack.pushVal<double>(mmu.stack.popVal<double>() * rhs);
                instruction++;
                break;
            }
            case MultiplyI:
            {
                auto rhs = mmu.stack.popVal<int32_t>();
                mmu.stack.pushVal<int32_t>(mmu.stack.popVal<int32_t>() * rhs);
                instruction++;
                break;
            }
            case MultiplyL:
            {
                auto rhs = mmu.stack.popVal<int64_t>();
                mmu.stack.pushVal<int64_t>(mmu.stack.popVal<int64_t>() * rhs);
                instruction++;
                break;
            }
            case DivideF:
            {
                auto rhs = mmu.stack.popVal<float>();
                mmu.stack.pushVal<float>(mmu.stack.popVal<float>() / rhs);
                instruction++;
                break;
            }
            case DivideD:
            {
                auto rhs = mmu.stack.popVal<double>();
                mmu.stack.pushVal<double>(mmu.stack.popVal<double>() / rhs);
                instruction++;
                break;
            }
            case DivideI:
            {
                auto rhs = mmu.stack.popVal<int32_t>();
                mmu.stack.pushVal<int32_t>(mmu.stack.popVal<int32_t>() / rhs);
                instruction++;
                break;
            }
            case DivideL:
            {
                auto rhs = mmu.stack.popVal<int64_t>();
                mmu.stack.pushVal<int64_t>(mmu.stack.popVal<int64_t>() / rhs);
                instruction++;
                break;
            }
            case ModuloI:
            {
                auto rhs = mmu.stack.popVal<int32_t>();
                mmu.stack.pushVal<int32_t>(mmu.stack.popVal<int32_t>() % rhs);
                instruction++;
                break;
            }
            case ModuloL:
            {
                auto rhs = mmu.stack.popVal<int64_t>();
                mmu.stack.pushVal<int64_t>(mmu.stack.popVal<int64_t>() % rhs);
                instruction++;
                break;
            }
            case PowerF:
            {
                auto rhs = mmu.stack.popVal<float>();
                mmu.stack.pushVal<float>(powf(mmu.stack.popVal<float>(), rhs));
                instruction++;
                break;
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
                break;
            }
            case PowerL:
            {
                auto rhs = mmu.stack.popVal<int64_t>();
                auto lhs = mmu.stack.popVal<int64_t>();
                for (int64_t i = 0; i < rhs; i++)
                    lhs *= lhs;
                mmu.stack.pushVal<int64_t>(lhs);
                instruction++;
                break;
            }
            case And:
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<uint8_t>() && mmu.stack.popVal<uint8_t>());
                instruction++;
                break;
            case Or:
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<uint8_t>() || mmu.stack.popVal<uint8_t>());
                instruction++;
                break;
            case Not:
                mmu.stack.pushVal<uint8_t>(!mmu.stack.popVal<uint8_t>());
                instruction++;
                break;
            case EqualF:
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<float>() == mmu.stack.popVal<float>());
                instruction++;
                break;
            case EqualD:
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<double>() == mmu.stack.popVal<double>());
                instruction++;
                break;
            case EqualI:
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<int32_t>() == mmu.stack.popVal<int32_t>());
                instruction++;
                break;
            case EqualL:
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<int64_t>() == mmu.stack.popVal<int64_t>());
                instruction++;
                break;
            case NotEqualF:
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<float>() != mmu.stack.popVal<float>());
                instruction++;
                break;
            case NotEqualD:
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<double>() != mmu.stack.popVal<double>());
                instruction++;
                break;
            case NotEqualI:
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<int32_t>() != mmu.stack.popVal<int32_t>());
                instruction++;
                break;
            case NotEqualL:
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<int64_t>() != mmu.stack.popVal<int64_t>());
                instruction++;
                break;
            case LessThanF:
            {
                auto rhs = mmu.stack.popVal<float>();
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<float>() < rhs);
                instruction++;
                break;
            }
            case LessThanD:
            {
                auto rhs = mmu.stack.popVal<double>();
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<double>() < rhs);
                instruction++;
                break;
            }
            case LessThanI:
            {
                auto rhs = mmu.stack.popVal<int32_t>();
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<int32_t>() < rhs);
                instruction++;
                break;
            }
            case LessThanL:
            {
                auto rhs = mmu.stack.popVal<int64_t>();
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<int64_t>() < rhs);
                instruction++;
                break;
            }
            case GreaterThanF:
            {
                auto rhs = mmu.stack.popVal<float>();
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<float>() > rhs);
                instruction++;
                break;
            }
            case GreaterThanD:
            {
                auto rhs = mmu.stack.popVal<double>();
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<double>() > rhs);
                instruction++;
                break;
            }
            case GreaterThanI:
            {
                auto rhs = mmu.stack.popVal<int32_t>();
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<int32_t>() > rhs);
                instruction++;
                break;
            }
            case GreaterThanL:
            {
                auto rhs = mmu.stack.popVal<int64_t>();
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<int64_t>() > rhs);
                instruction++;
                break;
            }
            case LessThanOrEqualF:
            {
                auto rhs = mmu.stack.popVal<float>();
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<float>() <= rhs);
                instruction++;
                break;
            }
            case LessThanOrEqualD:
            {
                auto rhs = mmu.stack.popVal<double>();
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<double>() <= rhs);
                instruction++;
                break;
            }
            case LessThanOrEqualI:
            {
                auto rhs = mmu.stack.popVal<int32_t>();
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<int32_t>() <= rhs);
                instruction++;
                break;
            }
            case LessThanOrEqualL:
            {
                auto rhs = mmu.stack.popVal<int64_t>();
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<int64_t>() <= rhs);
                instruction++;
                break;
            }
            case GreaterThanOrEqualF:
            {
                auto rhs = mmu.stack.popVal<float>();
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<float>() >= rhs);
                instruction++;
                break;
            }
            case GreaterThanOrEqualD:
            {
                auto rhs = mmu.stack.popVal<double>();
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<double>() >= rhs);
                instruction++;
                break;
            }
            case GreaterThanOrEqualI:
            {
                auto rhs = mmu.stack.popVal<int32_t>();
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<int32_t>() >= rhs);
                instruction++;
                break;
            }
            case GreaterThanOrEqualL:
            {
                auto rhs = mmu.stack.popVal<int64_t>();
                mmu.stack.pushVal<uint8_t>(mmu.stack.popVal<int64_t>() >= rhs);
                instruction++;
                break;
            }
            }
        }
    }
}