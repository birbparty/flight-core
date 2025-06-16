# =============================================================================
# Flight WASM Dreamcast Toolchain
# Cross-compilation toolchain for Dreamcast SH-4 architecture
# =============================================================================

# This toolchain file configures CMake for Dreamcast SH-4 compilation using KallistiOS
# Usage: cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/Dreamcast.cmake

# Set the system name and processor
set(CMAKE_SYSTEM_NAME Dreamcast)
set(CMAKE_SYSTEM_PROCESSOR sh4)

# Set the crosscompiling flag
set(CMAKE_CROSSCOMPILING TRUE)

# Specify the KallistiOS toolchain location
if(DEFINED ENV{KOS_BASE})
    set(KOS_BASE $ENV{KOS_BASE})
elseif(DEFINED ENV{KOS_ROOT})
    set(KOS_BASE $ENV{KOS_ROOT})
else()
    # Try common installation paths
    find_path(KOS_BASE
        NAMES environ.sh
        PATHS
            /opt/toolchains/dc/kos
            /usr/local/dc/kos
            ${CMAKE_CURRENT_LIST_DIR}/../../../kos
        NO_DEFAULT_PATH
    )
endif()

if(NOT KOS_BASE)
    message(FATAL_ERROR "KallistiOS not found. Please install KallistiOS and set KOS_BASE environment variable.")
endif()

# Set KallistiOS environment variables
set(ENV{KOS_BASE} ${KOS_BASE})
set(ENV{KOS_ARCH} "dreamcast")
set(ENV{KOS_SUBARCH} "")

# Determine toolchain prefix
if(DEFINED ENV{DC_TOOLCHAIN_PREFIX})
    set(TOOLCHAIN_PREFIX $ENV{DC_TOOLCHAIN_PREFIX})
else()
    set(TOOLCHAIN_PREFIX "sh-elf-")
endif()

# Find the cross compiler
find_program(CMAKE_C_COMPILER
    NAMES ${TOOLCHAIN_PREFIX}gcc sh-elf-gcc
    PATHS
        ${KOS_BASE}/../sh-elf/bin
        /opt/toolchains/dc/sh-elf/bin
        /usr/local/dc/sh-elf/bin
    NO_DEFAULT_PATH
)

find_program(CMAKE_CXX_COMPILER
    NAMES ${TOOLCHAIN_PREFIX}g++ sh-elf-g++
    PATHS
        ${KOS_BASE}/../sh-elf/bin
        /opt/toolchains/dc/sh-elf/bin
        /usr/local/dc/sh-elf/bin
    NO_DEFAULT_PATH
)

if(NOT CMAKE_C_COMPILER OR NOT CMAKE_CXX_COMPILER)
    message(FATAL_ERROR "Dreamcast SH-4 toolchain not found. Please install KallistiOS toolchain.")
endif()

# Set additional tools
find_program(CMAKE_AR
    NAMES ${TOOLCHAIN_PREFIX}ar sh-elf-ar
    PATHS
        ${KOS_BASE}/../sh-elf/bin
        /opt/toolchains/dc/sh-elf/bin
        /usr/local/dc/sh-elf/bin
    NO_DEFAULT_PATH
)

find_program(CMAKE_RANLIB
    NAMES ${TOOLCHAIN_PREFIX}ranlib sh-elf-ranlib
    PATHS
        ${KOS_BASE}/../sh-elf/bin
        /opt/toolchains/dc/sh-elf/bin
        /usr/local/dc/sh-elf/bin
    NO_DEFAULT_PATH
)

find_program(CMAKE_OBJCOPY
    NAMES ${TOOLCHAIN_PREFIX}objcopy sh-elf-objcopy
    PATHS
        ${KOS_BASE}/../sh-elf/bin
        /opt/toolchains/dc/sh-elf/bin
        /usr/local/dc/sh-elf/bin
    NO_DEFAULT_PATH
)

# Set the sysroot
set(CMAKE_SYSROOT ${KOS_BASE})

# Configure compiler behavior
set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_CXX_COMPILER_WORKS TRUE)

# Set file extensions
set(CMAKE_EXECUTABLE_SUFFIX ".elf")
set(CMAKE_STATIC_LIBRARY_SUFFIX ".a")

# Dreamcast SH-4 specific compiler flags
set(FLIGHT_WASM_DREAMCAST_FLAGS
    # SH-4 architecture flags
    -ml                     # Little endian mode
    -m4-single-only         # Single precision floating point only
    -ffunction-sections     # Enable function sections for size optimization
    -fdata-sections         # Enable data sections for size optimization
    
    # Dreamcast specific optimizations
    -ffast-math             # Fast math operations
    -fomit-frame-pointer    # Omit frame pointer for better performance
    
    # Memory constraints (16MB RAM)
    -mspace                 # Optimize for space
    -Os                     # Size optimization
    
    # Disable features not needed/available on Dreamcast
    -fno-exceptions         # No exception handling
    -fno-rtti               # No RTTI
    -fno-threadsafe-statics # No thread-safe statics
)

# Apply base flags to all build types
string(REPLACE ";" " " FLIGHT_WASM_DREAMCAST_FLAGS_STR "${FLIGHT_WASM_DREAMCAST_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLIGHT_WASM_DREAMCAST_FLAGS_STR}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLIGHT_WASM_DREAMCAST_FLAGS_STR}")

# Build type specific optimizations
set(CMAKE_C_FLAGS_DEBUG "-O0 -g3 -DDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -DDEBUG")

set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG -finline-functions -fmerge-all-constants")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG -finline-functions -fmerge-all-constants")

set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g1 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g1 -DNDEBUG")

set(CMAKE_C_FLAGS_MINSIZEREL "-Os -DNDEBUG -fmerge-all-constants")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG -fmerge-all-constants")

# KallistiOS include directories
include_directories(
    ${KOS_BASE}/include
    ${KOS_BASE}/kernel/arch/dreamcast/include
    ${KOS_BASE}/addons/libcpp
)

# KallistiOS library directories
link_directories(
    ${KOS_BASE}/lib/dreamcast
    ${KOS_BASE}/addons/libcpp
)

# Linker flags
set(FLIGHT_WASM_DREAMCAST_LINK_FLAGS
    # SH-4 linker flags
    -ml                     # Little endian
    -m4-single-only         # Single precision FPU
    
    # Optimization flags
    -Wl,--gc-sections       # Remove unused sections
    -Wl,--strip-all         # Strip all symbols
    
    # Memory layout
    -T ${KOS_BASE}/utils/ldscripts/shlelf.xc
    
    # KallistiOS specific
    -nostartfiles           # Don't use standard startup files
    -nostdlib               # Don't use standard library
)

string(REPLACE ";" " " FLIGHT_WASM_DREAMCAST_LINK_FLAGS_STR "${FLIGHT_WASM_DREAMCAST_LINK_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${FLIGHT_WASM_DREAMCAST_LINK_FLAGS_STR}")

# Standard libraries for Dreamcast
set(DREAMCAST_STANDARD_LIBRARIES
    -lkallisti              # KallistiOS main library
    -lc                     # C library
    -lgcc                   # GCC runtime
    -lm                     # Math library
)

string(REPLACE ";" " " DREAMCAST_STANDARD_LIBRARIES_STR "${DREAMCAST_STANDARD_LIBRARIES}")
set(CMAKE_C_STANDARD_LIBRARIES ${DREAMCAST_STANDARD_LIBRARIES_STR})
set(CMAKE_CXX_STANDARD_LIBRARIES ${DREAMCAST_STANDARD_LIBRARIES_STR})

# Configure find commands to search in the sysroot
set(CMAKE_FIND_ROOT_PATH ${KOS_BASE})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Platform identification
set(DREAMCAST TRUE)
set(FLIGHT_WASM_PLATFORM_DREAMCAST TRUE)
set(FLIGHT_WASM_PLATFORM_EMBEDDED TRUE)
set(FLIGHT_WASM_TARGET_PLATFORM "Dreamcast SH-4")

# Dreamcast specific definitions
add_definitions(
    -DFLIGHT_WASM_PLATFORM_DREAMCAST=1
    -DFLIGHT_WASM_PLATFORM_EMBEDDED=1
    -DFLIGHT_WASM_TARGET_SH4=1
    -DFLIGHT_WASM_MEMORY_LIMIT=16777216    # 16MB RAM limit
    -DFLIGHT_WASM_LITTLE_ENDIAN=1
    -D_arch_dreamcast=1
    -D__DREAMCAST__=1
)

# Validate toolchain
if(EXISTS "${CMAKE_C_COMPILER}")
    execute_process(
        COMMAND ${CMAKE_C_COMPILER} -dumpversion
        OUTPUT_VARIABLE GCC_VERSION
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    if(GCC_VERSION)
        message(STATUS "Found Dreamcast SH-4 GCC version: ${GCC_VERSION}")
        
        # Check minimum version requirement
        if(GCC_VERSION VERSION_LESS "4.7.0")
            message(WARNING "GCC version ${GCC_VERSION} may not be fully compatible. Recommended version: 4.7.0+")
        endif()
    endif()
endif()

# Helper function to create Dreamcast binary
function(create_dreamcast_binary target)
    # Convert ELF to Dreamcast binary format
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary ${target}.elf ${target}.bin
        COMMENT "Creating Dreamcast binary from ${target}.elf"
    )
    
    # Optionally create scrambled binary for CD-ROM
    find_program(SCRAMBLE_TOOL scramble
        PATHS ${KOS_BASE}/utils
        NO_DEFAULT_PATH
    )
    
    if(SCRAMBLE_TOOL)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${SCRAMBLE_TOOL} ${target}.bin ${target}_scrambled.bin
            DEPENDS ${target}.bin
            COMMENT "Creating scrambled binary for Dreamcast CD-ROM"
        )
    endif()
endfunction()

# Flight WASM specific validation
message(STATUS "Flight WASM Dreamcast Toolchain Configuration:")
message(STATUS "  - Target: Dreamcast SH-4")
message(STATUS "  - KallistiOS Base: ${KOS_BASE}")
message(STATUS "  - C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  - C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "  - Architecture: SH-4 (Little Endian)")
message(STATUS "  - FPU: Single Precision Only")
message(STATUS "  - Memory Limit: 16MB RAM")
message(STATUS "  - Exceptions: Disabled")
message(STATUS "  - RTTI: Disabled")
