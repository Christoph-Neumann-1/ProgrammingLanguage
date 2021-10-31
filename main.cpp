#include <iostream>
// #include <Preprocessor.hpp>
#include <filesystem>
#include <File.hpp>
#include <Preprocessor.hpp>
#include <Logger.hpp>
#include <CLI/CLI.hpp>
#include <Commands.hpp>
//TODO: Modernize code

//TODO proper interface
//TODO refactor everything
//TODO more const
int main(int argc, char *argv[])
{
    CLI::App app{"VWA Programming language"};

    std::string fileName;

    app.add_option("-f,--file, file", fileName, "Input file")->required()->check(CLI::ExistingFile);
    std::string output = "output.src";
    auto outopt = app.add_option("-o,--output", output, "Output file");
    bool toStdout = false;
    app.add_flag("--stdout, !--nstdout", toStdout)->excludes(outopt);
    //TODO: direct different loglevels to different outputs, use callbacks
    std::string logFile;
    app.add_option("-l,--log", logFile, "Log file");
    CLI11_PARSE(app, argc, argv);

    std::unique_ptr<VWA::ILogger> logger;
    if (logFile.empty())
    {
        logger = std::make_unique<VWA::ConsoleLogger>();
    }
    else
    {
        logger = std::make_unique<VWA::FileLogger>(logFile);
    }
    std::ifstream file(fileName);
    if (!file.is_open())
    {
        std::cout << "File not found" << std::endl;
        return -1;
    }
    VWA::File result;
    #pragma region CommandDefinition
    std::unordered_map<std::string, std::unique_ptr<VWA::PreprocessorCommand>> commands;
    commands["define"] = std::make_unique<VWA::DefineCommand>();
    commands["eval"] = std::make_unique<VWA::EvalCommand>();
    commands["add"] = std::make_unique<VWA::MathCommand>([](int a, int b)
                                                         { return a + b; });
    commands["sub"] = std::make_unique<VWA::MathCommand>([](int a, int b)
                                                         { return a - b; });
    commands["mul"] = std::make_unique<VWA::MathCommand>([](int a, int b)
                                                         { return a * b; });
    commands["div"] = std::make_unique<VWA::MathCommand>([](int a, int b)
                                                         { return a / b; });
    commands["macro"] = std::make_unique<VWA::MacrodefinitionCommand>();
    commands["endmacro"] = std::make_unique<VWA::ReservedCommand>();
    commands["/"] = std::make_unique<VWA::CommentCommand>();
    commands["undef"] = std::make_unique<VWA::DeleteCommand>();
    commands["del"] = std::make_unique<VWA::DeleteCommand>();
    commands["set"] = std::make_unique<VWA::IntSetCommand>();
    commands["include"] = std::make_unique<VWA::IncludeCommand>();
    commands["import"] = std::make_unique<VWA::ReservedCommand>();
    commands["using"] = std::make_unique<VWA::ReservedCommand>();
    commands["!"] = std::make_unique<VWA::NoEvalCommand>();
    commands[""] = std::make_unique<VWA::ExpandCommand>();
    commands["ifdef"] = std::make_unique<VWA::IfdefCommand>(false);
    commands["ifndef"] = std::make_unique<VWA::IfdefCommand>(true);
    {
        auto tmp = [](int a, int b) constexpr -> bool
        { return a == b; };
        commands["ifeq"] = std::make_unique<VWA::MathComparisonCommand<tmp>>();
    }
    {
        auto tmp = [](int a, int b) constexpr -> bool
        { return a != b; };
        commands["ifneq"] = std::make_unique<VWA::MathComparisonCommand<tmp>>();
    }
    {
        auto tmp = [](int a, int b) constexpr -> bool
        { return a > b; };
        commands["ifgt"] = std::make_unique<VWA::MathComparisonCommand<tmp>>();
    }
    {
        auto tmp = [](int a, int b) constexpr -> bool
        { return a >= b; };
        commands["ifge"] = std::make_unique<VWA::MathComparisonCommand<tmp>>();
    }
    {
        auto tmp = [](int a, int b) constexpr -> bool
        { return a < b; };
        commands["iflt"] = std::make_unique<VWA::MathComparisonCommand<tmp>>();
    }
    {
        auto tmp = [](int a, int b) constexpr -> bool
        { return a <= b; };
        commands["ifle"] = std::make_unique<VWA::MathComparisonCommand<tmp>>();
    }
    #pragma endregion CommandDefinition

    try
    {

        result = VWA::preprocess({.file = VWA::File{file, fileName}, .logger = *logger, .commands = commands});
    }
    catch (const VWA::PreprocessorException &e)
    {
        std::cout << "Preprocessor failed, see log for details. Reason: " << e.what() << std::endl;
        file.close();
        return -1;
    }
    if (toStdout)
    {
        std::cout << result.toString() << std::endl;
    }
    else
    {
        std::ofstream out(output);
        out << result.toString();
        out.close();
    }
    file.close();
    return 0;
}