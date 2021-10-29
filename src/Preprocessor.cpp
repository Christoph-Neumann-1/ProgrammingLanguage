#include <Preprocessor.hpp>
#include <variant>
#include <optional>

//TODO: better way to define new keywords
//TODO: assert that appending still works
//TODO: line numbers in macros
//TODO: handle // in strings either detect strings  in preprocessor or provide escape sequence
//TODO: figure out best time to expand macro parameters
//TODO: error trace for nested macros
//TODO: delete counter upon defining macro with same name and vice versa
//TODO: forbid redefining keywords
//TODO: move keywords to other file
//TODO: express comments as macros: #//(comment)

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

//TODO: remove
#define throw_if(condition, exception) \
    if (condition)                     \
    throw exception

namespace VWA
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
    //TODO: handle functions with parameters, count brackets
    size_t FindEndOfIdentifier(const std::string_view &line, size_t pos, const std::string &seperators = " \t")
    {
        auto end = line.find_first_of(seperators, pos + 1);
        return end == line.npos ? line.size() : end;
    }
    //TODO Find better way to handle errors
    std::variant<std::monostate, std::string, File, int> expandMacro(const std::string &identier, const PreprocessorContext &context, const std::optional<std::vector<std::string>> &args)
    {
        auto macro = context.macros.find(identier);
        if (macro != context.macros.end())
        {
            File body(macro->second);
            if (args)
                for (int i = 0; i < args->size(); ++i)
                {
                    auto name = "$" + std::to_string(i);
                    for (auto current = body.find(name); current.firstChar != std::string_view::npos; current = body.find(name))
                    {
                        current.line->content.replace(current.firstChar, current.firstChar + name.size(), args->at(i));
                    }
                }
            if (body.back() == body.begin())
                return body.begin()->content;
            return body;
        }
        auto counter = context.counters.find(identier);
        if (counter != context.counters.end())
            return counter->second;
        return std::monostate{};
    }

    //TODO: consider returning an empty vector instead of using optional
    //TODO: handle spaces in brackets
    std::optional<std::vector<std::string>> getMacroArgs(std::string &identifier)
    {
        if (identifier.back() == ')')
        {
            auto openParenthesis = identifier.find('(');
            if(openParenthesis==identifier.size()-1)
            {
                identifier.pop_back();
                identifier.pop_back();
                return std::nullopt;
            }
            std::string args = identifier.substr(openParenthesis + 1, identifier.size() - openParenthesis - 2);
            identifier.erase(openParenthesis);
            std::vector<std::string> argList;
            size_t begin = 0;
            for (size_t i = 0; i < args.size(); ++i)
            {
                if (args[i] == ',')
                {
                    if (i)
                    {
                        if (args[i - 1] == '\\')
                        {
                            args.erase(i - 1, 1);
                            --i;
                            continue;
                        }
                    }
                    argList.push_back(args.substr(begin, i - begin));
                    begin = i + 1;
                }
            }
            argList.push_back(args.substr(begin));
            return argList;
        }
        return std::nullopt;
    }

    //TODO: better errors. Pass the current line? Return error struct? Or just use exceptions?
    //TODO: don't require a space at  the end, or erase it. The former is prefered
    std::optional<std::string> expandIdentifier(std::string identifier, PreprocessorContext &context)
    {
        auto original = identifier;
        for (auto i = identifier.find('#'); i != identifier.npos; i = identifier.find('#', i))
        {
            auto end = FindEndOfIdentifier(identifier, i, " \t#");
            end = end == identifier.npos ? identifier.size() : end;
            //TODO: extract to other function and generalize. Use visitor pattern
            if (identifier[i + 1] == '!')
            {
                auto name = identifier.substr(i + 2, end - i - 2);
                auto expanded = expandMacro(name, context, getMacroArgs(identifier));
                if (expanded.index() == 0 || expanded.index() == 2)
                {
                    context.logger << ILogger::Error << "Failed to expand " << name << " in " << original << ILogger::FlushNewLine;
                    return std::nullopt;
                }
                std::string string;
                if (auto integer = std::get_if<int>(&expanded))
                {
                    string = std::to_string(*integer);
                }
                else if (auto s = std::get_if<std::string>(&expanded))
                {
                    string = *s;
                }
                identifier.replace(i, end - i + 1, string);
                i += string.size();
                continue;
            }
            auto name = identifier.substr(i + 1, end - i - 1);
            auto expanded = expandMacro(name, context, getMacroArgs(identifier));
            if (expanded.index() == 0 || expanded.index() == 2)
            {
                context.logger << ILogger::Error << "Failed to expand " << name << " in " << original << ILogger::FlushNewLine;
                return std::nullopt;
            }
            std::string string;
            if (auto integer = std::get_if<int>(&expanded))
            {
                string = std::to_string(*integer);
            }
            else if (auto s = std::get_if<std::string>(&expanded))
            {
                string = *s;
            }
            identifier.replace(i, end - i + 1, string);
            continue;
        }
        return identifier;
    }
    bool IsFirstNonSpace(const std::string_view &line, size_t pos)
    {
        if (!pos)
            return true;
        for (--pos; pos >= 0; --pos)
        {
            if (!isspace(line[pos]))
                return false;
        }
        return true;
    }
    void PasteMacro(std::string identifier, File::FilePos position, PreprocessorContext &context, File::FilePos *nextCharacter = nullptr, size_t prefixLength = 1)
    {
        //TODO: check if whitespaces are handled correctly
        auto expanded = expandMacro(identifier, context, getMacroArgs(identifier));
        if (!expanded.index())
        {
            context.logger.AtPos(*position.line) << ILogger::Error << "Failed to expand macro " << identifier << ILogger::FlushNewLine;
            throw PreprocessorException("Macro expansion failed");
        }
        if (auto s = std::get_if<std::string>(&expanded))
        {
            if (nextCharacter)
            {
                *nextCharacter = position;
                nextCharacter->firstChar += s->length();
            }
            position.line->content.replace(position.firstChar, identifier.size() + prefixLength, *s);
            return;
        }
        if (auto file = std::get_if<File>(&expanded))
        {
            std::string first = file->begin()->content;
            std::string last = file->back()->content;
            auto Body = file->extractLines(file->begin() + 1, file->back());
            std::string remainder = position.line->content.substr(position.firstChar + identifier.size() + prefixLength - 1);
            position.line->content.replace(position.firstChar, position.line->content.size() - position.firstChar, first);
            context.file.insertAfter(position.line, last + remainder);
            if (nextCharacter)
            {
                *nextCharacter = position;
                ++nextCharacter->line;
                nextCharacter->firstChar = last.size();
            }
            context.file.insertAfter(std::move(Body), position.line);
        }
        if (auto integer = std::get_if<int>(&expanded))
        {
            auto string = std::to_string(*integer);
            if (nextCharacter)
            {
                *nextCharacter = position;
                nextCharacter->firstChar += string.length();
            }
            position.line->content.replace(position.firstChar, identifier.size() + prefixLength, string);
            return;
        }
    }

    void SetterCommon::RemoveOldDefinition(PreprocessorContext &context, const std::string &identifier)
    {
        if (auto macro = context.macros.find(identifier); macro != context.macros.end())
        {
            context.macros.erase(macro);
        }
        if (auto counter = context.counters.find(identifier); counter != context.counters.end())
        {
            context.counters.erase(counter);
        }
    }

    File::FilePos DefineCommand::operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args)
    {
        if (args.empty())
            throw PreprocessorException("No arguments given to define");
        //TODO if more paramteters are passed, set all of them to the value
        auto expanded = expandIdentifier(args[0], context);
        if (!expanded)
        {
            throw PreprocessorException("Invalid identifier for define");
        }
        RemoveOldDefinition(context, *expanded);
        if (args.size() > 1)
        {
            std::istringstream stream(args.back());
            context.macros[*expanded] = File(stream, current.line->fileName);
        }
        else
        {
            context.macros[*expanded] = File(current.line->fileName);
        }
        //TODO: function that erases the identifier correctly to avoid repetititon and increase consistency
        current.line->content.erase(current.firstChar, fullIdentifier.length() + 1);
        return current;
    }
    File::FilePos EvalCommand::operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args)
    {
        if (args.empty())
            throw PreprocessorException("No arguments given to define");
        //TODO if more paramteters are passed, set all of them to the value
        auto expanded = expandIdentifier(args[0], context);
        if (!expanded)
        {
            throw PreprocessorException("Invalid identifier for define");
        }
        auto what = args.size() == 1 ? '#' + args[0] : args.back();
        auto value = expandIdentifier(what, context); //TODO: check for nullopt
        RemoveOldDefinition(context, *expanded);
        std::istringstream stream(*value);
        context.macros[*expanded] = File(stream, current.line->fileName);
        //TODO: function that erases the identifier correctly to avoid repetititon and increase consistency
        current.line->content.erase(current.firstChar, fullIdentifier.length() + 1);
        return current;
    }

    File::FilePos MathCommand::operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args)
    {
        //TODO: if more args are given compute the sum/product/etc of them and store them in the first val
        if (args.size() < 2)
            throw PreprocessorException("Invalid number of arguments for eval");
        auto destination = expandIdentifier(args[0], context);
        if (!destination)
            throw PreprocessorException("Invalid identifier for eval");

        int operands[2];
        //TODO: if given the name of a counter instead of a number, get the value, this would solve the problem when
        //given 2 args and having to store the result in args[0]. It also would make it easier to write simple additions

        for (int i = 0; i < 2; ++i)
            if (auto res = expandIdentifier(*(args.end() - i - 1), context))
            {
                if (!std::isdigit(res->front()))
                {
                    res = expandIdentifier('#' + *res, context);
                }
                try
                {
                    operands[i] = std::stoi(*res);
                }
                catch (std::invalid_argument &e)
                {
                    throw PreprocessorException("Invalid argument for math op");
                }
            }
            else
            {
                throw PreprocessorException("Invalid argument for math op");
            }
        RemoveOldDefinition(context, *destination);
        context.counters[*destination] = op(operands[0], operands[1]);
        current.line->content.erase(current.firstChar, fullIdentifier.length() + 1);
        return current;
    }

    File::FilePos MacrodefinitionCommand::operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args)
    {
        throw_if(args.size() != 1, PreprocessorException("Invalid number of arguments for macro"));
        auto end = context.file.find("#endmacro(" + args[0] + ")", current.line + 1);
        //TODO: handle commented out lines
        if (end.firstChar == std::string::npos)
            throw PreprocessorException("No endmacro found for macro");
        auto body = context.file.extractLines(current.line + 1, end.line);
        RemoveOldDefinition(context, args[0]);
        context.macros[args[0]] = std::move(body);
        auto next = end;
        ++next.line;
        next.firstChar = 0;
        context.file.removeLines(current.line, end.line + 1);
        return next;
    }

    //TODO: loop over line and not entire file
    File preprocess(PreprocessorContext context)
    {

        // auto requireSingleLine = [](File &file)
        // {
        //     if (file.empty())
        //         throw PreprocessorException("1 Line required, 0 found");
        //     if (file.begin() != file.end() - 1)
        //         throw PreprocessorException("1 Line required, more than 1 found");
        // };

        //If the last character in a line is a \, merge it with the next line. Also removes empty lines.
        for (auto line = context.file.begin(); line != context.file.end(); ++line)
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

        // for (auto line = context.file.begin(); line != context.file.end();)
        // {
        //     if (auto comment = line->content.find("//"); comment != line->content.npos)
        //     {
        //         line->content.erase(comment);
        //     }
        //     line = line->content.empty() ? context.file.removeLine(line) : line + 1;
        // }

        for (auto line = context.file.begin(); line != context.file.end(); ++line)
        {
            for (auto current = line->content.find("#"); current != line->content.npos; current = line->content.find("#", current))
            {
                if (current)
                    if (line->content[current - 1] == '\\')
                    {
                        line->content.erase(current - 1, 1);
                        continue;
                    }
                auto nameEnd = FindEndOfIdentifier(line->content, current + 1);
                auto identifier = line->content.substr(current + 1, nameEnd - current - 1);

                //Paste the content, but don't expand it further
                if (*identifier.begin() == '!')
                {
                    //TODO: consider removing identifier before function call
                    File::FilePos next;
                    auto name = expandIdentifier(identifier.erase(0, 1), context);
                    if (!name)
                    {
                        throw PreprocessorException("Macro expansion failed");
                    }
                    identifier = *name;
                    PasteMacro(identifier, File::FilePos{line, current}, context, &next, 2);
                    line = next.line;
                    current = next.firstChar;
                    continue;
                }

                //TODO: remove this constraint or make it optional
                if (IsFirstNonSpace(line->content, current))
                {
                    //TODO: functions to replace old lamdas
                    //TODO: more modular approach
                    //TODO: expand identifiers as well
                    //TODO: consider moving the evaluation and substring part to the command class
                    auto copy = identifier;
                    auto args = getMacroArgs(copy);
                    args = args ? args : std::vector<std::string>{};
                    if (auto command = context.commands.find(copy); command != context.commands.end())
                    {
                        auto [nextline, nextChar] = (*command->second)(context, {line, current}, identifier, *args);
                        line = nextline;
                        current = nextChar;
                        continue;
                    }
                }
                {
                    auto name = expandIdentifier(identifier, context);
                    if (!name)
                    {
                        throw PreprocessorException("Macro expansion failed");
                    }
                    identifier = *name;
                    PasteMacro(identifier, File::FilePos{line, current}, context);
                    continue;
                }
            }
        }

        //Removes empty lines or lines consisting only of whitespaces
        for (auto it = context.file.begin(); it != context.file.end();)
        {
            if (it->content.empty())
                it = context.file.removeLine(it);
            else if (it->content.find_first_not_of(" \t") == it->content.npos)
                it = context.file.removeLine(it);
            else
                ++it;
        }

#pragma region deprecated
        //         for (auto currentMacro = context.file.find('#'); currentMacro.firstChar != std::string::npos; currentMacro = context.file.find('#', currentMacro.line, currentMacro.firstChar))
        //         {
        //             auto handleEscapeSequence = [](std::string &string, size_t &pos) -> bool
        //             {
        //                 if (pos == 0)
        //                     return 0;
        //                 if (string[pos - 1] == '\\')
        //                 {
        //                     string.erase(pos - 1, 1);
        //                     pos--;
        //                     return true;
        //                 }
        //                 return false;
        //             };
        //             if (handleEscapeSequence(currentMacro.line->content, currentMacro.firstChar))
        //             {
        //                 //TODO escape directives
        //                 ++currentMacro.firstChar;
        //                 continue;
        //             }

        //             /**
        //              * @brief This function expands the macro name, replacing all mentions of other define/macros with their values
        //              *
        //              * @note Only single line macros are supported, because identifier may only span one line
        //              *
        //              */
        //             auto evaluateIdentifier = [&](std::string name) -> std::string
        //             {
        //                 auto getExpandedWithLength = [&](std::string input, size_t &length) -> std::string
        //                 {
        //                     std::string identifer = input;
        //                     std::vector<std::string> args;
        //                     if (identifer.back() == ')')
        //                     {
        //                         auto open = identifer.find('(');
        //                         if (open == std::string::npos)
        //                         {
        //                             context.logger << ILogger::Error;
        //                             context.logger.AtPos(*currentMacro.line) << "Invalid macro call. Missing opening parenthesis" << ILogger::FlushNewLine;
        //                             throw PreprocessorException("Invalid macro call");
        //                         }
        //                         std::string argsString = identifer.substr(open + 1);

        //                         size_t begin = 0;
        //                         for (size_t i = 0; i < argsString.size(); ++i)
        //                         {
        //                             if (argsString[i] == ',' && i > 0)
        //                             {
        //                                 if (argsString[i - 1] != '\\')
        //                                 {
        //                                     args.push_back(argsString.substr(begin, i - begin));
        //                                     begin = i + 1;
        //                                 }
        //                                 else
        //                                 {
        //                                     argsString.erase(i - 1, 1);
        //                                     i--;
        //                                 }
        //                             }
        //                         }
        //                         args.push_back(argsString.substr(begin));

        //                         identifer = identifer.substr(0, open);
        //                     }
        //                     if (auto macro = context.macros.find(identifer); macro != context.macros.end())
        //                     {
        //                         requireSingleLine(macro->second);
        //                         std::string line = macro->second.begin()->content;
        //                         for (int i = 0; i < args.size(); ++i)
        //                         {
        //                             line = line.replace(line.find("$" + std::to_string(i)), 2, args[i]);
        //                         }
        //                         return line;
        //                     }
        //                     if (auto counter = context.counters.find(identifer); counter != context.counters.end())
        //                     {
        //                         length = std::to_string(counter->second).length();
        //                         return std::to_string(counter->second);
        //                     }
        //                     context.logger << ILogger::Error;
        //                     context.logger.AtPos(*currentMacro.line) << "Unknown identifier " << identifer << (identifer != input ? ("Expanded from " + input) : "") << ILogger::FlushNewLine;
        //                     throw PreprocessorException("Unknown identifier");
        //                 };
        //                 auto findEnd = [&](size_t firstChar) -> size_t
        //                 {
        //                     auto res = name.find('#', firstChar);
        //                     if (res == std::string::npos)
        //                         res = name.size();
        //                     return res;
        //                 };
        //                 size_t current = 0;
        //                 while (1)
        //                 {
        //                     current = name.find('#', current);
        //                     if (current == name.npos)
        //                         break;
        //                     if (name[current + 1] == '!')
        //                     {
        //                         auto idend = findEnd(current + 2);
        //                         auto identifier = name.substr(current + 2, idend - current - 2);
        //                         size_t length;
        //                         name.replace(current, idend - current + 1, getExpandedWithLength(identifier, length));
        //                         current += length;
        //                     }
        //                     else
        //                     {
        //                         auto idend = findEnd(current + 1);
        //                         auto identifier = name.substr(current + 1, idend - current - 1);
        //                         size_t length;
        //                         name.replace(current, idend - current + 1, getExpandedWithLength(identifier, length));
        //                     }
        //                 }
        //                 return name;
        //             };

        //             if (currentMacro.line->content[currentMacro.firstChar + 1] == '#' && currentMacro.firstChar == 0)
        //             {
        // #pragma region setup
        //                 size_t macroNameEnd = currentMacro.line->content.find(' ', currentMacro.firstChar + 2);
        //                 if (macroNameEnd == std::string::npos)
        //                 {
        //                     macroNameEnd = currentMacro.line->content.length();
        //                 }
        //                 std::string macroName(currentMacro.line->content.substr(currentMacro.firstChar + 2, macroNameEnd - currentMacro.firstChar - 2));
        //                 size_t macroEnd = macroNameEnd;
        //                 auto advanceLine = [&]()
        //                 {
        //                     currentMacro.line = context.file.removeLine(currentMacro.line);
        //                     currentMacro.firstChar = 0;
        //                 };
        //                 auto ifblock = [&](auto &&condition, const std::string &name) -> File::iterator
        //                 {
        //                     auto endIf = currentMacro.line + 1;
        //                     File::FilePos res;
        //                     if (name.empty())
        //                     {
        //                         res = context.file.find("##endif", endIf);
        //                         if (res.firstChar == std::string::npos)
        //                         {
        //                             context.logger << ILogger::Error;
        //                             context.logger.AtPos(*currentMacro.line) << "No matching endif found for if" << ILogger::FlushNewLine;
        //                             throw PreprocessorException("No matching endif found for if");
        //                         }
        //                     }
        //                     else
        //                     {
        //                         res = context.file.find("##endif " + name, endIf);
        //                         if (res.firstChar == std::string::npos)
        //                         {
        //                             context.logger << ILogger::Error;
        //                             context.logger.AtPos(*currentMacro.line) << "No matching endif found for named if " << name << ILogger::FlushNewLine;
        //                             throw PreprocessorException("ENDIF not found " + name);
        //                         }
        //                     }
        //                     if (condition())
        //                     {
        //                         context.file.removeLine(res.line);
        //                         return context.file.removeLine(currentMacro.line);
        //                     }
        //                     else
        //                     {
        //                         return context.file.removeLines(currentMacro.line, res.line + 1);
        //                     }
        //                 };
        //                 auto evaluateArgument = [&](std::string arg) -> int
        //                 {
        //                     //If the argument is a number return it. It is considered a number if the first character is a digit.
        //                     auto arg1 = evaluateIdentifier(arg);
        //                     if (std::isdigit(arg1[0]) || arg1[0] == '-')
        //                     {
        //                         return std::stoi(arg1);
        //                     }
        //                     context.logger << ILogger::Error;
        //                     context.logger.AtPos(*currentMacro.line) << "Invalid argument " << arg << "Could not convert to a number" << ILogger::FlushNewLine;
        //                     throw PreprocessorException("Invalid argument");
        //                 };
        //                 auto getNextArg = [&]() -> std::string
        //                 {
        //                     auto nextLetter = currentMacro.line->content.find_first_not_of(' ', macroEnd + 1);
        //                     auto nextSpace = currentMacro.line->content.find(' ', nextLetter + 1);
        //                     if (nextSpace == std::string::npos)
        //                     {
        //                         if (macroEnd == currentMacro.line->content.length())
        //                         {
        //                             return std::string();
        //                         }
        //                         else
        //                         {
        //                             nextSpace = currentMacro.line->content.length();
        //                         }
        //                     }
        //                     auto arg = currentMacro.line->content.substr(nextLetter, nextSpace - nextLetter);
        //                     macroEnd = nextSpace;
        //                     return arg;
        //                 };
        //                 auto getArgs = [&](int min = -1, int max = -1) -> std::vector<std::string>
        //                 {
        //                     std::vector<std::string> args;
        //                     while (true)
        //                     {
        //                         if (args.size() == max)
        //                             break;
        //                         auto arg = getNextArg();
        //                         if (arg.empty())
        //                         {
        //                             break;
        //                         }
        //                         args.push_back(arg);
        //                     }
        //                     if (args.size() < min)
        //                     {
        //                         context.logger << ILogger::Error;
        //                         context.logger.AtPos(*currentMacro.line) << "Too few arguments for macro " << macroName << " expected at least " << min << " got " << args.size() << ILogger::FlushNewLine;
        //                         throw PreprocessorException("Not enough arguments given to preprocessor directive");
        //                     }
        //                     return args;
        //                 };
        // #pragma endregion setup

        //                 if (macroName == "include")
        //                 {
        //                     //TODO support include dirs
        //                     //TODO: allow macros
        //                     auto path = (std::filesystem::path(*currentMacro.line->fileName).parent_path() / currentMacro.line->content.substr(currentMacro.firstChar + 10)).string();
        //                     std::ifstream stream(path);
        //                     if (!stream.is_open())
        //                     {
        //                         context.logger << ILogger::Error;
        //                         context.logger.AtPos(*currentMacro.line) << "Could not open file " << path << ILogger::FlushNewLine;
        //                         throw PreprocessorException("Could not open file");
        //                     }
        //                     File includeFile(stream, path);
        //                     context.file.insertAfter(std::move(includeFile), currentMacro.line);
        //                 }
        //                 else if (macroName == "eval")
        //                 {
        //                     auto args = getArgs(2, 2);
        //                     auto arg = evaluateIdentifier(args[0]);
        //                     auto stream = std::istringstream(arg);
        //                     //TODO: support multiple lines
        //                     context.macros[args[1]] = File(stream, currentMacro.line->fileName);
        //                 }
        //                 else if (macroName == "define")
        //                 {
        //                     auto what = evaluateIdentifier(getNextArg());
        //                     std::string value = currentMacro.line->content.substr(macroEnd + 1);
        //                     auto stream = std::istringstream(value);
        //                     context.macros[what] = File(stream, currentMacro.line->fileName);
        //                 }
        //                 else if (macroName == "macro")
        //                 {
        //                     processMacro(currentMacro.line, context);
        //                     continue;
        //                 }
        //                 else if (macroName == "undef")
        //                 {
        //                     auto what = evaluateIdentifier(getNextArg());
        //                     if (what.empty())
        //                     {
        //                         context.logger << ILogger::Error;
        //                         context.logger.AtPos(*currentMacro.line) << "Missing argument for undef" << ILogger::FlushNewLine;
        //                         throw PreprocessorException("Missing argument for undef");
        //                     }

        //                     context.macros.erase(what);
        //                 }
        // #pragma region conditions
        //                 else if (macroName == "ifdef")
        //                 {
        //                     auto args = getArgs(1, 2);
        //                     auto name = evaluateIdentifier(args[0]);
        //                     currentMacro.line = ifblock([&]()
        //                                                 { return context.macros.contains(name); },
        //                                                 args.size() > 1 ? args[1] : "");
        //                     currentMacro.firstChar = 0;
        //                     continue;
        //                 }
        //                 else if (macroName == "ifndef")
        //                 {
        //                     auto args = getArgs(1, 2);
        //                     auto name = evaluateIdentifier(args[0]);
        //                     currentMacro.line = ifblock([&]()
        //                                                 { return !context.macros.contains(name); },
        //                                                 args.size() > 1 ? args[1] : "");
        //                     currentMacro.firstChar = 0;
        //                     continue;
        //                 }
        //                 else if (macroName == "ifeq")
        //                 {
        //                     PREPROCESSOR_MATH_COMPARISON(==)
        //                 }
        //                 else if (macroName == "ifneq")
        //                 {
        //                     PREPROCESSOR_MATH_COMPARISON(!=)
        //                 }
        //                 else if (macroName == "ifgt")
        //                 {
        //                     PREPROCESSOR_MATH_COMPARISON(>)
        //                 }
        //                 else if (macroName == "iflt")
        //                 {
        //                     PREPROCESSOR_MATH_COMPARISON(<)
        //                 }
        //                 else if (macroName == "ifgteq")
        //                 {
        //                     PREPROCESSOR_MATH_COMPARISON(>=)
        //                 }
        //                 else if (macroName == "iflteq")
        //                 {
        //                     PREPROCESSOR_MATH_COMPARISON(<=)
        //                 }
        // #pragma endregion conditions
        // #pragma region mathops
        //                 else if (macroName == "inc")
        //                 {
        //                     auto name = evaluateIdentifier(getNextArg());
        //                     auto it = context.counters.find(name);
        //                     if (it == context.counters.end())
        //                     {
        //                         context.logger << ILogger::Error;
        //                         context.logger.AtPos(*currentMacro.line) << "Invalid counter " << name << " Could not increment" << ILogger::FlushNewLine;
        //                         throw PreprocessorException("Identifier not found");
        //                     }
        //                     ++it->second;
        //                 }
        //                 else if (macroName == "dec")
        //                 {
        //                     auto name = evaluateIdentifier(getNextArg());
        //                     auto it = context.counters.find(name);
        //                     if (it == context.counters.end())
        //                     {
        //                         context.logger << ILogger::Error;
        //                         context.logger.AtPos(*currentMacro.line) << "Invalid counter " << name << " Could not decrement" << ILogger::FlushNewLine;
        //                         throw PreprocessorException("Identifier not found");
        //                     }
        //                 }
        //                 else if (macroName == "set")
        //                 {
        //                     auto args = getArgs(1, 2);
        //                     auto name = evaluateIdentifier(args[0]);
        //                     int value;
        //                     if (args.size() == 1)
        //                         value = 0;
        //                     else
        //                         value = evaluateArgument(args[1]);
        //                     context.counters[name] = value;
        //                 }
        //                 else if (macroName == "del")
        //                 {
        //                     auto name = evaluateIdentifier(getNextArg());
        //                     if (name.empty())
        //                     {
        //                         context.logger << ILogger::Error;
        //                         context.logger.AtPos(*currentMacro.line) << "Missing argument for del" << ILogger::FlushNewLine;
        //                         throw PreprocessorException("Missing argument for del");
        //                     }
        //                     auto it = context.counters.find(name);
        //                     if (it == context.counters.end())
        //                     {
        //                         context.logger << ILogger::Warning;
        //                         context.logger.AtPos(*currentMacro.line) << name << " does not exist, can't delete it." << ILogger::FlushNewLine;
        //                     }
        //                     context.counters.erase(it);
        //                 }
        //                 else if (macroName == "mul")
        //                 {
        //                     PREPROCESSOR_BINARY_MATH_OP(*)
        //                 }
        //                 else if (macroName == "div")
        //                 {
        //                     PREPROCESSOR_BINARY_MATH_OP(/)
        //                 }
        //                 else if (macroName == "mod")
        //                 {
        //                     PREPROCESSOR_BINARY_MATH_OP(%)
        //                 }
        //                 else if (macroName == "add")
        //                 {
        //                     PREPROCESSOR_BINARY_MATH_OP(+)
        //                 }
        //                 else if (macroName == "sub")
        //                 {
        //                     PREPROCESSOR_BINARY_MATH_OP(-)
        //                 }
        // #pragma endregion mathops

        //                 else
        //                 {
        //                     context.logger << ILogger::Error;
        //                     context.logger.AtPos(*currentMacro.line) << "Unknown preprocessor command " << macroName << ILogger::FlushNewLine;
        //                     throw PreprocessorException("Unknown preprocessor command");
        //                 }
        //                 advanceLine();
        //             }

        //             //Paste the macro, but don't evaluate its contents
        //             if (currentMacro.line->content[currentMacro.firstChar + 1] == '!')
        //             {
        //                 size_t macroNameEnd = currentMacro.line->content.find(' ', currentMacro.firstChar + 2);
        //                 if (macroNameEnd == std::string::npos)
        //                 {
        //                     macroNameEnd = currentMacro.line->content.length();
        //                 }
        //                 std::string macroName(currentMacro.line->content.substr(currentMacro.firstChar + 2, macroNameEnd - currentMacro.firstChar - 2));
        //                 if (auto it2 = context.counters.find(macroName); it2 != context.counters.end())
        //                 {
        //                     auto asString = std::to_string(it2->second);
        //                     currentMacro.line->content.replace(currentMacro.firstChar, macroNameEnd - currentMacro.firstChar + 1, asString);
        //                     currentMacro.firstChar += asString.length();
        //                     continue;
        //                 }
        //                 if (auto macro = context.macros.find(macroName); macro != context.macros.end())
        //                 {
        //                     File body(macro->second);
        //                     auto bodyCenter = body.begin() + 1 != body.end() ? body.extractLines(body.begin() + 1, body.end() - 1) : body.extractLines(body.end(), body.end());
        //                     //Remove macro and split string
        //                     currentMacro.line->content.erase(currentMacro.firstChar, macroNameEnd - currentMacro.firstChar + 1);
        //                     auto remaining = currentMacro.line->content.substr(currentMacro.firstChar);
        //                     currentMacro.line->content.erase(currentMacro.firstChar);
        //                     currentMacro.line->content += body.begin()->content;
        //                     auto nextPos = currentMacro;
        //                     //If there is a second line, insert it
        //                     if (body.begin() + 1 != body.end())
        //                     {
        //                         context.file.insertAfter(currentMacro.line, (body.end() - 1)->content + remaining);
        //                         nextPos.line = currentMacro.line + 1;
        //                         nextPos.firstChar = body.end()->content.length();
        //                     }
        //                     else
        //                     {
        //                         nextPos.firstChar = nextPos.line->content.length();
        //                         currentMacro.line->content += remaining;
        //                     }

        //                     context.file.insertAfter(std::move(bodyCenter), currentMacro.line);
        //                     currentMacro = nextPos;
        //                     continue;
        //                 }
        //                 context.logger << ILogger::Error;
        //                 context.logger.AtPos(*currentMacro.line) << "Unknown identifier " << macroName << ILogger::FlushNewLine;
        //                 throw PreprocessorException("Unknown Identifier");
        //             }

        //             bool macroHasArgs = true;
        //             auto nextSpace = currentMacro.line->content.find(' ', currentMacro.firstChar);
        //             size_t macroNameEnd = std::min(currentMacro.line->content.find('(', currentMacro.firstChar), nextSpace);
        //             if (macroNameEnd == nextSpace)
        //             {
        //                 macroHasArgs = false;
        //             }
        //             if (macroNameEnd == std::string::npos)
        //             {
        //                 macroNameEnd = currentMacro.line->content.length();
        //             }
        //             std::string macroName = currentMacro.line->content.substr(currentMacro.firstChar + 1, macroNameEnd - currentMacro.firstChar - 1);
        //             macroName = evaluateIdentifier(macroName);
        //             macroName.erase(std::remove(macroName.begin(), macroName.end(), ' '), macroName.end());

        //             if (!macroHasArgs)
        //             {
        //                 if (auto it2 = context.counters.find(macroName); it2 != context.counters.end())
        //                 {
        //                     auto asString = std::to_string(it2->second);
        //                     currentMacro.line->content.replace(currentMacro.firstChar, macroNameEnd - currentMacro.firstChar + 1, asString);
        //                     continue;
        //                 }
        //             }
        //             std::vector<std::string> args;
        //             auto macroEnd = macroNameEnd;
        //             if (macroHasArgs)
        //             {
        //                 size_t currentpos = macroNameEnd + 1;

        //                 while (true)
        //                 {
        //                     currentpos = currentMacro.line->content.find(')', currentpos);
        //                     if (currentpos == std::string::npos)
        //                     {
        //                         context.logger << ILogger::Error;
        //                         context.logger.AtPos(*currentMacro.line) << "Unmatched parenthesis" << ILogger::FlushNewLine;
        //                         throw PreprocessorException("Macro arguments not closed");
        //                     }
        //                     if (handleEscapeSequence(currentMacro.line->content, currentpos))
        //                     {
        //                         ++currentpos;
        //                         continue;
        //                     }
        //                     break;
        //                 }
        //                 auto macroArgsLast = currentpos - 1;
        //                 auto macroArgsStart = macroNameEnd + 1;
        //                 auto macroArgsCurrent = macroArgsStart;
        //                 for (size_t i = macroArgsStart; i <= macroArgsLast; ++i)
        //                 {
        //                     if (currentMacro.line->content[i] == ',')
        //                     {
        //                         if (i > macroArgsStart)
        //                         {
        //                             if (currentMacro.line->content[i - 1] != '\\')
        //                             {
        //                                 args.push_back(currentMacro.line->content.substr(macroArgsCurrent, i - macroArgsCurrent));
        //                                 macroArgsCurrent = i + 1;
        //                             }
        //                             else
        //                             {
        //                                 currentMacro.line->content.erase(i - 1, 1);
        //                                 i--;
        //                                 macroArgsLast--;
        //                             }
        //                         }
        //                     }
        //                 }
        //                 args.push_back(currentMacro.line->content.substr(macroArgsCurrent, macroArgsLast - macroArgsCurrent + 1));
        //                 macroEnd = currentpos;
        //             }
        //             auto macro = context.macros.find(macroName);
        //             if (macro == context.macros.end())
        //             {
        //                 context.logger << ILogger::Error;
        //                 context.logger.AtPos(*currentMacro.line) << "Unknown identifier " << macroName << ILogger::FlushNewLine;
        //                 throw PreprocessorException("Unknown Identifier");
        //             }

        //             File body(macro->second);
        //             for (size_t i = 0; i < args.size(); ++i)
        //             {
        //                 File::FilePos arg{body.begin(), 0};
        //                 while (1)
        //                 {
        //                     //TODO expand macros in arguments
        //                     arg = body.find("$" + std::to_string(i), arg.line, arg.firstChar);
        //                     if (arg.firstChar == std::string_view::npos)
        //                         break;
        //                     arg.line->content.replace(arg.firstChar, 2, args[i]);
        //                 }
        //             }
        //             auto bodyCenter = body.begin() + 1 != body.end() ? body.extractLines(body.begin() + 1, body.end() - 1) : body.extractLines(body.end(), body.end());
        //             //Remove macro and split string
        //             currentMacro.line->content.erase(currentMacro.firstChar, macroEnd - currentMacro.firstChar + 1);
        //             auto remaining = currentMacro.line->content.substr(currentMacro.firstChar);
        //             currentMacro.line->content.erase(currentMacro.firstChar);
        //             currentMacro.line->content += body.begin()->content;
        //             //If there is a second line, insert it
        //             if (body.begin() + 1 != body.end())
        //             {
        //                 context.file.insertAfter(currentMacro.line, (body.end() - 1)->content + remaining);
        //             }
        //             else
        //             {
        //                 currentMacro.line->content += remaining;
        //             }

        //             context.file.insertAfter(std::move(bodyCenter), currentMacro.line);
        //         }
#pragma endregion deprecated

        {
            //TODO: remove
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