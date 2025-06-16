# Flight Core Memory Module Benchmarks
cmake_minimum_required(VERSION 3.14)

# Benchmark executable
add_executable(flight-memory-benchmarks
    # Add benchmark source files here
    # bench_linear_allocator.cpp
    # bench_stack_allocator.cpp
    # bench_memory_pool.cpp
)

# Link benchmark dependencies
target_link_libraries(flight-memory-benchmarks
    PRIVATE
        flight-memory
        benchmark::benchmark
)

# Include directories
target_include_directories(flight-memory-benchmarks
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# Register benchmarks
add_custom_target(run-flight-memory-benchmarks
    COMMAND flight-memory-benchmarks
    DEPENDS flight-memory-benchmarks
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running Flight Core Memory benchmarks"
)
