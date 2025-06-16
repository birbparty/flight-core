# =============================================================================
# Flight WASM Tests - Comprehensive Testing Framework
# WebAssembly Specification Compliance and Embedded Platform Testing
# =============================================================================

# Only configure tests if explicitly enabled
if(NOT FLIGHT_WASM_BUILD_TESTS)
    message(STATUS "Flight WASM tests disabled")
    return()
endif()

message(STATUS "Configuring Flight WASM comprehensive test suite")

# =============================================================================
# Catch2 Dependency Management
# =============================================================================

# Try to find system-installed Catch2 first
find_package(Catch2 3 QUIET)

if(NOT Catch2_FOUND)
    message(STATUS "Catch2 not found locally, downloading via FetchContent")
    
    # Download and build Catch2 if not found
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.4.0
        GIT_SHALLOW ON
    )
    
    # Configure Catch2 for embedded platforms
    set(CATCH_INSTALL_DOCS OFF CACHE BOOL "Install documentation" FORCE)
    set(CATCH_INSTALL_HELPERS OFF CACHE BOOL "Install helpers" FORCE)
    
    FetchContent_MakeAvailable(Catch2)
    
    # Add Catch2 to the module path for test discovery
    list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
else()
    message(STATUS "Using system-installed Catch2")
    list(APPEND CMAKE_MODULE_PATH "${Catch2_DIR}/../extras")
endif()

# =============================================================================
# Platform-Specific Test Configuration
# =============================================================================

# Configure platform-specific memory limits and test parameters
if(FLIGHT_WASM_PLATFORM_DREAMCAST)
    set(FLIGHT_WASM_TEST_MEMORY_LIMIT 8388608)      # 8MB
    set(FLIGHT_WASM_TEST_STACK_LIMIT 512)
    set(FLIGHT_WASM_TEST_ITERATIONS 1000)
    set(FLIGHT_WASM_ENABLE_FUZZING OFF)
    message(STATUS "Dreamcast test limits: 8MB memory, 512 stack depth, 1K iterations")
elseif(FLIGHT_WASM_PLATFORM_PSP)
    set(FLIGHT_WASM_TEST_MEMORY_LIMIT 16777216)     # 16MB
    set(FLIGHT_WASM_TEST_STACK_LIMIT 1024)
    set(FLIGHT_WASM_TEST_ITERATIONS 5000)
    set(FLIGHT_WASM_ENABLE_FUZZING OFF)
    message(STATUS "PSP test limits: 16MB memory, 1K stack depth, 5K iterations")
elseif(FLIGHT_WASM_PLATFORM_PSVITA)
    set(FLIGHT_WASM_TEST_MEMORY_LIMIT 268435456)    # 256MB
    set(FLIGHT_WASM_TEST_STACK_LIMIT 4096)
    set(FLIGHT_WASM_TEST_ITERATIONS 50000)
    set(FLIGHT_WASM_ENABLE_FUZZING OFF)
    message(STATUS "PS Vita test limits: 256MB memory, 4K stack depth, 50K iterations")
else()
    # Desktop/server platforms - no artificial limits
    set(FLIGHT_WASM_TEST_MEMORY_LIMIT 0)            # Unlimited
    set(FLIGHT_WASM_TEST_STACK_LIMIT 0)             # Unlimited
    set(FLIGHT_WASM_TEST_ITERATIONS 100000)
    set(FLIGHT_WASM_ENABLE_FUZZING ON)
    message(STATUS "Desktop test limits: unlimited memory, 100K iterations, fuzzing enabled")
endif()

# =============================================================================
# Test Executable Configuration
# =============================================================================

# Create comprehensive test executable
add_executable(flight_wasm_tests)

# Configure test sources (only include files that exist)
target_sources(flight_wasm_tests PRIVATE
    # Main test runner with platform-specific configuration
    test_main.cpp
    
    # Type system tests
    types/test_values.cpp
    
    # Binary format tests
    binary/test_parser.cpp
    
    # Utility tests
    utilities/test_platform.cpp
    utilities/test_platform_compatibility.cpp
    
    # Integration tests
    integration/test_spec_compliance.cpp
    
    # Performance tests
    performance/test_benchmarks.cpp
)

# Link with Flight WASM library and Catch2
target_link_libraries(flight_wasm_tests PRIVATE
    flight-wasm
    Catch2::Catch2WithMain
)

# Configure test compile definitions
target_compile_definitions(flight_wasm_tests PRIVATE
    FLIGHT_WASM_TEST_MEMORY_LIMIT=${FLIGHT_WASM_TEST_MEMORY_LIMIT}
    FLIGHT_WASM_TEST_STACK_LIMIT=${FLIGHT_WASM_TEST_STACK_LIMIT}
    FLIGHT_WASM_TEST_ITERATIONS=${FLIGHT_WASM_TEST_ITERATIONS}
    $<$<BOOL:${FLIGHT_WASM_ENABLE_FUZZING}>:FLIGHT_WASM_ENABLE_FUZZING=1>
    $<$<CONFIG:Debug>:FLIGHT_WASM_TEST_DEBUG=1>
)

# =============================================================================
# Compiler-Specific Test Configurations
# =============================================================================

# GCC-specific sanitizers and warnings
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_CROSSCOMPILING)
    target_compile_options(flight_wasm_tests PRIVATE
        -Wall -Wextra -Wpedantic -Werror
        -fsanitize=address
        -fsanitize=undefined
        -fstack-protector-strong
        -fno-omit-frame-pointer
    )
    target_link_options(flight_wasm_tests PRIVATE
        -fsanitize=address
        -fsanitize=undefined
    )
    message(STATUS "GCC sanitizers enabled: AddressSanitizer, UndefinedBehaviorSanitizer")
endif()

# Clang-specific sanitizers and warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT CMAKE_CROSSCOMPILING)
    target_compile_options(flight_wasm_tests PRIVATE
        -Wall -Wextra -Wpedantic -Werror
        -fsanitize=address
        -fsanitize=undefined
        -fno-omit-frame-pointer
    )
    target_link_options(flight_wasm_tests PRIVATE
        -fsanitize=address
        -fsanitize=undefined
    )
    
    # Add memory sanitizer for non-embedded builds (but not on macOS ARM64)
    if(NOT FLIGHT_WASM_PLATFORM_EMBEDDED AND NOT (APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm64"))
        target_compile_options(flight_wasm_tests PRIVATE -fsanitize=memory)
        target_link_options(flight_wasm_tests PRIVATE -fsanitize=memory)
    endif()
    
    message(STATUS "Clang sanitizers enabled: AddressSanitizer, UndefinedBehaviorSanitizer")
endif()

# =============================================================================
# CTest Integration and Test Discovery
# =============================================================================

# Include Catch2 test discovery
include(CTest)
include(Catch)

# Discover tests automatically
catch_discover_tests(flight_wasm_tests
    TEST_PREFIX "flight_wasm."
    REPORTER XML
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/test_results"
    EXTRA_ARGS --verbosity high
)

# =============================================================================
# Platform-Specific Test Execution
# =============================================================================

# Configure platform-specific test execution
# Note: Test properties will be set after test discovery
if(CMAKE_CROSSCOMPILING)
    if(CMAKE_SYSTEM_NAME STREQUAL "Dreamcast")
        # Dreamcast testing via emulation
        set(FLIGHT_WASM_TEST_TIMEOUT 300)  # 5 minutes timeout for embedded
        set(FLIGHT_WASM_TEST_LABELS "embedded;dreamcast")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "PSP")
        # PSP testing via emulation
        set(FLIGHT_WASM_TEST_TIMEOUT 180)  # 3 minutes timeout
        set(FLIGHT_WASM_TEST_LABELS "embedded;psp")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "PSVita")
        # PS Vita testing
        set(FLIGHT_WASM_TEST_TIMEOUT 120)  # 2 minutes timeout
        set(FLIGHT_WASM_TEST_LABELS "embedded;psvita")
    endif()
else()
    # Native testing
    set(FLIGHT_WASM_TEST_TIMEOUT 60)   # 1 minute timeout for native
    set(FLIGHT_WASM_TEST_LABELS "native")
endif()

# =============================================================================
# Test Fixtures and Resources
# =============================================================================

# Create test fixture directories
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/fixtures")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/fixtures/valid_modules")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/fixtures/invalid_modules")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/fixtures/spec_tests")

# Copy test fixtures during configuration
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/fixtures")
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/"
         DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/fixtures/")
endif()

# =============================================================================
# Performance Benchmarking Integration
# =============================================================================

# Add performance benchmark target
add_custom_target(flight_wasm_benchmark
    COMMAND $<TARGET_FILE:flight_wasm_tests> --benchmark-samples 100 --benchmark-resamples 10000
    DEPENDS flight_wasm_tests
    COMMENT "Running Flight WASM performance benchmarks"
    VERBATIM
)

# =============================================================================
# Memory Usage Validation
# =============================================================================

# Add memory validation target
if(NOT CMAKE_CROSSCOMPILING)
    find_program(VALGRIND_EXECUTABLE valgrind)
    if(VALGRIND_EXECUTABLE)
        add_custom_target(flight_wasm_memcheck
            COMMAND ${VALGRIND_EXECUTABLE} 
                --tool=memcheck 
                --leak-check=full 
                --show-leak-kinds=all 
                --track-origins=yes 
                --error-exitcode=1
                $<TARGET_FILE:flight_wasm_tests>
            DEPENDS flight_wasm_tests
            COMMENT "Running Flight WASM memory validation with Valgrind"
            VERBATIM
        )
        message(STATUS "Valgrind memory checking target available: flight_wasm_memcheck")
    endif()
endif()

# =============================================================================
# Coverage Analysis
# =============================================================================

# Configure coverage analysis for GCC/Clang
if((CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang") AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    option(FLIGHT_WASM_ENABLE_COVERAGE "Enable test coverage analysis" OFF)
    
    if(FLIGHT_WASM_ENABLE_COVERAGE)
        target_compile_options(flight_wasm_tests PRIVATE --coverage)
        target_link_options(flight_wasm_tests PRIVATE --coverage)
        
        find_program(GCOVR_EXECUTABLE gcovr)
        if(GCOVR_EXECUTABLE)
            add_custom_target(flight_wasm_coverage
                COMMAND ${GCOVR_EXECUTABLE} 
                    --root ${CMAKE_SOURCE_DIR} 
                    --filter ${CMAKE_SOURCE_DIR}/include/
                    --html --html-details 
                    --output ${CMAKE_CURRENT_BINARY_DIR}/coverage.html
                DEPENDS flight_wasm_tests
                COMMENT "Generating Flight WASM test coverage report"
                VERBATIM
            )
            message(STATUS "Coverage analysis target available: flight_wasm_coverage")
        endif()
    endif()
endif()

# =============================================================================
# Test Summary
# =============================================================================

message(STATUS "=== Flight WASM Test Configuration Summary ===")
message(STATUS "Test Framework: Catch2 v3")
message(STATUS "Platform: ${FLIGHT_WASM_TARGET_PLATFORM}")
message(STATUS "Memory Limit: ${FLIGHT_WASM_TEST_MEMORY_LIMIT} bytes")
message(STATUS "Stack Limit: ${FLIGHT_WASM_TEST_STACK_LIMIT}")
message(STATUS "Test Iterations: ${FLIGHT_WASM_TEST_ITERATIONS}")
message(STATUS "Fuzzing Enabled: ${FLIGHT_WASM_ENABLE_FUZZING}")
message(STATUS "Cross Compiling: ${CMAKE_CROSSCOMPILING}")
if(NOT CMAKE_CROSSCOMPILING)
    message(STATUS "Sanitizers: Enabled")
    if(VALGRIND_EXECUTABLE)
        message(STATUS "Valgrind: Available")
    endif()
endif()
message(STATUS "=============================================")

message(STATUS "Flight WASM testing framework configured successfully! 🚀")
