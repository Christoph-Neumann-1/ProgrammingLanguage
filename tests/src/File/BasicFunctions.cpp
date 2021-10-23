#include <File.hpp>
#include <catch2/catch_test_macros.hpp>
#include <sstream>

using namespace VWA;

SCENARIO("Creating file from istream", "[file]")
{
    GIVEN("An istream(for example ifstream)")
    {
        std::istringstream stream("Example text\nMore text");
        WHEN("Creating a file from the stream")
        {
            File file(stream, "ExampleFile");
            THEN("The file should contain the same text as the stream")
            {
                REQUIRE(file.toString() == "Example text\nMore text");
            }
            THEN("The file should contain two lines with the correct contents")
            {
                REQUIRE(file.begin()->content == "Example text");
                REQUIRE(file.begin()->next->content == "More text");
                REQUIRE(file.begin() + 2 == file.end());
            }
            THEN("The file should be valid")
            {
                REQUIRE(file.valid());
            }
        }
    }
}

SCENARIO("Creating an empty file", "[file]")
{
    WHEN("Creating an empty file")
    {
        File file("");
        THEN("Begin should be equal to length")
        {
            REQUIRE(file.begin() == file.end());
        }
        THEN("empty() should return true")
        {
            REQUIRE(file.empty());
        }
        THEN("The file should be valid")
        {
            REQUIRE(file.valid());
        }
    }
}

SCENARIO("File to string", "[file]")
{
    GIVEN("A file with some content")
    {
        std::string content = "Example text\nMore text";
        std::istringstream stream(content);
        File file(stream, "");
        WHEN("Converting the file to a string")
        {
            THEN("The string should contain the same text as the file")
            {
                REQUIRE(file.toString() == content);
            }
        }
    }
    GIVEN("An empty file")
    {
        File f("");
        WHEN("Converting the file to a string")
        {
            THEN("The string should be empty")
            {
                REQUIRE(f.toString() == "");
            }
        }
    }
    GIVEN("An invalid file")
    {
        File file("");
        File tmpfile(std::move(file));
        WHEN("Converting the file to a string")
        {
            THEN("An exception should be thrown")
            {
                REQUIRE_THROWS(file.toString());
            }
        }
    }
}

SCENARIO("Copying a file", "[file]")
{
    GIVEN("A valid empty file")
    {
        File f("f1");
        WHEN("Calling the copy constructor")
        {
            File copy(f);
            THEN("The copy should be equal to the original")
            {
                REQUIRE(copy.valid());
                REQUIRE(f.valid());
                REQUIRE(copy.toString() == f.toString());
                REQUIRE(copy.getFileName() == f.getFileName());
            }
        }
        WHEN("Calling the copy assignment operator")
        {
            File copy("f2");
            copy = f;
            THEN("The copy should be equal to the original")
            {
                REQUIRE(copy.valid());
                REQUIRE(f.valid());
                REQUIRE(copy.toString() == f.toString());
                REQUIRE(copy.getFileName() == f.getFileName());
            }
        }
        WHEN("Calling the move constructor")
        {
            File copy(f);
            File move_copy(std::move(f));
            THEN("The original should be invalid")
            {
                REQUIRE_FALSE(f.valid());
            }
            THEN("The copy should be equal to the original")
            {
                REQUIRE(copy.valid());
                REQUIRE(move_copy.valid());
                REQUIRE(copy.toString() == move_copy.toString());
                REQUIRE(copy.getFileName() == move_copy.getFileName());
            }
        }
        WHEN("Calling the move assignment operator")
        {
            File copy(f);
            File move_copy("f2");
            move_copy = std::move(f);
            THEN("The original should be invalid")
            {
                REQUIRE_FALSE(f.valid());
            }
            THEN("The copy should be equal to the original")
            {
                REQUIRE(copy.valid());
                REQUIRE(move_copy.valid());
                REQUIRE(copy.toString() == move_copy.toString());
                REQUIRE(copy.getFileName() == move_copy.getFileName());
            }
        }
    }
    GIVEN("An invalid file")
    {
        File f("");
        File(std::move(f));
        REQUIRE_FALSE(f.valid());
        WHEN("Calling the copy constructor")
        {
            THEN("An exception should be thrown")
            {
                REQUIRE_THROWS(File(f));
            }
        }
        WHEN("Calling the copy assignment operator")
        {
            THEN("An exception should be thrown")
            {
                REQUIRE_THROWS(File("f2") = f);
            }
        }
        WHEN("Calling the move constructor")
        {
            THEN("An exception should be thrown")
            {
                REQUIRE_THROWS(File(f));
            }
        }
        WHEN("Calling the move assignment operator")
        {
            THEN("An exception should be thrown")
            {
                REQUIRE_THROWS(File("f2") = std::move(f));
            }
        }
    }
}