# Flight HAL Stress Testing Suite
# Comprehensive stress tests for HAL implementations

cmake_minimum_required(VERSION 3.20)

# Enable stress testing by default in debug builds
if(NOT DEFINED FLIGHT_ENABLE_STRESS_TESTS)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(FLIGHT_ENABLE_STRESS_TESTS ON)
    else()
        set(FLIGHT_ENABLE_STRESS_TESTS OFF)
    endif()
endif()

if(NOT FLIGHT_ENABLE_STRESS_TESTS)
    message(STATUS "Stress tests disabled. Set FLIGHT_ENABLE_STRESS_TESTS=ON to enable.")
    return()
endif()

message(STATUS "Building Flight HAL Stress Tests")

# Add stress test framework
add_subdirectory(framework)

# Add stress test categories
add_subdirectory(memory_pressure)
add_subdirectory(resource_exhaustion)
add_subdirectory(concurrent_access)
add_subdirectory(high_frequency)
add_subdirectory(stability)

# Create stress test runner executable
add_executable(hal_stress_test_runner
    stress_test_runner.cpp
)

target_link_libraries(hal_stress_test_runner
    PRIVATE
    flight_hal_core
    flight_hal_interfaces
    flight_hal_test_framework
    stress_test_framework
    stress_memory_pressure_tests
    stress_resource_exhaustion_tests
    stress_concurrent_access_tests
    stress_high_frequency_tests
    stress_stability_tests
)

target_include_directories(hal_stress_test_runner
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/framework
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/tests
)

# Set compiler flags for stress testing
target_compile_features(hal_stress_test_runner PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(hal_stress_test_runner PRIVATE /W4 /WX)
else()
    target_compile_options(hal_stress_test_runner PRIVATE -Wall -Wextra -Werror -pedantic)
endif()

# Add stress test configurations
set(STRESS_TEST_CONFIGS
    light      # Quick development tests
    standard   # CI/CD integration tests
    heavy      # Pre-release validation
    embedded   # Embedded system constraints
    extreme    # Maximum stress tests
)

# Create test targets for each configuration
foreach(CONFIG ${STRESS_TEST_CONFIGS})
    add_test(
        NAME stress_tests_${CONFIG}
        COMMAND hal_stress_test_runner --config=${CONFIG}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    
    # Set test properties
    set_tests_properties(stress_tests_${CONFIG} PROPERTIES
        TIMEOUT 300  # 5 minutes default timeout
        LABELS "stress;${CONFIG}"
    )
    
    # Configure timeout based on test severity
    if(CONFIG STREQUAL "light")
        set_tests_properties(stress_tests_${CONFIG} PROPERTIES TIMEOUT 60)
    elseif(CONFIG STREQUAL "heavy")
        set_tests_properties(stress_tests_${CONFIG} PROPERTIES TIMEOUT 600)
    elseif(CONFIG STREQUAL "extreme")
        set_tests_properties(stress_tests_${CONFIG} PROPERTIES TIMEOUT 1800)
    endif()
endforeach()

# Add memory sanitizer support for stress tests
if(FLIGHT_ENABLE_SANITIZERS)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(hal_stress_test_runner PRIVATE
            -fsanitize=address
            -fsanitize=leak
            -fsanitize=undefined
        )
        target_link_options(hal_stress_test_runner PRIVATE
            -fsanitize=address
            -fsanitize=leak
            -fsanitize=undefined
        )
    endif()
endif()

# Add thread sanitizer for concurrent tests
option(FLIGHT_ENABLE_THREAD_SANITIZER "Enable thread sanitizer for stress tests" OFF)
if(FLIGHT_ENABLE_THREAD_SANITIZER)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(hal_stress_test_runner PRIVATE -fsanitize=thread)
        target_link_options(hal_stress_test_runner PRIVATE -fsanitize=thread)
    endif()
endif()

# Create custom targets for convenient testing
add_custom_target(stress_tests_quick
    COMMAND ${CMAKE_CTEST_COMMAND} -L "stress;light" --output-on-failure
    DEPENDS hal_stress_test_runner
    COMMENT "Running quick stress tests"
)

add_custom_target(stress_tests_full
    COMMAND ${CMAKE_CTEST_COMMAND} -L "stress" --output-on-failure
    DEPENDS hal_stress_test_runner
    COMMENT "Running full stress test suite"
)

# Install stress test runner for deployment testing
install(TARGETS hal_stress_test_runner
    RUNTIME DESTINATION bin/tests
    COMPONENT StressTests
)

# Install stress test configuration files
install(FILES
    stress_test_configs.json
    DESTINATION share/flight-hal/tests
    COMPONENT StressTests
    OPTIONAL
)

# Performance profiling support
option(FLIGHT_ENABLE_STRESS_PROFILING "Enable performance profiling during stress tests" OFF)
if(FLIGHT_ENABLE_STRESS_PROFILING)
    find_program(PERF_EXECUTABLE perf)
    find_program(VALGRIND_EXECUTABLE valgrind)
    
    if(PERF_EXECUTABLE)
        add_custom_target(stress_tests_perf
            COMMAND ${PERF_EXECUTABLE} record -g ${CMAKE_CURRENT_BINARY_DIR}/hal_stress_test_runner --config=standard
            DEPENDS hal_stress_test_runner
            COMMENT "Running stress tests with perf profiling"
        )
    endif()
    
    if(VALGRIND_EXECUTABLE)
        add_custom_target(stress_tests_valgrind
            COMMAND ${VALGRIND_EXECUTABLE} --tool=memcheck --leak-check=full --show-leak-kinds=all 
                    ${CMAKE_CURRENT_BINARY_DIR}/hal_stress_test_runner --config=light
            DEPENDS hal_stress_test_runner
            COMMENT "Running stress tests with Valgrind"
        )
    endif()
endif()

# Documentation generation for stress tests
if(FLIGHT_BUILD_DOCS)
    find_program(DOXYGEN_EXECUTABLE doxygen)
    if(DOXYGEN_EXECUTABLE)
        configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in
            ${CMAKE_CURRENT_BINARY_DIR}/docs/Doxyfile
            @ONLY
        )
        
        add_custom_target(stress_tests_docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/docs/Doxyfile
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs
            COMMENT "Generating stress test documentation"
        )
    endif()
endif()

# Summary
message(STATUS "Stress Testing Configuration:")
message(STATUS "  - Framework: enabled")
message(STATUS "  - Memory pressure tests: enabled")
message(STATUS "  - Resource exhaustion tests: enabled")
message(STATUS "  - Concurrent access tests: enabled")
message(STATUS "  - High frequency tests: enabled")
message(STATUS "  - Stability tests: enabled")
message(STATUS "  - Sanitizers: ${FLIGHT_ENABLE_SANITIZERS}")
message(STATUS "  - Thread sanitizer: ${FLIGHT_ENABLE_THREAD_SANITIZER}")
message(STATUS "  - Profiling: ${FLIGHT_ENABLE_STRESS_PROFILING}")
