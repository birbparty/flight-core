# Flight Core WASM Module Tests
cmake_minimum_required(VERSION 3.14)

# Test executable
add_executable(flight-core-wasm-tests
    # Add test source files here
    # test_wasm_types.cpp
    # test_wasm_validator.cpp
)

# Link test dependencies
target_link_libraries(flight-core-wasm-tests
    PRIVATE
        flight-core-wasm
        Catch2::Catch2WithMain
)

# Include directories
target_include_directories(flight-core-wasm-tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# Register tests with CTest
include(Catch)
catch_discover_tests(flight-core-wasm-tests)
