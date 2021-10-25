#include <Preprocessor.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace VWA;
SCENARIO("Preprocessor include command", "[preprocessor]")
{
    GIVEN("A file including two other files in the same directory")
    {
        File f("Preprocessor/basefile");
        f.append("##include f1");
        f.append("##include f2");
        VoidLogger voidLogger;
        WHEN("The other files are included")
        {
            auto res = preprocess({f, voidLogger});
            auto string = res.toString();
            THEN("The file should contain the content of the other files")
            {
                REQUIRE(string == "included 1\nincluded 2");
            }
        }
    }
    GIVEN("A file including a file including a other file")
    {
        File f("Preprocessor/basefile");
        f.append("##include f3");
        VoidLogger voidLogger;
        WHEN("f3 is included")
        {
            auto res = preprocess({f, voidLogger});
            auto string = res.toString();
            THEN("The file should contain the content of f4")
            {
                REQUIRE(string == "included 4");
            }
        }
    }
}
SCENARIO("Includes in expanded macros", "[preprocessor]")
{
    GIVEN("A macro expanding to ##include f2")
    {
        File f("Preprocessor/basefile");
        f.append("MACRO f2");
        f.append("##include f2");
        f.append("ENDMACRO");
        f.append("#f2");
        WHEN("The macro is expanded")
        {
            VoidLogger voidLogger;
            auto res = preprocess({f, voidLogger});

            THEN("The file should be included")
            {
                REQUIRE(res.toString() == "included 2");
            }
        }
    }
}
SCENARIO("Include guards using ifdef", "[preprocessor]")
{
    GIVEN("A file including another file multiple times")
    {
        File f("Preprocessor/basefile");
        f.append("##include f5");
        f.append("##include f5");
        WHEN("The second time the file is included")
        {
            VoidLogger voidLogger;
            auto res = preprocess({f, voidLogger});
            THEN("The file should be ignored")
            {
                REQUIRE(res.toString() == "included 5");
            }
        }
    }
}
