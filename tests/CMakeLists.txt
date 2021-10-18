file(GLOB_RECURSE TEST_SRC "src/*.cpp")
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/${catch2_SOURCE_DIR}/extras)
include(Catch)

foreach(TEST ${TEST_SRC})
    string(REPLACE ".cpp" "" TEST_NAME_DIR ${TEST})
    get_filename_component(TEST_NAME ${TEST_NAME_DIR} NAME)
    add_executable(${TEST_NAME} ${TEST})
    target_link_libraries(${TEST_NAME} Catch2::Catch2WithMain VWALanguageLib)
    target_include_directories(${TEST_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src)
    catch_discover_tests(${TEST_NAME} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/inputs)
endforeach(TEST ${TEST_SRC})

add_subdirectory(other)