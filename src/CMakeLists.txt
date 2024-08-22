add_executable(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_sources(${PROJECT_NAME}
    PRIVATE
        main.cpp
        status.cpp
)