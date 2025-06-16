cmake_minimum_required(VERSION 3.16)

# CMakeLists.txt for Flight HAL Interface Generator
project(FlightHALGenerator VERSION 1.0.0)

# Create the interface generator executable
add_executable(hal_generator
    src/hal_generator_main.cpp
    src/hal_code_generator.cpp
)

# Set C++ standard
target_compile_features(hal_generator PRIVATE cxx_std_20)

# Include directories
target_include_directories(hal_generator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

# Link required libraries
target_link_libraries(hal_generator PRIVATE
    ${CMAKE_DL_LIBS}  # For dynamic loading if needed
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(hal_generator PRIVATE)
elseif(UNIX)
    target_link_libraries(hal_generator PRIVATE pthread)
endif()

# Compiler warnings
target_compile_options(hal_generator PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic>
)

# Set install directories
include(GNUInstallDirs)

# Install the generator tool
install(TARGETS hal_generator
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install templates
install(DIRECTORY templates/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/flight_hal/templates
    FILES_MATCHING 
    PATTERN "*.template"
)

# Create convenience scripts
if(UNIX)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/hal_generate.sh.in
        ${CMAKE_CURRENT_BINARY_DIR}/hal_generate.sh
        @ONLY
    )
    
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/hal_generate.sh
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

if(WIN32)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/hal_generate.bat.in
        ${CMAKE_CURRENT_BINARY_DIR}/hal_generate.bat
        @ONLY
    )
    
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/hal_generate.bat
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()
