file(GLOB_RECURSE TEST_SRC "src/*.cpp")
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/${catch2_SOURCE_DIR}/extras)
include(Catch)

# foreach(TEST ${TEST_SRC})
#     string(REPLACE ".cpp" "" TEST_NAME_DIR ${TEST})
#     get_filename_component(TEST_NAME ${TEST_NAME_DIR} NAME)
#     add_executable(${TEST_NAME} ${TEST})
#     target_link_libraries(${TEST_NAME} Catch2::Catch2WithMain VWALanguageLib)
#     catch_discover_tests(${TEST_NAME} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/inputs)
# endforeach(TEST ${TEST_SRC})

add_executable(test_all ${TEST_SRC})
target_link_libraries(test_all Catch2::Catch2WithMain VWALanguageLib)
catch_discover_tests(test_all WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/inputs)

add_subdirectory(other)