#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <boost/regex.hpp>

//TODO ifdef and variables
//TODO require keywords not have letters at end/start

namespace VWA
{
    namespace
    {
        struct Macro
        {
            std::string body;
        };
        bool handleEscapeSequence(std::string &string, size_t &pos)
        {
            if (pos == 0)
                return 0;
            if (string[pos - 1] == '\\')
            {
                string.erase(pos - 1, 1);
                pos--;
                return true;
            }
            return false;
        }
    }
    std::string preprocess(std::string input)
    {
        {
            auto newline = input.find("\\\n");
            while (newline != std::string::npos)
            {
                input.erase(newline, 2);
                newline = input.find("\\\n");
            }
        }
        {
            boost::regex commentMatcher("(^|\n)//.*?(\n|$)");
            input = boost::regex_replace(input, commentMatcher, "$1");
        }
        std::unordered_map<std::string, Macro> macros;
        std::unordered_map<std::string, std::string> defines;
        std::unordered_map<std::string, int> counters;

        size_t currentMacroDefine = 0;
        while (true)
        {
            currentMacroDefine = input.find("MACRO", currentMacroDefine);
            if (currentMacroDefine == std::string::npos)
                break;
            if (currentMacroDefine != 0)
            {
                if (input[currentMacroDefine - 1] != '\n')
                {
                    currentMacroDefine += 5;
                    continue;
                }
            }

            auto endMacro = currentMacroDefine + 5;

            while (true)
            {
                endMacro = input.find("ENDMACRO", endMacro);
                if (endMacro == std::string_view::npos)
                {
                    throw std::runtime_error("ENDMACRO not found");
                }
                if (input[endMacro - 1] == '\n')
                {
                    break;
                }
                endMacro += 8;
            }

            auto macroNextLine = input.find('\n', currentMacroDefine);
            if (macroNextLine <= currentMacroDefine + 6)
            {
                throw std::runtime_error("Macro name not found");
            }
            std::string macroName(input.substr(currentMacroDefine + 6, macroNextLine - currentMacroDefine - 6));
            macroName.erase(std::remove(macroName.begin(), macroName.end(), ' '), macroName.end());
            auto macroBody = input.substr(macroNextLine + 1, endMacro - macroNextLine - 2);
            macros[macroName] = Macro{.body = std::string(macroBody)};
            auto nextNewLine = input.find('\n', endMacro);
            if (nextNewLine == std::string::npos)
                nextNewLine = input.length() - 1;
            input.replace(currentMacroDefine, nextNewLine - currentMacroDefine + 1, "");
        }
        for (auto currentMacro = input.find('#'); currentMacro != std::string::npos; currentMacro = input.find('#', currentMacro))
        {
            if (handleEscapeSequence(input, currentMacro))
            {
                currentMacro++;
                continue;
            }

            if (input[currentMacro + 1] == '#')
            {
                //TODO: regex
                auto macroNextLine = input.find('\n', currentMacro + 2);
                if (macroNextLine == std::string::npos)
                {
                    macroNextLine = input.length();
                }
                size_t macroNameEnd = input.find(' ', currentMacro + 2);
                if (macroNameEnd == std::string::npos || macroNameEnd > macroNextLine)
                {
                    macroNameEnd = macroNextLine;
                }
                std::string macroName(input.substr(currentMacro + 2, macroNameEnd - currentMacro - 2));

                size_t macroEnd = macroNameEnd;

                auto getNextArg = [&]() -> std::string
                {
                    auto nextSpace = input.find(' ', macroEnd + 1);
                    if (nextSpace == std::string::npos || nextSpace > macroNextLine)
                    {
                        if (macroEnd == macroNextLine)
                        {
                            throw std::runtime_error("Macro argument not found");
                        }
                        else
                        {
                            nextSpace = macroNextLine;
                        }
                    }
                    auto arg = input.substr(macroEnd+1, nextSpace - macroEnd-1);
                    macroEnd = nextSpace;
                    return arg;
                };
                if(macroName=="define")
                {
                    auto what=getNextArg();
                    auto value=getNextArg();
                    defines[what]=value;
                input.erase(currentMacro, macroNextLine - currentMacro+1);
                }
                continue;
            }

            bool macroHasArgs = true;
            auto macroNextLine = input.find('\n', currentMacro);
            if (macroNextLine == std::string::npos)
            {
                macroNextLine = input.length() - 1;
            }
            size_t macroNameEnd = input.find('(', currentMacro);
            if (macroNameEnd == std::string::npos || macroNameEnd > macroNextLine)
            {
                macroHasArgs = false;
                macroNameEnd = input.find(' ');
            }
            if (macroNameEnd == std::string::npos)
            {
                macroNameEnd = macroNextLine + 1;
            }
            std::string macroName(input.substr(currentMacro + 1, macroNameEnd - currentMacro - 1));
            macroName.erase(std::remove(macroName.begin(), macroName.end(), ' '), macroName.end());
            if (!macroHasArgs)
            {
                if (auto it = defines.find(macroName); it != defines.end())
                {
                    input.replace(currentMacro, macroNameEnd - currentMacro + 1, it->second);
                    continue;
                }
                if (auto it2 = counters.find(macroName); it2 != counters.end())
                {
                    auto asString = std::to_string(it2->second);
                    input.replace(currentMacro, macroNameEnd - currentMacro + 1, asString);
                    continue;
                }

                if (auto macro = macros.find(macroName); macro == macros.end())
                {
                    throw std::runtime_error("Identifier not found " + macroName);
                }
                else
                {
                    input.replace(currentMacro, macroNameEnd - currentMacro + 1, macro->second.body);
                    continue;
                }
            }
            else
            {
                auto macro = macros.find(macroName);
                if (macro == macros.end())
                {
                    throw std::runtime_error("Identifier not found" + macroName);
                }
                size_t macroArgsLast;
                size_t currentpos = 0;

                while (true)
                {
                    currentpos = input.find(')', currentpos);
                    if (currentpos == std::string::npos)
                    {
                        throw std::runtime_error("Macro arguments not found");
                    }
                    if (handleEscapeSequence(input, currentpos))
                    {
                        currentpos++;
                        continue;
                    }
                    break;
                }

                macroArgsLast = currentpos - 1;

                if (macroArgsLast == std::string_view::npos)
                {
                    throw std::runtime_error("Macro arguments end not found");
                }
                std::vector<std::string> macroArgs;
                auto macroArgsStart = macroNameEnd + 1;
                auto macroArgsCurrent = macroArgsStart;
                for (size_t i = macroArgsStart; i <= macroArgsLast; i++)
                {
                    if (input[i] == ',')
                    {
                        if (!(i > macroArgsStart && input[i - 1] == '\\'))
                        {
                            macroArgs.push_back(input.substr(macroArgsCurrent, i - macroArgsCurrent));
                            macroArgsCurrent = i + 1;
                        }
                    }
                }
                macroArgs.push_back(input.substr(macroArgsCurrent, macroArgsLast - macroArgsCurrent + 1));
                std::string macroBody = macro->second.body;
                for (size_t i = 0; i < macroArgs.size(); i++)
                {
                    auto macroArg = macroBody.find("$" + std::to_string(i));
                    while (macroArg != std::string_view::npos)
                    {
                        macroBody.replace(macroArg, 2, macroArgs[i]);
                        macroArg = macroBody.find("$" + std::to_string(i));
                    }
                }
                input.replace(currentMacro, macroArgsLast - currentMacro + 2, macroBody);
            }
        }
        {
            auto space = input.find("\\ ");
            while (space != std::string::npos)
            {
                input.replace(space, 2, "");
                space = input.find("\\ ", space);
            }
        }
        return input;
    }
}