# Flight Core Memory Module
cmake_minimum_required(VERSION 3.14)

# Include CPM
include(${CMAKE_SOURCE_DIR}/cmake/modules/CPM.cmake)

# Module library
add_library(flight-core-memory STATIC)

# Source files
target_sources(flight-core-memory
    PRIVATE
        # Add source files here as they are created
        # src/linear_memory.cpp
        # src/memory_allocator.cpp
        # src/memory_pool.cpp
)

# Include directories
target_include_directories(flight-core-memory
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Compile features
target_compile_features(flight-core-memory PUBLIC cxx_std_14)

# Link libraries
target_link_libraries(flight-core-memory
    PUBLIC
        flight-core-hal  # Platform-specific memory operations
    PRIVATE
        # Add private dependencies
)

# Tests
if(FLIGHT_CORE_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Benchmarks
if(FLIGHT_CORE_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# Installation
install(TARGETS flight-core-memory
    EXPORT flight-core-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
