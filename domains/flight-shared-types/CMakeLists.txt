cmake_minimum_required(VERSION 3.20)
project(FlightSharedTypes VERSION 1.0.0 LANGUAGES CXX)

# C++17 requirement (PlayStation 1 support dropped)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Flight-Core integration
find_package(PkgConfig REQUIRED)

# WIT binding generation targets
find_program(WIT_BINDGEN wit-bindgen REQUIRED)

# Function to generate C++17 bindings from WIT files
function(add_wit_bindings target_name wit_file)
    set(output_dir "${CMAKE_CURRENT_BINARY_DIR}/generated/${target_name}")
    set(generated_sources "${output_dir}/bindings.cpp")
    set(generated_headers "${output_dir}/bindings.h")
    
    add_custom_command(
        OUTPUT ${generated_sources} ${generated_headers}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${output_dir}
        COMMAND ${WIT_BINDGEN} cpp17 ${wit_file} --out-dir ${output_dir}
        DEPENDS ${wit_file}
        COMMENT "Generating C++17 bindings from ${wit_file}"
    )
    
    add_library(${target_name} ${generated_sources})
    target_include_directories(${target_name} PUBLIC ${output_dir})
    target_compile_features(${target_name} PUBLIC cxx_std_17)
    
    # Enable modern C++17 features
    target_compile_definitions(${target_name} PUBLIC
        FLIGHT_CPP17_ENABLED=1
        FLIGHT_OPTIONAL_SUPPORT=1
        FLIGHT_VARIANT_SUPPORT=1
    )
endfunction()

# Subdirectories for different binding types
add_subdirectory(bindings/cpp17)

# Integration tests
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
