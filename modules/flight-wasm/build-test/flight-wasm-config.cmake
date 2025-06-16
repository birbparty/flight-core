# =============================================================================
# Flight WASM Package Configuration File
# Generated configuration for find_package(flight-wasm) support
# =============================================================================


####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was flight-wasm-config.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

# Package information
set(FLIGHT_WASM_VERSION "1.0.0")
set(FLIGHT_WASM_VERSION_MAJOR "1")
set(FLIGHT_WASM_VERSION_MINOR "0")
set(FLIGHT_WASM_VERSION_PATCH "0")

# WebAssembly specification compliance
set(FLIGHT_WASM_SPEC_VERSION 1)
set(FLIGHT_WASM_SPEC_COMPLIANT TRUE)

# Header-only library information
set(FLIGHT_WASM_HEADER_ONLY TRUE)
set(FLIGHT_WASM_INTERFACE_LIBRARY TRUE)

# Include the targets file
include("${CMAKE_CURRENT_LIST_DIR}/flight-wasm-targets.cmake")

# Validate that the targets exist
if(NOT TARGET flight::wasm)
    message(FATAL_ERROR "Flight WASM target not found - package may be corrupted")
endif()

# Set up imported target properties
get_target_property(FLIGHT_WASM_INCLUDE_DIRS flight::wasm INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(FLIGHT_WASM_COMPILE_FEATURES flight::wasm INTERFACE_COMPILE_FEATURES)
get_target_property(FLIGHT_WASM_COMPILE_DEFINITIONS flight::wasm INTERFACE_COMPILE_DEFINITIONS)

# Provide legacy variables for compatibility
set(FLIGHT_WASM_INCLUDE_DIRS ${FLIGHT_WASM_INCLUDE_DIRS})
set(FLIGHT_WASM_LIBRARIES flight::wasm)
set(FLIGHT_WASM_FOUND TRUE)

# Check required C++17 features
check_required_components(flight-wasm)

# Validate C++17 support
if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    message(STATUS "Flight WASM: Setting C++17 standard requirement")
elseif(CMAKE_CXX_STANDARD LESS 17)
    message(FATAL_ERROR "Flight WASM requires C++17 or higher (current: ${CMAKE_CXX_STANDARD})")
endif()

# Provide usage information
if(NOT flight-wasm_FIND_QUIETLY)
    message(STATUS "Found Flight WASM: ${FLIGHT_WASM_VERSION}")
    message(STATUS "  - Header-only library: ${FLIGHT_WASM_HEADER_ONLY}")
    message(STATUS "  - WebAssembly spec version: ${FLIGHT_WASM_SPEC_VERSION}")
    message(STATUS "  - Include directories: ${FLIGHT_WASM_INCLUDE_DIRS}")
    message(STATUS "  - Usage: target_link_libraries(your_target PRIVATE flight::wasm)")
endif()

# Helper function to apply Flight WASM to a target
function(flight_wasm_configure_target target)
    if(NOT TARGET ${target})
        message(FATAL_ERROR "Target ${target} does not exist")
    endif()
    
    # Link to Flight WASM
    target_link_libraries(${target} PRIVATE flight::wasm)
    
    # Ensure C++17 standard
    target_compile_features(${target} PRIVATE cxx_std_17)
    
    # Add WebAssembly specification definitions
    target_compile_definitions(${target} PRIVATE
        FLIGHT_WASM_VERSION_MAJOR=${FLIGHT_WASM_VERSION_MAJOR}
        FLIGHT_WASM_VERSION_MINOR=${FLIGHT_WASM_VERSION_MINOR}
        FLIGHT_WASM_VERSION_PATCH=${FLIGHT_WASM_VERSION_PATCH}
        FLIGHT_WASM_SPEC_VERSION=${FLIGHT_WASM_SPEC_VERSION}
    )
    
    if(NOT flight-wasm_FIND_QUIETLY)
        message(STATUS "Flight WASM: Configured target ${target}")
    endif()
endfunction()

# Export variables for parent scope
set(FLIGHT_WASM_VERSION ${FLIGHT_WASM_VERSION} PARENT_SCOPE)
set(FLIGHT_WASM_INCLUDE_DIRS ${FLIGHT_WASM_INCLUDE_DIRS} PARENT_SCOPE)
set(FLIGHT_WASM_LIBRARIES ${FLIGHT_WASM_LIBRARIES} PARENT_SCOPE)
set(FLIGHT_WASM_FOUND ${FLIGHT_WASM_FOUND} PARENT_SCOPE)
