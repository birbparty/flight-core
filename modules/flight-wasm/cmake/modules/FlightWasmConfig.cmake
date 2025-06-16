# =============================================================================
# Flight WASM Configuration Module
# Configuration helpers and validation for Flight WASM system
# =============================================================================

#[[
Provides configuration utilities and validation functions for Flight WASM.
This module centralizes configuration logic and ensures proper setup.
]]

# =============================================================================
# Configuration Validation Functions
# =============================================================================

#[[
Validates the complete Flight WASM configuration.
Checks all critical requirements are met before proceeding.
]]
function(flight_wasm_validate_configuration)
    message(STATUS "Validating Flight WASM configuration...")
    
    # Validate C++17 standard
    if(NOT CMAKE_CXX_STANDARD EQUAL 17)
        message(FATAL_ERROR "Flight WASM requires C++17 standard (found: ${CMAKE_CXX_STANDARD})")
    endif()
    
    # Validate header-only design
    if(NOT TARGET flight-wasm)
        message(FATAL_ERROR "flight-wasm target not found - ensure CMakeLists.txt is properly configured")
    endif()
    
    get_target_property(FLIGHT_WASM_TYPE flight-wasm TYPE)
    if(NOT FLIGHT_WASM_TYPE STREQUAL "INTERFACE_LIBRARY")
        message(FATAL_ERROR "flight-wasm must be an INTERFACE library for header-only design")
    endif()
    
    # Validate include directories
    get_target_property(FLIGHT_WASM_INCLUDES flight-wasm INTERFACE_INCLUDE_DIRECTORIES)
    if(NOT FLIGHT_WASM_INCLUDES)
        message(FATAL_ERROR "flight-wasm target missing include directories")
    endif()
    
    # Validate WebAssembly specification compliance
    flight_wasm_validate_webassembly_compliance()
    
    # Validate embedded platform requirements if applicable
    if(FLIGHT_WASM_PLATFORM_EMBEDDED)
        flight_wasm_validate_embedded_requirements()
    endif()
    
    message(STATUS "Flight WASM configuration validation passed ✓")
endfunction()

#[[
Validates WebAssembly specification compliance requirements.
]]
function(flight_wasm_validate_webassembly_compliance)
    # Check for required WebAssembly constants
    if(NOT DEFINED FLIGHT_WASM_SPEC_VERSION)
        message(FATAL_ERROR "FLIGHT_WASM_SPEC_VERSION not defined - ensure WebAssembly compliance")
    endif()
    
    # Validate header file exists
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/flight/wasm/wasm.hpp")
        message(FATAL_ERROR "Primary header file not found: include/flight/wasm/wasm.hpp")
    endif()
    
    # Validate WebAssembly magic number and version constants
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/include/flight/wasm/wasm.hpp" WASM_HEADER_CONTENT)
    if(NOT WASM_HEADER_CONTENT MATCHES "WASM_BINARY_MAGIC")
        message(WARNING "WASM_BINARY_MAGIC constant not found in header - ensure WebAssembly spec compliance")
    endif()
    
    if(NOT WASM_HEADER_CONTENT MATCHES "WASM_BINARY_VERSION")
        message(WARNING "WASM_BINARY_VERSION constant not found in header - ensure WebAssembly spec compliance")
    endif()
endfunction()

#[[
Validates embedded platform specific requirements.
]]
function(flight_wasm_validate_embedded_requirements)
    message(STATUS "Validating embedded platform requirements...")
    
    # Check for size optimization flags
    if(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel" OR CMAKE_BUILD_TYPE STREQUAL "Release")
        get_target_property(FLIGHT_WASM_COMPILE_OPTIONS flight-wasm INTERFACE_COMPILE_OPTIONS)
        if(FLIGHT_WASM_COMPILE_OPTIONS)
            string(JOIN " " COMPILE_OPTIONS_STR ${FLIGHT_WASM_COMPILE_OPTIONS})
            if(NOT COMPILE_OPTIONS_STR MATCHES "ffunction-sections|fdata-sections")
                message(WARNING "Embedded target missing function/data sections optimization")
            endif()
        endif()
    endif()
    
    # Validate memory constraints
    if(FLIGHT_WASM_PLATFORM_DREAMCAST)
        message(STATUS "Dreamcast memory constraints: 16MB RAM limit")
    elseif(FLIGHT_WASM_PLATFORM_PSP)
        message(STATUS "PSP memory constraints: 32MB RAM limit")
    elseif(FLIGHT_WASM_PLATFORM_PSVITA)
        message(STATUS "PS Vita memory constraints: 512MB RAM limit")
    endif()
endfunction()

# =============================================================================
# Configuration Helper Functions
# =============================================================================

#[[
Sets up Flight WASM build configuration options.
]]
function(flight_wasm_setup_build_options)
    # Development options
    option(FLIGHT_WASM_ENABLE_WARNINGS "Enable additional compiler warnings" ON)
    option(FLIGHT_WASM_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
    option(FLIGHT_WASM_ENABLE_SANITIZERS "Enable runtime sanitizers for debugging" OFF)
    
    # Performance options
    option(FLIGHT_WASM_ENABLE_LTO "Enable Link Time Optimization" ON)
    option(FLIGHT_WASM_ENABLE_NATIVE_ARCH "Enable native architecture optimizations" ON)
    
    # WebAssembly specific options
    option(FLIGHT_WASM_STRICT_SPEC_COMPLIANCE "Enable strict WebAssembly specification compliance checks" ON)
    option(FLIGHT_WASM_ENABLE_SPEC_VALIDATION "Enable runtime WebAssembly specification validation" ON)
    
    # Embedded platform options
    if(FLIGHT_WASM_PLATFORM_EMBEDDED)
        option(FLIGHT_WASM_EMBEDDED_OPTIMIZE_SIZE "Optimize for size on embedded platforms" ON)
        option(FLIGHT_WASM_EMBEDDED_NO_EXCEPTIONS "Disable C++ exceptions on embedded platforms" ON)
        option(FLIGHT_WASM_EMBEDDED_NO_RTTI "Disable C++ RTTI on embedded platforms" ON)
    endif()
endfunction()

#[[
Applies Flight WASM build configuration to the target.
]]
function(flight_wasm_apply_build_configuration target)
    # Apply warning configuration
    if(FLIGHT_WASM_ENABLE_WARNINGS)
        flight_wasm_apply_warning_configuration(${target})
    endif()
    
    # Apply sanitizer configuration
    if(FLIGHT_WASM_ENABLE_SANITIZERS AND NOT FLIGHT_WASM_PLATFORM_EMBEDDED)
        flight_wasm_apply_sanitizer_configuration(${target})
    endif()
    
    # Apply WebAssembly specification compliance
    if(FLIGHT_WASM_STRICT_SPEC_COMPLIANCE)
        flight_wasm_apply_spec_compliance_configuration(${target})
    endif()
endfunction()

#[[
Applies comprehensive warning configuration.
]]
function(flight_wasm_apply_warning_configuration target)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${target} INTERFACE
            -Wall
            -Wextra
            -Wpedantic
            -Wcast-align
            -Wcast-qual
            -Wctor-dtor-privacy
            -Wdisabled-optimization
            -Wformat=2
            -Winit-self
            -Wlogical-op
            -Wmissing-declarations
            -Wmissing-include-dirs
            -Wnoexcept
            -Wold-style-cast
            -Woverloaded-virtual
            -Wredundant-decls
            -Wshadow
            -Wsign-conversion
            -Wsign-promo
            -Wstrict-null-sentinel
            -Wstrict-overflow=5
            -Wswitch-default
            -Wundef
            -Wunreachable-code
            -Wunused
        )
        
        # GCC specific warnings
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_compile_options(${target} INTERFACE
                -Wlogical-op
                -Wnoexcept
                -Wstrict-null-sentinel
            )
        endif()
        
        # Clang specific warnings
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            target_compile_options(${target} INTERFACE
                -Wmost
                -Wextra-semi
                -Wunreachable-code-aggressive
            )
        endif()
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${target} INTERFACE
            /W4
            /permissive-
            /w14242 /w14254 /w14263 /w14265 /w14287 /we4289 /w14296 /w14311
            /w14545 /w14546 /w14547 /w14549 /w14555 /w14619 /w14640 /w14826
            /w14905 /w14906 /w14928
        )
    endif()
    
    # Warnings as errors
    if(FLIGHT_WASM_WARNINGS_AS_ERRORS)
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            target_compile_options(${target} INTERFACE -Werror)
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
            target_compile_options(${target} INTERFACE /WX)
        endif()
    endif()
endfunction()

#[[
Applies sanitizer configuration for debugging builds.
]]
function(flight_wasm_apply_sanitizer_configuration target)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            target_compile_options(${target} INTERFACE
                -fsanitize=address
                -fsanitize=undefined
                -fno-sanitize-recover=all
                -fsanitize-address-use-after-scope
            )
            target_link_options(${target} INTERFACE
                -fsanitize=address
                -fsanitize=undefined
            )
        endif()
    endif()
endfunction()

#[[
Applies WebAssembly specification compliance configuration.
]]
function(flight_wasm_apply_spec_compliance_configuration target)
    target_compile_definitions(${target} INTERFACE
        FLIGHT_WASM_STRICT_COMPLIANCE=1
        FLIGHT_WASM_ENABLE_VALIDATION=1
    )
    
    # Ensure no undefined behavior in WebAssembly context
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${target} INTERFACE
            -fno-strict-aliasing    # Avoid strict aliasing issues
            -fwrapv                 # Wrap on signed integer overflow
        )
    endif()
endfunction()

# =============================================================================
# Build Information Functions
# =============================================================================

#[[
Prints comprehensive build information for debugging.
]]
function(flight_wasm_print_build_info)
    message(STATUS "")
    message(STATUS "╔════════════════════════════════════════════════════════╗")
    message(STATUS "║                   Flight WASM Build Info               ║")
    message(STATUS "╠════════════════════════════════════════════════════════╣")
    message(STATUS "║ Project Version: ${PROJECT_VERSION}")
    message(STATUS "║ CMake Version: ${CMAKE_VERSION}")
    message(STATUS "║ Build Type: ${CMAKE_BUILD_TYPE}")
    message(STATUS "║ C++ Standard: ${CMAKE_CXX_STANDARD}")
    message(STATUS "║ Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
    message(STATUS "║ Target Platform: ${FLIGHT_WASM_TARGET_PLATFORM}")
    message(STATUS "║ Embedded Target: ${FLIGHT_WASM_PLATFORM_EMBEDDED}")
    message(STATUS "║ Install Prefix: ${CMAKE_INSTALL_PREFIX}")
    message(STATUS "╚════════════════════════════════════════════════════════╝")
    message(STATUS "")
    
    # Print feature flags
    message(STATUS "Feature Flags:")
    message(STATUS "  - Warnings Enabled: ${FLIGHT_WASM_ENABLE_WARNINGS}")
    message(STATUS "  - Warnings as Errors: ${FLIGHT_WASM_WARNINGS_AS_ERRORS}")
    message(STATUS "  - Sanitizers: ${FLIGHT_WASM_ENABLE_SANITIZERS}")
    message(STATUS "  - Link Time Optimization: ${FLIGHT_WASM_ENABLE_LTO}")
    message(STATUS "  - Strict Spec Compliance: ${FLIGHT_WASM_STRICT_SPEC_COMPLIANCE}")
    
    if(FLIGHT_WASM_PLATFORM_EMBEDDED)
        message(STATUS "  - Embedded Size Optimization: ${FLIGHT_WASM_EMBEDDED_OPTIMIZE_SIZE}")
        message(STATUS "  - Embedded No Exceptions: ${FLIGHT_WASM_EMBEDDED_NO_EXCEPTIONS}")
        message(STATUS "  - Embedded No RTTI: ${FLIGHT_WASM_EMBEDDED_NO_RTTI}")
    endif()
    
    message(STATUS "")
endfunction()

#[[
Generates a configuration summary file for debugging.
]]
function(flight_wasm_generate_config_summary)
    set(SUMMARY_FILE "${CMAKE_BINARY_DIR}/flight-wasm-config-summary.txt")
    
    file(WRITE ${SUMMARY_FILE}
        "Flight WASM Configuration Summary\n"
        "================================\n\n"
        "Project Version: ${PROJECT_VERSION}\n"
        "CMake Version: ${CMAKE_VERSION}\n"
        "Build Type: ${CMAKE_BUILD_TYPE}\n"
        "C++ Standard: ${CMAKE_CXX_STANDARD}\n"
        "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}\n"
        "Target Platform: ${FLIGHT_WASM_TARGET_PLATFORM}\n"
        "Embedded Target: ${FLIGHT_WASM_PLATFORM_EMBEDDED}\n"
        "Install Prefix: ${CMAKE_INSTALL_PREFIX}\n\n"
        
        "Feature Flags:\n"
        "- Warnings Enabled: ${FLIGHT_WASM_ENABLE_WARNINGS}\n"
        "- Warnings as Errors: ${FLIGHT_WASM_WARNINGS_AS_ERRORS}\n"
        "- Sanitizers: ${FLIGHT_WASM_ENABLE_SANITIZERS}\n"
        "- Link Time Optimization: ${FLIGHT_WASM_ENABLE_LTO}\n"
        "- Strict Spec Compliance: ${FLIGHT_WASM_STRICT_SPEC_COMPLIANCE}\n"
    )
    
    if(FLIGHT_WASM_PLATFORM_EMBEDDED)
        file(APPEND ${SUMMARY_FILE}
            "- Embedded Size Optimization: ${FLIGHT_WASM_EMBEDDED_OPTIMIZE_SIZE}\n"
            "- Embedded No Exceptions: ${FLIGHT_WASM_EMBEDDED_NO_EXCEPTIONS}\n"
            "- Embedded No RTTI: ${FLIGHT_WASM_EMBEDDED_NO_RTTI}\n"
        )
    endif()
    
    file(APPEND ${SUMMARY_FILE} "\nGenerated at: ${CMAKE_CURRENT_LIST_FILE}\n")
    
    message(STATUS "Configuration summary written to: ${SUMMARY_FILE}")
endfunction()

# =============================================================================
# Module Initialization
# =============================================================================

# Setup build options when module is loaded
flight_wasm_setup_build_options()

message(STATUS "Flight WASM Configuration Module Loaded")
