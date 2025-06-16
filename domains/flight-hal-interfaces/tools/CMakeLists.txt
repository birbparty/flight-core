# Flight HAL Interfaces Tools Configuration

# Build development tools for HAL interface management
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    
    # Interface code generator
    add_subdirectory(interface_generator)
    
    # Compliance validation tool
    add_subdirectory(compliance_validator)
    
    # Performance profiler
    add_subdirectory(performance_profiler)
    
    # Migration helper tool
    add_subdirectory(migration_helper)
    
endif()
