#include <iostream>
#include <Preprocessor.hpp>
#include <cstdio>
#include <filesystem>

int main(int argc, char* argv[])
{
    //Load file to string
    // auto fileName = argv[1];
    std::string fileName = "/home/christoph/GitRepos/VWA-Project/testfile.src";
    auto file=fopen(fileName.c_str(),"r");
    if(file==nullptr)
    {
        std::cout<<"File not found"<<std::endl;
        return 1;
    }
    std::string buffer;
    //read entire file to string
    fseek(file,0,SEEK_END);
    auto size=ftell(file);
    fseek(file,0,SEEK_SET);
    buffer.resize(size);
    fread(&buffer[0],size,1,file);
    fclose(file);
    //Change working directory
    std::filesystem::current_path(std::filesystem::path(fileName).parent_path());

    std::cout<<VWA::preprocess(buffer)<<std::endl;

    return 0;
}