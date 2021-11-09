#include <Interpreter.hpp>
#include <stdexcept>
namespace VWA
{

    void Interpreter::exec(uint64_t pos)
    {
        for (auto i = pos; i < code.size(); ++i)
        {
            switch (code[i])
            {
            case instruction::Exit:
                //TODO: use c++ style cast
                throw InterpreterExit(*((int *)&code[i + 1]));
            case instruction::Call:
            {
                auto idx = *((uint64_t *)&code[i + 1]);
                i += sizeof(uint64_t);
                if (idx > externFunctions.size())
                    throw std::runtime_error("Invalid function index");
                externFunctions[idx](&stack, this);
                break;
            }
            }
        }
    }

}