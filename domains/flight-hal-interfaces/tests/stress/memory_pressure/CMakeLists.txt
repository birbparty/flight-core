# Memory Pressure Stress Tests
# Tests HAL memory interface behavior under memory pressure

# Create memory pressure stress tests library
add_library(stress_memory_pressure_tests STATIC
    gradual_exhaustion_test.cpp
    sudden_spike_test.cpp
    fragmentation_stress_test.cpp
    allocation_cycle_test.cpp
    garbage_collection_stress.cpp
)

target_include_directories(stress_memory_pressure_tests
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/tests
)

target_link_libraries(stress_memory_pressure_tests
    PUBLIC
    stress_test_framework
    flight_hal_interfaces
    flight_hal_test_framework
)

target_compile_features(stress_memory_pressure_tests PUBLIC cxx_std_20)

# Individual test executables for standalone execution
add_executable(gradual_exhaustion_test gradual_exhaustion_test.cpp)
target_link_libraries(gradual_exhaustion_test 
    PRIVATE stress_test_framework flight_hal_interfaces flight_hal_test_framework)

add_executable(sudden_spike_test sudden_spike_test.cpp)
target_link_libraries(sudden_spike_test
    PRIVATE stress_test_framework flight_hal_interfaces flight_hal_test_framework)

add_executable(fragmentation_stress_test fragmentation_stress_test.cpp)
target_link_libraries(fragmentation_stress_test
    PRIVATE stress_test_framework flight_hal_interfaces flight_hal_test_framework)

add_executable(allocation_cycle_test allocation_cycle_test.cpp)
target_link_libraries(allocation_cycle_test
    PRIVATE stress_test_framework flight_hal_interfaces flight_hal_test_framework)

add_executable(garbage_collection_stress garbage_collection_stress.cpp)
target_link_libraries(garbage_collection_stress
    PRIVATE stress_test_framework flight_hal_interfaces flight_hal_test_framework)

# Add individual tests
add_test(NAME memory_pressure_gradual_exhaustion COMMAND gradual_exhaustion_test)
add_test(NAME memory_pressure_sudden_spike COMMAND sudden_spike_test)
add_test(NAME memory_pressure_fragmentation COMMAND fragmentation_stress_test)
add_test(NAME memory_pressure_allocation_cycle COMMAND allocation_cycle_test)
add_test(NAME memory_pressure_garbage_collection COMMAND garbage_collection_stress)

# Set test properties
set_tests_properties(
    memory_pressure_gradual_exhaustion
    memory_pressure_sudden_spike
    memory_pressure_fragmentation
    memory_pressure_allocation_cycle
    memory_pressure_garbage_collection
    PROPERTIES
    LABELS "stress;memory_pressure"
    TIMEOUT 120
)

# Install test executables
install(TARGETS
    gradual_exhaustion_test
    sudden_spike_test
    fragmentation_stress_test
    allocation_cycle_test
    garbage_collection_stress
    RUNTIME DESTINATION bin/tests/stress/memory_pressure
    COMPONENT StressTests
)
