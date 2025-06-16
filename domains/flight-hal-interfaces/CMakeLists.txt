cmake_minimum_required(VERSION 3.20)

# Project definition
project(
    flight-hal-interfaces
    VERSION 1.0.0
    DESCRIPTION "Flight HAL Interface Definitions for Cross-Platform Hardware Abstraction"
    LANGUAGES CXX
)

# Prevent in-source builds
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR 
        "In-source builds are not allowed. Please create a separate build directory.")
endif()

# Set project-wide C++ standard for Dreamcast compatibility
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE integration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to Release build if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS 
        "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Build options following modern CMake patterns
option(FLIGHT_HAL_BUILD_TESTS "Build test suite" ON)
option(FLIGHT_HAL_BUILD_BENCHMARKS "Build benchmarks" ON)
option(FLIGHT_HAL_BUILD_EXAMPLES "Build examples" ON)
option(FLIGHT_HAL_BUILD_DOCS "Build documentation" OFF)
option(FLIGHT_HAL_ENABLE_COVERAGE "Enable coverage reporting" OFF)
option(FLIGHT_HAL_ENABLE_SANITIZERS "Enable sanitizers in Debug builds" ON)
option(FLIGHT_HAL_ENABLE_WARNINGS "Enable compiler warnings" ON)
option(FLIGHT_HAL_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(FLIGHT_HAL_ENABLE_CLANG_TIDY "Enable clang-tidy checks" OFF)
option(FLIGHT_HAL_ENABLE_CPPCHECK "Enable cppcheck static analysis" OFF)
option(FLIGHT_HAL_ENABLE_IWYU "Enable include-what-you-use" OFF)
option(FLIGHT_HAL_USE_SYSTEM_DEPS "Use system-installed dependencies instead of FetchContent" OFF)

# Set module path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Include helper modules
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CompilerWarnings)
include(Sanitizers)
include(StaticAnalysis)
include(Dependencies)
include(TestingConfig)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Create main library target
add_library(flight-hal-interfaces)
add_library(Flight::HAL::Interfaces ALIAS flight-hal-interfaces)

# Set library properties
target_include_directories(flight-hal-interfaces
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Add source files
target_sources(flight-hal-interfaces
    PRIVATE
        src/core/driver_registry.cpp
        src/core/platform_detection.cpp
        src/core/error_handling.cpp
        src/core/resource_manager.cpp
        src/core/event_system.cpp
        src/core/hal_capabilities.cpp
        src/core/platform_capabilities.cpp
        src/coordination/resource_handle.cpp
        src/coordination/cross_driver_messenger.cpp
        src/coordination/deadlock_prevention.cpp
        src/platform/capabilities.cpp
        src/platform/configuration.cpp
        src/platform/integration.cpp
        src/validation/compliance_checker.cpp
        src/validation/mock_implementations.cpp
        src/validation/test_harness.cpp
)

# Apply modern compiler settings and warnings
flight_hal_configure_compiler_settings(flight-hal-interfaces)
if(FLIGHT_HAL_ENABLE_WARNINGS)
    set_project_warnings(flight-hal-interfaces)
endif()

# Apply sanitizers if requested
flight_hal_enable_sanitizers_for_target(flight-hal-interfaces)

# Apply static analysis if requested
flight_hal_enable_static_analysis(flight-hal-interfaces)

# Link common dependencies (fmt, spdlog)
flight_hal_link_common_deps(flight-hal-interfaces)

# Add subdirectories
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    enable_testing()
    
    if(FLIGHT_HAL_BUILD_TESTS)
        add_subdirectory(tests)
    endif()
    
    if(FLIGHT_HAL_BUILD_BENCHMARKS)
        add_subdirectory(benchmarks)
    endif()
    
    if(FLIGHT_HAL_BUILD_EXAMPLES)
        add_subdirectory(examples)
    endif()
    
    add_subdirectory(tools)
endif()

# Installation configuration (temporarily disabled due to FetchContent export issues)
# TODO: Re-enable once we have a proper solution for exporting FetchContent dependencies
# This follows the pattern from buck2-cpp-cpm reference implementation
if(FLIGHT_HAL_USE_SYSTEM_DEPS)
    include(GNUInstallDirs)

    install(TARGETS flight-hal-interfaces
        EXPORT flight-hal-interfaces-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.hpp"
    )

    # Export targets
    install(EXPORT flight-hal-interfaces-targets
        FILE flight-hal-interfaces-targets.cmake
        NAMESPACE Flight::HAL::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/flight-hal-interfaces
    )

    # Create config file
    configure_package_config_file(
        cmake/FlightHALConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/flight-hal-interfaces-config.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/flight-hal-interfaces
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/flight-hal-interfaces-config-version.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/flight-hal-interfaces-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/flight-hal-interfaces-config-version.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/flight-hal-interfaces
    )
else()
    message(STATUS "Installation disabled when using FetchContent dependencies")
    message(STATUS "Use -DFLIGHT_HAL_USE_SYSTEM_DEPS=ON to enable installation")
endif()

# CPack configuration
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_VENDOR "Flight HAL Contributors")
set(CPACK_PACKAGE_CONTACT "https://github.com/punk1290/flight-hal-interfaces")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

# Generator-specific settings
set(CPACK_GENERATOR "TGZ;ZIP")
if(UNIX AND NOT APPLE)
    list(APPEND CPACK_GENERATOR "DEB" "RPM")
endif()

include(CPack)

# Print configuration summary
message(STATUS "")
message(STATUS "Flight HAL Interfaces ${PROJECT_VERSION} Configuration Summary:")
message(STATUS "  Build type:             ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard:           ${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler:               ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "")
message(STATUS "  Build tests:            ${FLIGHT_HAL_BUILD_TESTS}")
message(STATUS "  Build benchmarks:       ${FLIGHT_HAL_BUILD_BENCHMARKS}")
message(STATUS "  Build examples:         ${FLIGHT_HAL_BUILD_EXAMPLES}")
message(STATUS "  Build docs:             ${FLIGHT_HAL_BUILD_DOCS}")
message(STATUS "  Enable coverage:        ${FLIGHT_HAL_ENABLE_COVERAGE}")
message(STATUS "  Enable sanitizers:      ${FLIGHT_HAL_ENABLE_SANITIZERS}")
message(STATUS "  Enable warnings:        ${FLIGHT_HAL_ENABLE_WARNINGS}")
message(STATUS "  Warnings as errors:     ${FLIGHT_HAL_WARNINGS_AS_ERRORS}")
message(STATUS "  Enable clang-tidy:      ${FLIGHT_HAL_ENABLE_CLANG_TIDY}")
message(STATUS "  Enable cppcheck:        ${FLIGHT_HAL_ENABLE_CPPCHECK}")
message(STATUS "  Enable IWYU:            ${FLIGHT_HAL_ENABLE_IWYU}")
message(STATUS "")
message(STATUS "  Install prefix:         ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")

# Print detailed configuration from modules
print_dependency_info()
print_compiler_config()
print_sanitizer_config()
print_static_analysis_config()
print_testing_config()
