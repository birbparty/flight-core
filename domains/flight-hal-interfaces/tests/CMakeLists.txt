# Flight HAL Interfaces Test Configuration

# Only build tests if testing is enabled
if(FLIGHT_HAL_ENABLE_UNIT_TESTS)
    
    # Unit tests
    add_subdirectory(unit)
    
    # Integration tests
    add_subdirectory(integration)
    
    # Compliance tests
    add_subdirectory(compliance)
    
    # Stress tests
    add_subdirectory(stress)
    
endif()
