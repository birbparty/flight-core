# Flight-Core Integration Validation Tests
cmake_minimum_required(VERSION 3.17)

project(FlightCoreIntegrationTests)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required packages
find_package(GTest REQUIRED)
find_package(benchmark REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bindings/cpp17/flight
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bindings/cpp17/memory-types
    ${GTEST_INCLUDE_DIRS}
)

# Link with Flight Shared Types library
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../bindings/cpp17)

# Common compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -flto")

# Platform-specific configurations
if(FLIGHT_TARGET_PLATFORM STREQUAL "dreamcast")
    add_compile_definitions(
        FLIGHT_PLATFORM_DREAMCAST=1
        FLIGHT_MEMORY_CONSTRAINED=1
        FLIGHT_MAX_MEMORY_MB=16
    )
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Os")
elseif(FLIGHT_TARGET_PLATFORM STREQUAL "psp")
    add_compile_definitions(
        FLIGHT_PLATFORM_PSP=1
        FLIGHT_MEMORY_CONSTRAINED=1
        FLIGHT_MAX_MEMORY_MB=32
    )
elseif(FLIGHT_TARGET_PLATFORM STREQUAL "v6r")
    add_compile_definitions(
        FLIGHT_PLATFORM_V6R=1
        FLIGHT_CLOUD_PLATFORM=1
    )
else()
    add_compile_definitions(FLIGHT_PLATFORM_DESKTOP=1)
endif()

# HAL Integration Tests
add_executable(hal_integration_tests
    hal-integration/hal-platform-integration.test.cpp
    hal-integration/component-model-integration.test.cpp
)

target_link_libraries(hal_integration_tests
    flight_shared_types_cpp17
    ${GTEST_LIBRARIES}
    ${GTEST_MAIN_LIBRARIES}
    pthread
)

# Memory Integration Tests
add_executable(memory_integration_tests
    memory-integration/memory-subsystem-integration.test.cpp
)

target_link_libraries(memory_integration_tests
    flight_shared_types_cpp17
    ${GTEST_LIBRARIES}
    ${GTEST_MAIN_LIBRARIES}
    pthread
)

# Performance Validation Tests
add_executable(performance_validation_tests
    performance-validation/cross-platform-performance.test.cpp
)

target_link_libraries(performance_validation_tests
    flight_shared_types_cpp17
    ${GTEST_LIBRARIES}
    ${GTEST_MAIN_LIBRARIES}
    benchmark::benchmark
    pthread
)

# Production Integration Tests
add_executable(production_integration_tests
    production-validation/production-integration.test.cpp
)

target_link_libraries(production_integration_tests
    flight_shared_types_cpp17
    ${GTEST_LIBRARIES}
    ${GTEST_MAIN_LIBRARIES}
    pthread
)

# Combined test runner
add_executable(flight_validation_suite
    hal-integration/hal-platform-integration.test.cpp
    hal-integration/component-model-integration.test.cpp
    memory-integration/memory-subsystem-integration.test.cpp
    performance-validation/cross-platform-performance.test.cpp
    production-validation/production-integration.test.cpp
)

target_link_libraries(flight_validation_suite
    flight_shared_types_cpp17
    ${GTEST_LIBRARIES}
    ${GTEST_MAIN_LIBRARIES}
    benchmark::benchmark
    pthread
)

# Test targets
add_custom_target(test_hal_integration
    COMMAND hal_integration_tests --gtest_output=xml:hal_integration_results.xml
    DEPENDS hal_integration_tests
    COMMENT "Running HAL integration tests"
)

add_custom_target(test_memory_integration
    COMMAND memory_integration_tests --gtest_output=xml:memory_integration_results.xml
    DEPENDS memory_integration_tests
    COMMENT "Running memory integration tests"
)

add_custom_target(test_performance_validation
    COMMAND performance_validation_tests --gtest_output=xml:performance_validation_results.xml
    DEPENDS performance_validation_tests
    COMMENT "Running performance validation tests"
)

add_custom_target(test_production_integration
    COMMAND production_integration_tests --gtest_output=xml:production_integration_results.xml
    DEPENDS production_integration_tests
    COMMENT "Running production integration tests"
)

add_custom_target(test_flight_validation
    COMMAND flight_validation_suite --gtest_output=xml:flight_validation_results.xml
    DEPENDS flight_validation_suite
    COMMENT "Running complete Flight-Core validation suite"
)

# Install targets
install(TARGETS 
    hal_integration_tests
    memory_integration_tests
    performance_validation_tests
    production_integration_tests
    flight_validation_suite
    DESTINATION bin
)

# CTest integration
enable_testing()

add_test(NAME HALIntegration COMMAND hal_integration_tests)
add_test(NAME MemoryIntegration COMMAND memory_integration_tests)
add_test(NAME PerformanceValidation COMMAND performance_validation_tests)
add_test(NAME ProductionIntegration COMMAND production_integration_tests)
add_test(NAME FlightCoreValidation COMMAND flight_validation_suite)

# Test properties
set_tests_properties(HALIntegration PROPERTIES
    TIMEOUT 300
    LABELS "integration;hal"
)

set_tests_properties(MemoryIntegration PROPERTIES
    TIMEOUT 300
    LABELS "integration;memory"
)

set_tests_properties(PerformanceValidation PROPERTIES
    TIMEOUT 600
    LABELS "performance;validation"
)

set_tests_properties(ProductionIntegration PROPERTIES
    TIMEOUT 900
    LABELS "production;validation"
)

set_tests_properties(FlightCoreValidation PROPERTIES
    TIMEOUT 1800
    LABELS "full;validation"
)

# Platform-specific test configurations
if(FLIGHT_TARGET_PLATFORM STREQUAL "dreamcast")
    set_tests_properties(PerformanceValidation PROPERTIES
        TIMEOUT 1200  # Longer timeout for constrained platform
    )
    set_tests_properties(ProductionIntegration PROPERTIES
        TIMEOUT 1800  # Longer timeout for constrained platform
    )
endif()

message(STATUS "Flight-Core integration tests configured")
message(STATUS "  - Target Platform: ${FLIGHT_TARGET_PLATFORM}")
message(STATUS "  - Test executables: hal_integration_tests, memory_integration_tests, performance_validation_tests, production_integration_tests")
message(STATUS "  - Combined test suite: flight_validation_suite")
