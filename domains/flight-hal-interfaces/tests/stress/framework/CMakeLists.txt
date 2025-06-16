# Stress Test Framework
# Core infrastructure for HAL stress testing

# Create stress test framework library
add_library(stress_test_framework STATIC
    stress_test_base.hpp
    stress_metrics_collector.hpp
    stress_orchestrator.hpp
    stress_report_generator.hpp
)

# Framework is header-only, but we create static lib for organization
set_target_properties(stress_test_framework PROPERTIES
    LINKER_LANGUAGE CXX
)

target_include_directories(stress_test_framework
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/tests
)

target_link_libraries(stress_test_framework
    PUBLIC
    flight_hal_core
    flight_hal_interfaces
)

target_compile_features(stress_test_framework PUBLIC cxx_std_20)

# Platform-specific libraries for metrics collection
if(WIN32)
    target_link_libraries(stress_test_framework PRIVATE psapi)
elseif(UNIX AND NOT APPLE)
    # Linux - no additional libraries needed for basic metrics
elseif(APPLE)
    # macOS - no additional libraries needed for basic metrics  
endif()

# Add threading support for metrics collection
find_package(Threads REQUIRED)
target_link_libraries(stress_test_framework PUBLIC Threads::Threads)

# Install headers for external stress test development
install(FILES
    stress_test_base.hpp
    stress_metrics_collector.hpp
    stress_orchestrator.hpp  
    stress_report_generator.hpp
    DESTINATION include/flight/hal/test/stress
    COMPONENT Development
)
