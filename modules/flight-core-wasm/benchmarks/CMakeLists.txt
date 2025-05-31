# Flight Core WASM Module Benchmarks
cmake_minimum_required(VERSION 3.14)

# Benchmark executable
add_executable(flight-core-wasm-benchmarks
    # Add benchmark source files here
    # bench_wasm_types.cpp
    # bench_wasm_validator.cpp
)

# Link benchmark dependencies
target_link_libraries(flight-core-wasm-benchmarks
    PRIVATE
        flight-core-wasm
        benchmark::benchmark
        benchmark::benchmark_main
)

# Include directories
target_include_directories(flight-core-wasm-benchmarks
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)
