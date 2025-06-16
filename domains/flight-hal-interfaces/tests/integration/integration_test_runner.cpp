/**
 * @file integration_test_runner.cpp
 * @brief Main executable for running HAL integration tests
 * 
 * Provides command-line interface for running integration tests with
 * various filtering and reporting options.
 */

#include "framework/integration_test_base.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>

using namespace flight::hal::integration;

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help           Show this help message\n";
    std::cout << "  -l, --list           List all available tests\n";
    std::cout << "  -r, --run TEST       Run specific test by name\n";
    std::cout << "  -p, --pattern REGEX  Run tests matching regex pattern\n";
    std::cout << "  -a, --all            Run all tests (default)\n";
    std::cout << "  -v, --verbose        Enable verbose output\n";
    std::cout << "  --report FILE        Save test report to file\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << program_name << " --list\n";
    std::cout << "  " << program_name << " --run gpu_audio_coordination\n";
    std::cout << "  " << program_name << " --pattern \".*gpu.*\" --verbose\n";
    std::cout << "  " << program_name << " --all --report results.txt\n";
}

void list_tests() {
    auto& registry = IntegrationTestRegistry::instance();
    auto test_names = registry.get_test_names();
    
    if (test_names.empty()) {
        std::cout << "No integration tests found.\n";
        return;
    }
    
    std::cout << "Available integration tests:\n";
    for (const auto& name : test_names) {
        auto test = registry.create_test(name);
        if (test) {
            auto scenario = test->get_scenario();
            std::cout << "  " << name << " - " << scenario.description << "\n";
        }
    }
}

bool save_report(const std::string& filename, const std::string& report) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file '" << filename << "' for writing.\n";
        return false;
    }
    
    file << report;
    file.close();
    
    if (file.fail()) {
        std::cerr << "Error: Failed to write to file '" << filename << "'.\n";
        return false;
    }
    
    return true;
}

int main(int argc, char* argv[]) {
    IntegrationTestRunner runner;
    std::string test_name;
    std::string pattern;
    std::string report_file;
    bool list_mode = false;
    bool run_all = true;
    bool verbose = false;
    bool run_specific = false;
    bool run_pattern = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        else if (std::strcmp(argv[i], "-l") == 0 || std::strcmp(argv[i], "--list") == 0) {
            list_mode = true;
            run_all = false;
        }
        else if (std::strcmp(argv[i], "-r") == 0 || std::strcmp(argv[i], "--run") == 0) {
            if (i + 1 < argc) {
                test_name = argv[++i];
                run_specific = true;
                run_all = false;
            } else {
                std::cerr << "Error: --run requires a test name.\n";
                return 1;
            }
        }
        else if (std::strcmp(argv[i], "-p") == 0 || std::strcmp(argv[i], "--pattern") == 0) {
            if (i + 1 < argc) {
                pattern = argv[++i];
                run_pattern = true;
                run_all = false;
            } else {
                std::cerr << "Error: --pattern requires a regex pattern.\n";
                return 1;
            }
        }
        else if (std::strcmp(argv[i], "-a") == 0 || std::strcmp(argv[i], "--all") == 0) {
            run_all = true;
            run_specific = false;
            run_pattern = false;
        }
        else if (std::strcmp(argv[i], "-v") == 0 || std::strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        }
        else if (std::strcmp(argv[i], "--report") == 0) {
            if (i + 1 < argc) {
                report_file = argv[++i];
            } else {
                std::cerr << "Error: --report requires a filename.\n";
                return 1;
            }
        }
        else {
            std::cerr << "Error: Unknown option '" << argv[i] << "'.\n";
            print_usage(argv[0]);
            return 1;
        }
    }
    
    // Handle list mode
    if (list_mode) {
        list_tests();
        return 0;
    }
    
    // Configure runner
    runner.set_verbose(verbose);
    
    std::vector<TestResult> results;
    
    try {
        // Run tests based on mode
        if (run_specific) {
            if (verbose) {
                std::cout << "Running specific test: " << test_name << "\n\n";
            }
            auto result = runner.run_test(test_name);
            results.push_back(result);
        }
        else if (run_pattern) {
            if (verbose) {
                std::cout << "Running tests matching pattern: " << pattern << "\n\n";
            }
            auto& registry = IntegrationTestRegistry::instance();
            results = registry.run_tests_matching(pattern);
        }
        else if (run_all) {
            if (verbose) {
                std::cout << "Running all integration tests...\n\n";
            }
            results = runner.run_all_tests();
        }
        
        // Generate and display report
        auto report = runner.generate_report(results);
        std::cout << report;
        
        // Save report to file if requested
        if (!report_file.empty()) {
            if (save_report(report_file, report)) {
                std::cout << "Report saved to: " << report_file << "\n";
            } else {
                return 1;
            }
        }
        
        // Return appropriate exit code
        bool all_passed = true;
        for (const auto& result : results) {
            if (!result.passed()) {
                all_passed = false;
                break;
            }
        }
        
        return all_passed ? 0 : 1;
        
    } catch (const std::exception& e) {
        std::cerr << "Error running integration tests: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Unknown error running integration tests.\n";
        return 1;
    }
}
