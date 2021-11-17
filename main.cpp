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
#include <Interpreter.hpp>
#include <VM.hpp>
//TODO: Modernize code

//TODO proper interface
//TODO refactor everything
//TODO more const
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
    bool pponly;
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
    auto tokens = VWA::Tokenizer(std::move(result));
    auto root = VWA::generateParseTree(tokens);
    // std::cout << VWA::TreeToString(root) << std::endl;
    VWA::AST tree(root);
    // std::cout<<tree.toString();
    using namespace VWA::instruction;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-braces"
    ByteCodeElement code[]{
        JumpFunc, 26, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        Return, 4, 0, 0, 0, 0, 0, 0, 0,
        PushConst32, 1, 0, 0, 0,
        PushConst32, 2, 0, 0, 0,
        AddI,
        Return, 4, 0, 0, 0, 0, 0, 0, 0};
#pragma clang diagnostic pop
    VWA::VM::VM::FileInfo fi;
    fi.bc = std::make_unique<ByteCodeElement[]>(sizeof(code) / sizeof(ByteCodeElement));
    std::memcpy(fi.bc.get(), code, sizeof(code));
    fi.bcSize = sizeof(code);
    fi.exportedFunctions.push_back({"main", "int", {}, {0}, false, true});
    VWA::VM::VM vm(std::move(fi));
    std::cout << "Ran vm with code" << vm.run() << '\n';
    return 0;
}