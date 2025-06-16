# Flight HAL macOS Integration Tests

# Find or fetch Catch2 testing framework
find_package(Catch2 3 QUIET)

if(NOT Catch2_FOUND)
    # Use FetchContent to get Catch2 if not found
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.4.0
    )
    FetchContent_MakeAvailable(Catch2)
endif()

# Test executable
add_executable(flight-hal-tests
    main.cpp
    unit/test_platform_coordinator.cpp
    unit/test_platform_detector.cpp
    unit/test_process_manager.cpp
    unit/test_file_system_bridge.cpp
    unit/test_development_tools.cpp
    unit/test_framework_integration.cpp
    integration/test_flight_integration.cpp
)

# Link with the main library and test framework
target_link_libraries(flight-hal-tests 
    PRIVATE 
    flight-hal-macos-integration
    Catch2::Catch2WithMain
)

# Include directories
target_include_directories(flight-hal-tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

# Apple Silicon optimizations for tests
if(FLIGHT_HAL_APPLE_SILICON)
    target_compile_options(flight-hal-tests PRIVATE
        -mcpu=apple-a14
        -mtune=native
        -O2  # Slightly less aggressive optimization for tests
    )
endif()

# Test discovery for CTest
if(TARGET Catch2::Catch2)
    include(Catch)
    catch_discover_tests(flight-hal-tests)
endif()

# Add custom test targets
add_custom_target(test-unit
    COMMAND flight-hal-tests [unit]
    DEPENDS flight-hal-tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running unit tests"
)

add_custom_target(test-integration
    COMMAND flight-hal-tests [integration]
    DEPENDS flight-hal-tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running integration tests"
)

add_custom_target(test-performance
    COMMAND flight-hal-tests [performance]
    DEPENDS flight-hal-tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running performance tests"
)
