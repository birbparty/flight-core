# =============================================================================
# Flight WASM Platform Detection Module
# Detects embedded platforms and cross-compilation environments
# =============================================================================

# Initialize platform detection variables
set(FLIGHT_WASM_PLATFORM_EMSCRIPTEN FALSE)
set(FLIGHT_WASM_PLATFORM_DREAMCAST FALSE)
set(FLIGHT_WASM_PLATFORM_PSP FALSE)
set(FLIGHT_WASM_PLATFORM_PSVITA FALSE)
set(FLIGHT_WASM_PLATFORM_EMBEDDED FALSE)
set(FLIGHT_WASM_TARGET_PLATFORM "Unknown")

#[[
Detects the target platform for Flight WASM compilation.
Sets platform-specific variables and configures build environment.

Platform Variables Set:
- FLIGHT_WASM_PLATFORM_EMSCRIPTEN: Building for WebAssembly via Emscripten
- FLIGHT_WASM_PLATFORM_DREAMCAST: Building for Dreamcast SH-4
- FLIGHT_WASM_PLATFORM_PSP: Building for PlayStation Portable MIPS
- FLIGHT_WASM_PLATFORM_PSVITA: Building for PlayStation Vita ARM
- FLIGHT_WASM_PLATFORM_EMBEDDED: Any embedded platform detected
- FLIGHT_WASM_TARGET_PLATFORM: Human-readable platform name
]]
function(flight_wasm_detect_platform)
    # Emscripten WebAssembly Detection
    if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten" OR EMSCRIPTEN)
        set(FLIGHT_WASM_PLATFORM_EMSCRIPTEN TRUE PARENT_SCOPE)
        set(FLIGHT_WASM_PLATFORM_EMBEDDED TRUE PARENT_SCOPE)
        set(FLIGHT_WASM_TARGET_PLATFORM "Emscripten/WebAssembly" PARENT_SCOPE)
        
        # Emscripten-specific validations
        if(NOT DEFINED EMSCRIPTEN_VERSION)
            message(WARNING "Emscripten version not detected - ensure proper toolchain setup")
        endif()
        
        # Set WebAssembly specific configurations
        set(CMAKE_EXECUTABLE_SUFFIX ".js" PARENT_SCOPE)
        
    # Dreamcast SH-4 Detection
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Dreamcast" OR 
           CMAKE_SYSTEM_PROCESSOR MATCHES "sh[0-9]" OR
           CMAKE_C_COMPILER MATCHES "sh-elf" OR
           CMAKE_CXX_COMPILER MATCHES "sh-elf")
        
        set(FLIGHT_WASM_PLATFORM_DREAMCAST TRUE PARENT_SCOPE)
        set(FLIGHT_WASM_PLATFORM_EMBEDDED TRUE PARENT_SCOPE)
        set(FLIGHT_WASM_TARGET_PLATFORM "Dreamcast SH-4" PARENT_SCOPE)
        
        # Dreamcast-specific validations
        if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "sh")
            message(WARNING "Dreamcast target detected but processor not set to SH architecture")
        endif()
        
    # PlayStation Portable MIPS Detection
    elseif(CMAKE_SYSTEM_NAME STREQUAL "PSP" OR
           CMAKE_C_COMPILER MATCHES "psp-gcc" OR
           CMAKE_CXX_COMPILER MATCHES "psp-g\\+\\+" OR
           DEFINED PSP_SDK_PATH)
        
        set(FLIGHT_WASM_PLATFORM_PSP TRUE PARENT_SCOPE)
        set(FLIGHT_WASM_PLATFORM_EMBEDDED TRUE PARENT_SCOPE)
        set(FLIGHT_WASM_TARGET_PLATFORM "PlayStation Portable MIPS" PARENT_SCOPE)
        
        # PSP-specific validations
        if(NOT DEFINED PSP_SDK_PATH AND NOT DEFINED PSPSDK)
            message(WARNING "PSP target detected but PSPSDK path not found")
        endif()
        
    # PlayStation Vita ARM Detection
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Vita" OR
           CMAKE_C_COMPILER MATCHES "arm-vita-eabi" OR
           CMAKE_CXX_COMPILER MATCHES "arm-vita-eabi" OR
           DEFINED VITA_SDK_PATH)
        
        set(FLIGHT_WASM_PLATFORM_PSVITA TRUE PARENT_SCOPE)
        set(FLIGHT_WASM_PLATFORM_EMBEDDED TRUE PARENT_SCOPE)
        set(FLIGHT_WASM_TARGET_PLATFORM "PlayStation Vita ARM" PARENT_SCOPE)
        
        # PS Vita-specific validations
        if(NOT DEFINED VITA_SDK_PATH AND NOT DEFINED VITASDK)
            message(WARNING "PS Vita target detected but VitaSDK path not found")
        endif()
        
    # Host Platform Detection
    else()
        set(FLIGHT_WASM_TARGET_PLATFORM "${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}" PARENT_SCOPE)
        
        # Validate host platform support
        if(NOT CMAKE_SYSTEM_NAME MATCHES "Linux|Darwin|Windows")
            message(WARNING "Host platform ${CMAKE_SYSTEM_NAME} may not be fully supported")
        endif()
    endif()
    
    # Set compiler architecture flags for embedded platforms
    if(FLIGHT_WASM_PLATFORM_EMBEDDED)
        # Embedded platforms require specific optimizations
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Os -ffunction-sections -fdata-sections" PARENT_SCOPE)
        set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -Os -ffunction-sections -fdata-sections" PARENT_SCOPE)
        
        # Disable exceptions and RTTI for embedded targets (optional)
        option(FLIGHT_WASM_EMBEDDED_NO_EXCEPTIONS "Disable C++ exceptions for embedded targets" ON)
        option(FLIGHT_WASM_EMBEDDED_NO_RTTI "Disable C++ RTTI for embedded targets" ON)
        
        if(FLIGHT_WASM_EMBEDDED_NO_EXCEPTIONS)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions" PARENT_SCOPE)
        endif()
        
        if(FLIGHT_WASM_EMBEDDED_NO_RTTI)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti" PARENT_SCOPE)
        endif()
    endif()
    
    # Export all platform variables to parent scope
    set(FLIGHT_WASM_PLATFORM_EMSCRIPTEN ${FLIGHT_WASM_PLATFORM_EMSCRIPTEN} PARENT_SCOPE)
    set(FLIGHT_WASM_PLATFORM_DREAMCAST ${FLIGHT_WASM_PLATFORM_DREAMCAST} PARENT_SCOPE)
    set(FLIGHT_WASM_PLATFORM_PSP ${FLIGHT_WASM_PLATFORM_PSP} PARENT_SCOPE)
    set(FLIGHT_WASM_PLATFORM_PSVITA ${FLIGHT_WASM_PLATFORM_PSVITA} PARENT_SCOPE)
    set(FLIGHT_WASM_PLATFORM_EMBEDDED ${FLIGHT_WASM_PLATFORM_EMBEDDED} PARENT_SCOPE)
    
endfunction()

#[[
Validates that the detected platform has necessary toolchain components.
Returns error messages if critical components are missing.
]]
function(flight_wasm_validate_platform_toolchain)
    if(FLIGHT_WASM_PLATFORM_EMSCRIPTEN)
        # Check for Emscripten tools
        find_program(EMCC_EXECUTABLE emcc)
        find_program(EMCXX_EXECUTABLE em++)
        if(NOT EMCC_EXECUTABLE OR NOT EMCXX_EXECUTABLE)
            message(FATAL_ERROR "Emscripten toolchain not found - install Emscripten SDK")
        endif()
        
    elseif(FLIGHT_WASM_PLATFORM_DREAMCAST)
        # Check for KallistiOS toolchain
        if(NOT CMAKE_C_COMPILER MATCHES "sh.*elf" OR NOT CMAKE_CXX_COMPILER MATCHES "sh.*elf")
            message(FATAL_ERROR "Dreamcast SH-4 toolchain not found - install KallistiOS")
        endif()
        
    elseif(FLIGHT_WASM_PLATFORM_PSP)
        # Check for PSPSDK
        if(NOT DEFINED PSP_SDK_PATH AND NOT DEFINED PSPSDK AND NOT EXISTS "/usr/local/pspdev")
            message(FATAL_ERROR "PSPSDK not found - install PlayStation Portable SDK")
        endif()
        
    elseif(FLIGHT_WASM_PLATFORM_PSVITA)
        # Check for VitaSDK
        if(NOT DEFINED VITA_SDK_PATH AND NOT DEFINED VITASDK AND NOT EXISTS "/usr/local/vitadev")
            message(FATAL_ERROR "VitaSDK not found - install PlayStation Vita SDK")
        endif()
    endif()
endfunction()

#[[
Sets up platform-specific include directories and library paths.
Call this after platform detection to configure environment.
]]
function(flight_wasm_setup_platform_environment)
    if(FLIGHT_WASM_PLATFORM_PSP AND DEFINED PSPSDK)
        include_directories("${PSPSDK}/include")
        link_directories("${PSPSDK}/lib")
    elseif(FLIGHT_WASM_PLATFORM_PSVITA AND DEFINED VITASDK)
        include_directories("${VITASDK}/arm-vita-eabi/include")
        link_directories("${VITASDK}/arm-vita-eabi/lib")
    endif()
endfunction()

# Provide information about supported platforms
message(STATUS "Flight WASM Platform Detection Module Loaded")
message(STATUS "Supported Embedded Platforms:")
message(STATUS "  - Emscripten/WebAssembly")
message(STATUS "  - Dreamcast SH-4 (KallistiOS)")
message(STATUS "  - PlayStation Portable MIPS (PSPSDK)")
message(STATUS "  - PlayStation Vita ARM (VitaSDK)")
