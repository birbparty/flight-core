# Flight HAL Interfaces Testing Configuration
# Configures testing framework, benchmarks, and coverage reporting using modern CMake

include(CTest)

# Only configure testing if this is the main project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    
    # Set default parallel job count if not specified
    if(NOT CMAKE_BUILD_PARALLEL_LEVEL)
        include(ProcessorCount)
        ProcessorCount(N)
        if(NOT N EQUAL 0)
            set(CMAKE_BUILD_PARALLEL_LEVEL ${N})
        else()
            set(CMAKE_BUILD_PARALLEL_LEVEL 4)  # Fallback to 4 jobs
        endif()
    endif()
    
    # Testing is enabled based on options, dependencies are handled by Dependencies.cmake
    set(FLIGHT_HAL_ENABLE_UNIT_TESTS ${FLIGHT_HAL_BUILD_TESTS})
    set(FLIGHT_HAL_ENABLE_BENCHMARKS ${FLIGHT_HAL_BUILD_BENCHMARKS})
    
    # Enhanced test configuration function
    function(flight_hal_add_test test_name)
        if(FLIGHT_HAL_ENABLE_UNIT_TESTS)
            add_executable(${test_name} ${ARGN})
            
            # Link libraries based on dependency system
            if(NOT FLIGHT_HAL_USE_SYSTEM_DEPS)
                target_link_libraries(${test_name} 
                    PRIVATE 
                        flight-hal-interfaces
                        gtest
                        gtest_main
                        gmock
                        gmock_main
                )
            else()
                target_link_libraries(${test_name} 
                    PRIVATE 
                        flight-hal-interfaces
                        GTest::gtest 
                        GTest::gtest_main
                        GTest::gmock
                        GTest::gmock_main
                )
            endif()
            
            # Apply compiler settings and warnings
            flight_hal_configure_compiler_settings(${test_name})
            if(FLIGHT_HAL_ENABLE_WARNINGS)
                set_project_warnings(${test_name})
            endif()
            
            # Apply sanitizers if enabled
            flight_hal_enable_sanitizers_for_target(${test_name})
            
            # Apply static analysis if enabled
            flight_hal_enable_static_analysis(${test_name})
            
            # Set test properties
            set_target_properties(${test_name} PROPERTIES
                CXX_STANDARD 17
                CXX_STANDARD_REQUIRED ON
                CXX_EXTENSIONS OFF
                RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
            )
            
            # Add to test suite
            add_test(NAME ${test_name} COMMAND ${test_name})
            
            # Test timeout and working directory
            set_tests_properties(${test_name} PROPERTIES 
                TIMEOUT 300
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests
            )
            
            # Add test labels for categorization
            if(test_name MATCHES "_unit_")
                set_tests_properties(${test_name} PROPERTIES LABELS "unit")
            elseif(test_name MATCHES "_integration_")
                set_tests_properties(${test_name} PROPERTIES LABELS "integration")
            elseif(test_name MATCHES "_compliance_")
                set_tests_properties(${test_name} PROPERTIES LABELS "compliance")
            elseif(test_name MATCHES "_stress_")
                set_tests_properties(${test_name} PROPERTIES LABELS "stress")
            endif()
            
            message(STATUS "Added test: ${test_name}")
        endif()
    endfunction()
    
    # Enhanced benchmark configuration function
    function(flight_hal_add_benchmark benchmark_name)
        if(FLIGHT_HAL_ENABLE_BENCHMARKS)
            add_executable(${benchmark_name} ${ARGN})
            
            # Link libraries based on dependency system
            if(NOT FLIGHT_HAL_USE_SYSTEM_DEPS)
                target_link_libraries(${benchmark_name} 
                    PRIVATE 
                        flight-hal-interfaces
                        benchmark::benchmark
                        benchmark::benchmark_main
                )
            else()
                target_link_libraries(${benchmark_name} 
                    PRIVATE 
                        flight-hal-interfaces
                        benchmark::benchmark
                        benchmark::benchmark_main
                )
            endif()
            
            # Apply compiler settings and warnings
            flight_hal_configure_compiler_settings(${benchmark_name})
            if(FLIGHT_HAL_ENABLE_WARNINGS)
                set_project_warnings(${benchmark_name})
            endif()
            
            # Set benchmark properties
            set_target_properties(${benchmark_name} PROPERTIES
                CXX_STANDARD 17
                CXX_STANDARD_REQUIRED ON
                CXX_EXTENSIONS OFF
                RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/benchmarks
            )
            
            # Benchmarks are not added to CTest by default
            # They can be run manually or via custom targets
            message(STATUS "Added benchmark: ${benchmark_name}")
        endif()
    endfunction()
    
    # Enhanced coverage configuration
    if(FLIGHT_HAL_ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # Coverage function for targets
        function(enable_coverage target)
            target_compile_options(${target} PRIVATE
                --coverage
                -fprofile-arcs
                -ftest-coverage
                -O0
                -g
            )
            target_link_libraries(${target} PRIVATE --coverage)
            message(STATUS "Enabled coverage for target: ${target}")
        endfunction()
        
        # Apply coverage to main library
        enable_coverage(flight-hal-interfaces)
        
        # Find coverage tools
        find_program(GCOV_PATH gcov)
        find_program(LCOV_PATH lcov)
        find_program(GENHTML_PATH genhtml)
        find_program(GCOVR_PATH gcovr)
        
        if(GCOV_PATH AND LCOV_PATH AND GENHTML_PATH)
            # Comprehensive coverage target
            add_custom_target(coverage
                # Clean previous coverage data
                COMMAND ${LCOV_PATH} --directory . --zerocounters --quiet
                
                # Run tests
                COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --parallel ${CMAKE_BUILD_PARALLEL_LEVEL}
                
                # Capture coverage data
                COMMAND ${LCOV_PATH} --directory . --capture --output-file coverage_raw.info --quiet
                
                # Remove system and third-party files from coverage
                COMMAND ${LCOV_PATH} --remove coverage_raw.info 
                    '/usr/*' 
                    '/opt/*'
                    '*/tests/*' 
                    '*/benchmarks/*' 
                    '*/examples/*'
                    '*/_deps/*'
                    '*/build/*'
                    --output-file coverage.info --quiet
                
                # Generate HTML report
                COMMAND ${GENHTML_PATH} coverage.info 
                    --output-directory coverage_report 
                    --title "Flight HAL Interfaces Coverage Report"
                    --num-spaces 4
                    --sort
                    --function-coverage
                    --branch-coverage
                    --quiet
                
                # Generate summary
                COMMAND ${LCOV_PATH} --summary coverage.info
                
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Generating comprehensive coverage report"
                VERBATIM
            )
            
            # Coverage summary target
            add_custom_target(coverage-summary
                COMMAND ${LCOV_PATH} --summary coverage.info
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Displaying coverage summary"
                VERBATIM
            )
        elseif(GCOVR_PATH)
            # Alternative coverage using gcovr
            add_custom_target(coverage
                COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
                COMMAND ${GCOVR_PATH} --root ${CMAKE_SOURCE_DIR} --html --html-details 
                    --output coverage_report/index.html
                    --exclude '.*/_deps/.*'
                    --exclude '.*/tests/.*'
                    --exclude '.*/benchmarks/.*'
                    --exclude '.*/examples/.*'
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Generating coverage report with gcovr"
                VERBATIM
            )
        else()
            message(WARNING "Coverage tools not found. Coverage reporting disabled.")
        endif()
    endif()
    
    # Enhanced test discovery and organization
    set(FLIGHT_HAL_TEST_CATEGORIES
        unit
        integration
        compliance
        stress
        performance
    )
    
    # Create test category targets with parallel execution
    foreach(category ${FLIGHT_HAL_TEST_CATEGORIES})
        add_custom_target(test_${category}
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --parallel ${CMAKE_BUILD_PARALLEL_LEVEL} -L ${category}
            COMMENT "Running ${category} tests in parallel"
        )
    endforeach()
    
    # Additional test targets
    add_custom_target(test_all
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --parallel ${CMAKE_BUILD_PARALLEL_LEVEL}
        COMMENT "Running all tests in parallel"
    )
    
    add_custom_target(test_verbose
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
        COMMENT "Running tests with verbose output"
    )
    
    add_custom_target(test_memcheck
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --memcheck
        COMMENT "Running tests with memory checking"
    )
    
    # Benchmark targets
    if(FLIGHT_HAL_ENABLE_BENCHMARKS)
        add_custom_target(benchmark_all
            COMMENT "Running all benchmarks"
        )
    endif()
    
    # Helper function to print testing configuration
    function(print_testing_config)
        message(STATUS "")
        message(STATUS "Flight HAL Testing Configuration:")
        message(STATUS "  Unit tests enabled:     ${FLIGHT_HAL_ENABLE_UNIT_TESTS}")
        message(STATUS "  Benchmarks enabled:     ${FLIGHT_HAL_ENABLE_BENCHMARKS}")
        message(STATUS "  Coverage enabled:       ${FLIGHT_HAL_ENABLE_COVERAGE}")
        message(STATUS "  Parallel jobs:          ${CMAKE_BUILD_PARALLEL_LEVEL}")
        
        if(FLIGHT_HAL_ENABLE_COVERAGE)
            if(GCOV_PATH AND LCOV_PATH AND GENHTML_PATH)
                message(STATUS "  Coverage tool:          lcov + genhtml")
            elseif(GCOVR_PATH)
                message(STATUS "  Coverage tool:          gcovr")
            else()
                message(STATUS "  Coverage tool:          NONE FOUND")
            endif()
        endif()
        
        message(STATUS "")
    endfunction()
    
endif()
