# Flight-Core C++17 Examples
cmake_minimum_required(VERSION 3.17)

# Flight-Core Integration Example
add_executable(flight_integration_example
    flight_integration_example.cpp
)

target_link_libraries(flight_integration_example
    flight_shared_types_cpp17
)

target_compile_features(flight_integration_example PRIVATE cxx_std_17)

# Set example-specific compile options
target_compile_options(flight_integration_example PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
        -Wall -Wextra -Wpedantic
    >
)

# Install example
install(TARGETS flight_integration_example
    RUNTIME DESTINATION bin/examples
)

# Add custom target to run the example
add_custom_target(run_example
    COMMAND flight_integration_example
    DEPENDS flight_integration_example
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running Flight-Core C++17 integration example"
)

message(STATUS "Flight-Core C++17 examples configured")
