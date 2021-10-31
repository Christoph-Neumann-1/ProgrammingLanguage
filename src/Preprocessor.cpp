#include <Preprocessor.hpp>
#include <Commands.hpp>

//TODO: better way to define new keywords
//TODO: assert that appending still works
//TODO: line numbers in macros
//TODO: handle // in strings either detect strings  in preprocessor or provide escape sequence
//TODO: figure out best time to expand macro parameters
//TODO: error trace for nested macros
//TODO: delete counter upon defining macro with same name and vice versa
//TODO: forbid redefining keywords
//TODO: move keywords to other file

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
    size_t FindEndOfIdentifier(const std::string_view &line, size_t pos)
    {
        uint counter{0};
        for (pos = line.find('('); pos < line.size(); ++pos)
        {
            if (line[pos] == '(')
                ++counter;
            else if (line[pos] == ')')
                --counter;
            if (!counter)
                return pos;
        }
        return std::string_view::npos;
    }
    //TODO Find better way to handle errors
    std::variant<std::monostate, std::string, File, int> expandMacro(const std::string &identier, const PreprocessorContext &context, const std::vector<std::string> &args)
    {
        auto macro = context.macros.find(identier);
        if (macro != context.macros.end())
        {
            File body(macro->second);
            for (int i = 0; i < args.size(); ++i)
            {
                auto name = "$" + std::to_string(i);
                for (auto current = body.find(name); current.firstChar != std::string_view::npos; current = body.find(name))
                {
                    current.line->content.replace(current.firstChar, current.firstChar + name.size(), args.at(i));
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
    std::vector<std::string> getMacroArgs(std::string &identifier)
    {
        if (identifier.back() != ')')
            return {};
        auto openParenthesis = identifier.find('(');
        if (openParenthesis == identifier.size() - 1)
        {
            identifier.erase(identifier.size() - 2);
            return {};
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

    //TODO: better errors. Pass the current line? Return error struct? Or just use exceptions?
    //TODO: don't require a space at  the end, or erase it. The former is prefered
    std::optional<std::string> expandIdentifier(std::string identifier, PreprocessorContext &context)
    {
        auto original = identifier;
        for (auto i = identifier.find('#'); i != identifier.npos; i = identifier.find('#', i))
        {
            auto end = FindEndOfIdentifier(identifier, i);
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
            auto name = identifier.substr(i + 2, end - i - 2);
            //TODO: same functions as main preprocessor
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
    void PasteMacro(std::string identifier, File::FilePos position, PreprocessorContext &context, File::FilePos *nextCharacter, size_t prefixLength, size_t postfixLength)
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
            position.line->content.replace(position.firstChar, identifier.size() + prefixLength + postfixLength, *s);
            return;
        }
        if (auto file = std::get_if<File>(&expanded))
        {
            std::string first = file->begin()->content;
            std::string last = file->back()->content;
            auto Body = file->extractLines(file->begin() + 1, file->back());
            std::string remainder = position.line->content.substr(position.firstChar + identifier.size() + prefixLength + postfixLength - 1);
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
            position.line->content.replace(position.firstChar, identifier.size() + prefixLength + postfixLength, string);
            return;
        }
    }

    bool isKeyword(const std::string &identifier, const PreprocessorContext &ctxt)
    {
        return ctxt.commands.find(identifier) != ctxt.commands.end();
    }

    //TODO: loop over line and not entire file
    File preprocess(PreprocessorContext context)
    {

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

        for (auto line = context.file.begin(); line != context.file.end(); ++line)
        {
            for (auto current = line->content.find("#"); current != line->content.npos && line != context.file.end(); current = line->content.find("#", current))
            {
                if (current)
                    if (line->content[current - 1] == '\\')
                    {
                        line->content.erase(current - 1, 1);
                        continue;
                    }
                auto nameEnd = FindEndOfIdentifier(line->content, current + 1);
                auto identifier = line->content.substr(current + 1, nameEnd - current);
                {
                    //TODO: consider moving the evaluation and substring part to the command class
                    auto copy = identifier;
                    auto args = getMacroArgs(copy);
                    if (auto command = context.commands.find(copy); command != context.commands.end())
                    {

                        if (command->second->requireStartOfLine)
                        {
                            if (!IsFirstNonSpace(line->content, current))
                            {
                                throw PreprocessorException("Command " + copy + " must be at the start of a line");
                            }
                        }
                        if (command->second->requireEndOfLine)
                        {
                            auto idlenght = identifier.length();
                            if (current + idlenght != line->content.length())
                            {
                                if (auto pos = line->content.find_first_not_of(" \t", current + identifier.length()); pos == line->content.npos)
                                {
                                    throw PreprocessorException("Command " + copy + " must be at the end of a line");
                                }
                            }
                        }
                        //TODO: avoid unnecessary copy and call to getMacroArgs
                        if (command->second->requiresRawArguments)
                        {
                            //If getMacroArgs returns nullopt it means there are no arguments, so an empty vector is returned
                            if (!args.empty())
                            {
                                auto openBrace = identifier.find('(');
                                args = {identifier.substr(openBrace + 1, identifier.size() - openBrace - 2)};
                            }
                        }
                        else
                        {
                            if (args.size() < command->second->minArguments)
                            {
                                throw PreprocessorException("Command " + copy + " requires at least " + std::to_string(command->second->minArguments) + " arguments");
                            }
                            if (args.size() > command->second->maxArguments)
                            {
                                throw PreprocessorException("Command " + copy + " requires at most " + std::to_string(command->second->maxArguments) + " arguments");
                            }
                        }
                        auto [nextline, nextChar] = (*command->second)(context, {line, current}, identifier, args);
                        if (command->second->erasesLine)
                        {
                            line = context.file.removeLine(line);
                            current = 0;
                        }
                        else
                        {
                            line = nextline;
                            current = nextChar;
                        }
                        continue;
                    }
                    else
                    {
                        throw PreprocessorException("Unknown command " + identifier);
                    }
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
        return context.file;
    }
}