# Flight Core Component Module
cmake_minimum_required(VERSION 3.14)

# Include CPM
include(${CMAKE_SOURCE_DIR}/cmake/modules/CPM.cmake)

# Module library
add_library(flight-component STATIC)

# Source files
target_sources(flight-component
    PRIVATE
        # Add source files here as they are created
        # src/component_model.cpp
        # src/canonical_abi.cpp
        # src/interface_types.cpp
)

# Include directories
target_include_directories(flight-component
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Compile features
target_compile_features(flight-component PUBLIC cxx_std_17)

# Link libraries
target_link_libraries(flight-component
    PUBLIC
        flight-wasm
        flight-runtime
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
install(TARGETS flight-component
    EXPORT flight-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
