cmake_minimum_required(VERSION 3.10)

project(MyProject)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 11)

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

# Testing
enable_testing()
foreach(test_file ${TAGFILTERDB_TESTS})
    get_filename_component(test_name ${test_file} NAME_WE)  
    add_executable(${test_name}_test ${test_file})          
    target_link_libraries(${test_name}_test tagfilterdb)   
    add_test(NAME ${test_name} COMMAND ${test_name}_test)    
endforeach()

