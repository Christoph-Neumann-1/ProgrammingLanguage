#include <iostream>
#include <Preprocessor.hpp>
#include <cstdio>

int main(int argc, char* argv[])
{
    //Load file to string
    auto file=fopen("/home/christoph/GitRepos/VWA-Project/testfile.src","r");
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

    std::cout<<VWA::preprocess(buffer)<<std::endl;

    return 0;
}