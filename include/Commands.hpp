#pragma once
#include <Preprocessor.hpp>

namespace VWA
{

    struct PreprocessorContext;

    class PreprocessorCommand
    {
    public:
        const bool requireStartOfLine;
        const bool requireEndOfLine;
        const bool erasesLine;
        const bool requiresRawArguments; //Skip breaking up the string into arguments and pass the unprocessed string as args[0]
        const int minArguments;
        const int maxArguments; //-1 for unlimited
        //TODO: additional constraints for args

        explicit PreprocessorCommand(const bool _requireStartOfLine = true, const bool _requireEndOfLine = true, const bool _erasesLine = true,
                                     const bool _requiresRawArguments = false, const int _minArgs = 0, const int _maxArgs = -1)
            : requireStartOfLine(_requireStartOfLine), requireEndOfLine(_requireEndOfLine), erasesLine(_erasesLine),
              requiresRawArguments(_requiresRawArguments), minArguments(_minArgs), maxArguments(_maxArgs) {}
        virtual ~PreprocessorCommand() = default;
        virtual File::FilePos operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args = {}) = 0;
    };

    class ReservedCommand : public PreprocessorCommand
    {
    public:
        ReservedCommand() : PreprocessorCommand(false, false, false) {}
        File::FilePos operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args = {}) override
        {
            throw PreprocessorException("Keyword may not be used in preprocessor " + fullIdentifier);
        }
    };

    class CommentCommand : public PreprocessorCommand
    {
    public:
        CommentCommand() : PreprocessorCommand(false, false, false, true) {}
        File::FilePos operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args = {}) override
        {
            if (args[0].empty())
                current.line->content.erase(current.firstChar);
            else
            {
                current.line->content.erase(current.firstChar, fullIdentifier.length() + 1);
            }
            return current;
        }
    };

    class SetterCommon : virtual public PreprocessorCommand
    {
    protected:
        void RemoveOldDefinition(PreprocessorContext &context, const std::string &identifier);
    };
    class DefineCommand : public SetterCommon
    {
    public:
        DefineCommand() : PreprocessorCommand(true, true, true, false, 1) {}
        File::FilePos operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args = {}) override;
    };

    class EvalCommand : public SetterCommon
    {
    public:
        EvalCommand() : PreprocessorCommand(true, true, true, false, 1) {}
        File::FilePos operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args = {}) override;
    };

    class MathCommand : public SetterCommon
    {
        int (*op)(int, int);

    public:
        File::FilePos operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args = {}) override;

    public:
        MathCommand(int (*op)(int, int)) : PreprocessorCommand(true, true, true, false, 2), op(op) {}
    };
    class MacrodefinitionCommand : public SetterCommon
    {
    public:
        //erasesLine is false, because the macro command needs to erase more than one line and its more efficient to delete all lines at once
        MacrodefinitionCommand() : PreprocessorCommand(true, true, false, false, 1, 1) {}
        File::FilePos operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args = {}) override;
    };

    class IntSetCommand : public SetterCommon
    {
    public:
        IntSetCommand() : PreprocessorCommand(true, true, true, false, 1) {}
        File::FilePos operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args = {}) override;
    };

    class DeleteCommand : public PreprocessorCommand
    {
    public:
        DeleteCommand() : PreprocessorCommand(true, true, true, false, 1) {}
        File::FilePos operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args = {}) override;
    };

    //TODO: crtp
    class BranchCommand : public PreprocessorCommand
    {
    protected:
        virtual bool IsTrue(PreprocessorContext &context, const std::vector<std::string> &args) = 0;

    public:
        BranchCommand(int minIn = 2, int maxIn = 3) : PreprocessorCommand(true, true, false, false, minIn, maxIn) {}
        File::FilePos operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args = {}) override;
    };

    class IfdefCommand : public BranchCommand
    {
        bool invert;
        bool IsTrue(PreprocessorContext &context, const std::vector<std::string> &args) override;

    public:
        IfdefCommand(bool _invert) : BranchCommand(1, 2), invert(_invert) {}
    };

    template <bool (*op)(int, int)>
    class MathComparisonCommand : public BranchCommand
    {
    public:
        MathComparisonCommand() : BranchCommand(2, 3) {}
        bool IsTrue(PreprocessorContext &context, const std::vector<std::string> &args) override
        {
            int operands[2];
            for (int i = 0; i < 2; ++i)
                if (auto res = expandIdentifier(args[i], context))
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
                        throw PreprocessorException("Invalid argument for math comp");
                    }
                }
                else
                {
                    throw PreprocessorException("Invalid argument for math comp");
                }
            return op(operands[0], operands[1]);
        }
    };

    class ExpandCommand : public PreprocessorCommand
    {
    public:
        ExpandCommand() : PreprocessorCommand(false, false, false, true) {}
        File::FilePos operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args = {}) override;
    };

    class NoEvalCommand : public PreprocessorCommand
    {
    public:
        NoEvalCommand() : PreprocessorCommand(false, false, false, true) {}
        File::FilePos operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args = {}) override;
    };

    class IncludeCommand : public PreprocessorCommand
    {
    public:
        IncludeCommand() : PreprocessorCommand(true, true, false, true) {}
        File::FilePos operator()(PreprocessorContext &context, File::FilePos current, const std::string &fullIdentifier, const std::vector<std::string> &args = {}) override;
    };
}