file(
    GLOB
    TEST_SOURCES
    *.cpp
)
add_executable(
    dbtest
    ${TEST_SOURCES}
)
target_include_directories(
    dbtest
    PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(
    dbtest 
    gtest_main
)
add_test(
    NAME dbtest 
    COMMAND dbtest
)