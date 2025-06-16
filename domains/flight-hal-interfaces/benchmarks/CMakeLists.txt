# Flight HAL Interfaces Benchmark Configuration

# Only build benchmarks if enabled
if(FLIGHT_HAL_ENABLE_BENCHMARKS)
    
    # HAL Interface Benchmarks (comprehensive suite)
    add_subdirectory(hal_interface_benchmarks)
    
    # Legacy benchmark categories (only if they exist)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/interface_overhead)
        add_subdirectory(interface_overhead)
    endif()
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/driver_switching)
        add_subdirectory(driver_switching)
    endif()
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/memory_allocation)
        add_subdirectory(memory_allocation)
    endif()
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cross_platform)
        add_subdirectory(cross_platform)
    endif()
    
    # Future extensions (only if they exist)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/regression_detection)
        add_subdirectory(regression_detection)
    endif()
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/reporting)
        add_subdirectory(reporting)
    endif()
    
endif()
