# Flight Core Telemetry Module Benchmarks
cmake_minimum_required(VERSION 3.14)

# Benchmark executable
add_executable(flight-telemetry-benchmarks
    # Add benchmark source files here
    # bench_span_creation.cpp
    # bench_metrics_recording.cpp
    # bench_attribute_handling.cpp
)

# Link benchmark dependencies
target_link_libraries(flight-telemetry-benchmarks
    PRIVATE
        flight-telemetry
        benchmark::benchmark
)

# Include directories
target_include_directories(flight-telemetry-benchmarks
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# Register benchmarks
add_custom_target(run-flight-telemetry-benchmarks
    COMMAND flight-telemetry-benchmarks
    DEPENDS flight-telemetry-benchmarks
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running Flight Core Telemetry benchmarks"
)
