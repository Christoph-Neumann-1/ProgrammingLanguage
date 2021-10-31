#include <Commands.hpp>
#include <Preprocessor.hpp>

namespace VWA
{
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
        //TODO if more paramteters are passed, set all of them to the value
        auto expanded = expandIdentifier(args[0], context);
        if (!expanded)
        {
            throw PreprocessorException("Invalid identifier for define");
        }
        if (isKeyword(*expanded, context))
        {
            context.logger.AtPos(*current.line) << ILogger::Error << "Cannot define keyword " << *expanded << ILogger::FlushNewLine;
            throw PreprocessorException("Cannot define keyword");
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
        return current;
    }
    File::FilePos EvalCommand::operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args)
    {
        //TODO if more paramteters are passed, set all of them to the value
        auto expanded = expandIdentifier(args[0], context);
        if (!expanded)
        {
            throw PreprocessorException("Invalid identifier for define");
        }
        if (isKeyword(*expanded, context))
        {
            context.logger.AtPos(*current.line) << ILogger::Error << "Cannot define keyword " << *expanded << ILogger::FlushNewLine;
            throw PreprocessorException("Cannot define keyword");
        }
        auto what = args.size() == 1 ? '#' + args[0] : args.back();
        auto value = expandIdentifier(what, context); //TODO: check for nullopt
        RemoveOldDefinition(context, *expanded);
        std::istringstream stream(*value);
        context.macros[*expanded] = File(stream, current.line->fileName);
        return current;
    }

    File::FilePos MathCommand::operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args)
    {
        //TODO: if more args are given compute the sum/product/etc of them and store them in the first val
        auto destination = expandIdentifier(args[0], context);
        if (!destination)
            throw PreprocessorException("Invalid identifier for eval");
        if (isKeyword(*destination, context))
        {
            context.logger.AtPos(*current.line) << ILogger::Error << "Cannot define keyword " << *destination << ILogger::FlushNewLine;
            throw PreprocessorException("Cannot define keyword");
        }
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
        return current;
    }

    File::FilePos MacrodefinitionCommand::operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args)
    {
        if (isKeyword(args[0], context))
        {
            context.logger.AtPos(*current.line) << ILogger::Error << "Cannot define keyword " << args[0] << ILogger::FlushNewLine;
            throw PreprocessorException("Cannot define keyword");
        }
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

    File::FilePos IntSetCommand::operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args)
    {
        auto destination = expandIdentifier(args[0], context);
        if (!destination)
            throw PreprocessorException("Invalid identifier for intset");
        if (isKeyword(*destination, context))
        {
            context.logger.AtPos(*current.line) << ILogger::Error << "Cannot define keyword " << *destination << ILogger::FlushNewLine;
            throw PreprocessorException("Cannot define keyword");
        }
        int value = 0;
        if (args.size() - 1)
        {
            if (auto res = expandIdentifier(args[1], context))
            {
                if (!std::isdigit(res->front()))
                {
                    res = expandIdentifier('#' + *res, context);
                }
                try
                {
                    value = std::stoi(*res);
                }
                catch (std::invalid_argument &e)
                {
                    throw PreprocessorException("Invalid argument for set");
                }
            }
            else
            {
                throw PreprocessorException("Invalid argument for set");
            }
        }
        RemoveOldDefinition(context, *destination);
        context.counters[*destination] = value;
        return current;
    }

    File::FilePos NoEvalCommand::operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args)
    {
        File::FilePos next;
        auto name = expandIdentifier(args[0], context);
        if (!name)
        {
            throw PreprocessorException("Macro expansion failed");
        }
        PasteMacro(*name, current, context, &next, 3, 1);
        return next;
    }

    File::FilePos ExpandCommand::operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args)
    {
        auto name = expandIdentifier(args[0], context);
        if (!name)
        {
            throw PreprocessorException("Macro expansion failed");
        }
        PasteMacro(*name, current, context, nullptr, 2, 1);
        return current;
    }

    //Do i need an escape sequence for endif?
    File::FilePos BranchCommand::operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args)
    {
        //TODO: best time to handle escaped endifs?
        std::string endifName="#endif(" + (args.size()==numArgs?"":args.back()) + ")";
        auto end = context.file.find(endifName, current.line + 1);
        for (; end.firstChar != std::string::npos; end = context.file.find(endifName, end.line + 1))
        {
            if (IsFirstNonSpace(end.line->content, end.firstChar))
                break;
        }
        if (end.firstChar == std::string::npos)
            throw PreprocessorException("No endif found for branch");
        if (IsTrue(context, args))
        {
            current.line = context.file.removeLine(current.line);
            current.firstChar = 0;
            current.line = current.line == end.line ? end.line + 1 : current.line;
            context.file.removeLine(end.line);
            return current;
        }
        else
        {
            current.line = context.file.removeLines(current.line, end.line + 1);
            current.firstChar = 0;
            return current;
        }
    }
    File::FilePos DeleteCommand::operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args)
    {
        if (isKeyword(args[0], context))
            throw PreprocessorException("Keyword may not be deleted " + args[0]);
        for (auto &arg : args)
        {
            if (auto it = context.macros.find(arg); it != context.macros.end())
            {
                context.macros.erase(it);
            }
            else if (auto it2 = context.counters.find(arg); it2 != context.counters.end())
            {
                context.counters.erase(it2);
            }
        }
        return current;
    }

    bool IfdefCommand::IsTrue(PreprocessorContext &context, const std::vector<std::string> &args)
    {
        return invert ^ (context.macros.find(args[0]) != context.macros.end() ? true : context.counters.find(args[0]) != context.counters.end());
    }

    File::FilePos IncludeCommand::operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args)
    {
        auto path = std::filesystem::path(*current.line->fileName).parent_path() / args[0];
        std::ifstream file(path);
        if (!file.is_open())
            throw PreprocessorException("Could not open file " + path.string());
        File filecontents(file, path.string());
        context.file.insertAfter(filecontents, current.line);
        current.line = context.file.removeLine(current.line);
        current.firstChar = 0;
        return current;
    }
}