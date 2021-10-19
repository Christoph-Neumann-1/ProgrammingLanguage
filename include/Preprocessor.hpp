#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <filesystem>
#include <File.hpp>
#include <Logger.hpp>

//TODO reduce copies of strings. Use segments of data to reduce the amount of characters moved when replacing stuff
//TODO integrate with math interpreter for full math support
//TODO DRY
//TODO document
//TODO debug logging
//TODO write specification
//TODO: Allow spaces in defines
//TODO: extract to cpp file
//TODO: extract lamdas
//TODO: unified representation for single and multiline macros
//Do i need to erase whitespaces after commands?



namespace VWA
{
    struct PreprocessorException : std::runtime_error
    {
        PreprocessorException(const std::string &what) : std::runtime_error(what) {}
    };

    /**
     * @brief Expands macros in the file
     * 
     * @param inputFile 
     * @param logger output for error messages
     * @param defines if you wish to pass defines, pass them as a map of string to string
     * @param counters same for integers
     * @return File with macros expanded
     */
    File preprocess(File inputFile, ILogger &logger, std::unordered_map<std::string, std::string> defines = {}, std::unordered_map<std::string, int> counters = {});
}

