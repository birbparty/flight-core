# =============================================================================
# Flight WASM PS Vita Toolchain
# Cross-compilation toolchain for PlayStation Vita ARM architecture
# =============================================================================

# Set system info
set(CMAKE_SYSTEM_NAME Vita)
set(CMAKE_SYSTEM_PROCESSOR armv7)
set(CMAKE_CROSSCOMPILING TRUE)

# Find VitaSDK
if(DEFINED ENV{VITASDK})
    set(VITASDK $ENV{VITASDK})
elseif(DEFINED ENV{VITA_SDK_PATH})
    set(VITASDK $ENV{VITA_SDK_PATH})
else()
    find_path(VITASDK
        NAMES bin/arm-vita-eabi-gcc
        PATHS /usr/local/vitadev /opt/vitasdk
        NO_DEFAULT_PATH
    )
endif()

if(NOT VITASDK)
    message(FATAL_ERROR "VitaSDK not found. Install VitaSDK and set VITASDK environment variable.")
endif()

# Set toolchain
set(TOOLCHAIN_PREFIX "arm-vita-eabi-")
set(CMAKE_C_COMPILER "${VITASDK}/bin/${TOOLCHAIN_PREFIX}gcc")
set(CMAKE_CXX_COMPILER "${VITASDK}/bin/${TOOLCHAIN_PREFIX}g++")
set(CMAKE_AR "${VITASDK}/bin/${TOOLCHAIN_PREFIX}ar")
set(CMAKE_RANLIB "${VITASDK}/bin/${TOOLCHAIN_PREFIX}ranlib")

if(NOT EXISTS ${CMAKE_C_COMPILER})
    message(FATAL_ERROR "PS Vita toolchain not found. Install VitaSDK.")
endif()

# Compiler flags
set(VITA_FLAGS
    -mcpu=cortex-a9 -mfpu=neon -mfloat-abi=hard
    -ffunction-sections -fdata-sections -ffast-math
    -fno-exceptions -fno-rtti -Os
)

string(REPLACE ";" " " VITA_FLAGS_STR "${VITA_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${VITA_FLAGS_STR}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${VITA_FLAGS_STR}")

# Build configurations
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_C_FLAGS_MINSIZEREL "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG")

# Include/library paths
include_directories(${VITASDK}/arm-vita-eabi/include)
link_directories(${VITASDK}/arm-vita-eabi/lib)

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS "-Wl,--gc-sections")

# Platform definitions
set(VITA TRUE)
set(FLIGHT_WASM_PLATFORM_PSVITA TRUE)
set(FLIGHT_WASM_PLATFORM_EMBEDDED TRUE)
set(FLIGHT_WASM_TARGET_PLATFORM "PlayStation Vita ARM")

add_definitions(
    -DFLIGHT_WASM_PLATFORM_PSVITA=1
    -DFLIGHT_WASM_PLATFORM_EMBEDDED=1
    -DFLIGHT_WASM_MEMORY_LIMIT=536870912  # 512MB
    -D__vita__=1
)

message(STATUS "Flight WASM PS Vita Toolchain:")
message(STATUS "  - VitaSDK: ${VITASDK}")
message(STATUS "  - Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "  - Architecture: ARM Cortex-A9")
message(STATUS "  - Memory: 512MB limit")
