# Flight Core Runtime Module Benchmarks
cmake_minimum_required(VERSION 3.14)

# Benchmark executable
add_executable(flight-runtime-benchmarks
    # Add benchmark source files here
    # bench_interpreter.cpp
    # bench_execution_context.cpp
    # bench_stack_machine.cpp
)

# Link benchmark dependencies
target_link_libraries(flight-runtime-benchmarks
    PRIVATE
        flight-runtime
        flight-wasm
        flight-memory
        benchmark::benchmark
)

# Include directories
target_include_directories(flight-runtime-benchmarks
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# Register benchmarks
add_custom_target(run-flight-runtime-benchmarks
    COMMAND flight-runtime-benchmarks
    DEPENDS flight-runtime-benchmarks
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running Flight Core Runtime benchmarks"
)
