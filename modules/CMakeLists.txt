file(MAKE_DIRECTORY bin)

add_library(stdlib SHARED stdlib.cpp)
target_link_libraries(stdlib VWALanguageLib)

add_custom_command(TARGET stdlib
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:stdlib> ${CMAKE_CURRENT_SOURCE_DIR}/bin/stdlib.native)
