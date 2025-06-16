# Software-Only HAL Reference Implementation
# Pure C++ implementation with no platform dependencies

cmake_minimum_required(VERSION 3.12)
project(flight_hal_software_only 
    VERSION 1.0.0
    DESCRIPTION "Software-Only HAL Reference Implementation"
    LANGUAGES CXX)

# Configure C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build configuration options
option(SOFTWARE_HAL_BUILD_EXAMPLES "Build software HAL examples" ON)
option(SOFTWARE_HAL_BUILD_TESTS "Build software HAL tests" ON)
option(SOFTWARE_HAL_ENABLE_DEBUG "Enable debug features" ON)
option(SOFTWARE_HAL_ENABLE_VALIDATION "Enable runtime validation" ON)

# Find the main HAL library
find_package(FlightHAL REQUIRED HINTS ../../../)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
)

# Compiler-specific settings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
        -fno-exceptions  # Pure C++ without exceptions for embedded compatibility
    )
    
    if(SOFTWARE_HAL_ENABLE_DEBUG)
        add_compile_options(-g -O0 -DDEBUG)
        add_compile_definitions(SOFTWARE_HAL_DEBUG_ENABLED)
    else()
        add_compile_options(-O2 -DNDEBUG)
    endif()
    
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/W4)
    
    if(SOFTWARE_HAL_ENABLE_DEBUG)
        add_compile_options(/Od /Zi /DDEBUG)
        add_compile_definitions(SOFTWARE_HAL_DEBUG_ENABLED)
    else()
        add_compile_options(/O2 /DNDEBUG)
    endif()
endif()

# Configuration definitions
if(SOFTWARE_HAL_ENABLE_VALIDATION)
    add_compile_definitions(SOFTWARE_HAL_VALIDATION_ENABLED)
endif()

# Create the software HAL drivers library
add_library(flight_hal_software_drivers
    drivers/software_memory_driver.cpp
    # Note: Additional driver implementations would go here:
    # drivers/software_graphics_driver.cpp
    # drivers/software_audio_driver.cpp
    # drivers/software_file_driver.cpp
    # drivers/software_time_driver.cpp
    # drivers/software_thread_driver.cpp
)

# Link against the main HAL library
target_link_libraries(flight_hal_software_drivers
    PRIVATE
        FlightHAL::Core
        FlightHAL::Interfaces
        FlightHAL::Allocators
)

# Platform-specific threading support
find_package(Threads REQUIRED)
target_link_libraries(flight_hal_software_drivers
    PRIVATE
        Threads::Threads
)

# Set target properties
set_target_properties(flight_hal_software_drivers PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
    OUTPUT_NAME "flight_hal_software"
)

# Export target for find_package
add_library(FlightHAL::Software ALIAS flight_hal_software_drivers)

# Build examples if requested
if(SOFTWARE_HAL_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Build tests if requested
if(SOFTWARE_HAL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation configuration
include(GNUInstallDirs)

install(TARGETS flight_hal_software_drivers
    EXPORT FlightHALSoftwareTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY drivers/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/flight/hal/software/drivers
    FILES_MATCHING PATTERN "*.hpp"
)

install(EXPORT FlightHALSoftwareTargets
    FILE FlightHALSoftwareTargets.cmake
    NAMESPACE FlightHAL::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/FlightHAL
)

# Create package config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    FlightHALSoftwareConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FlightHALSoftwareConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/FlightHALSoftwareConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/FlightHAL
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/FlightHALSoftwareConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/FlightHALSoftwareConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/FlightHAL
)

# Print configuration summary
message(STATUS "Software HAL Configuration:")
message(STATUS "  Build Examples: ${SOFTWARE_HAL_BUILD_EXAMPLES}")
message(STATUS "  Build Tests: ${SOFTWARE_HAL_BUILD_TESTS}")
message(STATUS "  Debug Features: ${SOFTWARE_HAL_ENABLE_DEBUG}")
message(STATUS "  Runtime Validation: ${SOFTWARE_HAL_ENABLE_VALIDATION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
