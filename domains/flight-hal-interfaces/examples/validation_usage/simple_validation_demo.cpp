/**
 * @file simple_validation_demo.cpp
 * @brief Simple demonstration of HAL validation framework
 * 
 * Shows the validation system without complex interface dependencies
 */

#include "../../include/flight/hal/validation/validation_core.hpp"
#include <iostream>
#include <chrono>

using namespace flight::hal::validation;

/**
 * @brief Simple demonstration validator
 */
class DemoValidator : public IValidator {
public:
    std::string get_name() const override {
        return "Demo Validator";
    }
    
    std::string get_description() const override {
        return "Simple demonstration validator for HAL validation framework";
    }
    
    std::vector<ValidationCategory> get_supported_categories() const override {
        return {
            ValidationCategory::Interface,
            ValidationCategory::Performance,
            ValidationCategory::Configuration
        };
    }
    
    ValidationReport validate(const std::any& context) override {
        ValidationReport report;
        report.set_start_time(std::chrono::system_clock::now());
        
        // Simulate various validation checks
        report.add_result(ValidationUtils::create_result(
            "BasicInterfaceCheck",
            ValidationCategory::Interface,
            ValidationSeverity::Info,
            "Interface validation passed",
            "All required methods are properly defined"
        ));
        
        report.add_result(ValidationUtils::create_result(
            "PerformanceCheck",
            ValidationCategory::Performance,
            ValidationSeverity::Warning,
            "Performance threshold exceeded",
            "Operation took 150ms, recommended threshold is 100ms"
        ));
        
        report.add_result(ValidationUtils::create_result(
            "ConfigurationCheck",
            ValidationCategory::Configuration,
            ValidationSeverity::Info,
            "Configuration parameters valid",
            "All configuration settings are within expected ranges"
        ));
        
        report.add_result(ValidationUtils::create_result(
            "ErrorHandlingCheck",
            ValidationCategory::ErrorHandling,
            ValidationSeverity::Error,
            "Error handling incomplete",
            "Some error paths are not properly handled"
        ));
        
        report.add_result(ValidationUtils::create_result(
            "ResourceCheck",
            ValidationCategory::ResourceUsage,
            ValidationSeverity::Critical,
            "Memory leak detected",
            "Resource allocated at startup was not properly released"
        ));
        
        report.set_end_time(std::chrono::system_clock::now());
        return report;
    }
    
    bool supports_platform(PlatformType platform) const override {
        // Support all platforms for demo
        return true;
    }
};

/**
 * @brief Demonstrate the validation registry
 */
void demonstrate_validation_registry() {
    std::cout << "=== Validation Registry Demo ===\n\n";
    
    // Get registry instance
    auto& registry = ValidationRegistry::instance();
    
    // Register our demo validator
    registry.register_validator(std::make_unique<DemoValidator>());
    
    // List available validators
    auto validator_names = registry.get_validator_names();
    std::cout << "Available validators:\n";
    for (const auto& name : validator_names) {
        auto* validator = registry.get_validator(name);
        if (validator) {
            std::cout << "  - " << name << ": " << validator->get_description() << "\n";
        }
    }
    std::cout << "\n";
    
    // Get validators by category
    auto interface_validators = registry.get_validators_by_category(ValidationCategory::Interface);
    std::cout << "Interface validators: " << interface_validators.size() << "\n";
    
    auto performance_validators = registry.get_validators_by_category(ValidationCategory::Performance);
    std::cout << "Performance validators: " << performance_validators.size() << "\n\n";
}

/**
 * @brief Demonstrate validation execution
 */
void demonstrate_validation_execution() {
    std::cout << "=== Validation Execution Demo ===\n\n";
    
    auto& registry = ValidationRegistry::instance();
    auto* validator = registry.get_validator("Demo Validator");
    
    if (validator) {
        std::cout << "Running validation with: " << validator->get_name() << "\n\n";
        
        // Create dummy context
        std::string context_data = "demo_context";
        auto report = validator->validate(std::any(context_data));
        
        // Display summary
        std::cout << ValidationUtils::format_summary(report) << "\n\n";
        
        // Display individual results
        std::cout << "Detailed Results:\n";
        for (const auto& result : report.get_results()) {
            std::cout << ValidationUtils::format_result(result) << "\n\n";
        }
    } else {
        std::cout << "Demo validator not found!\n";
    }
}

/**
 * @brief Demonstrate platform-specific validation
 */
void demonstrate_platform_validation() {
    std::cout << "=== Platform-Specific Validation Demo ===\n\n";
    
    auto& registry = ValidationRegistry::instance();
    
    // Test different platforms
    std::vector<PlatformType> platforms = {
        PlatformType::Dreamcast,
        PlatformType::PSP,
        PlatformType::Web,
        PlatformType::macOS
    };
    
    for (auto platform : platforms) {
        auto platform_validators = registry.get_validators_by_platform(platform);
        std::cout << "Platform " << static_cast<int>(platform) 
                  << " supported validators: " << platform_validators.size() << "\n";
    }
    std::cout << "\n";
}

/**
 * @brief Demonstrate validation filtering and analysis
 */
void demonstrate_validation_analysis() {
    std::cout << "=== Validation Analysis Demo ===\n\n";
    
    auto& registry = ValidationRegistry::instance();
    auto* validator = registry.get_validator("Demo Validator");
    
    if (validator) {
        auto report = validator->validate(std::any(std::string("analysis_demo")));
        
        // Analyze by severity
        std::cout << "Results by Severity:\n";
        auto criticals = report.get_results_by_severity(ValidationSeverity::Critical);
        auto errors = report.get_results_by_severity(ValidationSeverity::Error);
        auto warnings = report.get_results_by_severity(ValidationSeverity::Warning);
        auto infos = report.get_results_by_severity(ValidationSeverity::Info);
        
        std::cout << "  Critical: " << criticals.size() << "\n";
        std::cout << "  Errors: " << errors.size() << "\n";
        std::cout << "  Warnings: " << warnings.size() << "\n";
        std::cout << "  Info: " << infos.size() << "\n\n";
        
        // Analyze by category
        std::cout << "Results by Category:\n";
        for (auto category : {ValidationCategory::Interface, ValidationCategory::Performance,
                             ValidationCategory::ErrorHandling, ValidationCategory::ResourceUsage,
                             ValidationCategory::Configuration}) {
            auto category_results = report.get_results_by_category(category);
            if (!category_results.empty()) {
                std::cout << "  " << ValidationUtils::category_to_string(category) 
                         << ": " << category_results.size() << " checks\n";
            }
        }
        
        // Overall status
        std::cout << "\nOverall Status: " << (report.overall_passed() ? "PASSED" : "FAILED") << "\n\n";
    }
}

/**
 * @brief Print build instructions
 */
void print_build_instructions() {
    std::cout << "=== HAL Validation Tools Build Instructions ===\n\n";
    std::cout << "This demo shows the validation framework in action!\n\n";
    std::cout << "To build the full validation tools:\n";
    std::cout << "1. mkdir build && cd build\n";
    std::cout << "2. cmake .. -DBUILD_TOOLS=ON\n";
    std::cout << "3. make hal_validator\n";
    std::cout << "4. ./tools/compliance_validator/hal_validator --help\n\n";
}

int main() {
    print_build_instructions();
    demonstrate_validation_registry();
    demonstrate_validation_execution();
    demonstrate_platform_validation();
    demonstrate_validation_analysis();
    
    std::cout << "=== Demo Complete ===\n";
    std::cout << "The validation framework is working correctly!\n";
    std::cout << "Check the full documentation: docs/hal_validation_tools_guide.md\n";
    
    return 0;
}
