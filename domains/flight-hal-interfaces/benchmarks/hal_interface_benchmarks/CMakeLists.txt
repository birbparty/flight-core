# HAL Interface Benchmarks
cmake_minimum_required(VERSION 3.16)

# Only build if benchmarks are enabled
if(NOT FLIGHT_HAL_ENABLE_BENCHMARKS)
    return()
endif()

# Google Benchmark is available via FetchContent from main CMakeLists.txt
# Common benchmark configuration
set(BENCHMARK_COMMON_SOURCES)
set(BENCHMARK_COMMON_LIBRARIES
    benchmark
    flight-hal-interfaces
)

# Add compiler flags for optimized benchmarks
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(BENCHMARK_COMPILE_FLAGS
        -O3
        -DNDEBUG
        -march=native
        -flto
    )
else()
    set(BENCHMARK_COMPILE_FLAGS
        -O2
        -g
    )
endif()

# Standalone Memory Interface Benchmarks (no HAL dependencies)
add_executable(standalone_memory_benchmark
    standalone_memory_benchmark.cpp
    ${BENCHMARK_COMMON_SOURCES}
)

target_link_libraries(standalone_memory_benchmark
    benchmark
)

target_compile_options(standalone_memory_benchmark PRIVATE
    ${BENCHMARK_COMPILE_FLAGS}
)

# Memory Interface Benchmarks (with HAL dependencies - disabled due to compilation issues)
# add_executable(memory_benchmark_suite
#     memory_benchmark_suite.cpp
#     ${BENCHMARK_COMMON_SOURCES}
# )
# 
# target_link_libraries(memory_benchmark_suite
#     ${BENCHMARK_COMMON_LIBRARIES}
# )
# 
# target_compile_options(memory_benchmark_suite PRIVATE
#     ${BENCHMARK_COMPILE_FLAGS}
# )
# 
# target_compile_definitions(memory_benchmark_suite PRIVATE
#     FLIGHT_ENABLE_MOCK_DRIVERS=1
# )

# Register memory benchmark for CTest integration
add_test(NAME standalone_memory_benchmarks
    COMMAND standalone_memory_benchmark --benchmark_format=json --benchmark_out=standalone_memory_benchmark_results.json
)

# Install benchmark executable
install(TARGETS
    standalone_memory_benchmark
    DESTINATION bin/benchmarks
)

# TODO: Add other benchmark suites when source files are created:
# - graphics_benchmark_suite.cpp
# - audio_benchmark_suite.cpp  
# - comprehensive_benchmark_suite.cpp

# Note: Script generation removed for now - can be added later if needed
