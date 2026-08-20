cmake_minimum_required(VERSION 3.16)
project(FastSearch CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Threads REQUIRED)

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
# Persistence library: SQLite-backed repositories. Deliberately
# separate from fastsearch_core -- the Trie must never depend on this,
# and this must never depend on the Trie. They only meet in the service
# layer (src/service).
# ---------------------------------------------------------------------
find_package(SQLite3 REQUIRED)

add_library(fastsearch_db
    src/db/database_manager.cpp
    src/db/word_repository.cpp
    src/db/search_history_repository.cpp
)
target_include_directories(fastsearch_db PUBLIC src)
target_link_libraries(fastsearch_db PUBLIC SQLite::SQLite3)
target_link_libraries(fastsearch_db PRIVATE fastsearch_warnings)

# ---------------------------------------------------------------------
# Service library: SearchService, the only layer that knows about BOTH
# the Trie and the database. This is what the REST API (Phase 5's
# server) will call into -- it never touches Trie or repositories
# directly.
# ---------------------------------------------------------------------
add_library(fastsearch_service
    src/service/search_service.cpp
)
target_include_directories(fastsearch_service PUBLIC src)
target_link_libraries(fastsearch_service PUBLIC fastsearch_core fastsearch_db)
target_link_libraries(fastsearch_service PRIVATE fastsearch_warnings)

# ---------------------------------------------------------------------
# API library: thin HTTP layer (cpp-httplib + nlohmann/json, vendored as
# single headers under third_party/ -- see docs/architecture.md for why
# Crow was swapped for httplib: fewer transitive dependencies, trivial
# to vendor, no Boost/Asio requirement in this sandboxed build
# environment).
# ---------------------------------------------------------------------
add_library(fastsearch_api
    src/api/server.cpp
)
target_include_directories(fastsearch_api PUBLIC src)
target_include_directories(fastsearch_api SYSTEM PUBLIC third_party)
target_link_libraries(fastsearch_api PUBLIC fastsearch_service Threads::Threads)
target_link_libraries(fastsearch_api PRIVATE fastsearch_warnings)

# ---------------------------------------------------------------------
# Server executable
# ---------------------------------------------------------------------
add_executable(fastsearch_server src/main.cpp)
target_link_libraries(fastsearch_server PRIVATE fastsearch_api fastsearch_warnings)

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
        tests/test_autocomplete.cpp
        tests/test_repositories.cpp
        tests/test_search_service.cpp
    )
    target_link_libraries(fastsearch_tests PRIVATE
        fastsearch_core
        fastsearch_db
        fastsearch_service
        fastsearch_warnings
        gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(fastsearch_tests)
endif()
