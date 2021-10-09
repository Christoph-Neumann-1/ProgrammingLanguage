#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <regex>
#include <cstdio>
#include <filesystem>

//TODO variables
//TODO require keywords not have letters at end/start
//TODO reduce copies of strings. Use segments of data to reduce the amount of characters moved when replacing stuff
//TODO integrate with math interpreter for full math support
//TODO command to evaluate the value of a define and store it. Example: #define x #y #define y test ##eval x this should not support any directives, only try to look up the value,
//until there is no more # at the beginning
//TODO possible argument evaluation everywhere(like in include directives)
//TODO DRY

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
            size_t current = 0;
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
                // auto firstNewLine = buffer.find('\n');
                // if (firstNewLine == std::string::npos)
                //     firstNewLine = buffer.size();
                // std::string_view firstLine(buffer.data(), firstNewLine);
                // std::string_view contents(buffer);
                // if (firstLine.length() >= 6)
                //     if (firstLine.find("##once") != firstLine.npos)
                //     {
                //         bool hasBeenIncluded = false;
                //         //Check if file has already been included
                //         for (auto &mfile : includedFiles)
                //         {
                //             if (mfile.compare(path))
                //             {
                //                 hasBeenIncluded = true;
                //                 break;
                //             }
                //         }
                //         if (!hasBeenIncluded)
                //         {
                //             includedFiles.push_back(path);
                //             contents = contents.substr(firstNewLine, contents.size() - firstNewLine);
                //         }
                //         else
                //             contents = "";
                //     }
                input.replace(current, next - current, buffer);
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
            input = std::regex_replace(input, commentMatcher, "$1");
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
            macros[macroName] = Macro{.body = std::move(macroBody)};
            auto nextNewLine = input.find('\n', endMacro);
            if (nextNewLine == std::string::npos)
                nextNewLine = input.length() - 1;
            input.replace(currentMacroDefine, nextNewLine - currentMacroDefine + 1, "");
        }
        for (auto currentMacro = input.find('#'); currentMacro != std::string::npos; currentMacro = input.find('#', currentMacro))
        {
            if (handleEscapeSequence(input, currentMacro))
            {
                //TODO escape directives
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

                auto deleteLine = [&]()
                {
                    input.erase(currentMacro, macroNextLine - currentMacro + 1);
                };
                auto evaluateArgument = [&](const std::string &arg, auto &&evaluateArgument) -> int
                {
                    //If the argument is a number return it. It is considered a number if the first character is a digit.
                    if (std::isdigit(arg[0]))
                    {
                        return std::stoi(arg);
                    }
                    //Otherwise check if the argument is a counter
                    if (auto it = counters.find(arg); it != counters.end())
                    {
                        return it->second;
                    }
                    //As a last option check if it is a define and convert said refine to a number. This is recursive, so a this will run until the argument is a number, or empty.
                    if (auto it = defines.find(arg); it != defines.end())
                    {
                        return evaluateArgument(it->second, evaluateArgument);
                    }
                    throw std::runtime_error("Unknown argument: " + arg);
                };
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
                    deleteLine();
                    continue;
                }
                if (macroName == "undef")
                {
                    auto what = getNextArg();
                    defines.erase(what);
                    deleteLine();
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
                        deleteLine();
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
                        deleteLine();
                    }
                    continue;
                }
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
                        throw std::runtime_error("Could not find: " + name);
                    }

                    continue;
                }
                if (macroName == "inc")
                {
                    auto name = getNextArg();
                    auto it = counters.find(name);
                    if (it == counters.end())
                    {
                        throw std::runtime_error("Could not find: " + name);
                    }
                    it->second++;
                    deleteLine();
                    continue;
                }
                if (macroName == "dec")
                {
                    auto name = getNextArg();
                    auto it = counters.find(name);
                    if (it == counters.end())
                    {
                        throw std::runtime_error("Could not find: " + name);
                    }
                    it->second--;
                    deleteLine();
                    continue;
                }
                if (macroName == "set")
                {
                    auto name = getNextArg();
                    int value;
                    try
                    {
                        value = evaluateArgument(getNextArg(),evaluateArgument);
                    }
                    catch (...)
                    {
                        value=0;
                    }
                    counters[name] = value;
                    deleteLine();
                    continue;
                }
                if (macroName == "del")
                {
                    auto name = getNextArg();
                    auto it = counters.find(name);
                    if (it == counters.end())
                    {
                        //TODO: debug message about alredy being deleted
                    }
                    counters.erase(it);
                    deleteLine();
                    continue;
                }
                if (macroName == "mul")
                {
                    //TODO get number of args. If 2 then multiply and store in arg0
                    //This would not add any functionality, but it would make code more readable
                    auto result = getNextArg();
                    auto operand1 = getNextArg();
                    auto operand2 = getNextArg();
                    auto it = counters.find(operand1);
                    if (it == counters.end())
                    {
                        throw std::runtime_error("Could not find: " + operand1);
                    }
                    it->second = evaluateArgument(operand1, evaluateArgument) * evaluateArgument(operand2, evaluateArgument);
                    deleteLine();
                    continue;
                }
                if (macroName == "div")
                {
                    //TODO get number of args. If 2 then multiply and store in arg0
                    //This would not add any functionality, but it would make code more readable
                    auto result = getNextArg();
                    auto operand1 = getNextArg();
                    auto operand2 = getNextArg();
                    auto it = counters.find(operand1);
                    if (it == counters.end())
                    {
                        throw std::runtime_error("Could not find: " + operand1);
                    }
                    it->second = evaluateArgument(operand1, evaluateArgument) / evaluateArgument(operand2, evaluateArgument);
                    deleteLine();
                    continue;
                }
                if (macroName == "mod")
                {
                    //TODO get number of args. If 2 then multiply and store in arg0
                    //This would not add any functionality, but it would make code more readable
                    auto result = getNextArg();
                    auto operand1 = getNextArg();
                    auto operand2 = getNextArg();
                    auto it = counters.find(operand1);
                    if (it == counters.end())
                    {
                        throw std::runtime_error("Could not find: " + operand1);
                    }
                    it->second = evaluateArgument(operand1, evaluateArgument) % evaluateArgument(operand2, evaluateArgument);
                    deleteLine();
                    continue;
                }
                if (macroName == "add")
                {
                    //TODO get number of args. If 2 then multiply and store in arg0
                    //This would not add any functionality, but it would make code more readable
                    auto result = getNextArg();
                    auto operand1 = getNextArg();
                    auto operand2 = getNextArg();
                    auto it = counters.find(operand1);
                    if (it == counters.end())
                    {
                        throw std::runtime_error("Could not find: " + operand1);
                    }
                    it->second = evaluateArgument(operand1, evaluateArgument) + evaluateArgument(operand2, evaluateArgument);
                    deleteLine();
                    continue;
                }
                if (macroName == "sub")
                {
                    //TODO get number of args. If 2 then multiply and store in arg0
                    //This would not add any functionality, but it would make code more readable
                    auto result = getNextArg();
                    auto operand1 = getNextArg();
                    auto operand2 = getNextArg();
                    auto it = counters.find(operand1);
                    if (it == counters.end())
                    {
                        throw std::runtime_error("Could not find: " + operand1);
                    }
                    it->second = evaluateArgument(operand1, evaluateArgument) - evaluateArgument(operand2, evaluateArgument);
                    deleteLine();
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
            if (macroNameEnd == std::string::npos || macroNameEnd > macroNextLine)
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