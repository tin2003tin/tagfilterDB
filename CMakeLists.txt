cmake_minimum_required(VERSION 3.5.0)
project(TAGFILTERDB VERSION 0.1.0 LANGUAGES C CXX)


file(GLOB all_SRCS
        "${PROJECT_SOURCE_DIR}/include/*.h"
        "${PROJECT_SOURCE_DIR}/include/*.hpp"
        "${PROJECT_SOURCE_DIR}/src/*.cpp"
        "${PROJECT_SOURCE_DIR}/src/*.c"
        )
        
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(CXX_STANDARD 20)

add_subdirectory(libs)
add_subdirectory(src)

enable_testing()
add_subdirectory(tests)
