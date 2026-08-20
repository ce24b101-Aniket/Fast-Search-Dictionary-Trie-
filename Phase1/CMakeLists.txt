cmake_minimum_required(VERSION 3.16)
project(FastSearch CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Keep warnings visible but scoped to our own code (added per-target
# below), rather than as a blanket global flag that would also apply to
# fetched third-party dependencies like GoogleTest.
add_library(fastsearch_warnings INTERFACE)
target_compile_options(fastsearch_warnings INTERFACE -Wall -Wextra)

# ---------------------------------------------------------------------
# Core library: the Trie + Normalizer. No HTTP, no SQLite, no frontend
# dependencies here -- this is the piece that must stay independently
# testable and explainable (see docs/architecture.md, Phase 1).
# ---------------------------------------------------------------------
add_library(fastsearch_core
    src/core/trie.cpp
    src/core/normalizer.cpp
)
target_include_directories(fastsearch_core PUBLIC src)
target_link_libraries(fastsearch_core PRIVATE fastsearch_warnings)

# ---------------------------------------------------------------------
# Tests (GoogleTest via FetchContent)
# ---------------------------------------------------------------------
option(FASTSEARCH_BUILD_TESTS "Build the GoogleTest suite" ON)

if(FASTSEARCH_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
    )
    # Keep gtest's own build settings from leaking into ours.
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()

    add_executable(fastsearch_tests
        tests/test_trie.cpp
        tests/test_normalizer.cpp
    )
    target_link_libraries(fastsearch_tests PRIVATE
        fastsearch_core
        fastsearch_warnings
        gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(fastsearch_tests)
endif()
