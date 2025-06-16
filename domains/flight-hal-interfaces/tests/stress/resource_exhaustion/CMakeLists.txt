# Resource Exhaustion Stress Tests
# Tests HAL behavior when various resources are exhausted

# Create resource exhaustion stress tests library
add_library(stress_resource_exhaustion_tests STATIC
    graphics_resource_stress.cpp
    audio_channel_stress.cpp
    file_handle_stress.cpp
    network_connection_stress.cpp
    cross_interface_stress.cpp
)

target_include_directories(stress_resource_exhaustion_tests
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/tests
)

target_link_libraries(stress_resource_exhaustion_tests
    PUBLIC
    stress_test_framework
    flight_hal_interfaces
    flight_hal_test_framework
)

target_compile_features(stress_resource_exhaustion_tests PUBLIC cxx_std_20)

# Individual test executables for standalone execution
add_executable(graphics_resource_stress graphics_resource_stress.cpp)
target_link_libraries(graphics_resource_stress 
    PRIVATE stress_test_framework flight_hal_interfaces flight_hal_test_framework)

add_executable(audio_channel_stress audio_channel_stress.cpp)
target_link_libraries(audio_channel_stress
    PRIVATE stress_test_framework flight_hal_interfaces flight_hal_test_framework)

add_executable(file_handle_stress file_handle_stress.cpp)
target_link_libraries(file_handle_stress
    PRIVATE stress_test_framework flight_hal_interfaces flight_hal_test_framework)

add_executable(network_connection_stress network_connection_stress.cpp)
target_link_libraries(network_connection_stress
    PRIVATE stress_test_framework flight_hal_interfaces flight_hal_test_framework)

add_executable(cross_interface_stress cross_interface_stress.cpp)
target_link_libraries(cross_interface_stress
    PRIVATE stress_test_framework flight_hal_interfaces flight_hal_test_framework)

# Add individual tests
add_test(NAME resource_exhaustion_graphics COMMAND graphics_resource_stress)
add_test(NAME resource_exhaustion_audio COMMAND audio_channel_stress)
add_test(NAME resource_exhaustion_file COMMAND file_handle_stress)
add_test(NAME resource_exhaustion_network COMMAND network_connection_stress)
add_test(NAME resource_exhaustion_cross_interface COMMAND cross_interface_stress)

# Set test properties
set_tests_properties(
    resource_exhaustion_graphics
    resource_exhaustion_audio
    resource_exhaustion_file
    resource_exhaustion_network
    resource_exhaustion_cross_interface
    PROPERTIES
    LABELS "stress;resource_exhaustion"
    TIMEOUT 180
)
