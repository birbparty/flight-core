# Flight Core Runtime Module
cmake_minimum_required(VERSION 3.14)

# Include CPM
include(${CMAKE_SOURCE_DIR}/cmake/modules/CPM.cmake)

# Module library
add_library(flight-core-runtime STATIC)

# Source files
target_sources(flight-core-runtime
    PRIVATE
        # Add source files here as they are created
        # src/interpreter.cpp
        # src/execution_context.cpp
        # src/stack_machine.cpp
)

# Include directories
target_include_directories(flight-core-runtime
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Compile features
target_compile_features(flight-core-runtime PUBLIC cxx_std_14)

# Link libraries
target_link_libraries(flight-core-runtime
    PUBLIC
        flight-core-wasm
        flight-core-memory
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
install(TARGETS flight-core-runtime
    EXPORT flight-core-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
