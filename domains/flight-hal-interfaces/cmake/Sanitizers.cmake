# Flight HAL Interfaces Sanitizers Configuration
# Provides functions to enable various sanitizers for debugging and quality assurance

# Function to enable sanitizers on a target
function(enable_sanitizers target)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    
    # Sanitizer options
    option(FLIGHT_HAL_ENABLE_ASAN "Enable address sanitizer" ON)
    option(FLIGHT_HAL_ENABLE_UBSAN "Enable undefined behavior sanitizer" ON)
    option(FLIGHT_HAL_ENABLE_TSAN "Enable thread sanitizer" OFF)
    option(FLIGHT_HAL_ENABLE_MSAN "Enable memory sanitizer" OFF)
    option(FLIGHT_HAL_ENABLE_LSAN "Enable leak sanitizer" OFF)

    set(SANITIZERS "")

    # Address Sanitizer
    if(FLIGHT_HAL_ENABLE_ASAN)
      list(APPEND SANITIZERS "address")
      
      # Enable leak sanitizer with ASAN by default (if supported)
      if(FLIGHT_HAL_ENABLE_LSAN AND CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
        list(APPEND SANITIZERS "leak")
      endif()
    endif()

    # Undefined Behavior Sanitizer
    if(FLIGHT_HAL_ENABLE_UBSAN)
      list(APPEND SANITIZERS "undefined")
    endif()

    # Thread Sanitizer (incompatible with ASAN and MSAN)
    if(FLIGHT_HAL_ENABLE_TSAN)
      if(FLIGHT_HAL_ENABLE_ASAN OR FLIGHT_HAL_ENABLE_MSAN)
        message(WARNING 
          "Thread sanitizer is incompatible with address and memory sanitizers. "
          "Disabling TSAN.")
      else()
        list(APPEND SANITIZERS "thread")
      endif()
    endif()

    # Memory Sanitizer (Clang only, incompatible with ASAN and TSAN)
    if(FLIGHT_HAL_ENABLE_MSAN)
      if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
        if(FLIGHT_HAL_ENABLE_ASAN OR FLIGHT_HAL_ENABLE_TSAN)
          message(WARNING 
            "Memory sanitizer is incompatible with address and thread sanitizers. "
            "Disabling MSAN.")
        else()
          list(APPEND SANITIZERS "memory")
        endif()
      else()
        message(WARNING "Memory sanitizer is only supported by Clang")
      endif()
    endif()

    # Apply sanitizers if any are enabled
    if(SANITIZERS)
      list(JOIN SANITIZERS "," LIST_OF_SANITIZERS)
      
      target_compile_options(${target} PUBLIC -fsanitize=${LIST_OF_SANITIZERS})
      target_link_options(${target} PUBLIC -fsanitize=${LIST_OF_SANITIZERS})

      # Additional options for better sanitizer output
      target_compile_options(${target} PUBLIC -fno-omit-frame-pointer)
      target_compile_options(${target} PUBLIC -g)
      
      # Optimize for debugging
      if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(${target} PUBLIC -O1)
      endif()

      # For undefined behavior sanitizer, make it more strict
      if(FLIGHT_HAL_ENABLE_UBSAN)
        target_compile_options(${target} PUBLIC -fno-sanitize-recover=all)
        target_compile_options(${target} PUBLIC -fsanitize=float-divide-by-zero)
        target_compile_options(${target} PUBLIC -fsanitize=float-cast-overflow)
        
        # Additional UBSan options for comprehensive checking
        if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
          target_compile_options(${target} PUBLIC -fsanitize=implicit-conversion)
          target_compile_options(${target} PUBLIC -fsanitize=nullability)
        endif()
      endif()

      # Memory sanitizer specific options
      if(FLIGHT_HAL_ENABLE_MSAN)
        target_compile_options(${target} PUBLIC -fsanitize-memory-track-origins=2)
      endif()

      message(STATUS "Enabled sanitizers for ${target}: ${LIST_OF_SANITIZERS}")
    endif()

  elseif(MSVC)
    # MSVC has limited sanitizer support
    option(FLIGHT_HAL_ENABLE_ASAN "Enable address sanitizer" ON)

    if(FLIGHT_HAL_ENABLE_ASAN)
      target_compile_options(${target} PUBLIC /fsanitize=address)
      
      # MSVC ASAN requires runtime environment variable
      message(STATUS 
        "Address sanitizer enabled for ${target}. "
        "Set environment variable: ASAN_OPTIONS=windows_hook_rtl_allocators=true")
    endif()
  endif()
endfunction()

# Function to disable warnings for third-party targets
function(disable_warnings target)
  if(TARGET ${target})
    if(MSVC)
      target_compile_options(${target} PRIVATE /w)
    else()
      target_compile_options(${target} PRIVATE -w)
    endif()
  endif()
endfunction()

# Function to apply sanitizers to all Flight HAL targets
function(flight_hal_enable_sanitizers_for_target target)
  # Only enable sanitizers in Debug builds by default
  if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND FLIGHT_HAL_ENABLE_SANITIZERS)
    enable_sanitizers(${target})
  elseif(FLIGHT_HAL_ENABLE_SANITIZERS)
    # Allow explicit enabling in other build types
    enable_sanitizers(${target})
  endif()
endfunction()

# Function to configure sanitizer environment
function(configure_sanitizer_environment)
  if(FLIGHT_HAL_ENABLE_ASAN)
    # Set common ASAN options for better debugging
    set(ASAN_OPTIONS 
      "strict_string_checks=1"
      "detect_stack_use_after_return=1"
      "check_initialization_order=1"
      "strict_init_order=1"
      "detect_invalid_pointer_pairs=2")
    
    if(WIN32)
      list(APPEND ASAN_OPTIONS "windows_hook_rtl_allocators=true")
    endif()
    
    string(REPLACE ";" ":" ASAN_OPTIONS_STRING "${ASAN_OPTIONS}")
    message(STATUS "Recommended ASAN_OPTIONS: ${ASAN_OPTIONS_STRING}")
  endif()

  if(FLIGHT_HAL_ENABLE_UBSAN)
    # Set UBSAN options for comprehensive reporting
    set(UBSAN_OPTIONS
      "print_stacktrace=1"
      "halt_on_error=1")
    
    string(REPLACE ";" ":" UBSAN_OPTIONS_STRING "${UBSAN_OPTIONS}")
    message(STATUS "Recommended UBSAN_OPTIONS: ${UBSAN_OPTIONS_STRING}")
  endif()

  if(FLIGHT_HAL_ENABLE_TSAN)
    # Set TSAN options
    set(TSAN_OPTIONS
      "halt_on_error=1"
      "second_deadlock_stack=1")
    
    string(REPLACE ";" ":" TSAN_OPTIONS_STRING "${TSAN_OPTIONS}")
    message(STATUS "Recommended TSAN_OPTIONS: ${TSAN_OPTIONS_STRING}")
  endif()
endfunction()

# Function to print sanitizer configuration summary
function(print_sanitizer_config)
  message(STATUS "")
  message(STATUS "Flight HAL Sanitizer Configuration:")
  
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    message(STATUS "  Address Sanitizer (ASAN):           ${FLIGHT_HAL_ENABLE_ASAN}")
    message(STATUS "  Undefined Behavior Sanitizer (UBSAN): ${FLIGHT_HAL_ENABLE_UBSAN}")
    message(STATUS "  Thread Sanitizer (TSAN):            ${FLIGHT_HAL_ENABLE_TSAN}")
    message(STATUS "  Memory Sanitizer (MSAN):            ${FLIGHT_HAL_ENABLE_MSAN}")
    message(STATUS "  Leak Sanitizer (LSAN):              ${FLIGHT_HAL_ENABLE_LSAN}")
  elseif(MSVC)
    message(STATUS "  Address Sanitizer (ASAN):           ${FLIGHT_HAL_ENABLE_ASAN}")
    message(STATUS "  Other sanitizers:                   Not supported on MSVC")
  else()
    message(STATUS "  Sanitizers:                         Not supported on this compiler")
  endif()
  
  message(STATUS "  Enabled for build type:             ${CMAKE_BUILD_TYPE}")
  message(STATUS "")
endfunction()

# Initialize sanitizer configuration
if(FLIGHT_HAL_ENABLE_SANITIZERS)
  configure_sanitizer_environment()
endif()
