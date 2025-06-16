# Flight Shared Types C++17 Bindings
cmake_minimum_required(VERSION 3.17)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Flight-Core C++17 Integration Library
add_library(flight_shared_types_cpp17 SHARED
    flight/flight_shared_types.cpp
)

target_include_directories(flight_shared_types_cpp17 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/flight
    ${CMAKE_CURRENT_SOURCE_DIR}/memory-types
)

target_link_libraries(flight_shared_types_cpp17 PUBLIC
    # Link with the C memory types library (generated from WIT)
    # In a real implementation, this would link with the actual C bindings
)

# Set compiler-specific flags for maximum compatibility
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(flight_shared_types_cpp17 PRIVATE
        -Wall -Wextra -Wpedantic
        -fno-exceptions  # Flight-Core uses exception-free design
        -fno-rtti        # Minimize overhead for constrained platforms
    )
    
    # Platform-specific optimizations
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(flight_shared_types_cpp17 PRIVATE
            -O3 -DNDEBUG
            -flto  # Link-time optimization for zero-cost abstractions
        )
    endif()
endif()

# Platform-specific configurations
if(FLIGHT_TARGET_PLATFORM STREQUAL "dreamcast")
    target_compile_definitions(flight_shared_types_cpp17 PUBLIC
        FLIGHT_PLATFORM_DREAMCAST=1
        FLIGHT_MEMORY_CONSTRAINED=1
        FLIGHT_MAX_MEMORY_MB=16
    )
    target_compile_options(flight_shared_types_cpp17 PRIVATE
        -Os  # Optimize for size on Dreamcast
    )
elseif(FLIGHT_TARGET_PLATFORM STREQUAL "psp")
    target_compile_definitions(flight_shared_types_cpp17 PUBLIC
        FLIGHT_PLATFORM_PSP=1
        FLIGHT_MEMORY_CONSTRAINED=1
        FLIGHT_MAX_MEMORY_MB=32
    )
elseif(FLIGHT_TARGET_PLATFORM STREQUAL "vita")
    target_compile_definitions(flight_shared_types_cpp17 PUBLIC
        FLIGHT_PLATFORM_VITA=1
        FLIGHT_MAX_MEMORY_MB=512
    )
elseif(FLIGHT_TARGET_PLATFORM STREQUAL "v6r")
    target_compile_definitions(flight_shared_types_cpp17 PUBLIC
        FLIGHT_PLATFORM_V6R=1
        FLIGHT_CLOUD_PLATFORM=1
    )
else()
    # Default to modern desktop platform
    target_compile_definitions(flight_shared_types_cpp17 PUBLIC
        FLIGHT_PLATFORM_DESKTOP=1
    )
endif()

# Create static library variant for embedded platforms
add_library(flight_shared_types_cpp17_static STATIC
    flight/flight_shared_types.cpp
)

target_include_directories(flight_shared_types_cpp17_static PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/flight
    ${CMAKE_CURRENT_SOURCE_DIR}/memory-types
)

# Copy properties from shared library
get_target_property(SHARED_COMPILE_DEFS flight_shared_types_cpp17 COMPILE_DEFINITIONS)
get_target_property(SHARED_COMPILE_OPTS flight_shared_types_cpp17 COMPILE_OPTIONS)

if(SHARED_COMPILE_DEFS)
    target_compile_definitions(flight_shared_types_cpp17_static PUBLIC ${SHARED_COMPILE_DEFS})
endif()

if(SHARED_COMPILE_OPTS)
    target_compile_options(flight_shared_types_cpp17_static PRIVATE ${SHARED_COMPILE_OPTS})
endif()

# Install targets
install(TARGETS flight_shared_types_cpp17 flight_shared_types_cpp17_static
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES
    flight/flight_shared_types.hpp
    DESTINATION include/flight/shared-types
)

# Example and test targets (optional)
option(FLIGHT_BUILD_CPP17_EXAMPLES "Build C++17 examples" ON)
option(FLIGHT_BUILD_CPP17_TESTS "Build C++17 tests" ON)

if(FLIGHT_BUILD_CPP17_EXAMPLES)
    add_subdirectory(examples)
endif()

if(FLIGHT_BUILD_CPP17_TESTS)
    add_subdirectory(tests)
endif()

message(STATUS "Flight Shared Types C++17 bindings configured")
message(STATUS "  - Target Platform: ${FLIGHT_TARGET_PLATFORM}")
message(STATUS "  - C++ Standard: C++17")
message(STATUS "  - Build Type: ${CMAKE_BUILD_TYPE}")
