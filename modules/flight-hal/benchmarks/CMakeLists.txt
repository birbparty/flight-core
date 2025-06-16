# Flight Core HAL Module Benchmarks
cmake_minimum_required(VERSION 3.14)

# Benchmark executable
add_executable(flight-hal-benchmarks
    # Add benchmark source files here
    # bench_file_io.cpp
    # bench_threading.cpp
    # bench_time_functions.cpp
)

# Link benchmark dependencies
target_link_libraries(flight-hal-benchmarks
    PRIVATE
        flight-hal
        benchmark::benchmark
)

# Include directories
target_include_directories(flight-hal-benchmarks
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# Register benchmarks
add_custom_target(run-flight-hal-benchmarks
    COMMAND flight-hal-benchmarks
    DEPENDS flight-hal-benchmarks
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running Flight Core HAL benchmarks"
)
