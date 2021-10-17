include(GoogleTest)

file(GLOB_RECURSE TEST_SRC "src/*.cpp")
foreach(TEST ${TEST_SRC})
    string(REPLACE ".cpp" "" TEST_NAME_DIR ${TEST})
    get_filename_component(TEST_NAME ${TEST_NAME_DIR} NAME)
    add_executable(${TEST_NAME} ${TEST})
    target_link_libraries(${TEST_NAME} gtest_main VWALanguageLib)
    target_include_directories(${TEST_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src)
    gtest_discover_tests(${TEST_NAME})
endforeach(TEST ${TEST_SRC})

add_subdirectory(special)