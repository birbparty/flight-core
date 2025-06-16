/**
 * @file validation_example.cpp
 * @brief Example demonstrating HAL validation tools usage
 * 
 * Shows how to build and use the HAL validation tools programmatically
 * and via command line.
 */

#include "../../include/flight/hal/validation/validation_core.hpp"
#include "../../include/flight/hal/validation/interface_validator.hpp"
#include <iostream>

using namespace flight::hal::validation;

/**
 * @brief Example of programmatic validation usage
 */
void demonstrate_programmatic_validation() {
    std::cout << "=== Programmatic Validation Example ===\n\n";
    
    // Create and register validators
    auto& registry = ValidationRegistry::instance();
    registry.register_validator(std::make_unique<InterfaceValidator>());
    
    // Create validation context
    InterfaceValidationContext context;
    context.interface_name = "IMemoryInterface";
    context.target_platform = PlatformType::Dreamcast;
    context.interface_instance = nullptr; // Would be real interface in production
    
    // Get validator and run validation
    auto* validator = registry.get_validator("HAL Interface Validator");
    if (validator) {
        std::cout << "Running validation for " << context.interface_name 
                  << " on platform " << static_cast<int>(context.target_platform) << "\n";
        
        auto report = validator->validate(std::any(context));
        
        // Display results
        std::cout << "\n" << ValidationUtils::format_summary(report) << "\n\n";
        
        for (const auto& result : report.get_results()) {
            std::cout << ValidationUtils::format_result(result) << "\n";
        }
    } else {
        std::cout << "Validator not found!\n";
    }
}

/**
 * @brief Example showing how to create custom validation results
 */
void demonstrate_custom_validation() {
    std::cout << "\n=== Custom Validation Example ===\n\n";
    
    ValidationReport report;
    report.set_start_time(std::chrono::system_clock::now());
    
    // Add some example validation results
    report.add_result(ValidationUtils::create_result(
        "CustomCheck1",
        ValidationCategory::Interface,
        ValidationSeverity::Info,
        "Custom validation check passed",
        "This is a demonstration of custom validation"
    ));
    
    report.add_result(ValidationUtils::create_result(
        "CustomCheck2",
        ValidationCategory::Performance,
        ValidationSeverity::Warning,
        "Performance threshold exceeded",
        "Operation took 150ms, threshold is 100ms"
    ));
    
    report.add_result(ValidationUtils::create_result(
        "CustomCheck3",
        ValidationCategory::ResourceUsage,
        ValidationSeverity::Error,
        "Memory leak detected",
        "Allocation at 0x12345678 was not freed"
    ));
    
    report.set_end_time(std::chrono::system_clock::now());
    
    // Display custom results
    std::cout << ValidationUtils::format_summary(report) << "\n\n";
    
    // Show results by category
    auto performance_results = report.get_results_by_category(ValidationCategory::Performance);
    std::cout << "Performance Results (" << performance_results.size() << "):\n";
    for (const auto& result : performance_results) {
        std::cout << "  " << ValidationUtils::format_result(result) << "\n";
    }
}

/**
 * @brief Print build and usage instructions
 */
void print_build_instructions() {
    std::cout << "=== HAL Validation Tools Build Instructions ===\n\n";
    
    std::cout << "1. Build the validation tools:\n";
    std::cout << "   mkdir build && cd build\n";
    std::cout << "   cmake .. -DBUILD_TOOLS=ON\n";
    std::cout << "   make hal_validator\n\n";
    
    std::cout << "2. Run validation directly:\n";
    std::cout << "   ./tools/compliance_validator/hal_validator --help\n";
    std::cout << "   ./tools/compliance_validator/hal_validator --list-validators\n";
    std::cout << "   ./tools/compliance_validator/hal_validator --interface IMemoryInterface --platform dreamcast\n\n";
    
    std::cout << "3. Install tools (optional):\n";
    std::cout << "   make install\n";
    std::cout << "   hal_validator --all --platform psp\n";
    std::cout << "   hal_validate.sh validate-memory --platform dreamcast\n\n";
    
    std::cout << "4. Platform-specific examples:\n";
    std::cout << "   # Dreamcast (16MB memory constraint)\n";
    std::cout << "   hal_validator --interface IMemoryInterface --platform dreamcast --verbose\n\n";
    std::cout << "   # PSP (portable, WiFi capable)\n";
    std::cout << "   hal_validator --interface INetworkInterface --platform psp\n\n";
    std::cout << "   # Web (sandbox restrictions)\n";
    std::cout << "   hal_validator --interface IGraphicsInterface --platform web\n\n";
    std::cout << "   # Desktop (high performance)\n";
    std::cout << "   hal_validator --all --platform macos --output json\n\n";
}

int main() {
    print_build_instructions();
    demonstrate_programmatic_validation();
    demonstrate_custom_validation();
    
    std::cout << "\n=== Next Steps ===\n";
    std::cout << "1. Build the tools with the commands above\n";
    std::cout << "2. Run validation on your target platform\n";
    std::cout << "3. Review the validation documentation: docs/hal_validation_tools_guide.md\n";
    std::cout << "4. Integrate validation into your development workflow\n";
    
    return 0;
}
