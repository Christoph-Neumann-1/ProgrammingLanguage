#include <File.hpp>
#include <catch2/catch_test_macros.hpp>
#include <sstream>

using namespace VWA;

SCENARIO("Creating file from istream","[file]")
{
    GIVEN("An istream(for example ifstream)")
    {
        std::istringstream stream("Example text\nMore text");
        WHEN("Creating a file from the stream")
        {
            File file(stream,"ExampleFile");
            THEN("The file should contain the same text as the stream")
            {
                REQUIRE(file.toString() == "Example text\nMore text");
            }
            THEN("The file should contain two lines with the correct contents")
            {
                REQUIRE(file.begin()->content == "Example text");
                REQUIRE(file.begin()->next->content == "More text");
                REQUIRE(file.begin()+2 == file.end());
            }
        }
    }
}

SCENARIO("Appending to a file"){
    File file("examplefile");
}