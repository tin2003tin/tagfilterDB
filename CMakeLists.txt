cmake_minimum_required(VERSION 3.10)

# Set the project name
project(TAGFILTERDB)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Enable testing for this project
enable_testing()

# Find OpenSSL
find_package(OpenSSL REQUIRED)
find_package(PostgreSQL REQUIRED)

# Gather all .cpp files in the util and src directories
file(GLOB UTIL_SOURCES "util/*.cpp")  # Utility source files
file(GLOB SRC_SOURCES "src/*.cpp")    # Main source files

# Gather all test files in the test directory
file(GLOB TEST_SOURCES "test/*.cpp")  # Test source files

# Add an executable for the main program and specify its sources
add_executable(tagfilterdb)

target_sources(tagfilterdb
  PRIVATE
    ${UTIL_SOURCES}
    ${SRC_SOURCES}
)

# Link OpenSSL to the main program
target_link_libraries(tagfilterdb PRIVATE OpenSSL::SSL OpenSSL::Crypto)
target_include_directories(tagfilterdb PRIVATE ${PostgreSQL_INCLUDE_DIRS})
target_link_libraries(tagfilterdb PRIVATE ${PostgreSQL_LIBRARIES})

# Add an executable for the tests and specify its sources
add_executable(runTests)

target_sources(runTests
  PRIVATE
    ${TEST_SOURCES}
    ${UTIL_SOURCES}  # Include utility sources for testing
)

# Add include directories for headers
target_include_directories(tagfilterdb PUBLIC 
    ${PROJECT_SOURCE_DIR}/include       # Default include folder for headers
    ${PROJECT_SOURCE_DIR}               # Include folder for port headers (if needed)
)

# Add include directories for the test executable
target_include_directories(runTests PUBLIC 
    ${PROJECT_SOURCE_DIR}/include       # Include folder for headers
    ${PROJECT_SOURCE_DIR}               # Include folder for port headers (if needed)
)

# Link OpenSSL to the test executable
target_link_libraries(runTests PRIVATE OpenSSL::SSL OpenSSL::Crypto ${CMAKE_THREAD_LIBS_INIT})

# Register the test executable with CTest
add_test(NAME RunAllTests COMMAND runTests)