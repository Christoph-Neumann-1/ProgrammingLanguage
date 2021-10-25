#include <iostream>
// #include <Preprocessor.hpp>
#include <filesystem>
#include <File.hpp>
#include <Preprocessor.hpp>
#include <Logger.hpp>
#include <CLI/CLI.hpp>
//TODO: Modernize code

//TODO proper interface
//TODO refactor everything
//TODO more const
int main(int argc, char *argv[])
{
    CLI::App app{"VWA Programming language"};

    std::string fileName;

    app.add_option("-f,--file, file", fileName, "Input file")->required()->check(CLI::ExistingFile);
    std::string output="output.src";
    auto outopt=app.add_option("-o,--output", output, "Output file");
    bool toStdout=false;
    app.add_flag("--stdout, !--nstdout",toStdout)->excludes(outopt);
    //TODO: direct different loglevels to different outputs, use callbacks
    std::string logFile;
    app.add_option("-l,--log", logFile, "Log file");
    CLI11_PARSE(app, argc, argv);

    std::unique_ptr<VWA::ILogger> logger;
    if(logFile.empty())
    {
        logger=std::make_unique<VWA::ConsoleLogger>();
    }
    else
    {
        logger=std::make_unique<VWA::FileLogger>(logFile);
    }
    std::ifstream file(fileName);
    if (!file.is_open())
    {
        std::cout << "File not found" << std::endl;
        return -1;
    }
    VWA::File result;
    try
    {
        result = VWA::preprocess({.file = VWA::File{file, fileName}, .logger = *logger});
    }
    catch (const VWA::PreprocessorException &e)
    {
        std::cout << "Preprocessor failed, see log for details" << std::endl;
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