# Minimum required CMake version
cmake_minimum_required(VERSION 3.5.0)

# Project name, version, and supported languages
project(TAGFILTERDB VERSION 0.1.0 LANGUAGES C CXX)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)

# Glob all source and header files
file(GLOB all_SRCS
    "${PROJECT_SOURCE_DIR}/include/*.h"
    "${PROJECT_SOURCE_DIR}/include/*.hpp"
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/*.c"
)

# Set the output directory for executables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# Add the 'thirdparty' subdirectory (used for third-party dependencies like ANTLR4)
add_subdirectory(thirdparty)

# Add the 'src' subdirectory (where your source files reside)
add_subdirectory(src)

# Enable testing for the project
enable_testing()

# Add the 'tests' subdirectory (for test cases)
add_subdirectory(tests)