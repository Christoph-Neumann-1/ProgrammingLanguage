#include <Imports.hpp>

namespace VWA::Imports
{
    void ImportedFileData::Setup(ImportManager *manager)
    {
        performFunctionOffsets();
        for (auto &func : importedFunctions)
        {
            auto mod = manager->getModule(func.first);
            if (!mod)
                throw std::runtime_error("Module not found");
            auto funcDecl = mod->exportedFunctions.find(func.first);
            if (funcDecl == mod->exportedFunctions.end())
                throw std::runtime_error("Function not found");
            if (funcDecl->second != func.second)
                throw std::runtime_error("Function signature mismatch");
            if (funcDecl->second.isC)
            {
                func.second.func = funcDecl->second.func;
            }
            else
            {
                func.second.bc = funcDecl->second.bc;
            }
        }
        for (auto &struct_ : importedStructs)
        {
            auto mod = manager->getModule(struct_.first);
            if (!mod)
                throw std::runtime_error("Module not found");
            auto structDecl = mod->exportedStructs.find(struct_.first);
            if (structDecl == mod->exportedStructs.end())
                throw std::runtime_error("Struct not found");
            if (structDecl->second != struct_.second)
                throw std::runtime_error("Struct signature mismatch");
        }
        performBcOffsets();
    }

    void ImportedFileData::performFunctionOffsets()
    {
        //Dynamic libraries don't require any offseting and don't have main.
        //I still don't know when to load the functions from them.
        if (dlHandle)
        {
            return;
        }
        uint64_t offset = (uint64_t)bc.get();
        if (hasMain)
            main += offset;
        for (auto &func : exportedFunctions)
        {
            func.second.bc += offset;
        }
    }

    void SkipInstruction(instruction::ByteCodeElement *&instr)
    {
        switch (instr->byteCode)
        {
            using namespace instruction;
        case PushConstN:
            instr += *reinterpret_cast<uint64_t *>(instr + 1);
        case PopMiddle:
        case Dup:
        case JumpFFI:
        case JumpFunc:
        case FCall:
        case ReadLocal:
        case WriteLocal:
        case ReadGlobal:
        case WriteGlobal:
            instr += sizeof(uint64_t);
        case Return:
        case Push:
        case Pop:
        case PushConst64:
        case Jump:
        case JumpIfFalse:
        case JumpIfTrue:
            instr += sizeof(uint32_t);
        case PushConst32:
            instr += 3 * sizeof(uint8_t);
        case PushConst8:
            instr += sizeof(uint8_t);
        default:
            instr++;
            break;
        }
    }

    void ImportedFileData::staticLink()
    {
        for (auto position = bc.get(); position < bc.get() + bcSize;)
        {
            if (position->byteCode == instruction::FCall)
            {
                auto where = reinterpret_cast<uint64_t *>(position + 1);
                if (*where >= internalStart)
                {
                    auto &func = importedFunctions[*where];
                    *where = reinterpret_cast<uint64_t>(func.second.bc);
                }
                position->byteCode = instruction::JumpFunc;
            }
            SkipInstruction(position);
        }
        for (auto n = importedFunctions.size() - internalStart; n > 0; --n)
        {
            importedFunctions.pop_back();
        }
        importedFunctions.shrink_to_fit();
    }

    void ImportedFileData::performBcOffsets()
    {
        //Dynamic libraries don't require any offseting
        //they also need to handle this differently, but I haven't put any thought into that yet.
        if (dlHandle)
        {
            return;
        }
        uint64_t offset = (uint64_t)bc.get();

        for (auto position = bc.get(); position < bc.get() + bcSize;)
        {
            switch (position->byteCode)
            {
                using namespace instruction;
            case JumpFFI:
                position += sizeof(FFIFunc) + sizeof(uint64_t) + 1;
                break;
                //TODO: add this back.
                // throw std::runtime_error("JumpFFI before linking");
            case Jump:
            case JumpIfFalse:
            case JumpIfTrue:
            {
                auto address = reinterpret_cast<ByteCodeElement **>(position + 1);
                *address += offset;
                position += sizeof(ByteCodeElement *) + 1;
                break;
            }
            case JumpFunc:
            {
                auto address = reinterpret_cast<ByteCodeElement **>(position + 1);
                *address += offset;
                position += sizeof(ByteCodeElement *) + 1 + sizeof(uint64_t);
                break;
            }
            case FCall:
            {
                auto index = reinterpret_cast<uint64_t *>(position + 1);
                auto &function = importedFunctions[*index].second;
                if (function.isC)
                {
                    auto addr = reinterpret_cast<FFIFunc *>(index);
                    *addr = function.func;
                    position->byteCode = JumpFFI;
                    position += 2 * sizeof(uint64_t) + 1;
                    break;
                }
                else
                {
                    auto addr = reinterpret_cast<ByteCodeElement **>(position + 1);
                    *addr = function.bc;
                    position->byteCode = JumpFunc;
                    position += 2 * sizeof(ByteCodeElement *) + 1;
                    break;
                }
            }
            case PushConstN:
                position += *reinterpret_cast<uint64_t *>(position + 1);
            case Return:
            case ReadLocal:
            case WriteLocal:
            case ReadGlobal:
            case WriteGlobal:
            case Push:
            case Pop:
            case PushConst64:
                position += sizeof(uint32_t);
            case PushConst32:
                position += 3 * sizeof(uint8_t);
            case PushConst8:
                position += sizeof(uint8_t);
            default:
                position++;
                break;
            }
        }
    }

}