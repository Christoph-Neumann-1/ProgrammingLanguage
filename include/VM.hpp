#include <cstdint>
#include <Stack.hpp>
#include <ByteCode.hpp>
#include <unordered_map>
#include <list>
#include <vector>
#include <memory>

namespace VWA::VM
{
    class MemoryManager
    {
        Stack stack;
        //TODO:heap,constants,code? Memory map.

        friend class VM; //Vm needs the ability to access the stack and add new regions.
        //Returns a pointer the region of memory. Size is used to assert that it is within bounds
    public:
        template <typename T>
        const T *Read(uint64_t addr) const
        {
            return stack.readVal<T>(addr);
        }
        const uint8_t *Read(uint64_t addr, uint64_t size) const
        {
            return stack.readBytes(addr, size);
        }
        template <typename T>
        void Write(uint64_t addr, T value)
        {
            return stack.writeVal(addr, value);
        }
        void Write(uint64_t addr, uint64_t size, const uint8_t *source)
        {
            stack.writeBytes(addr, size, source);
        }
        //TODO: read n bytes
    };
    using FFIFunc = void (*)(Stack *, VM *);

    class VM
    {
    public: //TODO: make private again
        MemoryManager mmu;
        //When linking multiple files, this keeps track of local symbols initally, it gets deleted afterwards.
        // struct FileInfo
        // {
        //     //When performing local offsets, this gets changed to a pointer
        //     //This is what gets generated after deserialization. It is sufficient to compare type names, since there already is a check for different struct definitons
        //     struct FunctionSignature
        //     {
        //         std::string name;
        //         std::string returnType;
        //         struct Parameter
        //         {
        //             std::string typeName;
        //             bool isMutable;
        //         };
        //         std::vector<Parameter> parameters;
        //         union
        //         {
        //             FFIFunc func;
        //             instruction::ByteCodeElement *bc;
        //             uint64_t localIndex;
        //             FunctionSignature *declaration;
        //         };
        //         bool isC;
        //         bool hasBeenDeclared{false};
        //         bool operator==(const FunctionSignature &other) const
        //         {
        //             return name == other.name && returnType == other.returnType && [&]()
        //             {
        //                 if (parameters.size() != other.parameters.size())
        //                     return false;
        //                 for (size_t i = 0; i < parameters.size(); i++)
        //                 {
        //                     if (parameters[i].typeName != other.parameters[i].typeName || parameters[i].isMutable != other.parameters[i].isMutable)
        //                         return false;
        //                 }
        //                 return true;
        //             }();
        //         }
        //         bool operator!=(const FunctionSignature &other) const
        //         {
        //             return !(*this == other);
        //         }
        //     };
        //     std::vector<FunctionSignature> importedFunctions;
        //     std::vector<FunctionSignature> exportedFunctions;
        //     struct StructSignature
        //     {
        //         std::string name;
        //         struct Field
        //         {
        //             std::string typeName;
        //             bool isMutable;
        //         };
        //         std::vector<Field> fields;
        //         bool hasBeenDeclared{false};
        //         bool operator==(const StructSignature &other) const
        //         {
        //             return name == other.name && [&]()
        //             {
        //                 if (fields.size() != other.fields.size())
        //                     return false;
        //                 for (size_t i = 0; i < fields.size(); i++)
        //                 {
        //                     if (fields[i].typeName != other.fields[i].typeName || fields[i].isMutable != other.fields[i].isMutable)
        //                         return false;
        //                 }
        //                 return true;
        //             }();
        //         }
        //         bool operator!=(const StructSignature &other) const
        //         {
        //             return !(*this == other);
        //         }
        //     };
        //     //These vectors may be cleared after being loaded into the global table.
        //     std::vector<StructSignature> importedStructs;
        //     std::vector<StructSignature> exportedStructs;
        //     std::vector<std::string> importedFiles; //This still needs a proper implementation
        //     std::unique_ptr<instruction::ByteCodeElement[]> bc;
        //     uint64_t bcSize;
        //     //TODO: support dynamic libs.
        // };

        // std::vector<std::unique_ptr<instruction::ByteCodeElement[]>> byteCode;
        // std::unordered_map<std::string, FileInfo::FunctionSignature> functions;
        // std::unordered_map<std::string, FileInfo::StructSignature> structs;
        struct ExitException
        {
            int32_t exitCode;
        };

        template <typename T>
        T ReadInstructionArg(instruction::ByteCodeElement *begin)
        {
            return *reinterpret_cast<T *>(begin);
        }

        // void ProcessFile(FileInfo &file, std::list<FileInfo> &files);

        // void ValidateLinkage();

        // void PerformByteCodeOffsets(FileInfo &file);

        // void PerformLinking(FileInfo &&file)
        // {
        //     std::list<FileInfo> files;
        //     files.emplace_back(std::move(file));
        //     ProcessFile(*files.begin(), files);
        //     ValidateLinkage();
        //     for (auto &file_ : files)
        //         PerformByteCodeOffsets(file_);
        //     for (auto &file_ : files)
        //         byteCode.emplace_back(std::move(file_.bc));
        // }

    public:
        void exec(instruction::ByteCodeElement *instruction, uint8_t *stackBase);
        int run(instruction::ByteCodeElement *main)
        {
            try
            {
                mmu.stack.pushVal<uint8_t *>(nullptr);
                mmu.stack.pushVal<instruction::ByteCodeElement *>(nullptr);
                exec(main, mmu.stack.getData() + mmu.stack.getTop() - sizeof(uint8_t *) - sizeof(instruction::ByteCodeElement *));
            }
            catch (ExitException e)
            {
                return e.exitCode;
            }
            return mmu.stack.popVal<int32_t>();
        }
    };
}