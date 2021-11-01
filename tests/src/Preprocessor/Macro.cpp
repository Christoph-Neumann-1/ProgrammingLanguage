#include <Preprocessor.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace VWA;

SCENARIO("Defining function like macros", "[preprocessor]")
{
    File f;
    GIVEN("A file containing only the definition of the macro")
    {
        f.append("#macro(x)");
        f.append("random content");
        f.append("#endmacro(x)");
        WHEN("The file is processed")
        {
            auto res = preprocess({f});
            THEN("The file should be empty")
            {
                REQUIRE(res.toString() == "");
            }
        }
    }
    GIVEN("A file defining and later using the macro")
    {
        f.append("#macro(x)");
        f.append("random content");
        f.append("more content");
        f.append("#endmacro(x)");
        f.append("#(x)");
        WHEN("The file is processed")
        {
            auto res = preprocess({f});
            THEN("The file should contain the macro definition")
            {
                REQUIRE(res.toString() == "random content\nmore content");
            }
        }
    }
    GIVEN("A macro using define and one using macro")
    {
        f.append("#define(x1,content1)");
        f.append("#macro(x2)");
        f.append("content2");
        f.append("#endmacro(x2)");
        f.append("#(x1)");
        f.append("#(x2)");
        WHEN("They are expanded")
        {
            auto res = preprocess({f});
            THEN("The two should be equivalent")
            {
                REQUIRE(res.toString() == "content1\ncontent2");
            }
        }
    }
}

SCENARIO("Redefining macros")
{
    GIVEN("A file defining and using a macro")
    {
        File f;
        f.append("#define(x,content)");
        f.append("#(x)");
        WHEN("The macro is redefined")
        {
            f.append("#define(x,content2)");
            f.append("#(x)");
            THEN("The latest definition should be used")
            {
            auto res = preprocess({f});
                REQUIRE(res.toString() == "content\ncontent2");
            }
        }
    }
}

SCENARIO("Nested macros")
{
    GIVEN("A macro whose content is the definition of another macro")
    {
        File f;
        f.append("#macro(x)");
        f.append("#macro(y)");
        f.append("content2");
        f.append("#endmacro(y)");
        f.append("#endmacro(x)");

        WHEN("Trying to invoke the macro before calling the containing one")
        {
            f.append("#(y)");
            THEN("An error should be raised")
            {
                REQUIRE_THROWS_AS(preprocess({f}), PreprocessorException);
            }
        }
        WHEN("Trying to invoke the macro after calling the containing one")
        {
            f.append("#(x)");
            f.append("#(y)");
            THEN("It should succeed")
            {
                auto res = preprocess({f});
                REQUIRE(res.toString() == "content2");
            }
        }
    }
}

SCENARIO("Giving arguments to the macro")
{
    GIVEN("A macro expecting arguments")
    {
        File f;
        f.append("#macro(x)");
        f.append("$0$1");
        f.append("$2");
        f.append("#endmacro(x)");
        WHEN("The macro is invoked without arguments")
        {
            f.append("#(x)");
            THEN("The placeholders should be pasted")
            {
                auto res = preprocess({f});
                REQUIRE(res.toString() == "$0$1\n$2");
            }
        }
        WHEN("The macro is invoked with arguments")
        {
            f.append("#(x(1,2,3))");
            THEN("The arguments should be substituted")
            {
                auto res = preprocess({f});
                REQUIRE(res.toString() == "12\n3");
            }
        }
    }
}
