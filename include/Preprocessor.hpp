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

    struct PreprocessorContext
    {
        File file;
        ILogger &logger = defaultLogger;
        std::unordered_map<std::string, File> macros = {};
        std::unordered_map<std::string, int> counters = {};
        struct BuiltIn
        {
        };
        std::unordered_map<std::string,BuiltIn> builtins = {};

    private:
        static VoidLogger defaultLogger;
    };

    /**
     * @brief Expands macros in the file
     * 
     * @return File with macros expanded
     */
    File preprocess(PreprocessorContext context);
}
