#include <Preprocessor.hpp>

//TODO: better way to define new keywords
//TODO: assert that appending still works
//TODO: line numbers in macros

#define PREPROCESSOR_BINARY_MATH_OP(op)    \
    auto args = getArgs(2, 3);             \
    auto val1 = evaluateArgument(args[0]); \
    auto val2 = evaluateArgument(args[1]); \
    context.counters[args[args.size() == 3 ? 2 : 0]] = val1 op val2;

#define PREPROCESSOR_MATH_COMPARISON(op)                                                            \
    auto args = getArgs(2, 3);                                                                      \
    currentMacro.line = ifblock([&]()                                                               \
                                { return evaluateArgument(args[0]) op evaluateArgument(args[1]); }, \
                                args.size() > 2 ? args[2] : "");                                    \
    currentMacro.firstChar = 0;                                                                     \
    continue;

namespace VWA
{
    namespace
    {
        void processMacro(File::iterator &current, PreprocessorContext &context)
        {
            auto end = current + 1;
            while (true)
            {
                auto res = context.file.find("##endmacro", end);
                if (res.firstChar == std::string_view::npos)
                {
                    context.logger << ILogger::Error;
                    context.logger.AtPos(*current) << "Unterminated MACRO" << ILogger::FlushNewLine;
                    throw PreprocessorException("Unterminated MACRO");
                }
                if (!res.firstChar)
                {
                    end = res.line;
                    break;
                }
                end = res.line + 1;
            }
            auto identifier = current->content.substr(8);
            while (identifier.front() == ' ')
                identifier.erase(identifier.begin());
            while (identifier.back() == ' ')
                identifier.pop_back();
            if (identifier.empty())
            {
                context.logger.AtPos(*current) << "Invalid MACRO definition. Missing identifier." << ILogger::FlushNewLine;
                throw PreprocessorException("Invalid MACRO definition.");
            }
            if (identifier.find(' ') != identifier.npos)
            {
                context.logger.AtPos(*current) << "Invalid MACRO definition. Identifier contains spaces." << ILogger::FlushNewLine;
                throw PreprocessorException("Invalid MACRO definition. Identifier contains spaces.");
            }
            auto body = context.file.extractLines(current + 1, end);
            if (body.empty())
            {
                context.logger.AtPos(*current) << "Invalid MACRO definition. Missing body." << ILogger::FlushNewLine;
                throw PreprocessorException("Invalid MACRO definition.");
            }
            context.macros[identifier] = std::move(body);
            current = context.file.removeLines(current, end + 1);
        }
        void FindMacroDefinitions(PreprocessorContext &context)
        {
            auto current = context.file.begin();
            while (true)
            {
                auto res = context.file.find("##MACRO", current);
                if (res.firstChar == std::string::npos)
                    break;
                if (res.firstChar)
                {
                    current = res.line + 1;
                    continue;
                }
                current = res.line;
                processMacro(current, context);
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
    File preprocess(PreprocessorContext context)
    {

        auto requireSingleLine = [](File &file)
        {
            if (file.empty())
                throw PreprocessorException("1 Line required, 0 found");
            if (file.begin() != file.end() - 1)
                throw PreprocessorException("1 Line required, more than 1 found");
        };

        //If the last character in a line is a \, merge it with the next line. Also removes empty lines.
        for (auto line = context.file.begin(); line != context.file.end(); line++)
        {
            if (line->content.empty())
            {
                line = context.file.removeLine(line);
                continue;
            }
            if (line->content.back() == '\\')
            {
                auto next = line + 1;
                if (next == context.file.end())
                {
                    continue;
                }
                line->content.pop_back();
                line->content += next->content;
                context.file.removeLine(next);
            }
        }

        RemoveCommentLines(context.file);
        // FindMacroDefinitions(context);

        for (auto currentMacro = context.file.find('#'); currentMacro.firstChar != std::string::npos; currentMacro = context.file.find('#', currentMacro.line, currentMacro.firstChar))
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
                            context.logger << ILogger::Error;
                            context.logger.AtPos(*currentMacro.line) << "Invalid macro call. Missing opening parenthesis" << ILogger::FlushNewLine;
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
                    if (auto macro = context.macros.find(identifer); macro != context.macros.end())
                    {
                        requireSingleLine(macro->second);
                        std::string line = macro->second.begin()->content;
                        for (int i = 0; i < args.size(); i++)
                        {
                            line = line.replace(line.find("$" + std::to_string(i)), 2, args[i]);
                        }
                        return line;
                    }
                    if (auto counter = context.counters.find(identifer); counter != context.counters.end())
                    {
                        length = std::to_string(counter->second).length();
                        return std::to_string(counter->second);
                    }
                    context.logger << ILogger::Error;
                    context.logger.AtPos(*currentMacro.line) << "Unknown identifier " << identifer << (identifer != input ? ("Expanded from " + input) : "") << ILogger::FlushNewLine;
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
#pragma region setup
                size_t macroNameEnd = currentMacro.line->content.find(' ', currentMacro.firstChar + 2);
                if (macroNameEnd == std::string::npos)
                {
                    macroNameEnd = currentMacro.line->content.length();
                }
                std::string macroName(currentMacro.line->content.substr(currentMacro.firstChar + 2, macroNameEnd - currentMacro.firstChar - 2));
                size_t macroEnd = macroNameEnd;
                auto advanceLine = [&]()
                {
                    currentMacro.line = context.file.removeLine(currentMacro.line);
                    currentMacro.firstChar = 0;
                };
                auto ifblock = [&](auto &&condition, const std::string &name) -> File::iterator
                {
                    auto endIf = currentMacro.line + 1;
                    File::FilePos res;
                    if (name.empty())
                    {
                        res = context.file.find("##endif", endIf);
                        if (res.firstChar == std::string::npos)
                        {
                            context.logger << ILogger::Error;
                            context.logger.AtPos(*currentMacro.line) << "No matching endif found for if" << ILogger::FlushNewLine;
                            throw PreprocessorException("No matching endif found for if");
                        }
                    }
                    else
                    {
                        res = context.file.find("##endif " + name, endIf);
                        if (res.firstChar == std::string::npos)
                        {
                            context.logger << ILogger::Error;
                            context.logger.AtPos(*currentMacro.line) << "No matching endif found for named if " << name << ILogger::FlushNewLine;
                            throw PreprocessorException("ENDIF not found " + name);
                        }
                    }
                    if (condition())
                    {
                        context.file.removeLine(res.line);
                        return context.file.removeLine(currentMacro.line);
                    }
                    else
                    {
                        return context.file.removeLines(currentMacro.line, res.line + 1);
                    }
                };
                auto evaluateArgument = [&](std::string arg) -> int
                {
                    //If the argument is a number return it. It is considered a number if the first character is a digit.
                    auto arg1 = evaluateIdentifier(arg);
                    if (std::isdigit(arg1[0]) || arg1[0] == '-')
                    {
                        return std::stoi(arg1);
                    }
                    context.logger << ILogger::Error;
                    context.logger.AtPos(*currentMacro.line) << "Invalid argument " << arg << "Could not convert to a number" << ILogger::FlushNewLine;
                    throw PreprocessorException("Invalid argument");
                };
                auto getNextArg = [&]() -> std::string
                {
                    auto nextLetter= currentMacro.line->content.find_first_not_of(' ', macroEnd+1);
                    auto nextSpace = currentMacro.line->content.find(' ', nextLetter+1);
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
                    auto arg = currentMacro.line->content.substr(nextLetter, nextSpace - nextLetter);
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
                        context.logger << ILogger::Error;
                        context.logger.AtPos(*currentMacro.line) << "Too few arguments for macro " << macroName << " expected at least " << min << " got " << args.size() << ILogger::FlushNewLine;
                        throw PreprocessorException("Not enough arguments given to preprocessor directive");
                    }
                    return args;
                };
#pragma endregion setup

                if (macroName == "include")
                {
                    //TODO support include dirs
                    //TODO: allow macros
                    auto path = (std::filesystem::path(*currentMacro.line->fileName).parent_path() / currentMacro.line->content.substr(currentMacro.firstChar + 10)).string();
                    std::ifstream stream(path);
                    if (!stream.is_open())
                    {
                        context.logger << ILogger::Error;
                        context.logger.AtPos(*currentMacro.line) << "Could not open file " << path << ILogger::FlushNewLine;
                        throw PreprocessorException("Could not open file");
                    }
                    File includeFile(stream, path);
                    context.file.insertAfter(std::move(includeFile), currentMacro.line);
                }
                else if (macroName == "eval")
                {
                    auto args = getArgs(2, 2);
                    auto arg = evaluateIdentifier(args[0]);
                    auto stream = std::istringstream(arg);
                    //TODO: support multiple lines
                    context.macros[args[1]] = File(stream, currentMacro.line->fileName);
                }
                else if (macroName == "define")
                {
                    auto what = evaluateIdentifier(getNextArg());
                    std::string value = currentMacro.line->content.substr(macroEnd + 1);
                    auto stream = std::istringstream(value);
                    context.macros[what] = File(stream, currentMacro.line->fileName);
                }
                else if (macroName == "macro")
                {
                    processMacro(currentMacro.line, context);
                    continue;
                }
                else if (macroName == "undef")
                {
                    auto what = evaluateIdentifier(getNextArg());
                    if (what.empty())
                    {
                        context.logger << ILogger::Error;
                        context.logger.AtPos(*currentMacro.line) << "Missing argument for undef" << ILogger::FlushNewLine;
                        throw PreprocessorException("Missing argument for undef");
                    }

                    context.macros.erase(what);
                }
#pragma region conditions
                else if (macroName == "ifdef")
                {
                    auto args = getArgs(1, 2);
                    auto name = evaluateIdentifier(args[0]);
                    currentMacro.line = ifblock([&]()
                                                { return context.macros.contains(name); },
                                                args.size() > 1 ? args[1] : "");
                    currentMacro.firstChar = 0;
                    continue;
                }
                else if (macroName == "ifndef")
                {
                    auto args = getArgs(1, 2);
                    auto name = evaluateIdentifier(args[0]);
                    currentMacro.line = ifblock([&]()
                                                { return !context.macros.contains(name); },
                                                args.size() > 1 ? args[1] : "");
                    currentMacro.firstChar = 0;
                    continue;
                }
                else if (macroName == "ifeq")
                {
                    PREPROCESSOR_MATH_COMPARISON(==)
                }
                else if (macroName == "ifneq")
                {
                    PREPROCESSOR_MATH_COMPARISON(!=)
                }
                else if (macroName == "ifgt")
                {
                    PREPROCESSOR_MATH_COMPARISON(>)
                }
                else if (macroName == "iflt")
                {
                    PREPROCESSOR_MATH_COMPARISON(<)
                }
                else if (macroName == "ifgteq")
                {
                    PREPROCESSOR_MATH_COMPARISON(>=)
                }
                else if (macroName == "iflteq")
                {
                    PREPROCESSOR_MATH_COMPARISON(<=)
                }
#pragma endregion conditions
#pragma region mathops
                else if (macroName == "inc")
                {
                    auto name = evaluateIdentifier(getNextArg());
                    auto it = context.counters.find(name);
                    if (it == context.counters.end())
                    {
                        context.logger << ILogger::Error;
                        context.logger.AtPos(*currentMacro.line) << "Invalid counter " << name << " Could not increment" << ILogger::FlushNewLine;
                        throw PreprocessorException("Identifier not found");
                    }
                    it->second++;
                }
                else if (macroName == "dec")
                {
                    auto name = evaluateIdentifier(getNextArg());
                    auto it = context.counters.find(name);
                    if (it == context.counters.end())
                    {
                        context.logger << ILogger::Error;
                        context.logger.AtPos(*currentMacro.line) << "Invalid counter " << name << " Could not decrement" << ILogger::FlushNewLine;
                        throw PreprocessorException("Identifier not found");
                    }
                }
                else if (macroName == "set")
                {
                    auto args = getArgs(1, 2);
                    auto name = evaluateIdentifier(args[0]);
                    int value;
                    if (args.size() == 1)
                        value = 0;
                    else
                        value = evaluateArgument(args[1]);
                    context.counters[name] = value;
                }
                else if (macroName == "del")
                {
                    auto name = evaluateIdentifier(getNextArg());
                    if (name.empty())
                    {
                        context.logger << ILogger::Error;
                        context.logger.AtPos(*currentMacro.line) << "Missing argument for del" << ILogger::FlushNewLine;
                        throw PreprocessorException("Missing argument for del");
                    }
                    auto it = context.counters.find(name);
                    if (it == context.counters.end())
                    {
                        context.logger << ILogger::Warning;
                        context.logger.AtPos(*currentMacro.line) << name << " does not exist, can't delete it." << ILogger::FlushNewLine;
                    }
                    context.counters.erase(it);
                }
                else if (macroName == "mul")
                {
                    PREPROCESSOR_BINARY_MATH_OP(*)
                }
                else if (macroName == "div")
                {
                    PREPROCESSOR_BINARY_MATH_OP(/)
                }
                else if (macroName == "mod")
                {
                    PREPROCESSOR_BINARY_MATH_OP(%)
                }
                else if (macroName == "add")
                {
                    PREPROCESSOR_BINARY_MATH_OP(+)
                }
                else if (macroName == "sub")
                {
                    PREPROCESSOR_BINARY_MATH_OP(-)
                }
#pragma endregion mathops

                else
                {
                    context.logger << ILogger::Error;
                    context.logger.AtPos(*currentMacro.line) << "Unknown preprocessor command " << macroName << ILogger::FlushNewLine;
                    throw PreprocessorException("Unknown preprocessor command");
                }
                advanceLine();
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
                if (auto it2 = context.counters.find(macroName); it2 != context.counters.end())
                {
                    auto asString = std::to_string(it2->second);
                    currentMacro.line->content.replace(currentMacro.firstChar, macroNameEnd - currentMacro.firstChar + 1, asString);
                    currentMacro.firstChar += asString.length();
                    continue;
                }
                if (auto macro = context.macros.find(macroName); macro != context.macros.end())
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
                        context.file.insertAfter(currentMacro.line, (body.end() - 1)->content + remaining);
                        nextPos.line = currentMacro.line + 1;
                        nextPos.firstChar = body.end()->content.length();
                    }
                    else
                    {
                        nextPos.firstChar = nextPos.line->content.length();
                        currentMacro.line->content += remaining;
                    }

                    context.file.insertAfter(std::move(bodyCenter), currentMacro.line);
                    currentMacro = nextPos;
                    continue;
                }
                context.logger << ILogger::Error;
                context.logger.AtPos(*currentMacro.line) << "Unknown identifier " << macroName << ILogger::FlushNewLine;
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
                if (auto it2 = context.counters.find(macroName); it2 != context.counters.end())
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
                        context.logger << ILogger::Error;
                        context.logger.AtPos(*currentMacro.line) << "Unmatched parenthesis" << ILogger::FlushNewLine;
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
            auto macro = context.macros.find(macroName);
            if (macro == context.macros.end())
            {
                context.logger << ILogger::Error;
                context.logger.AtPos(*currentMacro.line) << "Unknown identifier " << macroName << ILogger::FlushNewLine;
                throw PreprocessorException("Unknown Identifier");
            }

            File body(macro->second);
            for (size_t i = 0; i < args.size(); i++)
            {
                File::FilePos arg{body.begin(), 0};
                while (1)
                {
                    //TODO expand macros in arguments
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
                context.file.insertAfter(currentMacro.line, (body.end() - 1)->content + remaining);
            }
            else
            {
                currentMacro.line->content += remaining;
            }

            context.file.insertAfter(std::move(bodyCenter), currentMacro.line);
        }
        {
            auto space = context.file.find("\\ ");
            while (space.firstChar != std::string::npos)
            {
                space.line->content.replace(space.firstChar, 2, "");
                space = context.file.find("\\ ", space.line, space.firstChar);
            }
        }
        return context.file;
    }
}

#undef PREPROCESSOR_MATH_COMPARISON
#undef PREPROCESSOR_BINARY_MATH_OP