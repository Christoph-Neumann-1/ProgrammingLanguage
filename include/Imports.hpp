#pragma once
#include <dlfcn.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>
#include <ByteCode.hpp>
namespace VWA
{
    namespace VM
    {
        class VM;
    }
    class Stack;
}
namespace VWA::Imports
{
    using FFIFunc = void (*)(Stack *, VM::VM *, uint8_t *stackBase);

    class ImportManager;
    struct DLHandle
    {
        void *handle = nullptr;
        ~DLHandle()
        {
            if (handle)
                dlclose(handle);
        }
        operator void *()
        {
            return handle;
        }
        DLHandle() = default;
        DLHandle(const DLHandle &) = delete;
        DLHandle(DLHandle &&other) : handle(other)
        {
            other.handle = nullptr;
        }
        DLHandle &operator=(const DLHandle &) = delete;
        DLHandle &operator=(DLHandle &&other)
        {
            if (handle)
                dlclose(handle);
            handle = other;
            other.handle = nullptr;
            return *this;
        }
    };
    struct ImportedFileData
    {
        struct FuncDef
        {
            std::string name;
            std::string returnType;
            struct Parameter
            {
                std::string typeName;
                bool isMutable;
            };
            std::vector<Parameter> parameters;
            union
            {
                FFIFunc func;
                instruction::ByteCodeElement *bc;
            };
            bool isC;
            bool operator==(const FuncDef &other) const
            {
                return name == other.name && returnType == other.returnType && [&]()
                {
                    if (parameters.size() != other.parameters.size())
                        return false;
                    for (size_t i = 0; i < parameters.size(); i++)
                    {
                        if (parameters[i].typeName != other.parameters[i].typeName || parameters[i].isMutable != other.parameters[i].isMutable)
                            return false;
                    }
                    return true;
                }();
            }
            bool operator!=(const FuncDef &other) const
            {
                return !(*this == other);
            }
        };

        struct StructDef
        {
            std::string name;
            struct Field
            {
                std::string typeName;
                bool isMutable;
            };
            std::vector<Field> fields;
            bool operator==(const StructDef &other) const
            {
                return name == other.name && [&]()
                {
                    if (fields.size() != other.fields.size())
                        return false;
                    for (size_t i = 0; i < fields.size(); i++)
                    {
                        if (fields[i].typeName != other.fields[i].typeName || fields[i].isMutable != other.fields[i].isMutable)
                            return false;
                    }
                    return true;
                }();
            }
            bool operator!=(const StructDef &other) const
            {
                return !(*this == other);
            }
        };
        std::unordered_map<std::string, FuncDef> exportedFunctions;
        std::vector<std::pair<std::string, FuncDef>> importedFunctions;
        std::unordered_map<std::string, StructDef> exportedStructs;
        std::vector<std::pair<std::string, StructDef>> importedStructs;
        DLHandle dlHandle;
        std::unique_ptr<instruction::ByteCodeElement[]> bc;
        uint64_t bcSize;
        instruction::ByteCodeElement *main;
        bool hasMain = false;
        void Setup(ImportManager *manager);
        void performFunctionOffsets();
        void performBcOffsets();
        //TODO: load from file
    };
    class ImportManager
    {
        std::unordered_map<std::string, ImportedFileData> importedFiles;
        instruction::ByteCodeElement *main = nullptr;
        //After linking the original files may be unloaded. All the relevant data is then stored here.

        std::vector<DLHandle> dlHandles;
        std::vector<std::unique_ptr<instruction::ByteCodeElement[]>> byteCodes;

        void TransferContents()
        {
            for (auto &[_, file] : importedFiles)
            {
                if (file.dlHandle)
                    dlHandles.emplace_back(std::move(file.dlHandle));
                else
                    byteCodes.emplace_back(std::move(file.bc));
            }
        }

    public:
        //TODO: replace by file path.
        ImportManager(ImportedFileData &&file)
        {
            //TODO: fix
            auto [f, _] = importedFiles.emplace(std::make_pair("", std::move(file)));
            if (!f->second.hasMain)
                throw std::runtime_error("No main function found");
            f->second.Setup(this);
            main = f->second.main;
            TransferContents();
            importedFiles.clear();
        }
        ImportedFileData *getModule(const std::string &moduleName)
        {
            auto it = importedFiles.find(moduleName);
            if (it == importedFiles.end())
            {
                //TODO: load file.
            }
            return &it->second;
        }
        //TODO: ability to tell it not to set anything up.
        void makeModuleAvailable(const std::string &moduleName, ImportedFileData &&file)
        {
            auto [f, _] = importedFiles.emplace(std::make_pair(moduleName, std::move(file)));
            f->second.Setup(this);
            TransferContents();
            importedFiles.clear();
        }
        instruction::ByteCodeElement *getMain()
        {
            return main;
        }
    };
}