file(
    GLOB
    TEST_SOURCES
    *.cpp
)
add_executable(
    dbtest
    ${TEST_SOURCES}
)
target_link_libraries(
    dbtest 
    gtest_main
)
add_test(
    NAME dbtest 
    COMMAND dbtest
)