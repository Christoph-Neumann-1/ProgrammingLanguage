#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <filesystem>
#include <File.hpp>
#include <Logger.hpp>
#include <functional>

//TODO reduce copies of strings. Use segments of data to reduce the amount of characters moved when replacing stuff
//TODO integrate with math interpreter for full math support
//TODO DRY
//TODO document
//TODO debug logging
//TODO write specification
//TODO: Allow spaces in defines
//TODO: extract to cpp file
//TODO: extract lamdas
//TODO: better escape sequences
//TODO: proper support for builtin functions
//TODO: single # for commands
//Do i need to erase whitespaces after commands?

namespace VWA
{
    struct PreprocessorException : std::runtime_error
    {
        PreprocessorException(const std::string &what) : std::runtime_error(what) {}
    };

    class PreprocessorCommand;

    struct PreprocessorContext
    {
        File file;
        ILogger &logger = defaultLogger;
        std::unordered_map<std::string, File> macros = {};
        std::unordered_map<std::string, int> counters = {};
        std::unordered_map<std::string, std::unique_ptr<PreprocessorCommand>> commands = {};

    private:
        static VoidLogger defaultLogger;
    };

    //TODO: name getters and a way to specify expected number of arguments and other constraints
    class PreprocessorCommand
    {
    public:
        virtual ~PreprocessorCommand() = default;
        virtual File::FilePos operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args = {}) = 0;
    };

    class SetterCommon : public PreprocessorCommand
    {
    protected:
        void RemoveOldDefinition(PreprocessorContext &context, const std::string &identifier);
    };
    class DefineCommand : public SetterCommon
    {
        File::FilePos operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args = {}) override;
    };

    class EvalCommand : public SetterCommon
    {
        File::FilePos operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args = {}) override;
    };

    /**
     * @brief Expands macros in the file
     * 
     * @return File with macros expanded
     */
    File
    preprocess(PreprocessorContext context);
}
