# Flight Core Telemetry Module
cmake_minimum_required(VERSION 3.14)

# Include CPM
include(${CMAKE_SOURCE_DIR}/cmake/modules/CPM.cmake)

# Module library
add_library(flight-telemetry STATIC)

# Source files
target_sources(flight-telemetry
    PRIVATE
        # Add source files here as they are created
        # src/tracer.cpp
        # src/meter.cpp
        # src/exporter.cpp
)

# Include directories
target_include_directories(flight-telemetry
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Compile features
target_compile_features(flight-telemetry PUBLIC cxx_std_17)

# Optional OpenTelemetry dependency (lightweight for embedded)
option(FLIGHT_USE_OPENTELEMETRY "Enable OpenTelemetry support" OFF)

if(FLIGHT_USE_OPENTELEMETRY)
    CPMAddPackage(
        NAME opentelemetry-cpp
        GITHUB_REPOSITORY open-telemetry/opentelemetry-cpp
        VERSION 1.13.0
        OPTIONS
            "WITH_STL OFF"  # Use custom STL for embedded
            "WITH_EXAMPLES OFF"
            "WITH_TESTS OFF"
            "BUILD_TESTING OFF"
    )
    
    target_link_libraries(flight-telemetry
        PUBLIC
            opentelemetry-cpp::api
        PRIVATE
            opentelemetry-cpp::sdk
    )
    target_compile_definitions(flight-telemetry PUBLIC FLIGHT_HAS_OPENTELEMETRY)
endif()

# Link libraries
target_link_libraries(flight-telemetry
    PUBLIC
        flight-hal  # Platform-specific timing
    PRIVATE
        # Add private dependencies
)

# Tests
if(FLIGHT_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Benchmarks
if(FLIGHT_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# Installation
install(TARGETS flight-telemetry
    EXPORT flight-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
