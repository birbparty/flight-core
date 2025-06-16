# Flight HAL macOS Integration - Domain Component
# Integrates with Flight monorepo build system

cmake_minimum_required(VERSION 3.14)
project(flight-hal-macos-integration VERSION 1.0.0 LANGUAGES CXX)

# Domain component - not a standalone project
if(NOT DEFINED FLIGHT_ROOT)
    set(FLIGHT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
endif()

# Modern C++17 with Apple Silicon optimizations
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Project options
option(FLIGHT_HAL_BUILD_TESTS "Build Flight HAL tests" ON)
option(FLIGHT_HAL_BUILD_EXAMPLES "Build Flight HAL examples" ON)

# Apple Silicon optimization detection
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    set(FLIGHT_HAL_APPLE_SILICON ON)
    message(STATUS "Flight HAL: Apple Silicon detected - enabling optimizations")
else()
    set(FLIGHT_HAL_APPLE_SILICON OFF)
endif()

# Main library target
add_library(flight-hal-macos-integration STATIC
    src/platform_coordinator.cpp
    src/platform_detector.cpp
    src/platform_detector.mm
    src/process_manager.cpp
    src/file_system_bridge.cpp
    src/development_tools.cpp
    src/core_foundation_bridge.cpp
    src/foundation_bridge.mm
)

# Library alias for consistent naming
add_library(flight::hal::macos ALIAS flight-hal-macos-integration)

# Public headers
target_include_directories(flight-hal-macos-integration 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Apple Silicon specific optimizations
if(FLIGHT_HAL_APPLE_SILICON)
    target_compile_options(flight-hal-macos-integration PRIVATE
        -mcpu=apple-a14  # Use apple-a14 which is widely supported
        -mtune=native
        -fvectorize
        -O3
        -DNDEBUG
    )
    target_compile_definitions(flight-hal-macos-integration PRIVATE
        FLIGHT_HAL_APPLE_SILICON=1
        FLIGHT_HAL_M4_MAX=1
    )
endif()

# Compiler-specific optimizations
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
    target_compile_options(flight-hal-macos-integration PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
        -ffast-math
        -funroll-loops
    )
endif()

# Platform-specific settings
if(APPLE)
    target_compile_definitions(flight-hal-macos-integration PRIVATE
        FLIGHT_HAL_PLATFORM_MACOS=1
    )
    
    # Link macOS frameworks
    target_link_libraries(flight-hal-macos-integration 
        PUBLIC
        "-framework Foundation"
        "-framework CoreFoundation"
        "-framework AppKit"
        "-framework IOKit"
        "-framework Metal"
        "-framework MetalKit"
    )
endif()

# Tests
if(FLIGHT_HAL_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Examples
if(FLIGHT_HAL_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Installation
install(TARGETS flight-hal-macos-integration
    EXPORT flight-hal-macos-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp"
)

# Export for use by other Flight domains
install(EXPORT flight-hal-macos-targets
    FILE flight-hal-macos-targets.cmake
    NAMESPACE flight::
    DESTINATION lib/cmake/flight-hal-macos
)
