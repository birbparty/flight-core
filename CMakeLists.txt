cmake_minimum_required(VERSION 3.14)

# Project definition
project(flight
    VERSION 0.1.0
    DESCRIPTION "High-performance, cross-platform WebAssembly Component Model runtime"
    LANGUAGES CXX
)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include CPM package manager
include(cmake/modules/CPM.cmake)

# Set CPM cache directory (for offline builds on embedded platforms)
set(CPM_SOURCE_CACHE "${CMAKE_CURRENT_SOURCE_DIR}/.cpm_cache")

# Project-wide compile options
add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)

# Platform-specific configurations
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # macOS specific settings
    add_definitions(-DPLATFORM_MACOS)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    # WebAssembly specific settings
    add_definitions(-DPLATFORM_EMSCRIPTEN)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Generic" AND CMAKE_SYSTEM_PROCESSOR STREQUAL "sh4")
    # Dreamcast specific settings
    add_definitions(-DPLATFORM_DREAMCAST)
elseif(CMAKE_SYSTEM_NAME STREQUAL "PSP")
    # PSP specific settings
    add_definitions(-DPLATFORM_PSP)
endif()

# Global dependencies (used by multiple modules)
CPMAddPackage(
    NAME Catch2
    GITHUB_REPOSITORY catchorg/Catch2
    VERSION 3.5.0
    OPTIONS
        "CATCH_INSTALL_DOCS OFF"
        "CATCH_INSTALL_EXTRAS OFF"
)

CPMAddPackage(
    NAME benchmark
    GITHUB_REPOSITORY google/benchmark
    VERSION 1.8.3
    OPTIONS
        "BENCHMARK_ENABLE_TESTING OFF"
        "BENCHMARK_ENABLE_INSTALL OFF"
)

# Build options
option(FLIGHT_BUILD_TESTS "Build tests" ON)
option(FLIGHT_BUILD_BENCHMARKS "Build benchmarks" ON)
option(FLIGHT_BUILD_EXAMPLES "Build examples" ON)
option(FLIGHT_BUILD_DOCS "Build documentation" OFF)

# Add modules
add_subdirectory(modules/flight-wasm)
add_subdirectory(modules/flight-runtime)
add_subdirectory(modules/flight-component)
add_subdirectory(modules/flight-memory)
add_subdirectory(modules/flight-hal)
add_subdirectory(modules/flight-telemetry)

# Add other directories if enabled
if(FLIGHT_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(FLIGHT_BUILD_DOCS)
    add_subdirectory(docs)
endif()

# Export compile commands for better IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Install rules
include(GNUInstallDirs)
install(DIRECTORY modules/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)
