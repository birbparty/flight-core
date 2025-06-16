# Flight HAL Interfaces Examples Configuration

# Build examples for development and testing
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    
    # Basic HAL usage example
    add_subdirectory(basic_hal_usage)
    
    # Custom driver implementation example
    add_subdirectory(custom_driver)
    
    # Platform detection example
    add_subdirectory(platform_detection)
    
    # Resource management example
    add_subdirectory(resource_management)
    
    # Cross-platform application example
    add_subdirectory(cross_platform_app)
    
endif()
