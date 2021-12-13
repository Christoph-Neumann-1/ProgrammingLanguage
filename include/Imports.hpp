#pragma once
#include <dlfcn.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>
#include <ByteCode.hpp>
#include <iostream>
#include <filesystem>
//TODO: map of all imported symbols
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
    using FFIFunc = void (*)(Stack *, VM::VM *);

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
            //TODO: convert to proper type
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
                //TODO: in the case of a ffi function, store the pointer to the function itself, as well as to the wrapper function
                //If the function is imported from a c++ module, the direct pointer should be used. The isC flag can then be removed.
                FFIFunc ffiFunc;
                instruction::ByteCodeElement *bc;
            };
            //To save space this is also used to tell the vm if the function is a ffi function or not
            void *directFunc = nullptr;
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
        uint64_t internalStart;
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
        void staticLink();
        //TODO: load from file
    };
    class ImportManager
    {
        std::unordered_map<std::string, ImportedFileData> importedFiles;
        instruction::ByteCodeElement *main = nullptr; //TODO: make this an exported function
        //After linking the original files may be unloaded. All the relevant data is then stored here.

        std::vector<DLHandle> dlHandles;
        std::vector<std::unique_ptr<instruction::ByteCodeElement[]>> byteCodes;

        std::vector<std::string> includePaths;

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

        //The pointers returned by the following functions are non-owning

        ImportedFileData *LoadNativeModule(const std::string &path, const std::string &name)
        {
            auto handle = dlopen(path.c_str(), RTLD_LAZY);
            if (!handle)
                throw std::runtime_error("Could not load native module: " + std::string(dlerror()));
            auto func = reinterpret_cast<VWA::Imports::ImportedFileData (*)(VWA::Imports::ImportManager * manager)>(dlsym(handle, "MODULE_ENTRY_POINT"));
            if (!func)
            {
                std::cerr << dlerror() << std::endl;
                throw std::runtime_error("Could not find MODULE_ENTRY_POINT in native module: " + path);
            }
            auto module = func(this);
            module.dlHandle.handle = handle;
            return makeModuleAvailable(name, std::move(module));
        }

        ImportedFileData *LoadInterfaceFile(const std::string &name)
        {
        }

        ImportedFileData *LoadCompiledFile(const std::string &name)
        {
        }

        ImportedFileData *LoadModuleFromFile(const std::string &fname)
        {
            //Find the file, to do this check all directories in the include path, starting with native modules, continuing with interfaces and ending with
            //compiled modules. I might implement a mechanism to parse a file directly later.
            //TODO: support modules
            for (auto &path : includePaths)
            {
                auto fullName = path + "/" + fname + ".native";
                if (std::filesystem::exists(fullName))
                {
                    return LoadNativeModule(fullName, fname);
                }
            }
            throw std::runtime_error("Could not find module: " + fname);
        }

    public:
        ImportManager()
        {
        }
        //Ideally provide a absolute path
        void AddIncludePath(const std::string &path)
        {
            includePaths.emplace_back(path);
        }
        ImportedFileData *getModule(const std::string &moduleName)
        {
            auto it = importedFiles.find(moduleName);
            if (it == importedFiles.end())
            {
                return LoadModuleFromFile(moduleName);
            }
            return &it->second;
        }
        //TODO: ability to tell it not to set anything up.
        ImportedFileData *makeModuleAvailable(const std::string &moduleName, ImportedFileData &&file)
        {
            auto [f, _] = importedFiles.emplace(std::make_pair(moduleName, std::move(file)));
            f->second.Setup(this);
            if (f->second.hasMain)
            {
                if (main)
                    throw std::runtime_error("Main function already defined");
                main = f->second.main;
            }
            return &f->second;
        }
        void compact()
        {
            TransferContents();
            importedFiles.clear();
        }
        instruction::ByteCodeElement *getMain()
        {
            return main;
        }
    };
}