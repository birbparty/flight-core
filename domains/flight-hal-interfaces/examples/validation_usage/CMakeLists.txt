# Validation Usage Example
cmake_minimum_required(VERSION 3.16)

# Add the validation example executable
add_executable(validation_example
    validation_example.cpp
    ../../src/validation/validation_core.cpp
    ../../src/validation/interface_validator.cpp
)

# Include directories
target_include_directories(validation_example PRIVATE
    ../../include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link libraries - we may need to create flight_hal_core if it doesn't exist
# For now, let's make this self-contained
target_link_libraries(validation_example PRIVATE
    # flight_hal_core  # Commented out in case it doesn't exist yet
)

# Set C++ standard
target_compile_features(validation_example PRIVATE cxx_std_20)

# Compiler-specific options
if(MSVC)
    target_compile_options(validation_example PRIVATE /W4)
else()
    target_compile_options(validation_example PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Install the example
install(TARGETS validation_example
    RUNTIME DESTINATION bin
    COMPONENT examples
)
