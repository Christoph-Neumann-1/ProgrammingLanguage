#include <iostream>
// #include <Preprocessor.hpp>
#include <filesystem>
#include <File.hpp>
#include <Preprocessor.hpp>
#include <Logger.hpp>
#include <CLI/CLI.hpp>
#include <Tokenizer.hpp>
#include <Parser.hpp>
#include <AST.hpp>
#include <VM.hpp>
#include <Imports.hpp>
#include <Compiler.hpp>
//TODO: Modernize code

//TODO proper interface
//TODO try different format styles
//TODO some kind of human readable assembly
int main(int argc, char *argv[])
{
    CLI::App app{"VWA Programming language"};

    std::string fileName;
    //TODO: allow piping using stdin
    app.add_option("-f,--file, file", fileName, "Input file")->required()->check(CLI::ExistingFile);
    std::string output = "output.src";
    auto outopt = app.add_option("-o,--output", output, "Output file");
    bool toStdout = false;
    app.add_flag("--stdout, !--nstdout", toStdout)->excludes(outopt);
    bool pponly = false;
    app.add_flag("-P", pponly);
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
    VWA::File result = VWA::preprocess({.file = VWA::File{file, fileName}, .logger = *logger});
    file.close();

    if (pponly)
    {
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
        return 0;
    }
    auto tokens = VWA::Tokenizer(std::move(result), *logger);
    auto root = VWA::generateParseTree(tokens);
    if (!root)
    {
        root.getError().log(*logger);
        return -1;
    }
    VWA::AST tree(root.getValue());
    std::cout << tree.toString();
    VWA::Imports::ImportManager manager;
    manager.AddIncludePath("modules/bin");
    VWA::Compiler compiler(manager);
    compiler.compile(tree);
    if (!manager.getMain())
        throw std::runtime_error("Invalid program no main funtion found");
    VWA::VM::VM vm;
    manager.compact();
    std::cout << "Running" << std::endl;
    auto rval = vm.run(manager.getMain());
    std::cout << "Ran vm with code " << rval << '\n';

    return 0;
}