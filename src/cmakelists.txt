add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PRIVATE
        ${all_SRCS}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        GTEST
)