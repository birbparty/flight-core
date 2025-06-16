# =============================================================================
# Flight WASM Foundation - Performance Benchmark Framework
# Mission Critical Performance Measurement System
# =============================================================================

cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

# =============================================================================
# Google Benchmark Integration
# =============================================================================

# Try to find Google Benchmark first
find_package(benchmark QUIET)

if(NOT benchmark_FOUND)
    message(STATUS "Google Benchmark not found, fetching from GitHub...")
    
    # Download and build Google Benchmark
    include(FetchContent)
    FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.8.3
        GIT_SHALLOW TRUE
    )
    
    # Configure benchmark to avoid building tests and examples
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Enable testing of the benchmark library." FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "Enable installation of benchmark." FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "Enable building the unit tests which depend on gtest" FORCE)
    set(BENCHMARK_ENABLE_ASSEMBLY_TESTS OFF CACHE BOOL "Enable building the assembly tests" FORCE)
    
    FetchContent_MakeAvailable(benchmark)
    
    message(STATUS "Google Benchmark successfully integrated")
endif()

# =============================================================================
# Platform-Specific Performance Configuration
# =============================================================================

# Define platform-specific benchmark parameters
if(FLIGHT_WASM_PLATFORM_DREAMCAST)
    set(BENCHMARK_DEFAULT_ITERATIONS 1000)
    set(ENABLE_DETAILED_PROFILING OFF)
    message(STATUS "Configuring benchmarks for Dreamcast SH-4 @ 200MHz")
elseif(FLIGHT_WASM_PLATFORM_PSP)
    set(BENCHMARK_DEFAULT_ITERATIONS 5000)
    set(ENABLE_DETAILED_PROFILING OFF)
    message(STATUS "Configuring benchmarks for PSP MIPS @ 333MHz")
elseif(FLIGHT_WASM_PLATFORM_PSVITA)
    set(BENCHMARK_DEFAULT_ITERATIONS 10000)
    set(ENABLE_DETAILED_PROFILING ON)
    message(STATUS "Configuring benchmarks for PS Vita ARM Cortex-A9 @ 444MHz")
else()
    set(BENCHMARK_DEFAULT_ITERATIONS 100000)
    set(ENABLE_DETAILED_PROFILING ON)
    message(STATUS "Configuring benchmarks for desktop/server hardware")
endif()

# =============================================================================
# Comprehensive Benchmark Executable
# =============================================================================

add_executable(flight-wasm-benchmarks
    benchmark_main.cpp
    types/benchmark_values.cpp
    binary/benchmark_parser.cpp
    utilities/benchmark_error.cpp
    utilities/benchmark_platform.cpp
    performance/benchmark_regression.cpp
    performance/benchmark_memory.cpp
)

# =============================================================================
# Compiler Optimizations for Accurate Benchmarking
# =============================================================================

# Essential optimization flags for accurate performance measurement
target_compile_options(flight-wasm-benchmarks PRIVATE
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Release>:-DNDEBUG>
    $<$<CONFIG:Release>:-ffast-math>
    $<$<CONFIG:Release>:-funroll-loops>
)

# Platform-specific optimizations
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(flight-wasm-benchmarks PRIVATE
        $<$<CONFIG:Release>:-march=native>
        $<$<CONFIG:Release>:-mtune=native>
        $<$<CONFIG:Release>:-flto>
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(flight-wasm-benchmarks PRIVATE
        $<$<CONFIG:Release>:-march=native>
        $<$<CONFIG:Release>:-flto>
        $<$<CONFIG:Release>:-fvectorize>
        $<$<CONFIG:Release>:-fslp-vectorize>
    )
endif()

# =============================================================================
# Link Dependencies
# =============================================================================

target_link_libraries(flight-wasm-benchmarks PRIVATE
    flight-wasm
    benchmark::benchmark
    benchmark::benchmark_main
)

# =============================================================================
# Include Directories
# =============================================================================

target_include_directories(flight-wasm-benchmarks PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# =============================================================================
# Performance Target Definitions
# =============================================================================

target_compile_definitions(flight-wasm-benchmarks PRIVATE
    BENCHMARK_DEFAULT_ITERATIONS=${BENCHMARK_DEFAULT_ITERATIONS}
    $<$<BOOL:${ENABLE_DETAILED_PROFILING}>:ENABLE_DETAILED_PROFILING=1>
    FLIGHT_WASM_PERFORMANCE_TARGETS=1
)

# =============================================================================
# Benchmark Execution Targets
# =============================================================================

# Basic benchmark execution
add_custom_target(run-benchmarks
    COMMAND flight-wasm-benchmarks
    DEPENDS flight-wasm-benchmarks
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running Flight WASM performance benchmarks"
)

# Detailed benchmark with JSON output
add_custom_target(run-benchmarks-detailed
    COMMAND flight-wasm-benchmarks
        --benchmark_format=json
        --benchmark_out=benchmark_results.json
        --benchmark_repetitions=10
        --benchmark_display_aggregates_only=true
        --benchmark_counters_tabular=true
    DEPENDS flight-wasm-benchmarks
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running detailed Flight WASM benchmarks with JSON output"
)

# Performance regression check
add_custom_target(check-performance-regression
    COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/scripts
        python3 ${CMAKE_CURRENT_SOURCE_DIR}/scripts/validate_performance.py benchmark_results.json
    DEPENDS run-benchmarks-detailed
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Checking for performance regressions"
)

# =============================================================================
# Benchmark Fixtures Setup
# =============================================================================

# Create benchmark fixtures directory structure
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/fixtures/small_modules)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/fixtures/large_modules)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/fixtures/real_world)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/baselines)

# =============================================================================
# Performance Validation
# =============================================================================

message(STATUS "=== Flight WASM Benchmark Configuration ===")
message(STATUS "Google Benchmark: ${benchmark_FOUND}")
message(STATUS "Target Platform: ${FLIGHT_WASM_TARGET_PLATFORM}")
message(STATUS "Default Iterations: ${BENCHMARK_DEFAULT_ITERATIONS}")
message(STATUS "Detailed Profiling: ${ENABLE_DETAILED_PROFILING}")
message(STATUS "Compiler Optimizations: Enabled for Release builds")
message(STATUS "==========================================")
