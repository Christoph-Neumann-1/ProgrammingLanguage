#include <iostream>
// #include <Preprocessor.hpp>
#include <filesystem>
#include <File.hpp>
#include <Preprocessor.hpp>
#include <Logger.hpp>
//TODO: concepts for everything
//TODO: Modernize code

//TODO proper interface
int main(int argc, char *argv[])
{
    VWA::ConsoleLogger logger;
    //Load file to string
    if (argc < 2)
    {
        std::cout << "No file specified" << std::endl;
        return 1;
    }
    auto fileName = argv[1];
    std::ifstream file(fileName);
    if (!file.is_open())
    {
        std::cout << "File not found" << std::endl;
        return -1;
    }
    VWA::File input(file, std::make_shared<std::string>(fileName));
    file.close();
    try
    {std::cout << VWA::preprocess(input, logger).toString() << std::endl;}
    catch (const VWA::PreprocessorException &e)
    {
        std::cout << "Preprocessor failed, see log for details" << std::endl;
        return -1;
    }

    return 0;
}