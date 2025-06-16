# =============================================================================
# Flight WASM Compiler Optimizations Module
# GCC 7+ and Clang 5+ specific performance optimizations
# =============================================================================

#[[
Applies compiler-specific optimizations to the Flight WASM target.
Supports GCC 7+ and Clang 5+ with embedded platform considerations.

Parameters:
  target - The CMake target to apply optimizations to
]]
function(flight_wasm_apply_compiler_optimizations target)
    # Validate compiler versions early
    flight_wasm_validate_compiler_version()
    
    # Apply base optimizations for all compilers
    flight_wasm_apply_base_optimizations(${target})
    
    # Apply compiler-specific optimizations
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        flight_wasm_apply_gcc_optimizations(${target})
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        flight_wasm_apply_clang_optimizations(${target})
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        flight_wasm_apply_msvc_optimizations(${target})
    else()
        message(WARNING "Compiler ${CMAKE_CXX_COMPILER_ID} not explicitly supported - using generic optimizations")
        flight_wasm_apply_generic_optimizations(${target})
    endif()
    
    # Apply platform-specific optimizations
    flight_wasm_apply_platform_optimizations(${target})
    
endfunction()

#[[
Validates that compiler versions meet Flight WASM requirements.
Fails configuration if compiler is too old.
]]
function(flight_wasm_validate_compiler_version)
    set(COMPILER_VALID FALSE)
    
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "7.0")
            message(FATAL_ERROR "Flight WASM requires GCC 7.0 or higher (found ${CMAKE_CXX_COMPILER_VERSION})")
        else()
            set(COMPILER_VALID TRUE)
            message(STATUS "GCC ${CMAKE_CXX_COMPILER_VERSION} meets requirements (≥7.0)")
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "5.0")
            message(FATAL_ERROR "Flight WASM requires Clang 5.0 or higher (found ${CMAKE_CXX_COMPILER_VERSION})")
        else()
            set(COMPILER_VALID TRUE)
            message(STATUS "Clang ${CMAKE_CXX_COMPILER_VERSION} meets requirements (≥5.0)")
        endif()
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "19.14")
            message(FATAL_ERROR "Flight WASM requires MSVC 2017 or higher (found ${CMAKE_CXX_COMPILER_VERSION})")
        else()
            set(COMPILER_VALID TRUE)
            message(STATUS "MSVC ${CMAKE_CXX_COMPILER_VERSION} meets requirements (≥19.14)")
        endif()
    endif()
    
    if(NOT COMPILER_VALID)
        message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
    endif()
endfunction()

#[[
Applies base optimizations common to all compilers.
]]
function(flight_wasm_apply_base_optimizations target)
    # Debug build optimizations
    target_compile_options(${target} INTERFACE
        $<$<CONFIG:Debug>:-Og>          # Optimize debugging experience
        $<$<CONFIG:Debug>:-g3>          # Maximum debug information
        $<$<CONFIG:Debug>:-fno-omit-frame-pointer>  # Keep frame pointers for debugging
    )
    
    # Release build optimizations
    target_compile_options(${target} INTERFACE
        $<$<CONFIG:Release>:-O3>        # Maximum optimization
        $<$<CONFIG:Release>:-DNDEBUG>   # Disable assertions
        $<$<CONFIG:Release>:-fomit-frame-pointer>  # Remove frame pointers
    )
    
    # RelWithDebInfo optimizations
    target_compile_options(${target} INTERFACE
        $<$<CONFIG:RelWithDebInfo>:-O2>
        $<$<CONFIG:RelWithDebInfo>:-g1>
        $<$<CONFIG:RelWithDebInfo>:-DNDEBUG>
    )
    
    # MinSizeRel optimizations (important for embedded)
    target_compile_options(${target} INTERFACE
        $<$<CONFIG:MinSizeRel>:-Os>     # Optimize for size
        $<$<CONFIG:MinSizeRel>:-DNDEBUG>
        $<$<CONFIG:MinSizeRel>:-ffunction-sections>
        $<$<CONFIG:MinSizeRel>:-fdata-sections>
    )
    
    # Header-only library specific optimizations
    target_compile_definitions(${target} INTERFACE
        $<$<CONFIG:Release>:FLIGHT_WASM_FORCE_INLINE=1>
        $<$<CONFIG:MinSizeRel>:FLIGHT_WASM_OPTIMIZE_SIZE=1>
    )
endfunction()

#[[
Applies GCC-specific optimizations (GCC 7+).
]]
function(flight_wasm_apply_gcc_optimizations target)
    message(STATUS "Applying GCC-specific optimizations")
    
    # GCC-specific performance flags
    target_compile_options(${target} INTERFACE
        # Release optimizations
        $<$<CONFIG:Release>:-ffast-math>        # Aggressive math optimizations
        $<$<CONFIG:Release>:-funroll-loops>     # Loop unrolling
        $<$<CONFIG:Release>:-fvect-cost-model=dynamic>  # Vectorization cost model
        $<$<CONFIG:Release>:-fno-signed-zeros>  # Assume no signed zeros
        $<$<CONFIG:Release>:-fno-trapping-math> # Assume no math traps
        
        # Link-time optimization
        $<$<CONFIG:Release>:-flto>
        $<$<CONFIG:Release>:-fuse-linker-plugin>
        
        # MinSizeRel optimizations
        $<$<CONFIG:MinSizeRel>:-flto>
        $<$<CONFIG:MinSizeRel>:-fuse-linker-plugin>
    )
    
    # GCC 8+ specific optimizations
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "8.0")
        target_compile_options(${target} INTERFACE
            $<$<CONFIG:Release>:-fstack-clash-protection>  # Stack protection
        )
    endif()
    
    # GCC 9+ specific optimizations
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "9.0")
        target_compile_options(${target} INTERFACE
            $<$<CONFIG:Release>:-fipa-pta>  # Inter-procedural pointer analysis
        )
    endif()
    
    # Architecture-specific optimizations for host builds
    if(NOT FLIGHT_WASM_PLATFORM_EMBEDDED)
        target_compile_options(${target} INTERFACE
            $<$<CONFIG:Release>:-march=native>  # Native CPU optimization
            $<$<CONFIG:Release>:-mtune=native>  # Native CPU tuning
        )
    endif()
endfunction()

#[[
Applies Clang-specific optimizations (Clang 5+).
]]
function(flight_wasm_apply_clang_optimizations target)
    message(STATUS "Applying Clang-specific optimizations")
    
    # Clang-specific performance flags
    target_compile_options(${target} INTERFACE
        # Release optimizations
        $<$<CONFIG:Release>:-ffast-math>        # Aggressive math optimizations
        $<$<CONFIG:Release>:-fvectorize>        # Enable vectorization
        $<$<CONFIG:Release>:-fslp-vectorize>    # Enable SLP vectorization
        $<$<CONFIG:Release>:-funroll-loops>     # Loop unrolling
        
        # Link-time optimization
        $<$<CONFIG:Release>:-flto=thin>         # Thin LTO for faster builds
        
        # MinSizeRel optimizations
        $<$<CONFIG:MinSizeRel>:-flto=thin>
        $<$<CONFIG:MinSizeRel>:-Oz>             # Clang's aggressive size optimization
    )
    
    # Clang 6+ specific optimizations
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "6.0")
        target_compile_options(${target} INTERFACE
            $<$<CONFIG:Release>:-fmerge-all-constants>  # Merge identical constants
        )
    endif()
    
    # Architecture-specific optimizations for host builds
    if(NOT FLIGHT_WASM_PLATFORM_EMBEDDED)
        target_compile_options(${target} INTERFACE
            $<$<CONFIG:Release>:-march=native>  # Native CPU optimization
        )
    endif()
    
    # Clang static analyzer support
    option(FLIGHT_WASM_ENABLE_CLANG_ANALYZER "Enable Clang static analyzer" OFF)
    if(FLIGHT_WASM_ENABLE_CLANG_ANALYZER)
        target_compile_options(${target} INTERFACE
            --analyze
            -Xanalyzer -analyzer-output=html
        )
    endif()
endfunction()

#[[
Applies MSVC-specific optimizations.
]]
function(flight_wasm_apply_msvc_optimizations target)
    message(STATUS "Applying MSVC-specific optimizations")
    
    target_compile_options(${target} INTERFACE
        # Release optimizations
        $<$<CONFIG:Release>:/O2>        # Maximum optimization
        $<$<CONFIG:Release>:/Ob2>       # Inline function expansion
        $<$<CONFIG:Release>:/Ot>        # Favor speed over size
        $<$<CONFIG:Release>:/GL>        # Whole program optimization
        
        # MinSizeRel optimizations
        $<$<CONFIG:MinSizeRel>:/O1>     # Minimize size
        $<$<CONFIG:MinSizeRel>:/Os>     # Favor size
        $<$<CONFIG:MinSizeRel>:/GL>     # Whole program optimization
    )
    
    # MSVC-specific definitions
    target_compile_definitions(${target} INTERFACE
        $<$<CONFIG:Release>:_SECURE_SCL=0>      # Disable iterator debugging
        $<$<CONFIG:Release>:_HAS_ITERATOR_DEBUGGING=0>
    )
endfunction()

#[[
Applies generic optimizations for unknown compilers.
]]
function(flight_wasm_apply_generic_optimizations target)
    message(STATUS "Applying generic compiler optimizations")
    
    # Basic optimization flags that most compilers support
    target_compile_options(${target} INTERFACE
        $<$<CONFIG:Release>:-O2>        # Standard optimization
        $<$<CONFIG:MinSizeRel>:-Os>     # Size optimization
    )
endfunction()

#[[
Applies platform-specific optimizations.
]]
function(flight_wasm_apply_platform_optimizations target)
    if(FLIGHT_WASM_PLATFORM_EMSCRIPTEN)
        # Emscripten/WebAssembly specific optimizations
        target_compile_options(${target} INTERFACE
            $<$<CONFIG:Release>:-O3>
            $<$<CONFIG:Release>:--closure=1>        # Closure compiler
            $<$<CONFIG:MinSizeRel>:-Oz>
            $<$<CONFIG:MinSizeRel>:--closure=1>
        )
        
        # WebAssembly-specific flags
        target_link_options(${target} INTERFACE
            $<$<CONFIG:Release>:-s AGGRESSIVE_VARIABLE_ELIMINATION=1>
            $<$<CONFIG:Release>:-s ELIMINATE_DUPLICATE_FUNCTIONS=1>
            $<$<CONFIG:MinSizeRel>:-s AGGRESSIVE_VARIABLE_ELIMINATION=1>
            $<$<CONFIG:MinSizeRel>:-s ELIMINATE_DUPLICATE_FUNCTIONS=1>
        )
        
    elseif(FLIGHT_WASM_PLATFORM_DREAMCAST)
        # Dreamcast SH-4 specific optimizations
        target_compile_options(${target} INTERFACE
            $<$<CONFIG:Release>:-m4-single-only>    # Single precision floating point
            $<$<CONFIG:Release>:-ffast-math>
            $<$<CONFIG:MinSizeRel>:-m4-single-only>
        )
        
    elseif(FLIGHT_WASM_PLATFORM_PSP)
        # PSP MIPS specific optimizations
        target_compile_options(${target} INTERFACE
            $<$<CONFIG:Release>:-mips2>             # MIPS II instruction set
            $<$<CONFIG:Release>:-mno-gpopt>         # Disable GP optimization
            $<$<CONFIG:MinSizeRel>:-mips2>
            $<$<CONFIG:MinSizeRel>:-mno-gpopt>
        )
        
    elseif(FLIGHT_WASM_PLATFORM_PSVITA)
        # PS Vita ARM specific optimizations
        target_compile_options(${target} INTERFACE
            $<$<CONFIG:Release>:-mcpu=cortex-a9>    # Cortex-A9 specific
            $<$<CONFIG:Release>:-mfpu=neon>         # NEON SIMD
            $<$<CONFIG:Release>:-mfloat-abi=hard>   # Hardware floating point
            $<$<CONFIG:MinSizeRel>:-mcpu=cortex-a9>
            $<$<CONFIG:MinSizeRel>:-mfpu=neon>
            $<$<CONFIG:MinSizeRel>:-mfloat-abi=hard>
        )
    endif()
    
    # Embedded platform common optimizations
    if(FLIGHT_WASM_PLATFORM_EMBEDDED)
        target_compile_options(${target} INTERFACE
            # Size optimizations for embedded targets
            $<$<CONFIG:Release>:-ffunction-sections>
            $<$<CONFIG:Release>:-fdata-sections>
            
            # Reduce binary size
            $<$<CONFIG:MinSizeRel>:-fno-unwind-tables>
            $<$<CONFIG:MinSizeRel>:-fno-asynchronous-unwind-tables>
        )
        
        # Linker optimizations for embedded
        target_link_options(${target} INTERFACE
            $<$<CONFIG:Release>:-Wl,--gc-sections>  # Remove unused sections
            $<$<CONFIG:MinSizeRel>:-Wl,--gc-sections>
            $<$<CONFIG:MinSizeRel>:-Wl,--strip-all> # Strip all symbols
        )
    endif()
endfunction()

#[[
Prints compiler optimization summary for debugging.
]]
function(flight_wasm_print_optimization_summary)
    message(STATUS "=== Flight WASM Compiler Optimization Summary ===")
    message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
    message(STATUS "Target Platform: ${FLIGHT_WASM_TARGET_PLATFORM}")
    message(STATUS "Embedded Target: ${FLIGHT_WASM_PLATFORM_EMBEDDED}")
    
    if(CMAKE_BUILD_TYPE)
        message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
        if(CMAKE_BUILD_TYPE STREQUAL "Release")
            message(STATUS "Optimizations: Maximum performance (-O3, LTO)")
        elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
            message(STATUS "Optimizations: Minimum size (-Os/-Oz)")
        elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
            message(STATUS "Optimizations: Debug-friendly (-Og, -g3)")
        endif()
    endif()
    
    message(STATUS "==================================================")
endfunction()

# Load optimization summary when module is included
flight_wasm_print_optimization_summary()

message(STATUS "Flight WASM Compiler Optimizations Module Loaded")
