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
//TODO: remove unneccessary empty lines
//TODO: reuse vectors
//TODO: try rewriting in rust
//Do i need to erase whitespaces after commands?

namespace VWA
{
    struct PreprocessorException : std::runtime_error
    {
        PreprocessorException(const std::string &what) : std::runtime_error(what) {}
    };

    class PreprocessorCommand; //TODO: autoregistration and stuff

    struct PreprocessorContext
    {
        File file;
        ILogger &logger = defaultLogger;
        std::unordered_map<std::string, File> macros = {};
        std::unordered_map<std::string, int> counters = {};
        const std::unordered_map<std::string, std::unique_ptr<PreprocessorCommand>> &commands = {};

    private:
        static VoidLogger defaultLogger;
    };

    //TODO: name getters and a way to specify expected number of arguments and other constraints
    class PreprocessorCommand
    {
    public:
        const bool requireStartOfLine;
        const bool requireEndOfLine;
        const bool erasesLine;
        const bool requiresRawArguments; //Skip breaking up the string into arguments and pass the unprocessed string as args[0]
        const int minArguments;
        const int maxArguments; //-1 for unlimited
        //TODO: additional constraints for args

        explicit PreprocessorCommand(const bool _requireStartOfLine = true, const bool _requireEndOfLine = true, const bool _erasesLine = true,
                                     const bool _requiresRawArguments = false, const int _minArgs = 0, const int _maxArgs = -1)
            : requireStartOfLine(_requireStartOfLine), requireEndOfLine(_requireEndOfLine), erasesLine(_erasesLine),
              requiresRawArguments(_requiresRawArguments), minArguments(_minArgs), maxArguments(_maxArgs) {}
        virtual ~PreprocessorCommand() = default;
        virtual File::FilePos operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args = {}) = 0;
    };

    class ReservedCommand : public PreprocessorCommand
    {
    public:
        ReservedCommand() : PreprocessorCommand(false, false, false) {}
        File::FilePos operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args = {}) override
        {
            throw PreprocessorException("Keyword may not be used in preprocessor " + fullIdentifier);
        }
    };

    class CommentCommand : public PreprocessorCommand
    {
    public:
        CommentCommand() : PreprocessorCommand(false, false, false, true) {}
        File::FilePos operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args = {}) override
        {
            if (args[0].empty())
                current.line->content.erase(current.firstChar);
            else
            {
                current.line->content.erase(current.firstChar, fullIdentifier.length() + 1);
            }
            return current;
        }
    };

    class SetterCommon : virtual public PreprocessorCommand
    {
    protected:
        void RemoveOldDefinition(PreprocessorContext &context, const std::string &identifier);
    };
    class DefineCommand : public SetterCommon
    {
    public:
        DefineCommand() : PreprocessorCommand(true, true, true, false, 1) {}
        File::FilePos operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args = {}) override;
    };

    class EvalCommand : public SetterCommon
    {
    public:
        EvalCommand() : PreprocessorCommand(true, true, true, false, 1) {}
        File::FilePos operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args = {}) override;
    };

    class MathCommand : public SetterCommon
    {
        int (*op)(int, int);

    public:
        File::FilePos operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args = {}) override;

    public:
        MathCommand(int (*op)(int, int)) : PreprocessorCommand(true, true, true, false, 2), op(op) {}
    };
    class MacrodefinitionCommand : public SetterCommon
    {
    public:
        //erasesLine is false, because the macro command needs to erase more than one line and its more efficient to delete all lines at once
        MacrodefinitionCommand() : PreprocessorCommand(true, true, false, false, 1, 1) {}
        File::FilePos operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args = {}) override;
    };

    class IntSetCommand : public SetterCommon
    {
    public:
        IntSetCommand() : PreprocessorCommand(true, true, true, false, 1) {}
        File::FilePos operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args = {}) override;
    };

    class DeleteCommand : public PreprocessorCommand
    {
    public:
        DeleteCommand() : PreprocessorCommand(true, true, true, false, 1) {}
        File::FilePos operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args = {}) override
        {
            for (auto &arg : args)
            {
                if (auto it = context.macros.find(arg); it != context.macros.end())
                {
                    context.macros.erase(it);
                }
                else if (auto it2 = context.counters.find(arg); it2 != context.counters.end())
                {
                    context.counters.erase(it2);
                }
            }
            return current;
        }
    };

    class NoEvalCommand : public PreprocessorCommand
    {
        public:
        NoEvalCommand() : PreprocessorCommand(false, false, false, true) {}
        File::FilePos operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args = {}) override;
    };

    class IncludeCommand : public PreprocessorCommand
    {
    public:
        IncludeCommand() : PreprocessorCommand(true, true, false, true) {}
        File::FilePos operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args = {}) override
        {
            auto path=std::filesystem::path(*current.line->fileName).parent_path() / args[0];
            std::ifstream file(path);
            if (!file.is_open())
                throw PreprocessorException("Could not open file " + path.string());
            File filecontents(file, path.string());
            context.file.insertAfter(filecontents, current.line);
            current.line=context.file.removeLine(current.line);
            current.firstChar=0;
            return current;
        }
    };

    /**
     * @brief Expands macros in the file
     * 
     * @return File with macros expanded
     */
    File
    preprocess(PreprocessorContext context);
}
