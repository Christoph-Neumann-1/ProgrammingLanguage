#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <File.hpp>
#include <Logger.hpp>
#include <functional>
#include <optional>
#include <variant>
#include <stdexcept>

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
//TODO: try reducing number of classes by passing a lambda to the constructor
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
        const std::unordered_map<std::string, std::unique_ptr<PreprocessorCommand>> &commands = {};

    private:
        static VoidLogger defaultLogger;
    };
    std::optional<std::string> expandIdentifier(std::string identifier, PreprocessorContext &context);

    bool isKeyword(const std::string &identifier, const PreprocessorContext &ctxt);

    bool IsFirstNonSpace(const std::string_view &line, size_t pos);

    void PasteMacro(std::string identifier, File::FilePos position, PreprocessorContext &context, File::FilePos *nextCharacter = nullptr, size_t prefixLength = 1, size_t postfixLength = 0);

    std::optional<std::string> expandIdentifier(std::string identifier, PreprocessorContext &context);

    /**
     * @brief Expands macros in the file
     * 
     * @return File with macros expanded
     */
    File
    preprocess(PreprocessorContext context);
}
#include <Commands.hpp>