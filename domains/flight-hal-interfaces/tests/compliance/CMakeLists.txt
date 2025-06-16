# Flight HAL Interface Compliance Testing Framework
# Comprehensive testing for all HAL interface implementations

# Base testing framework
add_subdirectory(base)

# Interface-specific compliance tests
add_subdirectory(interfaces)

# Performance benchmarks
add_subdirectory(performance)

# Platform-specific test adaptations
add_subdirectory(platform)

# Standalone compliance test executable (independent of main library)
add_executable(hal_compliance_tests
    interfaces/simple_compliance_test.cpp
)

# Link with testing dependencies only
target_link_libraries(hal_compliance_tests 
    PRIVATE 
    gtest
    gtest_main
)

# Set test properties
set_target_properties(hal_compliance_tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Register as test
add_test(NAME hal_compliance_tests COMMAND hal_compliance_tests)

# Platform-specific test configurations
if(FLIGHT_HAL_PLATFORM STREQUAL "Dreamcast")
    target_compile_definitions(hal_compliance_tests PRIVATE 
        FLIGHT_HAL_DREAMCAST_LIMITS=1
        FLIGHT_HAL_MAX_MEMORY_TEST=1048576  # 1MB
    )
elseif(FLIGHT_HAL_PLATFORM STREQUAL "PSP")
    target_compile_definitions(hal_compliance_tests PRIVATE 
        FLIGHT_HAL_PSP_LIMITS=1
        FLIGHT_HAL_MAX_MEMORY_TEST=2097152  # 2MB
    )
elseif(FLIGHT_HAL_PLATFORM STREQUAL "Web")
    target_compile_definitions(hal_compliance_tests PRIVATE 
        FLIGHT_HAL_WEB_LIMITS=1
        FLIGHT_HAL_MAX_MEMORY_TEST=52428800  # 50MB
    )
else()
    target_compile_definitions(hal_compliance_tests PRIVATE 
        FLIGHT_HAL_DESKTOP_LIMITS=1
        FLIGHT_HAL_MAX_MEMORY_TEST=104857600  # 100MB
    )
endif()

# Compliance test categories
add_custom_target(compliance_basic
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L "compliance_basic"
    COMMENT "Running basic compliance tests"
)

add_custom_target(compliance_performance
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L "compliance_performance"
    COMMENT "Running performance compliance tests"
)

add_custom_target(compliance_stress
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L "compliance_stress"
    COMMENT "Running stress compliance tests"
)

add_custom_target(compliance_error
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L "compliance_error"
    COMMENT "Running error handling compliance tests"
)

add_custom_target(compliance_all
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L "compliance"
    COMMENT "Running all compliance tests"
)

# Platform-specific compliance testing
add_custom_target(compliance_platform
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L "compliance_platform"
    COMMENT "Running platform-specific compliance tests"
)

# Performance benchmarks (placeholder - files not yet implemented)
# if(FLIGHT_HAL_BUILD_BENCHMARKS)
#     flight_hal_add_benchmark(hal_interface_benchmarks
#         # Performance benchmarks for each interface
#         performance/memory_benchmarks.cpp
#     )
# endif()

# Test data and configuration files
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/test_config.json.in
    ${CMAKE_CURRENT_BINARY_DIR}/test_config.json
    @ONLY
)

# Copy test data files
file(COPY test_data/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/test_data/)

# Documentation generation for test results
if(FLIGHT_HAL_BUILD_DOCS)
    find_program(PYTHON_EXECUTABLE python3)
    if(PYTHON_EXECUTABLE)
        add_custom_target(compliance_report
            COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_compliance_report.py
                --test-results ${CMAKE_CURRENT_BINARY_DIR}/test_results.xml
                --output ${CMAKE_CURRENT_BINARY_DIR}/compliance_report.html
            COMMENT "Generating compliance test report"
        )
    endif()
endif()
