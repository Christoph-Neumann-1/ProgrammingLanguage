#include <Preprocessor.hpp>
#include <gtest/gtest.h>

namespace VWA
{

    extern std::function<std::unique_ptr<std::istream>(const std::string &)> ReadFile;

    TEST(Preprocessor, Include)
    {
        ReadFile = [&](const std::string &filename) -> std::unique_ptr<std::istream>
        {
            if (filename == "f1")
            {
                return std::make_unique<std::istringstream>("included 1");
            }
            if (filename == "f2")
            {
                return std::make_unique<std::istringstream>("included 2");
            }
            throw std::runtime_error("Test tried accessing file " + filename);
        };
        File f(std::make_shared<std::string>("basefile"));
        f.append("##include f1");
        f.append("##include f2");
        VoidLogger voidLogger;
        auto res=preprocess(f,voidLogger);
        auto string=res.toString();
        EXPECT_EQ(string,"included 1\nincluded 2");
    }
}