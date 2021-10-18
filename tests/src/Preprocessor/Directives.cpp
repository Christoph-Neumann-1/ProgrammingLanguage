#include <Preprocessor.hpp>
#include <catch2/catch_test_macros.hpp>

namespace VWA
{
    SCENARIO("Preprocessor include command", "[preprocessor]")
    {
        GIVEN("A file including two other files in the same directory")
        {
            std::ifstream stream("Preprocessor/include1");
            File f(stream,std::make_shared<std::string>("Preprocessor/basefile"));
            f.append("##include f1");
            f.append("##include f2");
            VoidLogger voidLogger;
            WHEN("The other files are included")
            {
                auto res = preprocess(f, voidLogger);
                auto string = res.toString();
                THEN("The file should contain the content of the other files")
                {
                    REQUIRE(string == "included 1\nincluded 2");
                }
            }
        }
    }
}