# Glob all .cpp files in the current directory
file(GLOB TEST_SOURCES "*.cpp")

# Create an executable called dbtest that includes all test sources
add_executable(dbtest ${TEST_SOURCES})

# Specify include directories
target_include_directories(dbtest
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

# Link the dbtest executable to the gtest_main library
target_link_libraries(dbtest 
    PRIVATE
        gtest_main
        # Add any other libraries your tests may depend on here
)

# Register the dbtest executable as a test with CMake
add_test(
    NAME dbtest 
    COMMAND dbtest
)
