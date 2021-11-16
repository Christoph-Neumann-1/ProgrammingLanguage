// #include <Interpreter.hpp>
// #include <stdexcept>
// namespace VWA
// {

//     void Interpreter::exec(uint64_t pos)
//     {
//         using namespace instruction;
//         for (auto i = pos; i < code.size(); ++i)
//         {
//             switch (code[i].instr)
//             {
//             case Exit:
//                 //TODO: use c++ style cast
//                 throw InterpreterExit(*((int *)&code[i + 1]));
//             case Call:
//             {
//                 auto idx = *((uint64_t *)&code[i + 1].byte);
//                 i += sizeof(uint64_t);
//                 if (idx > externFunctions.size())
//                     throw std::runtime_error("Invalid function index");
//                 externFunctions[idx](&stack, this); //TODO: address instead of index
//                 break;
//             }
//             case InstructionAddr:
//             {
//                 stack.pushVal(i + 1);
//                 break;
//             }
//             case Duplicate:
//             {
//                 uint64_t n = *((uint64_t *)&code[i + 1].byte);
//                 i += sizeof(uint64_t);
//                 uint64_t idx = stack.getTop() -n;
//                 for (uint64_t j = 0; j < n; ++j)
//                     stack.pushVal<std::byte>(stack.readVal<std::byte>(idx + j));
//                 break;
//             }
//             case JumpFromStack:
//             {
//                 auto addr = stack.popVal<uint64_t>();
//                 if (addr > code.size())
//                     throw std::runtime_error("Invalid jump address");
//                 i = addr - 1;
//                 break;
//             }
//             case StackTop:
//             {
//                 stack.pushVal(stack.getTop());
//                 break;
//             }
//             case StackPop:
//             {
//                 auto size = *((uint64_t *)&code[i + 1].byte);
//                 i += sizeof(uint64_t);
//                 stack.pop(size);
//                 break;
//             }
//             case StackPush:
//             {
//                 auto size = *((uint64_t *)&code[i + 1].byte);
//                 i += sizeof(uint64_t);
//                 stack.push(size);
//                 break;
//             }
//             case Subtractl:
//             {
//                 auto n2 = stack.popVal<uint64_t>();
//                 auto n1 = stack.popVal<uint64_t>();
//                 stack.pushVal(n1 - n2);
//                 break;
//             }
//             case Read:
//             {
//                 auto addr = stack.popVal<uint64_t>();
//                 auto size = *((uint64_t *)&code[i + 1].byte);
//                 i += sizeof(uint64_t);
//                 auto begin = stack.getTop();
//                 stack.push(size);
//                 for (uint64_t j = 0; j < size; ++j)
//                     stack.writeVal<uint8_t>(begin + j, stack.readVal<uint8_t>(addr + j));
//                 break;
//             }
//             case ReadConst:
//             {
//                 auto addr = *((uint64_t *)&code[i + 1].byte);
//                 i += sizeof(uint64_t);
//                 auto size = *((uint64_t *)&code[i + 1].byte);
//                 i += sizeof(uint64_t);
//                 auto begin = stack.getTop();
//                 stack.push(size);
//                 for (uint64_t j = 0; j < size; ++j)
//                     stack.writeVal<uint8_t>(begin + j, constants[addr + j]);
//                 break;
//             }
//             }
//         }
//     }
// }