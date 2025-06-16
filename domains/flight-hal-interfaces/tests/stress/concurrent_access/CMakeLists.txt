# Concurrent Access Stress Tests
# Tests HAL behavior under concurrent multi-threaded access

# Create concurrent access stress tests library
add_library(stress_concurrent_access_tests STATIC
    multi_thread_stress.cpp
    driver_contention_test.cpp
    deadlock_stress_test.cpp
    race_condition_detection.cpp
)

target_include_directories(stress_concurrent_access_tests
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/tests
)

target_link_libraries(stress_concurrent_access_tests
    PUBLIC
    stress_test_framework
    flight_hal_interfaces
    flight_hal_test_framework
    Threads::Threads
)

target_compile_features(stress_concurrent_access_tests PUBLIC cxx_std_20)

# Individual test executables for standalone execution
add_executable(multi_thread_stress multi_thread_stress.cpp)
target_link_libraries(multi_thread_stress 
    PRIVATE stress_test_framework flight_hal_interfaces flight_hal_test_framework Threads::Threads)

add_executable(driver_contention_test driver_contention_test.cpp)
target_link_libraries(driver_contention_test
    PRIVATE stress_test_framework flight_hal_interfaces flight_hal_test_framework Threads::Threads)

add_executable(deadlock_stress_test deadlock_stress_test.cpp)
target_link_libraries(deadlock_stress_test
    PRIVATE stress_test_framework flight_hal_interfaces flight_hal_test_framework Threads::Threads)

add_executable(race_condition_detection race_condition_detection.cpp)
target_link_libraries(race_condition_detection
    PRIVATE stress_test_framework flight_hal_interfaces flight_hal_test_framework Threads::Threads)

# Add individual tests
add_test(NAME concurrent_access_multi_thread COMMAND multi_thread_stress)
add_test(NAME concurrent_access_driver_contention COMMAND driver_contention_test)
add_test(NAME concurrent_access_deadlock COMMAND deadlock_stress_test)
add_test(NAME concurrent_access_race_condition COMMAND race_condition_detection)

# Set test properties
set_tests_properties(
    concurrent_access_multi_thread
    concurrent_access_driver_contention
    concurrent_access_deadlock
    concurrent_access_race_condition
    PROPERTIES
    LABELS "stress;concurrent_access"
    TIMEOUT 240
)
