# Flight HAL Interfaces Static Analysis Configuration
# Provides integration for static analysis tools like clang-tidy and cppcheck

# Function to enable clang-tidy for a target
function(enable_clang_tidy target)
  # Find clang-tidy
  find_program(CLANG_TIDY_EXE 
    NAMES clang-tidy-18 clang-tidy-17 clang-tidy-16 clang-tidy-15 clang-tidy
    DOC "Path to clang-tidy executable")

  if(CLANG_TIDY_EXE)
    # Configure clang-tidy checks
    set(CLANG_TIDY_CHECKS
      "-*"
      "bugprone-*"
      "cert-*"
      "clang-analyzer-*"
      "cppcoreguidelines-*"
      "google-*"
      "hicpp-*"
      "llvm-*"
      "misc-*"
      "modernize-*"
      "performance-*"
      "portability-*"
      "readability-*"
      
      # Disable some overly strict checks
      "-bugprone-easily-swappable-parameters"
      "-cert-err58-cpp"
      "-cppcoreguidelines-avoid-magic-numbers"
      "-cppcoreguidelines-pro-bounds-array-to-pointer-decay"
      "-cppcoreguidelines-pro-bounds-pointer-arithmetic"
      "-cppcoreguidelines-pro-type-vararg"
      "-google-readability-todo"
      "-hicpp-vararg"
      "-llvm-header-guard"
      "-misc-non-private-member-variables-in-classes"
      "-modernize-use-trailing-return-type"
      "-readability-magic-numbers"
      "-readability-named-parameter"
    )
    
    # Join checks into a single string
    string(REPLACE ";" "," CLANG_TIDY_CHECKS_STR "${CLANG_TIDY_CHECKS}")
    
    # Configure clang-tidy command
    set(CLANG_TIDY_COMMAND 
      ${CLANG_TIDY_EXE}
      --checks=${CLANG_TIDY_CHECKS_STR}
      --warnings-as-errors=*
      --header-filter=.*
      --format-style=file
    )
    
    # Apply to target
    set_target_properties(${target} PROPERTIES
      CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}"
    )
    
    message(STATUS "Enabled clang-tidy for target: ${target}")
  else()
    message(WARNING "clang-tidy not found. Static analysis disabled for ${target}")
  endif()
endfunction()

# Function to enable cppcheck for a target  
function(enable_cppcheck target)
  # Find cppcheck
  find_program(CPPCHECK_EXE
    NAMES cppcheck
    DOC "Path to cppcheck executable")

  if(CPPCHECK_EXE)
    # Configure cppcheck options
    set(CPPCHECK_OPTIONS
      --enable=warning,style,performance,portability,information,missingInclude
      --std=c++17
      --verbose
      --quiet
      --suppressions-list=${CMAKE_SOURCE_DIR}/.cppcheck-suppressions
      --inline-suppr
      --error-exitcode=2
      --check-config
    )

    # Apply to target
    set_target_properties(${target} PROPERTIES
      CXX_CPPCHECK "${CPPCHECK_EXE};${CPPCHECK_OPTIONS}"
    )
    
    message(STATUS "Enabled cppcheck for target: ${target}")
  else()
    message(WARNING "cppcheck not found. Static analysis disabled for ${target}")
  endif()
endfunction()

# Function to apply static analysis to Flight HAL targets
function(flight_hal_enable_static_analysis target)
  if(FLIGHT_HAL_ENABLE_CLANG_TIDY)
    enable_clang_tidy(${target})
  endif()
  
  if(FLIGHT_HAL_ENABLE_CPPCHECK)
    enable_cppcheck(${target})
  endif()
endfunction()

# Function to create a custom target for running static analysis
function(create_static_analysis_targets)
  if(FLIGHT_HAL_ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE 
      NAMES clang-tidy-18 clang-tidy-17 clang-tidy-16 clang-tidy-15 clang-tidy)
    
    if(CLANG_TIDY_EXE)
      # Create a target to run clang-tidy on all source files
      file(GLOB_RECURSE ALL_SOURCE_FILES
        ${CMAKE_SOURCE_DIR}/src/*.cpp
        ${CMAKE_SOURCE_DIR}/src/*.hpp
        ${CMAKE_SOURCE_DIR}/include/*.hpp
      )
      
      add_custom_target(
        clang-tidy-check
        COMMAND ${CLANG_TIDY_EXE}
          --checks=*
          --header-filter=.*
          --format-style=file
          ${ALL_SOURCE_FILES}
        COMMENT "Running clang-tidy on all source files"
        VERBATIM
      )
    endif()
  endif()

  if(FLIGHT_HAL_ENABLE_CPPCHECK)
    find_program(CPPCHECK_EXE NAMES cppcheck)
    
    if(CPPCHECK_EXE)
      # Create a target to run cppcheck on all source files
      add_custom_target(
        cppcheck-analysis
        COMMAND ${CPPCHECK_EXE}
          --enable=all
          --std=c++17
          --verbose
          --project=${CMAKE_BINARY_DIR}/compile_commands.json
          --suppressions-list=${CMAKE_SOURCE_DIR}/.cppcheck-suppressions
        COMMENT "Running cppcheck analysis"
        VERBATIM
      )
    endif()
  endif()
endfunction()

# Function to create cppcheck suppressions file
function(create_cppcheck_suppressions_file)
  set(SUPPRESSIONS_FILE "${CMAKE_SOURCE_DIR}/.cppcheck-suppressions")
  
  if(NOT EXISTS ${SUPPRESSIONS_FILE})
    file(WRITE ${SUPPRESSIONS_FILE}
      "# Flight HAL Interfaces - cppcheck suppressions\n"
      "# Suppress warnings for third-party code\n"
      "unmatchedSuppression\n"
      "missingIncludeSystem\n"
      "unusedFunction\n"
      "constParameter:*/test/*\n"
      "constParameter:*/tests/*\n" 
      "constParameter:*/examples/*\n"
      "constParameter:*/benchmarks/*\n"
    )
    message(STATUS "Created cppcheck suppressions file: ${SUPPRESSIONS_FILE}")
  endif()
endfunction()

# Function to configure include-what-you-use (if available)
function(enable_include_what_you_use target)
  find_program(IWYU_EXE
    NAMES include-what-you-use iwyu
    DOC "Path to include-what-you-use executable")

  if(IWYU_EXE)
    set(IWYU_COMMAND 
      ${IWYU_EXE}
      -Xiwyu --mapping_file=${CMAKE_SOURCE_DIR}/.iwyu-mappings
      -Xiwyu --error_on_missing_include
    )
    
    set_target_properties(${target} PROPERTIES
      CXX_INCLUDE_WHAT_YOU_USE "${IWYU_COMMAND}"
    )
    
    message(STATUS "Enabled include-what-you-use for target: ${target}")
  else()
    message(STATUS "include-what-you-use not found. Skipping IWYU for ${target}")
  endif()
endfunction()

# Function to create IWYU mappings file
function(create_iwyu_mappings_file)
  set(MAPPINGS_FILE "${CMAKE_SOURCE_DIR}/.iwyu-mappings")
  
  if(NOT EXISTS ${MAPPINGS_FILE})
    file(WRITE ${MAPPINGS_FILE}
      "[\n"
      "  { include: [\"<bits/std_abs.h>\", \"private\", \"<cstdlib>\", \"public\"] },\n"
      "  { include: [\"<bits/types/FILE.h>\", \"private\", \"<cstdio>\", \"public\"] },\n"
      "  { include: [\"<ext/alloc_traits.h>\", \"private\", \"<memory>\", \"public\"] }\n"
      "]\n"
    )
    message(STATUS "Created IWYU mappings file: ${MAPPINGS_FILE}")
  endif()
endfunction()

# Function to print static analysis configuration
function(print_static_analysis_config)
  message(STATUS "")
  message(STATUS "Flight HAL Static Analysis Configuration:")
  message(STATUS "  clang-tidy enabled:     ${FLIGHT_HAL_ENABLE_CLANG_TIDY}")
  message(STATUS "  cppcheck enabled:       ${FLIGHT_HAL_ENABLE_CPPCHECK}")
  message(STATUS "  IWYU enabled:           ${FLIGHT_HAL_ENABLE_IWYU}")
  
  if(FLIGHT_HAL_ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE 
      NAMES clang-tidy-18 clang-tidy-17 clang-tidy-16 clang-tidy-15 clang-tidy)
    if(CLANG_TIDY_EXE)
      message(STATUS "  clang-tidy path:        ${CLANG_TIDY_EXE}")
    else()
      message(STATUS "  clang-tidy path:        NOT FOUND")
    endif()
  endif()
  
  if(FLIGHT_HAL_ENABLE_CPPCHECK)
    find_program(CPPCHECK_EXE NAMES cppcheck)
    if(CPPCHECK_EXE)
      message(STATUS "  cppcheck path:          ${CPPCHECK_EXE}")
    else()
      message(STATUS "  cppcheck path:          NOT FOUND")
    endif()
  endif()
  
  message(STATUS "")
endfunction()

# Initialize static analysis configuration
if(FLIGHT_HAL_ENABLE_CPPCHECK)
  create_cppcheck_suppressions_file()
endif()

if(FLIGHT_HAL_ENABLE_IWYU)
  create_iwyu_mappings_file()
endif()

# Create custom targets for static analysis
create_static_analysis_targets()
