file(MAKE_DIRECTORY bin)

add_library(stdlib SHARED stdlib.cpp)
target_link_libraries(stdlib VWALanguageLib)

add_custom_command(TARGET stdlib
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:stdlib> ${CMAKE_CURRENT_SOURCE_DIR}/bin/stdlib.native)

add_library(testlib SHARED testlib.cpp)
target_link_libraries(testlib VWALanguageLib)

add_custom_command(TARGET testlib
                     POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:testlib> ${CMAKE_CURRENT_SOURCE_DIR}/bin/testlib.native)
                        

                   #TODO: prober build system for this