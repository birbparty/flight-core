# =============================================================================
# Flight WASM Emscripten Toolchain
# Cross-compilation toolchain for WebAssembly via Emscripten
# =============================================================================

# This toolchain file configures CMake for WebAssembly compilation using Emscripten
# Usage: cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/Emscripten.cmake

# Set the system name and processor
set(CMAKE_SYSTEM_NAME Emscripten)
set(CMAKE_SYSTEM_PROCESSOR wasm32)

# Set the crosscompiling flag
set(CMAKE_CROSSCOMPILING TRUE)

# Specify the Emscripten toolchain location
if(DEFINED ENV{EMSCRIPTEN})
    set(EMSCRIPTEN_ROOT_PATH $ENV{EMSCRIPTEN})
elseif(DEFINED ENV{EMSDK})
    set(EMSCRIPTEN_ROOT_PATH $ENV{EMSDK}/upstream/emscripten)
else()
    # Try common installation paths
    find_path(EMSCRIPTEN_ROOT_PATH
        NAMES emcc emcc.py
        PATHS
            /usr/local/emscripten
            /opt/emscripten
            /usr/share/emscripten
            ${CMAKE_CURRENT_LIST_DIR}/../../../emsdk/upstream/emscripten
        NO_DEFAULT_PATH
    )
endif()

if(NOT EMSCRIPTEN_ROOT_PATH)
    message(FATAL_ERROR "Emscripten not found. Please install Emscripten SDK and set EMSCRIPTEN or EMSDK environment variable.")
endif()

# Set compilers
set(CMAKE_C_COMPILER "${EMSCRIPTEN_ROOT_PATH}/emcc")
set(CMAKE_CXX_COMPILER "${EMSCRIPTEN_ROOT_PATH}/em++")
set(CMAKE_AR "${EMSCRIPTEN_ROOT_PATH}/emar" CACHE FILEPATH "Emscripten ar")
set(CMAKE_RANLIB "${EMSCRIPTEN_ROOT_PATH}/emranlib" CACHE FILEPATH "Emscripten ranlib")

# Set the sysroot
set(CMAKE_SYSROOT "${EMSCRIPTEN_ROOT_PATH}/system")

# Configure compiler behavior
set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_CXX_COMPILER_WORKS TRUE)

# Set file extensions
set(CMAKE_EXECUTABLE_SUFFIX ".js")
set(CMAKE_STATIC_LIBRARY_SUFFIX ".a")

# Flight WASM specific WebAssembly optimizations
set(FLIGHT_WASM_EMSCRIPTEN_FLAGS
    # WebAssembly specific flags
    -s WASM=1
    -s ALLOW_MEMORY_GROWTH=1
    -s MODULARIZE=1
    -s EXPORT_ES6=1
    
    # Memory management
    -s TOTAL_MEMORY=16MB
    -s ALLOW_TABLE_GROWTH=1
    
    # Exception handling (disabled for performance)
    -s DISABLE_EXCEPTION_CATCHING=1
    -fno-exceptions
    
    # RTTI (disabled for size)
    -fno-rtti
    
    # Optimization flags
    -ffast-math
    -ffunction-sections
    -fdata-sections
)

# Apply base flags to all build types
string(REPLACE ";" " " FLIGHT_WASM_EMSCRIPTEN_FLAGS_STR "${FLIGHT_WASM_EMSCRIPTEN_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLIGHT_WASM_EMSCRIPTEN_FLAGS_STR}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLIGHT_WASM_EMSCRIPTEN_FLAGS_STR}")

# Build type specific optimizations
set(CMAKE_C_FLAGS_DEBUG "-O0 -g3 -gsource-map")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -gsource-map")

set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG --closure=1 -s AGGRESSIVE_VARIABLE_ELIMINATION=1")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG --closure=1 -s AGGRESSIVE_VARIABLE_ELIMINATION=1")

set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g1 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g1 -DNDEBUG")

set(CMAKE_C_FLAGS_MINSIZEREL "-Oz -DNDEBUG -s AGGRESSIVE_VARIABLE_ELIMINATION=1")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Oz -DNDEBUG -s AGGRESSIVE_VARIABLE_ELIMINATION=1")

# Linker flags
set(FLIGHT_WASM_EMSCRIPTEN_LINK_FLAGS
    # WebAssembly linker flags
    -s WASM=1
    -s MODULARIZE=1
    -s EXPORT_ES6=1
    
    # Memory and table settings
    -s ALLOW_MEMORY_GROWTH=1
    -s ALLOW_TABLE_GROWTH=1
    
    # Remove unused functions
    -s ELIMINATE_DUPLICATE_FUNCTIONS=1
    
    # Disable filesystem if not needed
    -s NO_FILESYSTEM=1
    
    # Minimal runtime
    -s MINIMAL_RUNTIME=1
)

string(REPLACE ";" " " FLIGHT_WASM_EMSCRIPTEN_LINK_FLAGS_STR "${FLIGHT_WASM_EMSCRIPTEN_LINK_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${FLIGHT_WASM_EMSCRIPTEN_LINK_FLAGS_STR}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${FLIGHT_WASM_EMSCRIPTEN_LINK_FLAGS_STR}")

# Build type specific linker flags
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} --closure=1")
set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} --closure=1")

# Configure find commands to search in the sysroot
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Set cache variables
set(CMAKE_CROSSCOMPILING_EMULATOR "" CACHE FILEPATH "Path to emulator for cross-compiled executables")

# Platform identification
set(EMSCRIPTEN TRUE)
set(FLIGHT_WASM_PLATFORM_EMSCRIPTEN TRUE)
set(FLIGHT_WASM_PLATFORM_EMBEDDED TRUE)
set(FLIGHT_WASM_TARGET_PLATFORM "Emscripten/WebAssembly")

# WebAssembly specific definitions
add_definitions(
    -DFLIGHT_WASM_PLATFORM_EMSCRIPTEN=1
    -DFLIGHT_WASM_PLATFORM_EMBEDDED=1
    -DFLIGHT_WASM_TARGET_WEBASSEMBLY=1
    -DFLIGHT_WASM_WEBASSEMBLY_SPEC_VERSION=1
)

# Validate Emscripten version
if(EXISTS "${CMAKE_C_COMPILER}")
    execute_process(
        COMMAND ${CMAKE_C_COMPILER} --version
        OUTPUT_VARIABLE EMSCRIPTEN_VERSION_OUTPUT
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    if(EMSCRIPTEN_VERSION_OUTPUT MATCHES "emcc \\(Emscripten gcc/clang-like replacement\\) ([0-9]+\\.[0-9]+\\.[0-9]+)")
        set(EMSCRIPTEN_VERSION ${CMAKE_MATCH_1})
        message(STATUS "Found Emscripten version: ${EMSCRIPTEN_VERSION}")
        
        # Check minimum version requirement
        if(EMSCRIPTEN_VERSION VERSION_LESS "2.0.0")
            message(WARNING "Emscripten version ${EMSCRIPTEN_VERSION} may not be fully compatible. Recommended version: 2.0.0+")
        endif()
    endif()
endif()

# Flight WASM specific WebAssembly validation
message(STATUS "Flight WASM Emscripten Toolchain Configuration:")
message(STATUS "  - Target: WebAssembly (WASM)")
message(STATUS "  - Emscripten Root: ${EMSCRIPTEN_ROOT_PATH}")
message(STATUS "  - C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  - C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "  - Memory Growth: Enabled")
message(STATUS "  - Exceptions: Disabled (for performance)")
message(STATUS "  - RTTI: Disabled (for size)")
message(STATUS "  - Closure Compiler: Enabled (Release builds)")
