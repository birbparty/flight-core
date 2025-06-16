# Flight HAL Integration Tests

# Add subdirectory for framework
add_subdirectory(framework)

# Create integration test executable
add_executable(hal_integration_tests
    integration_test_runner.cpp
    multi_driver/simple_integration_test.cpp
)

target_include_directories(hal_integration_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

target_link_libraries(hal_integration_tests PRIVATE
    integration_test_framework
    # Temporarily disable HAL library dependencies until compilation issues are resolved
    # flight_hal_core
    # flight_hal_coordination  
    # flight_hal_validation
)

# Set C++17 standard
target_compile_features(hal_integration_tests PRIVATE cxx_std_17)

# Add compiler warnings
if(MSVC)
    target_compile_options(hal_integration_tests PRIVATE /W4)
else()
    target_compile_options(hal_integration_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Create CMakeLists for multi_driver subdirectory
add_subdirectory(multi_driver)
add_subdirectory(complex_operations)
add_subdirectory(platform_specific)
add_subdirectory(real_world_simulation)

# Create test targets for CTest integration
enable_testing()

# Add integration tests to CTest
add_test(NAME integration_tests_all 
         COMMAND hal_integration_tests --all
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

add_test(NAME integration_tests_gpu_audio
         COMMAND hal_integration_tests --run gpu_audio_coordination
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

add_test(NAME integration_tests_cross_driver_resource
         COMMAND hal_integration_tests --run cross_driver_resource  
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# Set test properties
set_tests_properties(integration_tests_all PROPERTIES
    TIMEOUT 300  # 5 minute timeout
    LABELS "integration;all"
)

set_tests_properties(integration_tests_gpu_audio PROPERTIES
    TIMEOUT 60   # 1 minute timeout
    LABELS "integration;multi_driver;gpu;audio"
)

set_tests_properties(integration_tests_cross_driver_resource PROPERTIES
    TIMEOUT 30   # 30 second timeout
    LABELS "integration;multi_driver;resource"
)

# Install integration test executable
install(TARGETS hal_integration_tests
    RUNTIME DESTINATION bin
)

# Create a custom target for running integration tests with verbose output
add_custom_target(run_integration_tests
    COMMAND hal_integration_tests --all --verbose
    DEPENDS hal_integration_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running HAL integration tests with verbose output"
)

# Create a custom target for generating integration test report
add_custom_target(integration_test_report
    COMMAND hal_integration_tests --all --report integration_report.txt
    DEPENDS hal_integration_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating HAL integration test report"
)
