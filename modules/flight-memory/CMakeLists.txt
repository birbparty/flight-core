# Flight Core Memory Module
cmake_minimum_required(VERSION 3.14)

# Include CPM
include(${CMAKE_SOURCE_DIR}/cmake/modules/CPM.cmake)

# Module library
add_library(flight-memory STATIC)

# Source files
target_sources(flight-memory
    PRIVATE
        # Add source files here as they are created
        # src/linear_memory.cpp
        # src/memory_allocator.cpp
        # src/memory_pool.cpp
)

# Include directories
target_include_directories(flight-memory
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Compile features
target_compile_features(flight-memory PUBLIC cxx_std_17)

# Link libraries
target_link_libraries(flight-memory
    PUBLIC
        flight-hal  # Platform-specific memory operations
    PRIVATE
        # Add private dependencies
)

# Tests
if(FLIGHT_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Benchmarks
if(FLIGHT_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# Installation
install(TARGETS flight-memory
    EXPORT flight-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
