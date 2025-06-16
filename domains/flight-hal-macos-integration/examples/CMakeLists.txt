# Flight HAL macOS Integration Examples

# Basic usage example
add_executable(basic_usage
    basic_usage.cpp
)

target_link_libraries(basic_usage
    PRIVATE
    flight-hal-macos-integration
)

target_include_directories(basic_usage PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Apple Silicon optimizations for examples
if(FLIGHT_HAL_APPLE_SILICON)
    target_compile_options(basic_usage PRIVATE
        -mcpu=apple-a14
        -mtune=native
        -O2
    )
endif()

# Performance benchmark example
add_executable(performance_benchmark
    performance_benchmark.cpp
)

target_link_libraries(performance_benchmark
    PRIVATE
    flight-hal-macos-integration
)

target_include_directories(performance_benchmark PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

if(FLIGHT_HAL_APPLE_SILICON)
    target_compile_options(performance_benchmark PRIVATE
        -mcpu=apple-a14
        -mtune=native
        -O2
    )
endif()
