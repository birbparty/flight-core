# Flight HAL Interfaces Dependencies Management
# Manages external dependencies using FetchContent for modern CMake practices

include(FetchContent)

# Set FetchContent to be quiet by default
set(FETCHCONTENT_QUIET ON)

# Allow users to provide their own versions of dependencies
option(FLIGHT_HAL_USE_SYSTEM_DEPS
       "Use system-installed dependencies instead of FetchContent" OFF)

# ------------------------------------------------------------------------------
# fmt - Modern formatting library
# ------------------------------------------------------------------------------
if(NOT FLIGHT_HAL_USE_SYSTEM_DEPS)
  FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 10.2.1
    GIT_SHALLOW TRUE)

  # Disable fmt tests and docs
  set(FMT_TEST
      OFF
      CACHE BOOL "" FORCE)
  set(FMT_DOC
      OFF
      CACHE BOOL "" FORCE)
  set(FMT_INSTALL
      OFF
      CACHE BOOL "" FORCE)
else()
  find_package(fmt REQUIRED)
endif()

# ------------------------------------------------------------------------------
# spdlog - Fast C++ logging library
# ------------------------------------------------------------------------------
if(NOT FLIGHT_HAL_USE_SYSTEM_DEPS)
  FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.13.0
    GIT_SHALLOW TRUE)

  # Configure spdlog to use external fmt
  set(SPDLOG_FMT_EXTERNAL
      ON
      CACHE BOOL "" FORCE)
  set(SPDLOG_BUILD_EXAMPLE
      OFF
      CACHE BOOL "" FORCE)
  set(SPDLOG_BUILD_TESTS
      OFF
      CACHE BOOL "" FORCE)
  set(SPDLOG_INSTALL
      OFF
      CACHE BOOL "" FORCE)
else()
  find_package(spdlog REQUIRED)
endif()

# ------------------------------------------------------------------------------
# GoogleTest - Unit testing framework (only if tests are enabled)
# ------------------------------------------------------------------------------
if(FLIGHT_HAL_BUILD_TESTS)
  if(NOT FLIGHT_HAL_USE_SYSTEM_DEPS)
    FetchContent_Declare(
      googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG v1.14.0
      GIT_SHALLOW TRUE)

    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt
        ON
        CACHE BOOL "" FORCE)

    # Disable install for googletest
    set(INSTALL_GTEST
        OFF
        CACHE BOOL "" FORCE)
    
    # Disable building gmock apps
    set(BUILD_GMOCK
        ON
        CACHE BOOL "" FORCE)
    set(INSTALL_GMOCK
        OFF
        CACHE BOOL "" FORCE)
  else()
    find_package(GTest REQUIRED)
  endif()
endif()

# ------------------------------------------------------------------------------
# Google Benchmark - Performance testing framework (only if benchmarks enabled)
# ------------------------------------------------------------------------------
if(FLIGHT_HAL_BUILD_BENCHMARKS)
  if(NOT FLIGHT_HAL_USE_SYSTEM_DEPS)
    FetchContent_Declare(
      benchmark
      GIT_REPOSITORY https://github.com/google/benchmark.git
      GIT_TAG v1.8.3
      GIT_SHALLOW TRUE)

    # Configure benchmark
    set(BENCHMARK_ENABLE_TESTING
        OFF
        CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL
        OFF
        CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_DOXYGEN
        OFF
        CACHE BOOL "" FORCE)
    set(BENCHMARK_INSTALL_DOCS
        OFF
        CACHE BOOL "" FORCE)
    set(BENCHMARK_DOWNLOAD_DEPENDENCIES
        ON
        CACHE BOOL "" FORCE)
  else()
    find_package(benchmark REQUIRED)
  endif()
endif()

# ------------------------------------------------------------------------------
# Make dependencies available
# ------------------------------------------------------------------------------
if(NOT FLIGHT_HAL_USE_SYSTEM_DEPS)
  # Always fetch fmt and spdlog
  FetchContent_MakeAvailable(fmt spdlog)

  # Conditionally fetch testing dependencies
  if(FLIGHT_HAL_BUILD_TESTS)
    FetchContent_MakeAvailable(googletest)
  endif()

  if(FLIGHT_HAL_BUILD_BENCHMARKS)
    FetchContent_MakeAvailable(benchmark)
  endif()

  # Disable warnings for third-party code if function exists
  if(COMMAND disable_warnings)
    if(TARGET fmt)
      disable_warnings(fmt)
    endif()

    if(TARGET spdlog)
      disable_warnings(spdlog)
    endif()

    if(TARGET gtest AND FLIGHT_HAL_BUILD_TESTS)
      disable_warnings(gtest)
      disable_warnings(gtest_main)
      disable_warnings(gmock)
      disable_warnings(gmock_main)
    endif()

    if(TARGET benchmark AND FLIGHT_HAL_BUILD_BENCHMARKS)
      disable_warnings(benchmark)
      disable_warnings(benchmark_main)
    endif()
  endif()
endif()

# ------------------------------------------------------------------------------
# Helper function to link common dependencies
# ------------------------------------------------------------------------------
function(flight_hal_link_common_deps target)
  # Link fmt and spdlog for logging capabilities as PRIVATE to avoid export issues
  if(NOT FLIGHT_HAL_USE_SYSTEM_DEPS)
    target_link_libraries(${target} PRIVATE fmt::fmt spdlog::spdlog)
  else()
    target_link_libraries(${target} PRIVATE fmt::fmt spdlog::spdlog)
  endif()
  
  # Note: Dependencies are linked as PRIVATE to avoid CMake export issues with FetchContent
  # This follows the pattern from buck2-cpp-cpm reference implementation
endfunction()

# ------------------------------------------------------------------------------
# Print dependency information
# ------------------------------------------------------------------------------
function(print_dependency_info)
  message(STATUS "")
  message(STATUS "Flight HAL Dependencies:")
  message(STATUS "  Using system dependencies: ${FLIGHT_HAL_USE_SYSTEM_DEPS}")
  if(NOT FLIGHT_HAL_USE_SYSTEM_DEPS)
    message(STATUS "  fmt:           10.2.1")
    message(STATUS "  spdlog:        1.13.0")
    if(FLIGHT_HAL_BUILD_TESTS)
      message(STATUS "  GoogleTest:    1.14.0")
    endif()
    if(FLIGHT_HAL_BUILD_BENCHMARKS)
      message(STATUS "  Benchmark:     1.8.3")
    endif()
  endif()
  message(STATUS "")
endfunction()

# Call the function at the end
print_dependency_info()
