#include <Imports.hpp>

namespace VWA::Imports
{
    void ImportedFileData::Setup(ImportManager *manager)
    {
        performFunctionOffsets();
        for (auto &func : importedFunctions)
        {
            auto module = manager->getModule(func.first);
            if (!module)
                throw std::runtime_error("Module not found");
            auto funcDecl = module->exportedFunctions.find(func.first);
            if (funcDecl == module->exportedFunctions.end())
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
            auto module = manager->getModule(struct_.first);
            if (!module)
                throw std::runtime_error("Module not found");
            auto structDecl = module->exportedStructs.find(struct_.first);
            if (structDecl == module->exportedStructs.end())
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

    void ImportedFileData::performBcOffsets()
    {
        //Dynamic libraries don't require any offseting
        //they also need to handle this differently, but I haven't put any thought into that yet.
        if (dlHandle)
        {
            throw std::runtime_error("Dynamic libraries not supported");
        }
        uint64_t offset = (uint64_t)bc.get();

        for (auto position = bc.get(); position < bc.get() + bcSize;)
        {
            switch (position->byteCode)
            {
                using namespace instruction;
            case JumpFFI:
                throw std::runtime_error("JumpFFI before linking");
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
            case JumpExternal:
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