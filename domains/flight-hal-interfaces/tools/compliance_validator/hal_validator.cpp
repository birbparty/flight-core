/**
 * @file hal_validator.cpp
 * @brief HAL Interface Validation Command-Line Tool
 * 
 * Comprehensive validation tool for HAL interface implementations.
 * Provides interface compliance checking, driver validation, and
 * platform-specific requirement verification.
 */

#include "../../include/flight/hal/validation/validation_core.hpp"
#include "../../include/flight/hal/validation/interface_validator.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <getopt.h>

using namespace flight::hal::validation;

/**
 * @brief Command-line options structure
 */
struct CommandLineOptions {
    std::string interface_name;
    std::string platform;
    std::string output_format = "console";
    std::string output_file;
    bool verbose = false;
    bool help = false;
    bool list_validators = false;
    bool list_platforms = false;
    bool run_all_checks = false;
    std::vector<std::string> specific_checks;
};

/**
 * @brief Available validation modes
 */
enum class ValidationMode {
    Interface,          ///< Validate specific interface
    Driver,            ///< Validate driver implementation
    Platform,          ///< Validate platform compliance
    All               ///< Run all available validations
};

/**
 * @brief Output formatters
 */
class OutputFormatter {
public:
    virtual ~OutputFormatter() = default;
    virtual void format_report(const ValidationReport& report, std::ostream& output) = 0;
    virtual std::string get_name() const = 0;
};

/**
 * @brief Console output formatter
 */
class ConsoleFormatter : public OutputFormatter {
public:
    void format_report(const ValidationReport& report, std::ostream& output) override {
        const auto& summary = report.get_summary();
        
        output << "=== HAL Validation Report ===\n";
        output << "Duration: " << summary.duration.count() << "ms\n";
        output << "Total Checks: " << summary.total_checks << "\n";
        output << "Status: " << (summary.passed ? "PASSED" : "FAILED") << "\n\n";
        
        // Results by severity
        auto errors = report.get_results_by_severity(ValidationSeverity::Error);
        auto criticals = report.get_results_by_severity(ValidationSeverity::Critical);
        auto warnings = report.get_results_by_severity(ValidationSeverity::Warning);
        auto infos = report.get_results_by_severity(ValidationSeverity::Info);
        
        if (!criticals.empty()) {
            output << "CRITICAL ISSUES (" << criticals.size() << "):\n";
            for (const auto& result : criticals) {
                output << "  ❌ " << result.check_name << ": " << result.message << "\n";
                if (!result.details.empty()) {
                    output << "     " << result.details << "\n";
                }
            }
            output << "\n";
        }
        
        if (!errors.empty()) {
            output << "ERRORS (" << errors.size() << "):\n";
            for (const auto& result : errors) {
                output << "  🔴 " << result.check_name << ": " << result.message << "\n";
                if (!result.details.empty()) {
                    output << "     " << result.details << "\n";
                }
            }
            output << "\n";
        }
        
        if (!warnings.empty()) {
            output << "WARNINGS (" << warnings.size() << "):\n";
            for (const auto& result : warnings) {
                output << "  🟡 " << result.check_name << ": " << result.message << "\n";
                if (!result.details.empty()) {
                    output << "     " << result.details << "\n";
                }
            }
            output << "\n";
        }
        
        if (!infos.empty()) {
            output << "INFO (" << infos.size() << "):\n";
            for (const auto& result : infos) {
                output << "  ✅ " << result.check_name << ": " << result.message << "\n";
            }
            output << "\n";
        }
        
        // Summary by category
        output << "Results by Category:\n";
        for (auto category : {ValidationCategory::Interface, ValidationCategory::ErrorHandling,
                             ValidationCategory::ResourceUsage, ValidationCategory::Threading,
                             ValidationCategory::PlatformSupport, ValidationCategory::Performance,
                             ValidationCategory::Configuration, ValidationCategory::Memory}) {
            auto category_results = report.get_results_by_category(category);
            if (!category_results.empty()) {
                size_t passed = 0, failed = 0;
                for (const auto& result : category_results) {
                    if (result.passed()) passed++; else failed++;
                }
                output << "  " << ValidationUtils::category_to_string(category) 
                       << ": " << passed << " passed, " << failed << " failed\n";
            }
        }
    }
    
    std::string get_name() const override { return "console"; }
};

/**
 * @brief JSON output formatter
 */
class JsonFormatter : public OutputFormatter {
public:
    void format_report(const ValidationReport& report, std::ostream& output) override {
        const auto& summary = report.get_summary();
        const auto& results = report.get_results();
        
        output << "{\n";
        output << "  \"summary\": {\n";
        output << "    \"total_checks\": " << summary.total_checks << ",\n";
        output << "    \"duration_ms\": " << summary.duration.count() << ",\n";
        output << "    \"passed\": " << (summary.passed ? "true" : "false") << ",\n";
        output << "    \"info_count\": " << summary.info_count << ",\n";
        output << "    \"warning_count\": " << summary.warning_count << ",\n";
        output << "    \"error_count\": " << summary.error_count << ",\n";
        output << "    \"critical_count\": " << summary.critical_count << "\n";
        output << "  },\n";
        output << "  \"results\": [\n";
        
        for (size_t i = 0; i < results.size(); ++i) {
            const auto& result = results[i];
            output << "    {\n";
            output << "      \"check_name\": \"" << escape_json(result.check_name) << "\",\n";
            output << "      \"category\": \"" << ValidationUtils::category_to_string(result.category) << "\",\n";
            output << "      \"severity\": \"" << ValidationUtils::severity_to_string(result.severity) << "\",\n";
            output << "      \"message\": \"" << escape_json(result.message) << "\",\n";
            output << "      \"details\": \"" << escape_json(result.details) << "\",\n";
            output << "      \"passed\": " << (result.passed() ? "true" : "false") << "\n";
            output << "    }";
            if (i < results.size() - 1) output << ",";
            output << "\n";
        }
        
        output << "  ]\n";
        output << "}\n";
    }
    
    std::string get_name() const override { return "json"; }
    
private:
    std::string escape_json(const std::string& str) {
        std::string escaped;
        for (char c : str) {
            switch (c) {
                case '"': escaped += "\\\""; break;
                case '\\': escaped += "\\\\"; break;
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                default: escaped += c; break;
            }
        }
        return escaped;
    }
};

/**
 * @brief HAL Validation Tool
 */
class HALValidationTool {
public:
    HALValidationTool() {
        // Register validators
        auto& registry = ValidationRegistry::instance();
        registry.register_validator(std::make_unique<InterfaceValidator>());
        
        // Register output formatters
        formatters_["console"] = std::make_unique<ConsoleFormatter>();
        formatters_["json"] = std::make_unique<JsonFormatter>();
    }
    
    /**
     * @brief Run validation with given options
     */
    int run(const CommandLineOptions& options) {
        if (options.help) {
            print_help();
            return 0;
        }
        
        if (options.list_validators) {
            list_validators();
            return 0;
        }
        
        if (options.list_platforms) {
            list_platforms();
            return 0;
        }
        
        // Parse platform
        PlatformType platform = parse_platform(options.platform);
        if (platform == PlatformType::Unknown && !options.platform.empty()) {
            std::cerr << "Error: Unknown platform '" << options.platform << "'\n";
            return 1;
        }
        
        // Get output formatter
        auto formatter_it = formatters_.find(options.output_format);
        if (formatter_it == formatters_.end()) {
            std::cerr << "Error: Unknown output format '" << options.output_format << "'\n";
            return 1;
        }
        
        // Run validation
        ValidationReport overall_report;
        overall_report.set_start_time(std::chrono::system_clock::now());
        
        if (options.run_all_checks) {
            run_all_validations(overall_report, platform, options.verbose);
        } else if (!options.interface_name.empty()) {
            run_interface_validation(overall_report, options.interface_name, platform, options.verbose);
        } else {
            std::cerr << "Error: No validation target specified. Use --interface, --all, or --help\n";
            return 1;
        }
        
        overall_report.set_end_time(std::chrono::system_clock::now());
        
        // Output results
        output_results(overall_report, *formatter_it->second, options.output_file);
        
        return overall_report.overall_passed() ? 0 : 1;
    }

private:
    std::map<std::string, std::unique_ptr<OutputFormatter>> formatters_;
    
    void print_help() {
        std::cout << "HAL Interface Validation Tool\n\n";
        std::cout << "Usage: hal_validator [OPTIONS]\n\n";
        std::cout << "Options:\n";
        std::cout << "  --interface NAME     Validate specific interface\n";
        std::cout << "  --platform NAME      Target platform (dreamcast, psp, web, macos, windows, linux)\n";
        std::cout << "  --all               Run all available validations\n";
        std::cout << "  --output FORMAT     Output format (console, json)\n";
        std::cout << "  --output-file FILE  Write output to file\n";
        std::cout << "  --verbose           Enable verbose output\n";
        std::cout << "  --list-validators   List available validators\n";
        std::cout << "  --list-platforms    List supported platforms\n";
        std::cout << "  --help              Show this help message\n\n";
        std::cout << "Examples:\n";
        std::cout << "  hal_validator --interface memory --platform dreamcast\n";
        std::cout << "  hal_validator --all --output json --output-file report.json\n";
        std::cout << "  hal_validator --list-validators\n";
    }
    
    void list_validators() {
        auto& registry = ValidationRegistry::instance();
        auto validators = registry.get_validator_names();
        
        std::cout << "Available Validators:\n";
        for (const auto& name : validators) {
            auto* validator = registry.get_validator(name);
            if (validator) {
                std::cout << "  " << name << ": " << validator->get_description() << "\n";
                
                auto categories = validator->get_supported_categories();
                std::cout << "    Categories: ";
                for (size_t i = 0; i < categories.size(); ++i) {
                    std::cout << ValidationUtils::category_to_string(categories[i]);
                    if (i < categories.size() - 1) std::cout << ", ";
                }
                std::cout << "\n\n";
            }
        }
    }
    
    void list_platforms() {
        std::cout << "Supported Platforms:\n";
        std::cout << "  dreamcast - Sega Dreamcast\n";
        std::cout << "  psp       - Sony PlayStation Portable\n";
        std::cout << "  web       - Web/Emscripten\n";
        std::cout << "  macos     - Apple macOS\n";
        std::cout << "  windows   - Microsoft Windows\n";
        std::cout << "  linux     - Linux\n";
    }
    
    PlatformType parse_platform(const std::string& platform_str) {
        if (platform_str.empty()) return PlatformType::Unknown;
        
        if (platform_str == "dreamcast") return PlatformType::Dreamcast;
        if (platform_str == "psp") return PlatformType::PSP;
        if (platform_str == "web") return PlatformType::Web;
        if (platform_str == "macos") return PlatformType::macOS;
        if (platform_str == "windows") return PlatformType::Windows;
        if (platform_str == "linux") return PlatformType::Linux;
        
        return PlatformType::Unknown;
    }
    
    void run_all_validations(ValidationReport& report, PlatformType platform, bool verbose) {
        if (verbose) {
            std::cout << "Running all available validations...\n";
        }
        
        // Run interface validations for all known interfaces
        std::vector<std::string> interfaces = {
            "IMemoryInterface", "IGraphicsInterface", "IAudioInterface",
            "IInputInterface", "IFileInterface", "INetworkInterface",
            "ITimeInterface", "IThreadInterface", "IPerformanceInterface"
        };
        
        for (const auto& interface_name : interfaces) {
            if (verbose) {
                std::cout << "Validating " << interface_name << "...\n";
            }
            run_interface_validation(report, interface_name, platform, false);
        }
    }
    
    void run_interface_validation(ValidationReport& report, const std::string& interface_name, 
                                 PlatformType platform, bool verbose) {
        auto& registry = ValidationRegistry::instance();
        auto* validator = registry.get_validator("HAL Interface Validator");
        
        if (!validator) {
            report.add_result(ValidationUtils::create_result(
                "ValidatorNotFound",
                ValidationCategory::Interface,
                ValidationSeverity::Critical,
                "HAL Interface Validator not found in registry"
            ));
            return;
        }
        
        // Create validation context
        InterfaceValidationContext context;
        context.interface_name = interface_name;
        context.target_platform = platform;
        context.interface_instance = nullptr; // In a real implementation, this would be a real interface
        
        try {
            auto interface_report = validator->validate(std::any(context));
            report.add_results(interface_report.get_results());
            
            if (verbose) {
                std::cout << "Completed validation for " << interface_name 
                         << " (" << interface_report.get_results().size() << " checks)\n";
            }
        } catch (const std::exception& e) {
            report.add_result(ValidationUtils::create_result(
                "ValidationException",
                ValidationCategory::Interface,
                ValidationSeverity::Error,
                "Exception during " + interface_name + " validation",
                "Exception: " + std::string(e.what())
            ));
        }
    }
    
    void output_results(const ValidationReport& report, OutputFormatter& formatter, 
                       const std::string& output_file) {
        if (output_file.empty()) {
            formatter.format_report(report, std::cout);
        } else {
            std::ofstream file(output_file);
            if (file.is_open()) {
                formatter.format_report(report, file);
                std::cout << "Results written to " << output_file << "\n";
            } else {
                std::cerr << "Error: Could not open output file " << output_file << "\n";
                formatter.format_report(report, std::cout);
            }
        }
    }
};

/**
 * @brief Parse command line arguments
 */
CommandLineOptions parse_arguments(int argc, char* argv[]) {
    CommandLineOptions options;
    
    static struct option long_options[] = {
        {"interface", required_argument, 0, 'i'},
        {"platform", required_argument, 0, 'p'},
        {"all", no_argument, 0, 'a'},
        {"output", required_argument, 0, 'o'},
        {"output-file", required_argument, 0, 'f'},
        {"verbose", no_argument, 0, 'v'},
        {"list-validators", no_argument, 0, 'l'},
        {"list-platforms", no_argument, 0, 'P'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int c;
    int option_index = 0;
    
    while ((c = getopt_long(argc, argv, "i:p:ao:f:vlPh", long_options, &option_index)) != -1) {
        switch (c) {
            case 'i':
                options.interface_name = optarg;
                break;
            case 'p':
                options.platform = optarg;
                break;
            case 'a':
                options.run_all_checks = true;
                break;
            case 'o':
                options.output_format = optarg;
                break;
            case 'f':
                options.output_file = optarg;
                break;
            case 'v':
                options.verbose = true;
                break;
            case 'l':
                options.list_validators = true;
                break;
            case 'P':
                options.list_platforms = true;
                break;
            case 'h':
                options.help = true;
                break;
            case '?':
                // Error message already printed by getopt_long
                exit(1);
                break;
            default:
                break;
        }
    }
    
    return options;
}

/**
 * @brief Main entry point
 */
int main(int argc, char* argv[]) {
    try {
        auto options = parse_arguments(argc, argv);
        
        HALValidationTool tool;
        return tool.run(options);
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Fatal error: Unknown exception\n";
        return 1;
    }
}
