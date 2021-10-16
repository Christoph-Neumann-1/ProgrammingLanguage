#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <filesystem>
#include <File.hpp>

//TODO reduce copies of strings. Use segments of data to reduce the amount of characters moved when replacing stuff
//TODO integrate with math interpreter for full math support
//TODO command to evaluate the value of a define and store it. Example: #define x #y #define y test ##eval x this should not support any directives, only try to look up the value,
//until there is no more # at the beginning
//TODO possible argument evaluation everywhere(like in include directives)
//TODO DRY
//TODO track lines
//TODO more built in functions(file name, line number, error, assert, to uppercase ...)
//TODO possible injection of functions for later
//TODO includes relative to the file they are written in
//TODO document
//TODO debug logging
//Do i need to erase withspaces after commands?

#define PREPROCESSOR_BINARY_MATH_OP(op)                      \
    auto args = getArgs(2, 3);                               \
    auto val1 = evaluateArgument(args[0], evaluateArgument); \
    auto val2 = evaluateArgument(args[1], evaluateArgument); \
    counters[args[args.size() == 3 ? 2 : 0]] = val1 op val2; \
    advanceLine();                                           \
    continue;

#define PREPROCESSOR_MATH_COMPARISON(op)                                                                                                \
    auto args = getArgs(1, 2);                                                                                                          \
    currentMacro.line = ifblock([&]()                                                                                                   \
                                { return evaluateArgument(args[0], evaluateArgument) op evaluateArgument(args[1], evaluateArgument); }, \
                                args.size() > 2 ? args[2] : "");                                                                        \
    currentMacro.firstChar = 0;                                                                                                         \
    continue;

namespace VWA
{

    namespace
    {
    }

    File preprocess(File inputFile, std::unordered_map<std::string, std::string> defines = {}, std::unordered_map<std::string, int> counters = {})
    {
        //If the last character in a line is a \, merge it with the next line. Also removes empty lines.
        for (auto line = inputFile.begin(); line != inputFile.end(); line++)
        {
            if (line->content.empty())
            {
                line = inputFile.removeLine(line);
                continue;
            }
            if (line->content.back() == '\\')
            {
                auto next = line + 1;
                if (next == inputFile.end())
                {
                    continue;
                }
                line->content.pop_back();
                line->content += next->content;
                inputFile.removeLine(next);
            }
        }

        //All lines where starting with // are comments, this loop exists, too allow for multiline comments with escaped newlines
        for (auto it = inputFile.begin(); it != inputFile.end(); it++)
        {
            if (it->content.length() >= 2)
                if (it->content[0] == '/' && it->content[1] == '/')
                {
                    it = inputFile.removeLine(it);
                }
        }

        std::unordered_map<std::string, File> macros;

        auto current = inputFile.begin();
        while (true)
        {
            auto res = inputFile.find("MACRO", current);
            if (res.firstChar == std::string::npos)
                break;
            if (res.firstChar != 0)
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
                    throw std::runtime_error("ENDMACRO not found");
                }
                if (!res2.firstChar)
                {
                    endMacro = res2.line;
                    break;
                }
                endMacro = res2.line + 1;
            }

            if (res.line->content.size() < 7)
                throw std::runtime_error("macro name not found");

            auto space = res.line->content.find(' ');
            std::string macroName = res.line->content.substr(6, space - 6);
            auto body = inputFile.subFile(res.line + 1, endMacro);
            auto macro = macros.find(macroName);
            if (macro != macros.end())
            {
                throw std::runtime_error("Macro already defined: " + macroName);
            }
            macros.emplace(std::pair<std::string, File>{macroName, body});
            current = inputFile.removeLines(res.line, endMacro + 1);
        }
        for (auto currentMacro = inputFile.find('#'); currentMacro.firstChar != std::string::npos; currentMacro = inputFile.find('#', currentMacro.line, currentMacro.firstChar))
        {
            auto handleEscapeSequence = [](std::string &string, size_t &pos) -> bool
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
            };
            if (handleEscapeSequence(currentMacro.line->content, currentMacro.firstChar))
            {
                //TODO escape directives
                currentMacro.firstChar++;
                continue;
            }

            if (currentMacro.line->content[currentMacro.firstChar + 1] == '#' && currentMacro.firstChar == 0)
            {
                size_t macroNameEnd = currentMacro.line->content.find(' ', currentMacro.firstChar + 2);
                if (macroNameEnd == std::string::npos)
                {
                    macroNameEnd = currentMacro.line->content.length();
                }
                std::string macroName(currentMacro.line->content.substr(currentMacro.firstChar + 2, macroNameEnd - currentMacro.firstChar - 2));
                size_t macroEnd = macroNameEnd;
                auto advanceLine = [&]()
                {
                    currentMacro.line = inputFile.removeLine(currentMacro.line);
                    currentMacro.firstChar = 0;
                };
                auto ifblock = [&](auto &&condition, const std::string &name) -> File::iterator
                {
                    auto endIf = currentMacro.line + 1;
                    File::FilePos res;
                    if (name.empty())
                    {
                        res = inputFile.find("##endif", endIf);
                        if (res.firstChar == std::string::npos)
                        {
                            throw std::runtime_error("ENDIF not found");
                        }
                    }
                    else
                    {
                        res = inputFile.find("##endif " + name, endIf);
                        if (res.firstChar == std::string::npos)
                        {
                            throw std::runtime_error("ENDIF not found " + name);
                        }
                    }
                    if (condition())
                    {
                        inputFile.removeLine(res.line);
                        return inputFile.removeLine(currentMacro.line);
                    }
                    else
                    {
                        return inputFile.removeLines(currentMacro.line, res.line + 1);
                    }
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
                    auto nextSpace = currentMacro.line->content.find(' ', macroEnd + 1);
                    if (nextSpace == std::string::npos)
                    {
                        if (macroEnd == currentMacro.line->content.length())
                        {
                            return std::string();
                        }
                        else
                        {
                            nextSpace = currentMacro.line->content.length();
                        }
                    }
                    auto arg = currentMacro.line->content.substr(macroEnd + 1, nextSpace - macroEnd - 1);
                    macroEnd = nextSpace;
                    return arg;
                };
                auto getArgs = [&](int min = -1, int max = -1) -> std::vector<std::string>
                {
                    std::vector<std::string> args;
                    while (true)
                    {
                        if (args.size() == max)
                            break;
                        auto arg = getNextArg();
                        if (arg.empty())
                        {
                            break;
                        }
                        args.push_back(arg);
                    }
                    if (args.size() < min)
                        throw std::runtime_error("Not enough arguments given to preprocessor directive");
                    return args;
                };
                if (macroName == "include")
                {
                    //TODO support include dirs
                    auto path = (std::filesystem::absolute(std::filesystem::path(*currentMacro.line->fileName).parent_path()) / currentMacro.line->content.substr(currentMacro.firstChar + 10)).string();
                    std::ifstream file(path);
                    if (!file.is_open())
                    {
                        throw std::runtime_error("Could not open file: " + path);
                    }
                    File includeFile(file, std::make_shared<std::string>(path));
                    inputFile.insertAfter(std::move(includeFile), currentMacro.line);
                    advanceLine();
                    continue;
                }
                if (macroName == "define")
                {
                    auto what = getNextArg();
                    std::string value;
                    value = getNextArg();
                    defines[what] = value;
                    advanceLine();
                    continue;
                }
                if (macroName == "undef")
                {
                    auto what = getNextArg();
                    if (what.empty())
                        throw std::runtime_error("No argument given to undef");

                    defines.erase(what);
                    advanceLine();
                    continue;
                }
                if (macroName == "ifdef")
                {
                    auto args = getArgs(1, 2);
                    currentMacro.line = ifblock([&]()
                                                { return defines.contains(args[0]); },
                                                args.size() > 1 ? args[1] : "");
                    currentMacro.firstChar = 0;
                    continue;
                }
                if (macroName == "ifndef")
                {
                    auto args = getArgs(1, 2);
                    currentMacro.line = ifblock([&]()
                                                { return !defines.contains(args[0]); },
                                                args.size() > 1 ? args[1] : "");
                    currentMacro.firstChar = 0;
                    continue;
                }
                if (macroName == "ifeq")
                {
                    PREPROCESSOR_MATH_COMPARISON(==)
                }
                if (macroName == "ifneq")
                {
                    PREPROCESSOR_MATH_COMPARISON(!=)
                }
                if (macroName == "ifgt")
                {
                    PREPROCESSOR_MATH_COMPARISON(>)
                }
                if (macroName == "iflt")
                {
                    PREPROCESSOR_MATH_COMPARISON(<)
                }
                if (macroName == "ifgteq")
                {
                    PREPROCESSOR_MATH_COMPARISON(>=)
                }
                if (macroName == "iflteq")
                {
                    PREPROCESSOR_MATH_COMPARISON(<=)
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
                    advanceLine();
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
                    advanceLine();
                    continue;
                }
                if (macroName == "set")
                {
                    auto args = getArgs(1, 2);
                    int value;
                    if (args.size() == 1)
                        value = 0;
                    else
                        value = evaluateArgument(args[1], evaluateArgument);
                    counters[args[0]] = value;
                    advanceLine();
                    continue;
                }
                if (macroName == "del")
                {
                    auto name = getNextArg();
                    if (name.empty())
                        throw std::runtime_error("del: no name");
                    auto it = counters.find(name);
                    if (it == counters.end())
                    {
                        //TODO: debug message about alredy being deleted
                    }
                    counters.erase(it);
                    advanceLine();
                    continue;
                }
                if (macroName == "mul")
                {
                    PREPROCESSOR_BINARY_MATH_OP(*)
                }
                if (macroName == "div")
                {
                    PREPROCESSOR_BINARY_MATH_OP(/)
                }
                if (macroName == "mod")
                {
                    PREPROCESSOR_BINARY_MATH_OP(%)
                }
                if (macroName == "add")
                {
                    PREPROCESSOR_BINARY_MATH_OP(+)
                }
                if (macroName == "sub")
                {
                    PREPROCESSOR_BINARY_MATH_OP(-)
                }
                throw std::runtime_error("Unknown preprocessor command");
            }

            //Paste the macro, but don't evaluate its contents
            if (currentMacro.line->content[currentMacro.firstChar + 1] == '!')
            {
                size_t macroNameEnd = currentMacro.line->content.find(' ', currentMacro.firstChar + 2);
                if (macroNameEnd == std::string::npos)
                {
                    macroNameEnd = currentMacro.line->content.length();
                }
                std::string macroName(currentMacro.line->content.substr(currentMacro.firstChar + 2, macroNameEnd - currentMacro.firstChar - 2));
                if (auto it = defines.find(macroName); it != defines.end())
                {
                    currentMacro.line->content.replace(currentMacro.firstChar, macroNameEnd - currentMacro.firstChar + 1, it->second);
                    currentMacro.firstChar += it->second.length();
                    continue;
                }
                if (auto it2 = counters.find(macroName); it2 != counters.end())
                {
                    auto asString = std::to_string(it2->second);
                    currentMacro.line->content.replace(currentMacro.firstChar, macroNameEnd - currentMacro.firstChar + 1, asString);
                    currentMacro.firstChar += asString.length();
                    continue;
                }
                if (auto macro = macros.find(macroName); macro != macros.end())
                {
                    File body(macro->second);
                    auto bodyCenter = body.begin() + 1 != body.end() ? body.extractLines(body.begin() + 1, body.end() - 1) : body.extractLines(body.end(), body.end());
                    //Remove macro and split string
                    currentMacro.line->content.erase(currentMacro.firstChar, macroNameEnd - currentMacro.firstChar + 1);
                    auto remaining = currentMacro.line->content.substr(currentMacro.firstChar);
                    currentMacro.line->content.erase(currentMacro.firstChar);
                    currentMacro.line->content += body.begin()->content;
                    auto nextPos = currentMacro;
                    //If there is a second line, insert it
                    if (body.begin() + 1 != body.end())
                    {
                        inputFile.insertAfter(currentMacro.line, (body.end() - 1)->content + remaining);
                        nextPos.line = currentMacro.line + 1;
                        nextPos.firstChar = body.end()->content.length();
                    }
                    else
                    {
                        nextPos.firstChar=nextPos.line->content.length();
                        currentMacro.line->content += remaining;
                    }

                    inputFile.insertAfter(std::move(bodyCenter), currentMacro.line);
                    currentMacro = nextPos;
                    continue;
                }
                throw std::runtime_error("Unknown Identifier: " + macroName);
            }

            bool macroHasArgs = true;
            size_t macroNameEnd = currentMacro.line->content.find('(', currentMacro.firstChar);
            if (macroNameEnd == std::string::npos)
            {
                macroHasArgs = false;
                macroNameEnd = currentMacro.line->content.find(' ', currentMacro.firstChar);
            }
            if (macroNameEnd == std::string::npos)
            {
                macroNameEnd = currentMacro.line->content.length();
            }
            std::string macroName = currentMacro.line->content.substr(currentMacro.firstChar + 1, macroNameEnd - currentMacro.firstChar - 1);
            macroName.erase(std::remove(macroName.begin(), macroName.end(), ' '), macroName.end());

            if (!macroHasArgs)
            {
                if (auto it = defines.find(macroName); it != defines.end())
                {
                    currentMacro.line->content.replace(currentMacro.firstChar, macroNameEnd - currentMacro.firstChar + 1, it->second);
                    continue;
                }
                if (auto it2 = counters.find(macroName); it2 != counters.end())
                {
                    auto asString = std::to_string(it2->second);
                    currentMacro.line->content.replace(currentMacro.firstChar, macroNameEnd - currentMacro.firstChar + 1, asString);
                    continue;
                }
            }
            std::vector<std::string> args;
            auto macroEnd = macroNameEnd;
            if (macroHasArgs)
            {
                size_t currentpos = macroNameEnd + 1;

                while (true)
                {
                    currentpos = currentMacro.line->content.find(')', currentpos);
                    if (currentpos == std::string::npos)
                    {
                        throw std::runtime_error("Macro arguments not closed");
                    }
                    if (handleEscapeSequence(currentMacro.line->content, currentpos))
                    {
                        currentpos++;
                        continue;
                    }
                    break;
                }
                auto macroArgsLast = currentpos - 1;
                auto macroArgsStart = macroNameEnd + 1;
                auto macroArgsCurrent = macroArgsStart - 1;
                for (size_t i = macroArgsStart; i <= macroArgsLast; i++)
                {
                    if (currentMacro.line->content[i] == ',')
                    {
                        if (i > macroArgsStart)
                        {
                            if (currentMacro.line->content[i - 1] != '\\')
                            {
                                args.push_back(currentMacro.line->content.substr(macroArgsCurrent + 1, i - macroArgsCurrent - 1));
                                macroArgsCurrent = i + 1;
                            }
                            else
                            {
                                currentMacro.line->content.erase(i - 1, 1);
                                i--;
                            }
                        }
                    }
                }
                if (macroArgsLast != macroArgsCurrent)
                    args.push_back(currentMacro.line->content.substr(macroArgsCurrent + 1, macroArgsLast - macroArgsCurrent));
                macroEnd = currentpos;
            }
            auto macro = macros.find(macroName);
            if (macro == macros.end())
            {
                throw std::runtime_error("Identifier not found" + macroName);
            }

            File body(macro->second);
            for (size_t i = 0; i < args.size(); i++)
            {
                File::FilePos arg{body.begin(), 0};
                while (1)
                {
                    arg = body.find("$" + std::to_string(i), arg.line, arg.firstChar);
                    if (arg.firstChar == std::string_view::npos)
                        break;
                    arg.line->content.replace(arg.firstChar, 2, args[i]);
                }
            }
            auto bodyCenter = body.begin() + 1 != body.end() ? body.extractLines(body.begin() + 1, body.end() - 1) : body.extractLines(body.end(), body.end());
            //Remove macro and split string
            currentMacro.line->content.erase(currentMacro.firstChar, macroEnd - currentMacro.firstChar + 1);
            auto remaining = currentMacro.line->content.substr(currentMacro.firstChar);
            currentMacro.line->content.erase(currentMacro.firstChar);
            currentMacro.line->content += body.begin()->content;
            //If there is a second line, insert it
            if (body.begin() + 1 != body.end())
            {
                inputFile.insertAfter(currentMacro.line, (body.end() - 1)->content + remaining);
            }
            else
            {
                currentMacro.line->content += remaining;
            }

            inputFile.insertAfter(std::move(bodyCenter), currentMacro.line);
        }
        {
            auto space = inputFile.find("\\ ");
            while (space.firstChar != std::string::npos)
            {
                space.line->content.replace(space.firstChar, 2, "");
                space = inputFile.find("\\ ", space.line, space.firstChar);
            }
        }
        return inputFile;
    }
}

#undef PREPROCESSOR_MATH_COMPARISON
#undef PREPROCESSOR_BINARY_MATH_OP