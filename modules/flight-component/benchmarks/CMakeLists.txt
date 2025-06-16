# Flight Core Component Module Benchmarks
cmake_minimum_required(VERSION 3.14)

# Benchmark executable
add_executable(flight-component-benchmarks
    # Add benchmark source files here
    # bench_component_instantiation.cpp
    # bench_interface_types.cpp
    # bench_canonical_abi.cpp
)

# Link benchmark dependencies
target_link_libraries(flight-component-benchmarks
    PRIVATE
        flight-component
        flight-wasm
        flight-runtime
        benchmark::benchmark
)

# Include directories
target_include_directories(flight-component-benchmarks
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# Register benchmarks
add_custom_target(run-flight-component-benchmarks
    COMMAND flight-component-benchmarks
    DEPENDS flight-component-benchmarks
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running Flight Core Component benchmarks"
)
