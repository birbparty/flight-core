# Flight HAL Interfaces Compiler Warnings Configuration
# Provides comprehensive compiler warning settings for cross-platform development

# Function to set project warnings for a target
function(set_project_warnings target)
  
  # Warning level options
  option(FLIGHT_HAL_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
  
  # Common warning flags for all compilers
  set(CLANG_WARNINGS
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wcast-align
    -Wunused
    -Woverloaded-virtual
    -Wconversion
    -Wsign-conversion
    -Wnull-dereference
    -Wdouble-promotion
    -Wformat=2
    -Wimplicit-fallthrough
    -Wuninitialized
    -Wstrict-aliasing
    -Wcast-qual
    -Wno-unknown-pragmas
  )

  # Additional Clang-specific warnings
  set(CLANG_EXTRA_WARNINGS
    -Wloop-analysis
    -Wshift-sign-overflow
    -Wcomma
    -Wstring-conversion
    -Wbool-conversion
    -Wconstant-conversion
    -Wint-conversion
    -Wshorten-64-to-32
    -Wconditional-uninitialized
    -Wthread-safety
    -Wthread-safety-negative
  )

  set(GCC_WARNINGS
    ${CLANG_WARNINGS}
    -Wmisleading-indentation
    -Wduplicated-cond
    -Wduplicated-branches
    -Wlogical-op
    -Wuseless-cast
    -Wdangling-else
    -Wstack-protector
  )

  # Additional GCC-specific warnings
  set(GCC_EXTRA_WARNINGS
    -Wnoexcept
    -Wstrict-null-sentinel
    -Warray-bounds=2
    -Wformat-overflow=2
    -Wformat-truncation=2
    -Wstringop-overflow=4
  )

  set(MSVC_WARNINGS
    /W4          # High warning level
    /permissive- # Strict conformance mode
    /Zc:__cplusplus # Correct __cplusplus macro
    /w14242      # 'identifier': conversion from 'type1' to 'type1', possible loss of data
    /w14254      # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
    /w14263      # 'function': member function does not override any base class virtual member function
    /w14265      # 'classname': class has virtual functions, but destructor is not virtual
    /w14287      # 'operator': unsigned/negative constant mismatch
    /we4289      # nonstandard extension used: 'variable': loop control variable declared in the for-loop is used outside the for-loop scope
    /w14296      # 'operator': expression is always 'boolean_value'
    /w14311      # 'variable': pointer truncation from 'type1' to 'type2'
    /w14545      # expression before comma evaluates to a function which is missing an argument list
    /w14546      # function call before comma missing argument list
    /w14547      # 'operator': operator before comma has no effect; expected operator with side-effect
    /w14549      # 'operator': operator before comma has no effect; did you intend 'operator'?
    /w14555      # expression has no effect; expected expression with side-effect
    /w14619      # pragma warning: there is no warning number 'number'
    /w14640      # Enable warning on thread unsafe static member initialization
    /w14826      # Conversion from 'type1' to 'type_2' is sign-extended. This may cause unexpected runtime behavior.
    /w14905      # wide string literal cast to 'LPSTR'
    /w14906      # string literal cast to 'LPWSTR'
    /w14928      # illegal copy-initialization; more than one user-defined conversion has been implicitly applied
  )

  # Apply warnings based on compiler
  if(MSVC)
    set(PROJECT_WARNINGS ${MSVC_WARNINGS})
  elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    set(PROJECT_WARNINGS ${CLANG_WARNINGS})
    
    # Add Clang-specific warnings if available
    foreach(WARNING ${CLANG_EXTRA_WARNINGS})
      check_cxx_compiler_flag(${WARNING} HAS_WARNING_${WARNING})
      if(HAS_WARNING_${WARNING})
        list(APPEND PROJECT_WARNINGS ${WARNING})
      endif()
    endforeach()
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(PROJECT_WARNINGS ${GCC_WARNINGS})
    
    # Add GCC-specific warnings if available
    foreach(WARNING ${GCC_EXTRA_WARNINGS})
      check_cxx_compiler_flag(${WARNING} HAS_WARNING_${WARNING})
      if(HAS_WARNING_${WARNING})
        list(APPEND PROJECT_WARNINGS ${WARNING})
      endif()
    endforeach()
  else()
    message(AUTHOR_WARNING "No compiler warnings set for '${CMAKE_CXX_COMPILER_ID}' compiler.")
  endif()

  # Add warnings as errors if requested
  if(FLIGHT_HAL_WARNINGS_AS_ERRORS)
    if(MSVC)
      list(APPEND PROJECT_WARNINGS /WX)
    else()
      list(APPEND PROJECT_WARNINGS -Werror)
    endif()
  endif()

  # Apply warnings to target
  target_compile_options(${target} INTERFACE ${PROJECT_WARNINGS})
  
  message(STATUS "Applied compiler warnings to target: ${target}")
endfunction()

# Function to disable warnings for third-party targets
function(disable_warnings target)
  if(TARGET ${target})
    if(MSVC)
      target_compile_options(${target} PRIVATE /w)
    else()
      target_compile_options(${target} PRIVATE -w)
    endif()
    message(STATUS "Disabled warnings for third-party target: ${target}")
  endif()
endfunction()

# Function to apply Flight HAL specific compiler settings
function(flight_hal_configure_compiler_settings target)
  # Set C++17 standard
  target_compile_features(${target} PUBLIC cxx_std_17)
  
  # Platform-specific optimizations
  if(CMAKE_SYSTEM_NAME STREQUAL "Dreamcast")
    target_compile_definitions(${target} PRIVATE
      FLIGHT_PLATFORM_DREAMCAST=1
      FLIGHT_MEMORY_CONSTRAINED=1
    )
    target_compile_options(${target} PRIVATE
      -Os          # Optimize for size
      -ffunction-sections
      -fdata-sections
    )
  elseif(CMAKE_SYSTEM_NAME STREQUAL "PSP")
    target_compile_definitions(${target} PRIVATE
      FLIGHT_PLATFORM_PSP=1
      FLIGHT_MEMORY_CONSTRAINED=1
    )
    target_compile_options(${target} PRIVATE
      -Os          # Optimize for size
      -ffunction-sections
      -fdata-sections
    )
  else()
    target_compile_definitions(${target} PRIVATE
      FLIGHT_PLATFORM_MODERN=1
    )
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
      if(MSVC)
        target_compile_options(${target} PRIVATE /O2)
      else()
        target_compile_options(${target} PRIVATE -O3)
      endif()
    endif()
  endif()

  # Flight HAL Interface definitions
  target_compile_definitions(${target} PRIVATE
    FLIGHT_HAL_INTERFACES_VERSION_MAJOR=1
    FLIGHT_HAL_INTERFACES_VERSION_MINOR=0
    FLIGHT_HAL_INTERFACES_VERSION_PATCH=0
  )

  # Security hardening flags
  if(NOT MSVC)
    # Stack protection
    target_compile_options(${target} PRIVATE -fstack-protector-strong)
    
    # Position independent code
    set_property(TARGET ${target} PROPERTY POSITION_INDEPENDENT_CODE ON)
    
    # Fortify source (requires optimization)
    if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
      target_compile_definitions(${target} PRIVATE _FORTIFY_SOURCE=2)
    endif()
  endif()

  # Debug information
  if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    if(MSVC)
      target_compile_options(${target} PRIVATE /Zi)
    else()
      target_compile_options(${target} PRIVATE -g)
    endif()
  endif()
  
  message(STATUS "Applied Flight HAL compiler settings to target: ${target}")
endfunction()

# Function to print compiler configuration
function(print_compiler_config)
  message(STATUS "")
  message(STATUS "Flight HAL Compiler Configuration:")
  message(STATUS "  Compiler:               ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
  message(STATUS "  C++ Standard:           ${CMAKE_CXX_STANDARD}")
  message(STATUS "  Build Type:             ${CMAKE_BUILD_TYPE}")
  message(STATUS "  Warnings as errors:     ${FLIGHT_HAL_WARNINGS_AS_ERRORS}")
  message(STATUS "  Platform:               ${CMAKE_SYSTEM_NAME}")
  
  # Show optimization level
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "  Optimization:           Debug (-g)")
  elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(CMAKE_SYSTEM_NAME STREQUAL "Dreamcast" OR CMAKE_SYSTEM_NAME STREQUAL "PSP")
      message(STATUS "  Optimization:           Size (-Os)")
    else()
      message(STATUS "  Optimization:           Speed (-O3/O2)")
    endif()
  elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    message(STATUS "  Optimization:           Speed with debug info")
  elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    message(STATUS "  Optimization:           Minimum size")
  endif()
  
  message(STATUS "")
endfunction()

# Include CheckCXXCompilerFlag for feature detection
include(CheckCXXCompilerFlag)
