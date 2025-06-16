# Flight Core Runtime Module
cmake_minimum_required(VERSION 3.14)

# Include CPM
include(${CMAKE_SOURCE_DIR}/cmake/modules/CPM.cmake)

# Module library
add_library(flight-runtime STATIC)

# Source files
target_sources(flight-runtime
    PRIVATE
        # Add source files here as they are created
        # src/interpreter.cpp
        # src/execution_context.cpp
        # src/stack_machine.cpp
)

# Include directories
target_include_directories(flight-runtime
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Compile features
target_compile_features(flight-runtime PUBLIC cxx_std_17)

# Link libraries
target_link_libraries(flight-runtime
    PUBLIC
        flight-wasm
        flight-memory
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
install(TARGETS flight-runtime
    EXPORT flight-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
