# HAL Compliance Validator Tool
cmake_minimum_required(VERSION 3.16)

# Add the validation tool executable
add_executable(hal_validator
    hal_validator.cpp
    ../../src/validation/validation_core.cpp
    ../../src/validation/interface_validator.cpp
)

# Include directories
target_include_directories(hal_validator PRIVATE
    ../../include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link libraries
target_link_libraries(hal_validator PRIVATE
    flight_hal_core
)

# Set C++ standard
target_compile_features(hal_validator PRIVATE cxx_std_20)

# Compiler-specific options
if(MSVC)
    target_compile_options(hal_validator PRIVATE /W4)
else()
    target_compile_options(hal_validator PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Install the tool
install(TARGETS hal_validator
    RUNTIME DESTINATION bin
    COMPONENT tools
)

# Create a convenience script for running validations
if(UNIX)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/hal_validate.sh.in
        ${CMAKE_CURRENT_BINARY_DIR}/hal_validate.sh
        @ONLY
    )
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/hal_validate.sh
        DESTINATION bin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        COMPONENT tools
    )
endif()
