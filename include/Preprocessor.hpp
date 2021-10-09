#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <regex>
#include <cstdio>
#include <filesystem>

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
            //TODO include directories
            std::vector<std::filesystem::path> includedFiles;
            auto current = 0;
            while (true)
            {
                current = input.find("##include", current);
                if (current == std::string::npos)
                    break;
                if (input[current - 1] == '\\')
                {
                    input.erase(current - 1, 1);
                    current += 8;
                    continue;
                }
                auto next = input.find('\n', current);
                if (next == std::string::npos)
                    next = input.size();
                auto path = input.substr(current + 10, next - current - 10);
                auto file = fopen(path.c_str(), "r");
                if (!file)
                {
                    throw std::runtime_error("Could not open file: " + path);
                }
                std::string buffer;
                fseek(file, 0, SEEK_END);
                auto size = ftell(file);
                fseek(file, 0, SEEK_SET);
                buffer.resize(size);
                fread(&buffer[0], size, 1, file);
                fclose(file);
                input.replace(current, next - current, buffer);
                continue;
            }
        }
        {
            auto newline = input.find("\\\n");
            while (newline != std::string::npos)
            {
                input.erase(newline, 2);
                newline = input.find("\\\n");
            }
        }
        {
            std::regex commentMatcher("(^|\n)//.*?(\n|$)");
            input=std::regex_replace(input, commentMatcher, "$1");
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
                            throw std::runtime_error("Argument not found");
                        }
                        else
                        {
                            nextSpace = macroNextLine;
                        }
                    }
                    auto arg = input.substr(macroEnd + 1, nextSpace - macroEnd - 1);
                    macroEnd = nextSpace;
                    return arg;
                };
                if (macroName == "define")
                {
                    auto what = getNextArg();
                    std::string value;
                    try
                    {
                        value = getNextArg();
                    }
                    catch (...)
                    {
                        value = "";
                    }
                    defines[what] = value;
                    input.erase(currentMacro, macroNextLine - currentMacro + 1);
                    continue;
                }
                if (macroName == "undef")
                {
                    auto what = getNextArg();
                    defines.erase(what);
                    input.erase(currentMacro, macroNextLine - currentMacro + 1);
                    continue;
                }
                if (macroName == "ifdef")
                {
                    auto what = defines.contains(getNextArg());
                    size_t endIfDef = macroNextLine;
                    endIfDef = input.find("##endifdef", endIfDef);
                    if (endIfDef == std::string::npos)
                    {
                        throw std::runtime_error("#endifdef not found");
                    }
                    auto endIfDefNextLine = input.find('\n', endIfDef);
                    if (endIfDefNextLine == std::string::npos)
                    {
                        endIfDefNextLine = input.length() - 1;
                    }
                    if (!what)
                    {
                        input.erase(currentMacro, endIfDefNextLine - currentMacro + 1);
                    }
                    else
                    {
                        input.erase(endIfDef, endIfDefNextLine - endIfDef + 1);
                        input.erase(currentMacro, macroNextLine - currentMacro + 1);
                    }
                    continue;
                }
                if (macroName == "ifndef")
                {
                    auto what = defines.contains(getNextArg());
                    size_t endIfDef = macroNextLine;
                    endIfDef = input.find("##endifndef", endIfDef);
                    if (endIfDef == std::string::npos)
                    {
                        throw std::runtime_error("#endifndef not found");
                    }
                    auto endIfDefNextLine = input.find('\n', endIfDef);
                    if (endIfDefNextLine == std::string::npos)
                    {
                        endIfDefNextLine = input.length() - 1;
                    }
                    if (what)
                    {
                        input.erase(currentMacro, endIfDefNextLine - currentMacro + 1);
                    }
                    else
                    {
                        input.erase(endIfDef, endIfDefNextLine - endIfDef + 1);
                        input.erase(currentMacro, macroNextLine - currentMacro + 1);
                    }
                    continue;
                }
                //Only for defines and counters
                if (macroName == "pureText")
                {
                    auto name = getNextArg();
                    if (auto it = defines.find(name); it != defines.end())
                    {
                        input.replace(currentMacro, macroEnd - currentMacro + 1, it->second);
                        currentMacro += it->second.length();
                    }
                    else if (auto it2 = counters.find(name); it2 != counters.end())
                    {
                        auto asString = std::to_string(it2->second);
                        input.replace(currentMacro, macroEnd - currentMacro + 1, asString);
                        currentMacro += asString.length();
                    }
                    //Ignores args
                    else if (auto it3 = macros.find(name); it3 != macros.end())
                    {
                        input.replace(currentMacro, macroEnd - currentMacro + 1, it3->second.body);
                        currentMacro += it3->second.body.length();
                    }
                    else
                    {
                        throw std::runtime_error("Could not find: "+name);
                    }

                    continue;
                }

                throw std::runtime_error("Unknown preprocessor command");
            }

            bool macroHasArgs = true;
            auto macroNextLine = input.find('\n', currentMacro);
            if (macroNextLine == std::string::npos)
            {
                macroNextLine = input.length();
            }
            size_t macroNameEnd = input.find('(', currentMacro);
            if (macroNameEnd == std::string::npos || macroNameEnd > macroNextLine)
            {
                macroHasArgs = false;
                macroNameEnd = input.find(' ');
            }
            if (macroNameEnd == std::string::npos)
            {
                macroNameEnd = macroNextLine;
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
                auto macroArgsCurrent = macroArgsStart - 1;
                for (size_t i = macroArgsStart; i <= macroArgsLast; i++)
                {
                    if (input[i] == ',')
                    {
                        if (i > macroArgsStart)
                        {
                            if (input[i - 1] != '\\')
                            {
                                macroArgs.push_back(input.substr(macroArgsCurrent + 1, i - macroArgsCurrent - 1));
                                macroArgsCurrent = i + 1;
                            }
                            else
                            {
                                input.erase(i - 1, 1);
                                i--;
                            }
                        }
                    }
                }
                macroArgs.push_back(input.substr(macroArgsCurrent + 1, macroArgsLast - macroArgsCurrent));
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