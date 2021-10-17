cmake_minimum_required(VERSION 3.0.0)
project(ProgrammingLanguage VERSION 0.1.0)
enable_testing()

# set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

set(CMAKE_CXX_STANDARD 20)


include_directories(include)

add_compile_options(-Wall -Wno-unknown-pragmas -Wshadow)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_library(VWALanguageLib STATIC ${SOURCES})
add_executable(VWALanguage main.cpp)
target_link_libraries(VWALanguage VWALanguageLib)
add_dependencies(VWALanguage VWALanguageLib)
add_subdirectory(extern/googletest)
add_subdirectory(tests)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

if (CMAKE_GENERATOR MATCHES "Ninja")
    file(
        WRITE "${CMAKE_BINARY_DIR}/GNUMakeRulesOverwrite.cmake" 
            "STRING(REPLACE \"-MD\" \"-MMD\" CMAKE_DEPFILE_FLAGS_C \"\${CMAKE_DEPFILE_FLAGS_C}\")\n"
            "STRING(REPLACE \"-MD\" \"-MMD\" CMAKE_DEPFILE_FLAGS_CXX \"\${CMAKE_DEPFILE_FLAGS_CXX}\")\n"
    )
    set(CMAKE_USER_MAKE_RULES_OVERRIDE "${CMAKE_BINARY_DIR}/GNUMakeRulesOverwrite.cmake" CACHE INTERNAL "")
endif(CMAKE_GENERATOR MATCHES "Ninja")