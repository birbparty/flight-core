# Integration test framework
add_library(integration_test_framework
    integration_test_base.cpp
    hal_stubs.cpp
)

target_include_directories(integration_test_framework PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
)

target_link_libraries(integration_test_framework PUBLIC
    # Temporarily disable HAL library dependencies until compilation issues are resolved
    # flight_hal_core
    # flight_hal_coordination
    # flight_hal_validation
)

# Set C++17 standard
target_compile_features(integration_test_framework PUBLIC cxx_std_17)

# Add compiler warnings
if(MSVC)
    target_compile_options(integration_test_framework PRIVATE /W4)
else()
    target_compile_options(integration_test_framework PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Install framework library
install(TARGETS integration_test_framework
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install headers
install(FILES integration_test_base.hpp
    DESTINATION include/flight/hal/integration
)
