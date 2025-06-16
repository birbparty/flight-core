# Flight Core HAL Module
cmake_minimum_required(VERSION 3.14)

# Include CPM
include(${CMAKE_SOURCE_DIR}/cmake/modules/CPM.cmake)

# Module library
add_library(flight-hal STATIC)

# Platform-specific source files
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_sources(flight-hal
        PRIVATE
            # src/macos/platform_init.cpp
            # src/macos/memory_mapping.cpp
            # src/macos/thread_local.cpp
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    target_sources(flight-hal
        PRIVATE
            # src/emscripten/platform_init.cpp
            # src/emscripten/memory_mapping.cpp
            # src/emscripten/thread_local.cpp
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Generic" AND CMAKE_SYSTEM_PROCESSOR STREQUAL "sh4")
    target_sources(flight-hal
        PRIVATE
            # src/dreamcast/platform_init.cpp
            # src/dreamcast/memory_mapping.cpp
            # src/dreamcast/thread_local.cpp
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "PSP")
    target_sources(flight-hal
        PRIVATE
            # src/psp/platform_init.cpp
            # src/psp/memory_mapping.cpp
            # src/psp/thread_local.cpp
    )
else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif()

# Include directories
target_include_directories(flight-hal
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Compile features
target_compile_features(flight-hal PUBLIC cxx_std_17)

# Platform-specific link libraries
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    find_library(FOUNDATION_FRAMEWORK Foundation)
    target_link_libraries(flight-hal PRIVATE ${FOUNDATION_FRAMEWORK})
elseif(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    # Emscripten-specific flags handled by toolchain
elseif(CMAKE_SYSTEM_NAME STREQUAL "Generic" AND CMAKE_SYSTEM_PROCESSOR STREQUAL "sh4")
    # Dreamcast-specific libraries
elseif(CMAKE_SYSTEM_NAME STREQUAL "PSP")
    # PSP-specific libraries
endif()

# Tests
if(FLIGHT_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Benchmarks
if(FLIGHT_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# Installation
install(TARGETS flight-hal
    EXPORT flight-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
