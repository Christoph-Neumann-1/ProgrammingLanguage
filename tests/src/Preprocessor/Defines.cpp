#include <Preprocessor.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace VWA;
SCENARIO("Defining and later using a macro")
{
    GIVEN("A file")
    {
        File f("mfile");
        VoidLogger logger;
        WHEN("A macro is created using #define")
        {
            f.append("#define(test,expanded)");
            THEN("Using ifdef should be true")
            {
                f.append("#ifdef(test)");
                f.append("true");
                f.append("#endif()");
                REQUIRE(preprocess({f,logger}).toString() == "true");
            }
            WHEN("The macro is expanded")
            {
                THEN("The content of the macro should be pasted")
                {
                    f.append("#(test)");
                    REQUIRE(preprocess({f, logger}).toString() == "expanded");
                }
                WHEN("There are other characters on the line")
                {
                    f.append("before#(test)after");
                    THEN("The everything preceding the macro should stay the same and everything after the space following the macro should stay the same")
                    {
                        REQUIRE(preprocess({f, logger}).toString() == "beforeexpandedafter");
                    }
                }
            }
            WHEN("Using nested macros")
            {
                f.append("#define(macro2,#(test))");
                f.append("#(macro2)");
                THEN("The content of the macro should also be evaluated")
                {
                    REQUIRE(preprocess({f, logger}).toString() == "expanded");
                }
            }
            WHEN("Trying to expand an undefined macro")
            {
                THEN("An exception should be thrown")
                {
                    f.append("#(undefinedmacro)");
                    REQUIRE_THROWS_AS(preprocess({f, logger}), PreprocessorException);
                }
            }
        }
    }
}