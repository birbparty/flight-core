# Stability Stress Tests
# Long-running tests for system stability validation

# Create stability stress tests library
add_library(stress_stability_tests STATIC
    endurance_test_suite.cpp
    memory_leak_detection.cpp
    performance_degradation.cpp
    embedded_lifecycle_test.cpp
)

target_include_directories(stress_stability_tests
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/tests
)

target_link_libraries(stress_stability_tests
    PUBLIC
    stress_test_framework
    flight_hal_interfaces
    flight_hal_test_framework
)

target_compile_features(stress_stability_tests PUBLIC cxx_std_20)

# Individual test executables for standalone execution
add_executable(endurance_test_suite endurance_test_suite.cpp)
target_link_libraries(endurance_test_suite 
    PRIVATE stress_test_framework flight_hal_interfaces flight_hal_test_framework)

add_executable(memory_leak_detection memory_leak_detection.cpp)
target_link_libraries(memory_leak_detection
    PRIVATE stress_test_framework flight_hal_interfaces flight_hal_test_framework)

add_executable(performance_degradation performance_degradation.cpp)
target_link_libraries(performance_degradation
    PRIVATE stress_test_framework flight_hal_interfaces flight_hal_test_framework)

add_executable(embedded_lifecycle_test embedded_lifecycle_test.cpp)
target_link_libraries(embedded_lifecycle_test
    PRIVATE stress_test_framework flight_hal_interfaces flight_hal_test_framework)

# Add individual tests with longer timeouts
add_test(NAME stability_endurance COMMAND endurance_test_suite)
add_test(NAME stability_memory_leak COMMAND memory_leak_detection)
add_test(NAME stability_performance_degradation COMMAND performance_degradation)
add_test(NAME stability_embedded_lifecycle COMMAND embedded_lifecycle_test)

# Set test properties with extended timeouts for stability tests
set_tests_properties(
    stability_endurance
    stability_memory_leak
    stability_performance_degradation
    stability_embedded_lifecycle
    PROPERTIES
    LABELS "stress;stability;long_running"
    TIMEOUT 3600  # 1 hour timeout for stability tests
)

# Create shorter versions for CI/CD
add_test(NAME stability_endurance_quick COMMAND endurance_test_suite --quick)
add_test(NAME stability_memory_leak_quick COMMAND memory_leak_detection --quick)
add_test(NAME stability_performance_degradation_quick COMMAND performance_degradation --quick)
add_test(NAME stability_embedded_lifecycle_quick COMMAND embedded_lifecycle_test --quick)

set_tests_properties(
    stability_endurance_quick
    stability_memory_leak_quick
    stability_performance_degradation_quick
    stability_embedded_lifecycle_quick
    PROPERTIES
    LABELS "stress;stability;quick"
    TIMEOUT 300  # 5 minutes for quick variants
)
