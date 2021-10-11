#include <iostream>
// #include <Preprocessor.hpp>
#include <filesystem>
#include <File.hpp>
#include <Preprocessor.hpp>

//TODO proper interface
int main(int argc, char *argv[])
{
    //Load file to string
    // auto fileName = argv[1];
    std::string fileName = "/home/christoph/GitRepos/VWA-Project/testfile.src";
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