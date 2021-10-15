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
    std::cout << VWA::preprocess(input).toString() << std::endl;

    return 0;
}