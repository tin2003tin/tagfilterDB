cmake_minimum_required(VERSION 3.11)

# Prevent CMake from trying to compile executables during checks
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

# Default to Debug build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Enable debug symbols for g++
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_C_FLAGS_DEBUG "-g -O0")

project(MyProject)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)

# Define the tagfilterdb library
add_library(tagfilterdb "")
target_include_directories(tagfilterdb PUBLIC ${PROJECT_SOURCE_DIR}/include/)
file(GLOB TAGFILTERDB_HEADERS "include/tagfilterdb/*.h")
file(GLOB TAGFILTERDB_SOURCES "tagfilterdb/*.cpp")
file(GLOB TAGFILTERDB_TESTS "tagfilterdb/*_test.cpp")
target_sources(tagfilterdb
  PRIVATE
    ${TAGFILTERDB_SOURCES}          
    ${TAGFILTERDB_HEADERS}           
)


# Main
file(GLOB SRC_SOURCES "src/*.cpp")
add_executable(MyProject ${SRC_SOURCES})
target_link_libraries(MyProject tagfilterdb)


# Add the benchmark tests
file(GLOB BENCHMARK_SOURCES "benchmark/*.cpp")
foreach(benchmark_file ${BENCHMARK_SOURCES})
    get_filename_component(benchmark_name ${benchmark_file} NAME_WE)
    add_executable(${benchmark_name}_benchmark ${benchmark_file})
    target_link_libraries(${benchmark_name}_benchmark benchmark::benchmark tagfilterdb)
endforeach()

# Testing
enable_testing()
foreach(test_file ${TAGFILTERDB_TESTS})
    get_filename_component(test_name ${test_file} NAME_WE)  
    add_executable(${test_name}_test ${test_file})          
    target_link_libraries(${test_name}_test tagfilterdb)   
    add_test(NAME ${test_name} COMMAND ${test_name}_test)    
endforeach()
