add_subdirectory(googletest)
add_library(GTEST INTERFACE)
target_link_libraries(GTEST INTERFACE GTest::gtest)