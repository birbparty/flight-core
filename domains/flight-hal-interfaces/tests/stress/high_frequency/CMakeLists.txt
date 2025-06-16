# High Frequency Operations Stress Tests
# Tests HAL behavior under high-frequency operation patterns

# Create high frequency stress tests library
add_library(stress_high_frequency_tests STATIC
    rapid_operation_cycles.cpp
    event_flooding_test.cpp
    interrupt_storm_simulation.cpp
    real_time_stress_test.cpp
)

target_include_directories(stress_high_frequency_tests
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/tests
)

target_link_libraries(stress_high_frequency_tests
    PUBLIC
    stress_test_framework
    flight_hal_interfaces
    flight_hal_test_framework
)

target_compile_features(stress_high_frequency_tests PUBLIC cxx_std_20)

# Individual test executables for standalone execution
add_executable(rapid_operation_cycles rapid_operation_cycles.cpp)
target_link_libraries(rapid_operation_cycles 
    PRIVATE stress_test_framework flight_hal_interfaces flight_hal_test_framework)

add_executable(event_flooding_test event_flooding_test.cpp)
target_link_libraries(event_flooding_test
    PRIVATE stress_test_framework flight_hal_interfaces flight_hal_test_framework)

add_executable(interrupt_storm_simulation interrupt_storm_simulation.cpp)
target_link_libraries(interrupt_storm_simulation
    PRIVATE stress_test_framework flight_hal_interfaces flight_hal_test_framework)

add_executable(real_time_stress_test real_time_stress_test.cpp)
target_link_libraries(real_time_stress_test
    PRIVATE stress_test_framework flight_hal_interfaces flight_hal_test_framework)

# Add individual tests
add_test(NAME high_frequency_rapid_operations COMMAND rapid_operation_cycles)
add_test(NAME high_frequency_event_flooding COMMAND event_flooding_test)
add_test(NAME high_frequency_interrupt_storm COMMAND interrupt_storm_simulation)
add_test(NAME high_frequency_real_time COMMAND real_time_stress_test)

# Set test properties
set_tests_properties(
    high_frequency_rapid_operations
    high_frequency_event_flooding
    high_frequency_interrupt_storm
    high_frequency_real_time
    PROPERTIES
    LABELS "stress;high_frequency"
    TIMEOUT 90
)
