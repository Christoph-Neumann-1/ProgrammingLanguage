#include <Preprocessor.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace VWA;

SCENARIO("Defining and later using a macro")
{
    GIVEN("A file")
    {
        File f("mfile");
        VoidLogger logger;
        WHEN("A macro is created using ##define")
        {
            f.append("##define macro expanded");
            THEN("Using ifdef should be true")
            {
                auto tmp = f;
                tmp.append("##ifdef macro");
                tmp.append("true");
                tmp.append("##endif");
                REQUIRE(preprocess(tmp, logger).toString() == "true");
            }
            WHEN("The macro is expanded")
            {
                auto tmp = f;
                tmp.append("#macro");
                THEN("The content of the macro should be pasted")
                {
                    REQUIRE(preprocess(tmp, logger).toString() == "expanded");
                }
                WHEN("There are other characters on the line")
                {
                    auto tmp2 = f;
                    tmp2.append("before#macro after");
                    THEN("The everything preceding the macro should stay the same and everything after the space following the macro should stay the same")
                    {
                        REQUIRE(preprocess(tmp2, logger).toString() == "beforeexpandedafter");
                    }
                }
            }
            WHEN("Using nested macros")
            {
                auto tmp = f;
                tmp.append("##define macro2 #macro");
                tmp.append("#macro2");
                THEN("The content of the macro should also be evaluated")
                {
                    REQUIRE(preprocess(tmp, logger).toString() == "expanded");
                }
            }
            WHEN("Trying to expand an undefined macro")
            {
                THEN("An exception should be thrown")
                {
                    auto tmp = f;
                    tmp.append("#undefinedmacro");
                    REQUIRE_THROWS_AS(preprocess(tmp, logger), PreprocessorException);
                }
            }
        }
    }
}