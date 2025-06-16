# =============================================================================
# Flight WASM PSP Toolchain
# Cross-compilation toolchain for PlayStation Portable MIPS architecture
# =============================================================================

# Set system info
set(CMAKE_SYSTEM_NAME PSP)
set(CMAKE_SYSTEM_PROCESSOR mipsallegrexel)
set(CMAKE_CROSSCOMPILING TRUE)

# Find PSPSDK
if(DEFINED ENV{PSPSDK})
    set(PSPSDK $ENV{PSPSDK})
elseif(DEFINED ENV{PSP_SDK_PATH})
    set(PSPSDK $ENV{PSP_SDK_PATH})
else()
    find_path(PSPSDK
        NAMES include/pspkernel.h
        PATHS /usr/local/pspdev/psp /opt/pspsdk
        NO_DEFAULT_PATH
    )
endif()

if(NOT PSPSDK)
    message(FATAL_ERROR "PSPSDK not found. Install PSPSDK and set PSPSDK environment variable.")
endif()

# Set toolchain
set(TOOLCHAIN_PREFIX "psp-")
find_program(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc)
find_program(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
find_program(CMAKE_AR ${TOOLCHAIN_PREFIX}ar)
find_program(CMAKE_RANLIB ${TOOLCHAIN_PREFIX}ranlib)

if(NOT CMAKE_C_COMPILER OR NOT CMAKE_CXX_COMPILER)
    message(FATAL_ERROR "PSP toolchain not found. Install PSPSDK.")
endif()

# Compiler flags
set(PSP_FLAGS
    -G0 -mno-gpopt -mips2 -mabi=eabi -march=allegrex
    -ffunction-sections -fdata-sections -ffast-math
    -fno-exceptions -fno-rtti -Os
)

string(REPLACE ";" " " PSP_FLAGS_STR "${PSP_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${PSP_FLAGS_STR}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PSP_FLAGS_STR}")

# Build configurations
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_C_FLAGS_MINSIZEREL "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG")

# Include/library paths
include_directories(${PSPSDK}/include)
link_directories(${PSPSDK}/lib)

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS "-Wl,--gc-sections")

# Platform definitions
set(PSP TRUE)
set(FLIGHT_WASM_PLATFORM_PSP TRUE)
set(FLIGHT_WASM_PLATFORM_EMBEDDED TRUE)
set(FLIGHT_WASM_TARGET_PLATFORM "PlayStation Portable MIPS")

add_definitions(
    -DFLIGHT_WASM_PLATFORM_PSP=1
    -DFLIGHT_WASM_PLATFORM_EMBEDDED=1
    -DFLIGHT_WASM_MEMORY_LIMIT=33554432  # 32MB
    -D__PSP__=1
)

message(STATUS "Flight WASM PSP Toolchain:")
message(STATUS "  - PSPSDK: ${PSPSDK}")
message(STATUS "  - Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "  - Architecture: MIPS Allegrex")
message(STATUS "  - Memory: 32MB limit")
