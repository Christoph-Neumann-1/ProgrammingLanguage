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

    app.add_option("-f,--file, file", fileName, "Input file")->required();

    CLI11_PARSE(app, argc, argv);

    VWA::ConsoleLogger logger;
    std::ifstream file(fileName);
    if (!file.is_open())
    {
        std::cout << "File not found" << std::endl;
        return -1;
    }
    try
    {
        std::cout << VWA::preprocess({.file = VWA::File{file, fileName}, .logger = logger}).toString() << std::flush;
    }
    catch (const VWA::PreprocessorException &e)
    {
        std::cout << "Preprocessor failed, see log for details" << std::endl;
        file.close();
        return -1;
    }
    file.close();
    return 0;
}