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

project(MyProject VERSION 1.0)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (WIN32)
  set(TAGFILTERDB_PLATFORM_NAME TAGFILTERDB_PLATFORM_WINDOWS)
  add_definitions(-D_UNICODE -DUNICODE)
else (WIN32)
  set(TAGFILTERDB_PLATFORM_NAME TAGFILTERDB_PLATFORM_POSIX)
endif (WIN32)

option(TAGFILTERDB_BUILD_TESTS "Build LevelDB's unit tests" ON)

include(CheckIncludeFile)
check_include_file("unistd.h" HAVE_UNISTD_H)

include(CheckLibraryExists)
check_library_exists(crc32c crc32c_value "" HAVE_CRC32C)
check_library_exists(snappy snappy_compress "" HAVE_SNAPPY)
check_library_exists(zstd zstd_compress "" HAVE_ZSTD)
check_library_exists(tcmalloc malloc "" HAVE_TCMALLOC)

include(CheckCXXSymbolExists)
check_cxx_symbol_exists(fdatasync "unistd.h" HAVE_FDATASYNC)
check_cxx_symbol_exists(F_FULLFSYNC "fcntl.h" HAVE_FULLFSYNC)
check_cxx_symbol_exists(O_CLOEXEC "fcntl.h" HAVE_O_CLOEXEC)

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-Wthread-safety HAVE_CLANG_THREAD_SAFETY)
check_cxx_compiler_flag(-Wno-missing-field-initializers
TAGFILTERDB_HAVE_NO_MISSING_FIELD_INITIALIZERS)

set(TAGFILTERDB_PUBLIC_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include/tagfilterdb")
set(TAGFILTERDB_PORT_CONFIG_DIR "include/port")

configure_file(
  "port/port_config.h.in"
  "${PROJECT_BINARY_DIR}/${TAGFILTERDB_PORT_CONFIG_DIR}/port_config.h"
)

include_directories(
  "${PROJECT_BINARY_DIR}/include"
  "."
)

include(GNUInstallDirs)

add_library(tagfilterdb "")
target_sources(tagfilterdb
    PRIVATE
        "${PROJECT_BINARY_DIR}/${TAGFILTERDB_PORT_CONFIG_DIR}/port_config.h"
        "port/port.h"
        "port/thread_annotations.h"
        "port/port_stdcxx.h"

        "helpers/memenv/memenv.cpp"
        "helpers/memenv/memenv.h"

        "util/status.cpp"
        "util/filter_policy.cpp"
        "util/coding.cpp"
        "util/coding.h"
        "util/hash.cpp"
        "util/hash.h"
        "util/env.cpp"
        "util/env_posix.cpp"
        "util/bloom.cpp"
        "util/logging.cpp"
        "util/logging.h"
        "util/mutexlock.h"
        "util/posix_logger.h"
        "util/testutil.cpp"
        "util/env_posix_test_helper.h"
        "util/arena.h"
        "util/arena.cpp"
        "util/cache.cpp"
        "util/histogram.h"
        "util/histogram.cpp"
        "util/crc32c.h"
        "util/crc32c.cpp"
        "util/no_destructor.h"
        "util/comparator.cpp"

        "db/index/bounding_box.cpp"
        "db/index/bounding_box.h"

    PUBLIC
        "${TAGFILTERDB_PUBLIC_INCLUDE_DIR}/comparator.h"
        "${TAGFILTERDB_PUBLIC_INCLUDE_DIR}/cache.h"
        "${TAGFILTERDB_PUBLIC_INCLUDE_DIR}/env.h"
        "${TAGFILTERDB_PUBLIC_INCLUDE_DIR}/export.h"
        "${TAGFILTERDB_PUBLIC_INCLUDE_DIR}/filter_policy.h"
        "${TAGFILTERDB_PUBLIC_INCLUDE_DIR}/iterator.h"
        "${TAGFILTERDB_PUBLIC_INCLUDE_DIR}/options.h"
        "${TAGFILTERDB_PUBLIC_INCLUDE_DIR}/dataView.h"
        "${TAGFILTERDB_PUBLIC_INCLUDE_DIR}/status.h"
        "${TAGFILTERDB_PUBLIC_INCLUDE_DIR}/table_builder.h"
        "${TAGFILTERDB_PUBLIC_INCLUDE_DIR}/table.h"
        "${TAGFILTERDB_PUBLIC_INCLUDE_DIR}/db.h"
        "${TAGFILTERDB_PUBLIC_INCLUDE_DIR}/write_batch.h"

        "db/index/r_tree.cpp"
        "db/index/r_tree.h"
)

# Set include directories
target_include_directories(tagfilterdb
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_definitions(tagfilterdb
  PRIVATE
    # Used by include/export.h when building shared libraries.
    TAGFILTERDB_COMPILE_LIBRARY
    # Used by port/port.h.
    ${TAGFILTERDB_PLATFORM_NAME}=1
)

file(GLOB SRC_SOURCES "src/main.cpp")

add_executable(MyProject ${SRC_SOURCES})

target_link_libraries(MyProject PRIVATE tagfilterdb)

if (HAVE_SNAPPY)
    target_link_libraries(MyProject PRIVATE snappy)
endif()

if (HAVE_ZSTD)
    target_link_libraries(MyProject PRIVATE zstd)
endif()

set(TAGFILTERDB_BUILD_TESTS TRUE CACHE BOOL "Enable tests" FORCE)
if(TAGFILTERDB_BUILD_TESTS)
  enable_testing()

  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  set(install_gtest OFF)
  set(install_gmock OFF)
  set(build_gmock ON)

  add_subdirectory("third_party/googletest")

  if(TAGFILTERDB_HAVE_NO_MISSING_FIELD_INITIALIZERS)
  set_property(TARGET gtest
      APPEND PROPERTY COMPILE_OPTIONS -Wno-missing-field-initializers)
  set_property(TARGET gmock
      APPEND PROPERTY COMPILE_OPTIONS -Wno-missing-field-initializers)
  endif(TAGFILTERDB_HAVE_NO_MISSING_FIELD_INITIALIZERS)

  add_executable(tagfilterdb_tests "")
  target_sources(tagfilterdb_tests
  PRIVATE
    "util/status_test.cpp"
    "util/coding_test.cpp"
    "util/hash_test.cpp"
    "util/logging_test.cpp"
    "util/arena_test.cpp"
    "util/crc32c_test.cpp"
    "util/no_destructor_test.cpp"
    "util/bloom_test.cpp"
    "util/cache_test.cpp"
    "util/env_test.cpp"
    "util/env_posix_test.cpp"

    "db/index/bounding_box_test.cpp"
    "db/index/r_tree_test.cpp"
  )

  target_link_libraries(tagfilterdb_tests tagfilterdb gmock gtest gtest_main)

  add_test(NAME "tagfilterdb_tests" COMMAND "tagfilterdb_tests")

  function(tagfilterdb_tests test_file)
    get_filename_component(test_target_name "${test_file}" NAME_WE)

    add_executable("${test_target_name}" "")
    target_sources("${test_target_name}"
      PRIVATE
        "${test_file}"
    )
    target_link_libraries("${test_target_name}" tagfilterdb gmock gtest)
    target_compile_definitions("${test_target_name}"
      PRIVATE
        ${TAGFILTERDB_PLATFORM_NAME}=1
    )
    if (NOT HAVE_CXX17_HAS_INCLUDE)
      target_compile_definitions("${test_target_name}"
        PRIVATE
          TAGFILTERDB_HAS_PORT_CONFIG_H=1
      )
    endif(NOT HAVE_CXX17_HAS_INCLUDE)

    add_test(NAME "${test_target_name}" COMMAND "${test_target_name}")
  endfunction(tagfilterdb_tests)

endif(TAGFILTERDB_BUILD_TESTS)
