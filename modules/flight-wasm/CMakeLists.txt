# =============================================================================
# Flight WASM Foundation - Mission Critical Build System
# WebAssembly Core Specification 1.0 Compliant Header-Only Library
# =============================================================================

cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

# Project definition with comprehensive metadata
project(flight-wasm
    VERSION 1.0.0
    LANGUAGES CXX
    DESCRIPTION "Flight WASM Foundation - WebAssembly Types and Utilities"
    HOMEPAGE_URL "https://github.com/flight-lang/flight"
)

# =============================================================================
# Core Build Configuration
# =============================================================================

# C++17 is non-negotiable for WebAssembly specification compliance
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)  # Pure C++17, no compiler extensions

# Include custom CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

# Load platform detection and compiler optimization modules
include(PlatformDetection)
include(CompilerOptimizations)
include(FlightWasmConfig)

# =============================================================================
# Platform Detection and Configuration
# =============================================================================

# Detect target platform early for cross-compilation setup
flight_wasm_detect_platform()

# Set platform-specific configurations
if(FLIGHT_WASM_PLATFORM_EMSCRIPTEN)
    message(STATUS "Configuring for Emscripten/WebAssembly target")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s WASM=1 -s ALLOW_MEMORY_GROWTH=1")
elseif(FLIGHT_WASM_PLATFORM_DREAMCAST)
    message(STATUS "Configuring for Dreamcast SH-4 target")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ml -m4-single-only")
elseif(FLIGHT_WASM_PLATFORM_PSP)
    message(STATUS "Configuring for PSP MIPS target")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -G0 -mno-gpopt")
elseif(FLIGHT_WASM_PLATFORM_PSVITA)
    message(STATUS "Configuring for PS Vita ARM target")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpu=neon")
else()
    message(STATUS "Configuring for host platform: ${CMAKE_SYSTEM_NAME}")
endif()

# =============================================================================
# Header-Only Interface Library Definition
# =============================================================================

# Create pure header-only interface library (zero compilation)
add_library(flight-wasm INTERFACE)

# Add alias for consistent naming across Flight ecosystem
add_library(flight::wasm ALIAS flight-wasm)

# Configure include directories for build and install interfaces
target_include_directories(flight-wasm INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# =============================================================================
# C++17 Feature Requirements and Compiler Compatibility
# =============================================================================

# Ensure essential C++17 features are available
target_compile_features(flight-wasm INTERFACE
    cxx_std_17
    cxx_constexpr
    cxx_variadic_templates
    cxx_auto_type
    cxx_decltype_auto
    cxx_lambda_init_captures
)

# WebAssembly specification compliance definitions
target_compile_definitions(flight-wasm INTERFACE
    FLIGHT_WASM_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    FLIGHT_WASM_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    FLIGHT_WASM_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    FLIGHT_WASM_SPEC_VERSION=1  # WebAssembly Core Specification 1.0
)

# Platform-specific compile definitions
if(FLIGHT_WASM_PLATFORM_DREAMCAST)
    target_compile_definitions(flight-wasm INTERFACE
        __DREAMCAST__=1
        FLIGHT_WASM_BIG_ENDIAN=1
        FLIGHT_WASM_EMBEDDED=1
        FLIGHT_WASM_PLATFORM_DREAMCAST=1
    )
elseif(FLIGHT_WASM_PLATFORM_PSP)
    target_compile_definitions(flight-wasm INTERFACE
        __PSP__=1
        FLIGHT_WASM_LITTLE_ENDIAN=1
        FLIGHT_WASM_EMBEDDED=1
        FLIGHT_WASM_PLATFORM_PSP=1
    )
elseif(FLIGHT_WASM_PLATFORM_PSVITA)
    target_compile_definitions(flight-wasm INTERFACE
        __vita__=1
        FLIGHT_WASM_LITTLE_ENDIAN=1
        FLIGHT_WASM_HAS_NEON=1
        FLIGHT_WASM_PLATFORM_VITA=1
    )
elseif(FLIGHT_WASM_PLATFORM_EMSCRIPTEN)
    target_compile_definitions(flight-wasm INTERFACE
        __EMSCRIPTEN__=1
        FLIGHT_WASM_LITTLE_ENDIAN=1
        FLIGHT_WASM_WEBASSEMBLY_TARGET=1
        FLIGHT_WASM_PLATFORM_EMSCRIPTEN=1
    )
else()
    target_compile_definitions(flight-wasm INTERFACE
        FLIGHT_WASM_DESKTOP=1
    )
    
    # Detect endianness on desktop platforms
    include(TestBigEndian)
    test_big_endian(IS_BIG_ENDIAN)
    if(IS_BIG_ENDIAN)
        target_compile_definitions(flight-wasm INTERFACE FLIGHT_WASM_BIG_ENDIAN=1)
    else()
        target_compile_definitions(flight-wasm INTERFACE FLIGHT_WASM_LITTLE_ENDIAN=1)
    endif()
endif()

# Platform-specific compiler optimizations
if(FLIGHT_WASM_PLATFORM_EMBEDDED)
    target_compile_options(flight-wasm INTERFACE
        $<$<CONFIG:Release>:-Os>          # Optimize for size on embedded
        $<$<CONFIG:Release>:-ffunction-sections>
        $<$<CONFIG:Release>:-fdata-sections>
        $<$<CONFIG:Release>:-fno-rtti>    # Disable RTTI for size
        $<$<CONFIG:Release>:-fno-exceptions> # Disable exceptions for size
    )
else()
    target_compile_options(flight-wasm INTERFACE
        $<$<CONFIG:Release>:-O3>          # Optimize for speed on desktop
        $<$<CONFIG:Release>:-DNDEBUG>
    )
endif()

# Platform-specific linker optimizations for embedded targets
if(FLIGHT_WASM_PLATFORM_EMBEDDED)
    target_link_options(flight-wasm INTERFACE
        $<$<CONFIG:Release>:-Wl,--gc-sections>  # Remove unused sections
        $<$<CONFIG:Release>:-Wl,--strip-all>    # Strip symbols
    )
endif()

# =============================================================================
# Compiler-Specific Optimizations
# =============================================================================

# Apply compiler-specific optimizations
flight_wasm_apply_compiler_optimizations(flight-wasm)

# Performance optimization definitions based on build type
target_compile_definitions(flight-wasm INTERFACE
    $<$<CONFIG:Release>:FLIGHT_WASM_INLINE=inline>
    $<$<CONFIG:Release>:FLIGHT_WASM_FORCE_INLINE=__forceinline>
    $<$<CONFIG:Debug>:FLIGHT_WASM_INLINE=>
    $<$<CONFIG:Debug>:FLIGHT_WASM_FORCE_INLINE=>
    $<$<CONFIG:Debug>:FLIGHT_WASM_DEBUG=1>
)

# =============================================================================
# Testing Integration
# =============================================================================

# Enable testing if this is the main project or explicitly requested
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
    enable_testing()
endif()

# Optional testing override
option(FLIGHT_WASM_BUILD_TESTS "Build Flight WASM test suite" ${BUILD_TESTING})
if(FLIGHT_WASM_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    add_subdirectory(tests)
endif()

# =============================================================================
# Benchmarking Integration
# =============================================================================

# Optional benchmarking support
option(FLIGHT_WASM_BUILD_BENCHMARKS "Build performance benchmarks" OFF)
if(FLIGHT_WASM_BUILD_BENCHMARKS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks")
    add_subdirectory(benchmarks)
endif()

# =============================================================================
# Installation and Export Configuration
# =============================================================================

# Include GNUInstallDirs for standard installation directories
include(GNUInstallDirs)

# Install the interface library target
install(TARGETS flight-wasm
    EXPORT flight-wasm-targets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install header files
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)

# Install export targets
install(EXPORT flight-wasm-targets
    FILE flight-wasm-targets.cmake
    NAMESPACE flight::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/flight-wasm
)

# =============================================================================
# CMake Package Configuration
# =============================================================================

# Include CMakePackageConfigHelpers for config file generation
include(CMakePackageConfigHelpers)

# Generate package configuration file
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/flight-wasm-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/flight-wasm-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/flight-wasm
)

# Generate package version file
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/flight-wasm-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install package configuration files
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/flight-wasm-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/flight-wasm-config-version.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/flight-wasm
)

# =============================================================================
# Development and Validation Support
# =============================================================================

# Add custom target for validation builds
add_custom_target(flight-wasm-validate
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target all
    COMMENT "Validating Flight WASM build system"
    VERBATIM
)

# Print configuration summary
message(STATUS "=== Flight WASM Configuration Summary ===")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Target Platform: ${FLIGHT_WASM_TARGET_PLATFORM}")
message(STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Testing Enabled: ${FLIGHT_WASM_BUILD_TESTS}")
message(STATUS "Benchmarks Enabled: ${FLIGHT_WASM_BUILD_BENCHMARKS}")
message(STATUS "=========================================")

# =============================================================================
# Embedded Platform Validation
# =============================================================================

# Validate embedded platform requirements
if(FLIGHT_WASM_PLATFORM_EMBEDDED)
    # Ensure critical compiler flags are set for embedded targets
    if(NOT CMAKE_CXX_FLAGS_RELEASE MATCHES "-O")
        message(WARNING "Release optimization flags not set for embedded target")
    endif()
    
    # Warn about potential issues
    if(BUILD_SHARED_LIBS)
        message(WARNING "Shared libraries may not be suitable for embedded targets")
    endif()
endif()

# Final validation - ensure we have a header-only interface library
get_target_property(FLIGHT_WASM_TYPE flight-wasm TYPE)
if(NOT FLIGHT_WASM_TYPE STREQUAL "INTERFACE_LIBRARY")
    message(FATAL_ERROR "flight-wasm must be an INTERFACE library for header-only design")
endif()

message(STATUS "Flight WASM foundation configured successfully - Ready for WebAssembly excellence! 🚀")
