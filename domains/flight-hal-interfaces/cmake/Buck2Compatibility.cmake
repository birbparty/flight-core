# Buck2 Compatibility Layer for CMake
# This module provides patterns that translate well to Buck2

# Function to create Buck2-compatible library targets
function(flight_hal_add_component_library)
    cmake_parse_arguments(ARGS "" "NAME;NAMESPACE" "SOURCES;HEADERS;DEPS" ${ARGN})
    
    # Create the library target
    add_library(${ARGS_NAME} ${ARGS_SOURCES})
    
    # Set properties that translate well to Buck2
    target_compile_features(${ARGS_NAME} PUBLIC cxx_std_17)
    target_compile_definitions(${ARGS_NAME} PRIVATE -DFLIGHT_HAL_${ARGS_NAMESPACE})
    
    # Add include directories
    target_include_directories(${ARGS_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )
    
    # Add dependencies
    if(ARGS_DEPS)
        target_link_libraries(${ARGS_NAME} PUBLIC ${ARGS_DEPS})
    endif()
    
    # Create alias that matches Buck2 naming convention
    add_library(Flight::HAL::${ARGS_NAME} ALIAS ${ARGS_NAME})
    
    # Apply common configuration
    flight_hal_configure_compiler_settings(${ARGS_NAME})
    if(FLIGHT_HAL_ENABLE_WARNINGS)
        set_project_warnings(${ARGS_NAME})
    endif()
endfunction()

# Function to create Buck2-compatible test targets
function(flight_hal_add_test_suite)
    cmake_parse_arguments(ARGS "" "NAME;CATEGORY" "SOURCES;DEPS" ${ARGN})
    
    # Create test executable
    add_executable(${ARGS_NAME}_test ${ARGS_SOURCES})
    
    # Set properties
    target_compile_features(${ARGS_NAME}_test PRIVATE cxx_std_17)
    target_compile_definitions(${ARGS_NAME}_test PRIVATE 
        -DFLIGHT_HAL_TESTING
        -DFLIGHT_HAL_${ARGS_CATEGORY}_TEST
    )
    
    # Add dependencies
    if(ARGS_DEPS)
        target_link_libraries(${ARGS_NAME}_test PRIVATE ${ARGS_DEPS})
    endif()
    
    # Register with CTest
    add_test(NAME ${ARGS_NAME} COMMAND ${ARGS_NAME}_test)
    
    # Apply common configuration
    flight_hal_configure_compiler_settings(${ARGS_NAME}_test)
endfunction()

# Function to create Buck2-compatible example executables
function(flight_hal_add_example)
    cmake_parse_arguments(ARGS "" "NAME" "SOURCES;DEPS" ${ARGN})
    
    # Create example executable
    add_executable(${ARGS_NAME} ${ARGS_SOURCES})
    
    # Set properties
    target_compile_features(${ARGS_NAME} PRIVATE cxx_std_17)
    
    # Add dependencies
    if(ARGS_DEPS)
        target_link_libraries(${ARGS_NAME} PRIVATE ${ARGS_DEPS})
    endif()
    
    # Apply common configuration
    flight_hal_configure_compiler_settings(${ARGS_NAME})
endfunction()

# Utility function to get Buck2 target name from CMake target
function(flight_hal_get_buck2_target_name CMAKE_TARGET OUTPUT_VAR)
    string(REPLACE "::" "-" BUCK2_NAME ${CMAKE_TARGET})
    string(TOLOWER ${BUCK2_NAME} BUCK2_NAME)
    set(${OUTPUT_VAR} ${BUCK2_NAME} PARENT_SCOPE)
endfunction()

# Function to generate Buck2 BUILD file content (for migration tool)
function(flight_hal_generate_buck2_target_info)
    cmake_parse_arguments(ARGS "" "TARGET;OUTPUT_FILE" "" ${ARGN})
    
    # This would be used by buck2-cpp-cpm tool to generate Buck2 files
    # For now, we just document the target structure
    
    get_target_property(TARGET_TYPE ${ARGS_TARGET} TYPE)
    get_target_property(TARGET_SOURCES ${ARGS_TARGET} SOURCES)
    get_target_property(TARGET_INCLUDE_DIRS ${ARGS_TARGET} INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(TARGET_COMPILE_DEFS ${ARGS_TARGET} COMPILE_DEFINITIONS)
    get_target_property(TARGET_LINK_LIBS ${ARGS_TARGET} LINK_LIBRARIES)
    
    # Write target information to file for migration tool
    if(ARGS_OUTPUT_FILE)
        file(APPEND ${ARGS_OUTPUT_FILE} "# Target: ${ARGS_TARGET}\n")
        file(APPEND ${ARGS_OUTPUT_FILE} "# Type: ${TARGET_TYPE}\n")
        file(APPEND ${ARGS_OUTPUT_FILE} "# Sources: ${TARGET_SOURCES}\n")
        file(APPEND ${ARGS_OUTPUT_FILE} "# Include Dirs: ${TARGET_INCLUDE_DIRS}\n")
        file(APPEND ${ARGS_OUTPUT_FILE} "# Compile Defs: ${TARGET_COMPILE_DEFS}\n")
        file(APPEND ${ARGS_OUTPUT_FILE} "# Link Libs: ${TARGET_LINK_LIBS}\n")
        file(APPEND ${ARGS_OUTPUT_FILE} "\n")
    endif()
endfunction()

message(STATUS "Buck2 compatibility layer loaded")
