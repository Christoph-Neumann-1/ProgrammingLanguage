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
//Do i need to erase whitespaces after commands?



namespace VWA
{
    struct PreprocessorException : std::runtime_error
    {
        PreprocessorException(const std::string &what) : std::runtime_error(what) {}
    };

    namespace
    {
        void FindMacroDefinitions(File &inputFile, std::unordered_map<std::string, File> &macros, ILogger &logger)
        {
            auto current = inputFile.begin();
            while (true)
            {
                auto res = inputFile.find("MACRO", current);
                if (res.firstChar == std::string::npos)
                    break;
                if (res.firstChar)
                {
                    current = res.line + 1;
                    continue;
                }

                auto endMacro = res.line + 1;

                while (true)
                {
                    auto res2 = inputFile.find("ENDMACRO", endMacro);
                    if (res2.firstChar == std::string_view::npos)
                    {
                        logger << ILogger::Error;
                        logger.AtPos(*res.line) << "Unterminated MACRO " << ILogger::FlushNewLine;
                        throw PreprocessorException("Unterminated MACRO");
                    }
                    if (!res2.firstChar)
                    {
                        endMacro = res2.line;
                        break;
                    }
                    endMacro = res2.line + 1;
                }

                if (res.line->content.size() < 7)
                {
                    logger << ILogger::Error;
                    logger.AtPos(*res.line) << "Invalid MACRO definition. Missing identifier." << ILogger::FlushNewLine;
                    throw PreprocessorException("No MACRO identifier");
                }

                auto space = res.line->content.find(' ');
                std::string macroName = res.line->content.substr(6, space - 6);
                auto body = inputFile.subFile(res.line + 1, endMacro);
                auto macro = macros.find(macroName);
                if (macro != macros.end())
                {
                    logger << ILogger::Error;
                    logger.AtPos(*res.line) << "Duplicate MACRO definition for " << macroName << "Redefinition of multiline macros is not supported" << ILogger::FlushNewLine;
                    throw PreprocessorException("MACRO redefinition");
                }
                macros.emplace(std::pair<std::string, File>{macroName, body});
                current = inputFile.removeLines(res.line, endMacro + 1);
            }
        }

        void RemoveCommentLines(File &inputFile)
        {
            for (auto it = inputFile.begin(); it != inputFile.end(); it++)
            {
                if (it->content.length() >= 2)
                    if (it->content[0] == '/' && it->content[1] == '/')
                    {
                        it = inputFile.removeLine(it);
                    }
            }
        }
    }

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

