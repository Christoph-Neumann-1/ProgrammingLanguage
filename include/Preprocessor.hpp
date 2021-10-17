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

//For internal use only
#define PREPROCESSOR_BINARY_MATH_OP(op)                      \
    auto args = getArgs(2, 3);                               \
    auto val1 = evaluateArgument(args[0], evaluateArgument); \
    auto val2 = evaluateArgument(args[1], evaluateArgument); \
    counters[args[args.size() == 3 ? 2 : 0]] = val1 op val2; \
    advanceLine();                                           \
    continue;

//For internal use only
#define PREPROCESSOR_MATH_COMPARISON(op)                                                                                                \
    auto args = getArgs(2, 3);                                                                                                          \
    currentMacro.line = ifblock([&]()                                                                                                   \
                                { return evaluateArgument(args[0], evaluateArgument) op evaluateArgument(args[1], evaluateArgument); }, \
                                args.size() > 2 ? args[2] : "");                                                                        \
    currentMacro.firstChar = 0;                                                                                                         \
    continue;

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
    File preprocess(File inputFile, ILogger &logger, std::unordered_map<std::string, std::string> defines = {}, std::unordered_map<std::string, int> counters = {})
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

        RemoveCommentLines(inputFile);
        std::unordered_map<std::string, File> macros;
        FindMacroDefinitions(inputFile, macros, logger);

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

            /**
             * @brief This function expands the macro name, replacing all mentions of other define/macros with their values
             * 
             * @note Only single line macros are supported, because identifier may only span one line
             * 
             */
            auto evaluateIdentifier = [&](std::string name) -> std::string
            {
                auto getExpandedWithLength = [&](std::string input, size_t &length) -> std::string
                {
                    std::string identifer = input;
                    std::vector<std::string> args;
                    if (identifer.back() == ')')
                    {
                        auto open = identifer.find('(');
                        if (open == std::string::npos)
                        {
                            logger << ILogger::Error;
                            logger.AtPos(*currentMacro.line) << "Invalid macro call. Missing opening parenthesis" << ILogger::FlushNewLine;
                            throw PreprocessorException("Invalid macro call");
                        }
                        std::string argsString = identifer.substr(open + 1);

                        size_t begin = 0;
                        for (size_t i = 0; i < argsString.size(); i++)
                        {
                            if (argsString[i] == ',' && i > 0)
                            {
                                if (argsString[i - 1] != '\\')
                                {
                                    args.push_back(argsString.substr(begin, i - begin));
                                    begin = i + 1;
                                }
                                else
                                {
                                    argsString.erase(i - 1, 1);
                                    i--;
                                }
                            }
                        }
                        args.push_back(argsString.substr(begin));

                        identifer = identifer.substr(0, open);
                    }
                    if (auto macro = macros.find(identifer); macro != macros.end())
                    {
                        if (macro->second.begin() != macro->second.end() - 1)
                        {
                            logger << ILogger::Error;
                            logger.AtPos(*currentMacro.line) << "Macro " << identifer << " has more than one line and can't be used in identifiers" << ILogger::FlushNewLine;
                            throw PreprocessorException("Multiline Macro in identifier");
                        }
                        else
                        {
                            std::string line= macro->second.begin()->content;
                            for(int i=0;i<args.size();i++)
                            {
                                line = line.replace(line.find("$" + std::to_string(i)), 2, args[i]);
                            }
                            return line;
                        }
                    }
                    if (auto define = defines.find(identifer); define != defines.end())
                    {
                        length = define->second.length();
                        return define->second;
                    }
                    if (auto counter = counters.find(identifer); counter != counters.end())
                    {
                        length = std::to_string(counter->second).length();
                        return std::to_string(counter->second);
                    }
                    logger << ILogger::Error;
                    logger.AtPos(*currentMacro.line) << "Unknown identifier " << identifer << (identifer != input ? ("Expanded from " + input) : "") << ILogger::FlushNewLine;
                    throw PreprocessorException("Unknown identifier");
                };
                auto findEnd = [&](size_t firstChar) -> size_t
                {
                    auto res = name.find('#', firstChar);
                    if (res == std::string::npos)
                        res = name.size();
                    return res;
                };
                size_t current = 0;
                while (1)
                {
                    current = name.find('#', current);
                    if (current == name.npos)
                        break;
                    if (name[current + 1] == '!')
                    {
                        auto idend = findEnd(current + 2);
                        auto identifier = name.substr(current + 2, idend - current - 2);
                        size_t length;
                        name.replace(current, idend - current + 1, getExpandedWithLength(identifier, length));
                        current += length;
                    }
                    else
                    {
                        auto idend = findEnd(current + 1);
                        auto identifier = name.substr(current + 1, idend - current - 1);
                        size_t length;
                        name.replace(current, idend - current + 1, getExpandedWithLength(identifier, length));
                    }
                }
                return name;
            };

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
                            logger << ILogger::Error;
                            logger.AtPos(*currentMacro.line) << "No matching endif found for if" << ILogger::FlushNewLine;
                            throw PreprocessorException("No matching endif found for if");
                        }
                    }
                    else
                    {
                        res = inputFile.find("##endif " + name, endIf);
                        if (res.firstChar == std::string::npos)
                        {
                            logger << ILogger::Error;
                            logger.AtPos(*currentMacro.line) << "No matching endif found for named if " << name << ILogger::FlushNewLine;
                            throw PreprocessorException("ENDIF not found " + name);
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
                auto evaluateArgument = [&](std::string arg, auto &&evaluateArgument) -> int
                {
                    //If the argument is a number return it. It is considered a number if the first character is a digit.
                    if (std::isdigit(arg[0]) || arg[0] == '-')
                    {
                        return std::stoi(arg);
                    }
                    arg = evaluateIdentifier(arg);
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
                    logger << ILogger::Error;
                    logger.AtPos(*currentMacro.line) << "Invalid argument " << arg << "Could not convert to a number" << ILogger::FlushNewLine;
                    throw PreprocessorException("Invalid argument");
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
                    {
                        logger << ILogger::Error;
                        logger.AtPos(*currentMacro.line) << "Too few arguments for macro " << macroName << " expected at least " << min << " got " << args.size() << ILogger::FlushNewLine;
                        throw PreprocessorException("Not enough arguments given to preprocessor directive");
                    }
                    return args;
                };
                if (macroName == "include")
                {
                    //TODO support include dirs
                    auto path = (std::filesystem::absolute(std::filesystem::path(*currentMacro.line->fileName).parent_path()) / currentMacro.line->content.substr(currentMacro.firstChar + 10)).string();
                    std::ifstream file(path);
                    if (!file.is_open())
                    {
                        logger << ILogger::Error;
                        logger.AtPos(*currentMacro.line) << "Could not include file " << path << ILogger::FlushNewLine;
                        throw PreprocessorException("Include file not found");
                    }
                    File includeFile(file, std::make_shared<std::string>(path));
                    inputFile.insertAfter(std::move(includeFile), currentMacro.line);
                    advanceLine();
                    continue;
                }
                if (macroName == "eval")
                {
                    auto args = getArgs(2, 2);
                    auto arg = evaluateIdentifier(args[0]);
                    defines[args[1]] = arg;
                    advanceLine();
                    continue;
                }
                if (macroName == "define")
                {
                    auto what = evaluateIdentifier(getNextArg());
                    std::string value;
                    value = getNextArg();
                    defines[what] = value;
                    advanceLine();
                    continue;
                }
                if (macroName == "undef")
                {
                    auto what = evaluateIdentifier(getNextArg());
                    if (what.empty())
                    {
                        logger << ILogger::Error;
                        logger.AtPos(*currentMacro.line) << "Missing argument for undef" << ILogger::FlushNewLine;
                        throw PreprocessorException("Missing argument for undef");
                    }

                    defines.erase(what);
                    advanceLine();
                    continue;
                }
                if (macroName == "ifdef")
                {
                    auto args = getArgs(1, 2);
                    auto name = evaluateIdentifier(args[0]);
                    currentMacro.line = ifblock([&]()
                                                { return defines.contains(name); },
                                                args.size() > 1 ? args[1] : "");
                    currentMacro.firstChar = 0;
                    continue;
                }
                if (macroName == "ifndef")
                {
                    auto args = getArgs(1, 2);
                    auto name = evaluateIdentifier(args[0]);
                    currentMacro.line = ifblock([&]()
                                                { return !defines.contains(name); },
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
                    auto name = evaluateIdentifier(getNextArg());
                    auto it = counters.find(name);
                    if (it == counters.end())
                    {
                        logger << ILogger::Error;
                        logger.AtPos(*currentMacro.line) << "Invalid counter " << name << " Could not increment" << ILogger::FlushNewLine;
                        throw PreprocessorException("Identifier not found");
                    }
                    it->second++;
                    advanceLine();
                    continue;
                }
                if (macroName == "dec")
                {
                    auto name = evaluateIdentifier(getNextArg());
                    auto it = counters.find(name);
                    if (it == counters.end())
                    {
                        logger << ILogger::Error;
                        logger.AtPos(*currentMacro.line) << "Invalid counter " << name << " Could not decrement" << ILogger::FlushNewLine;
                        throw PreprocessorException("Identifier not found");
                    }
                    advanceLine();
                    continue;
                }
                if (macroName == "set")
                {
                    auto args = getArgs(1, 2);
                    auto name = evaluateIdentifier(args[0]);
                    int value;
                    if (args.size() == 1)
                        value = 0;
                    else
                        value = evaluateArgument(args[1], evaluateArgument);
                    counters[name] = value;
                    advanceLine();
                    continue;
                }
                if (macroName == "del")
                {
                    auto name = evaluateIdentifier(getNextArg());
                    if (name.empty())
                    {
                        logger << ILogger::Error;
                        logger.AtPos(*currentMacro.line) << "Missing argument for del" << ILogger::FlushNewLine;
                        throw PreprocessorException("Missing argument for del");
                    }
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

                logger << ILogger::Error;
                logger.AtPos(*currentMacro.line) << "Unknown preprocessor command " << macroName << ILogger::FlushNewLine;
                throw PreprocessorException("Unknown preprocessor command");
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
                        nextPos.firstChar = nextPos.line->content.length();
                        currentMacro.line->content += remaining;
                    }

                    inputFile.insertAfter(std::move(bodyCenter), currentMacro.line);
                    currentMacro = nextPos;
                    continue;
                }
                logger << ILogger::Error;
                logger.AtPos(*currentMacro.line) << "Unknown identifier " << macroName << ILogger::FlushNewLine;
                throw PreprocessorException("Unknown Identifier");
            }

            bool macroHasArgs = true;
            auto nextSpace = currentMacro.line->content.find(' ', currentMacro.firstChar);
            size_t macroNameEnd = std::min(currentMacro.line->content.find('(', currentMacro.firstChar), nextSpace);
            if (macroNameEnd == nextSpace)
            {
                macroHasArgs = false;
            }
            if (macroNameEnd == std::string::npos)
            {
                macroNameEnd = currentMacro.line->content.length();
            }
            std::string macroName = currentMacro.line->content.substr(currentMacro.firstChar + 1, macroNameEnd - currentMacro.firstChar - 1);
            macroName = evaluateIdentifier(macroName);
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
                        logger << ILogger::Error;
                        logger.AtPos(*currentMacro.line) << "Unmatched parenthesis" << ILogger::FlushNewLine;
                        throw PreprocessorException("Macro arguments not closed");
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
                auto macroArgsCurrent = macroArgsStart;
                for (size_t i = macroArgsStart; i <= macroArgsLast; i++)
                {
                    if (currentMacro.line->content[i] == ',')
                    {
                        if (i > macroArgsStart)
                        {
                            if (currentMacro.line->content[i - 1] != '\\')
                            {
                                args.push_back(currentMacro.line->content.substr(macroArgsCurrent, i - macroArgsCurrent));
                                macroArgsCurrent = i + 1;
                            }
                            else
                            {
                                currentMacro.line->content.erase(i - 1, 1);
                                i--;
                                macroArgsLast--;
                            }
                        }
                    }
                }
                args.push_back(currentMacro.line->content.substr(macroArgsCurrent, macroArgsLast - macroArgsCurrent + 1));
                macroEnd = currentpos;
            }
            auto macro = macros.find(macroName);
            if (macro == macros.end())
            {
                logger << ILogger::Error;
                logger.AtPos(*currentMacro.line) << "Unknown identifier " << macroName << ILogger::FlushNewLine;
                throw PreprocessorException("Unknown Identifier");
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