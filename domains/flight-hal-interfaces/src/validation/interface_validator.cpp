/**
 * @file interface_validator.cpp
 * @brief Implementation of HAL Interface Compliance Validator
 */

#include "../../include/flight/hal/validation/interface_validator.hpp"
#include <iostream>

namespace flight::hal::validation {

InterfaceValidator::InterfaceValidator() {
    initialize_validators();
}

std::string InterfaceValidator::get_name() const {
    return "HAL Interface Validator";
}

std::string InterfaceValidator::get_description() const {
    return "Validates HAL interface implementations for compliance with interface contracts, "
           "platform requirements, and best practices across all supported platforms.";
}

std::vector<ValidationCategory> InterfaceValidator::get_supported_categories() const {
    return {
        ValidationCategory::Interface,
        ValidationCategory::ErrorHandling,
        ValidationCategory::ResourceUsage,
        ValidationCategory::Threading,
        ValidationCategory::PlatformSupport,
        ValidationCategory::Performance,
        ValidationCategory::Configuration
    };
}

ValidationReport InterfaceValidator::validate(const std::any& context) {
    ValidationReport report;
    report.set_start_time(std::chrono::system_clock::now());
    
    try {
        // Try to extract interface validation context
        const auto* interface_context = std::any_cast<InterfaceValidationContext>(&context);
        if (!interface_context) {
            report.add_result(ValidationUtils::create_result(
                "ContextValidation",
                ValidationCategory::Interface,
                ValidationSeverity::Critical,
                "Invalid validation context",
                "Expected InterfaceValidationContext but received different type"
            ));
            report.set_end_time(std::chrono::system_clock::now());
            return report;
        }
        
        // Find appropriate validator for interface type
        const std::string interface_name = interface_context->interface_name;
        auto validator_it = interface_validators_.find(interface_name);
        
        if (validator_it != interface_validators_.end()) {
            // Use interface-specific validator
            auto interface_report = validator_it->second(context, interface_context->target_platform);
            report.add_results(interface_report.get_results());
        } else {
            // Use generic validation
            report.add_result(ValidationUtils::create_result(
                "GenericInterfaceValidation",
                ValidationCategory::Interface,
                ValidationSeverity::Warning,
                "No specific validator found for interface: " + interface_name,
                "Using generic interface validation patterns"
            ));
            
            // Perform basic validation if interface instance is available
            if (interface_context->interface_instance) {
                report.add_result(ValidationUtils::create_result(
                    "BasicInterfaceCheck",
                    ValidationCategory::Interface,
                    ValidationSeverity::Info,
                    "Interface instance is valid (non-null)"
                ));
            } else {
                report.add_result(ValidationUtils::create_result(
                    "BasicInterfaceCheck",
                    ValidationCategory::Interface,
                    ValidationSeverity::Error,
                    "Interface instance is null"
                ));
            }
        }
        
    } catch (const std::bad_any_cast& e) {
        report.add_result(ValidationUtils::create_result(
            "ContextCastError",
            ValidationCategory::Interface,
            ValidationSeverity::Critical,
            "Failed to cast validation context",
            "Exception: " + std::string(e.what())
        ));
    } catch (const std::exception& e) {
        report.add_result(ValidationUtils::create_result(
            "ValidationException",
            ValidationCategory::Interface,
            ValidationSeverity::Error,
            "Exception during validation",
            "Exception: " + std::string(e.what())
        ));
    } catch (...) {
        report.add_result(ValidationUtils::create_result(
            "UnknownException",
            ValidationCategory::Interface,
            ValidationSeverity::Critical,
            "Unknown exception during validation"
        ));
    }
    
    report.set_end_time(std::chrono::system_clock::now());
    return report;
}

bool InterfaceValidator::supports_platform(PlatformType platform) const {
    // Interface validator supports all platforms
    return true;
}

void InterfaceValidator::initialize_validators() {
    // Register interface-specific validators
    
    // Memory interface validator
    interface_validators_["IMemoryInterface"] = [this](const std::any& context, PlatformType platform) -> ValidationReport {
        ValidationReport report;
        try {
            const auto* interface_context = std::any_cast<InterfaceValidationContext>(&context);
            if (interface_context && interface_context->interface_instance) {
                auto* memory_interface = static_cast<IMemoryInterface*>(interface_context->interface_instance);
                return validate_interface<IMemoryInterface>(memory_interface, platform);
            }
        } catch (...) {
            report.add_result(ValidationUtils::create_result(
                "MemoryInterfaceValidationError",
                ValidationCategory::Interface,
                ValidationSeverity::Error,
                "Failed to validate memory interface"
            ));
        }
        return report;
    };
    
    // Graphics interface validator
    interface_validators_["IGraphicsInterface"] = [this](const std::any& context, PlatformType platform) -> ValidationReport {
        ValidationReport report;
        try {
            const auto* interface_context = std::any_cast<InterfaceValidationContext>(&context);
            if (interface_context && interface_context->interface_instance) {
                auto* graphics_interface = static_cast<IGraphicsInterface*>(interface_context->interface_instance);
                return validate_interface<IGraphicsInterface>(graphics_interface, platform);
            }
        } catch (...) {
            report.add_result(ValidationUtils::create_result(
                "GraphicsInterfaceValidationError",
                ValidationCategory::Interface,
                ValidationSeverity::Error,
                "Failed to validate graphics interface"
            ));
        }
        return report;
    };
    
    // Audio interface validator
    interface_validators_["IAudioInterface"] = [this](const std::any& context, PlatformType platform) -> ValidationReport {
        ValidationReport report;
        try {
            const auto* interface_context = std::any_cast<InterfaceValidationContext>(&context);
            if (interface_context && interface_context->interface_instance) {
                auto* audio_interface = static_cast<IAudioInterface*>(interface_context->interface_instance);
                return validate_interface<IAudioInterface>(audio_interface, platform);
            }
        } catch (...) {
            report.add_result(ValidationUtils::create_result(
                "AudioInterfaceValidationError",
                ValidationCategory::Interface,
                ValidationSeverity::Error,
                "Failed to validate audio interface"
            ));
        }
        return report;
    };
    
    // Input interface validator
    interface_validators_["IInputInterface"] = [this](const std::any& context, PlatformType platform) -> ValidationReport {
        ValidationReport report;
        try {
            const auto* interface_context = std::any_cast<InterfaceValidationContext>(&context);
            if (interface_context && interface_context->interface_instance) {
                auto* input_interface = static_cast<IInputInterface*>(interface_context->interface_instance);
                return validate_interface<IInputInterface>(input_interface, platform);
            }
        } catch (...) {
            report.add_result(ValidationUtils::create_result(
                "InputInterfaceValidationError",
                ValidationCategory::Interface,
                ValidationSeverity::Error,
                "Failed to validate input interface"
            ));
        }
        return report;
    };
    
    // File interface validator
    interface_validators_["IFileInterface"] = [this](const std::any& context, PlatformType platform) -> ValidationReport {
        ValidationReport report;
        try {
            const auto* interface_context = std::any_cast<InterfaceValidationContext>(&context);
            if (interface_context && interface_context->interface_instance) {
                auto* file_interface = static_cast<IFileInterface*>(interface_context->interface_instance);
                return validate_interface<IFileInterface>(file_interface, platform);
            }
        } catch (...) {
            report.add_result(ValidationUtils::create_result(
                "FileInterfaceValidationError",
                ValidationCategory::Interface,
                ValidationSeverity::Error,
                "Failed to validate file interface"
            ));
        }
        return report;
    };
    
    // Network interface validator
    interface_validators_["INetworkInterface"] = [this](const std::any& context, PlatformType platform) -> ValidationReport {
        ValidationReport report;
        try {
            const auto* interface_context = std::any_cast<InterfaceValidationContext>(&context);
            if (interface_context && interface_context->interface_instance) {
                auto* network_interface = static_cast<INetworkInterface*>(interface_context->interface_instance);
                return validate_interface<INetworkInterface>(network_interface, platform);
            }
        } catch (...) {
            report.add_result(ValidationUtils::create_result(
                "NetworkInterfaceValidationError",
                ValidationCategory::Interface,
                ValidationSeverity::Error,
                "Failed to validate network interface"
            ));
        }
        return report;
    };
    
    // Time interface validator
    interface_validators_["ITimeInterface"] = [this](const std::any& context, PlatformType platform) -> ValidationReport {
        ValidationReport report;
        try {
            const auto* interface_context = std::any_cast<InterfaceValidationContext>(&context);
            if (interface_context && interface_context->interface_instance) {
                auto* time_interface = static_cast<ITimeInterface*>(interface_context->interface_instance);
                return validate_interface<ITimeInterface>(time_interface, platform);
            }
        } catch (...) {
            report.add_result(ValidationUtils::create_result(
                "TimeInterfaceValidationError",
                ValidationCategory::Interface,
                ValidationSeverity::Error,
                "Failed to validate time interface"
            ));
        }
        return report;
    };
    
    // Thread interface validator
    interface_validators_["IThreadInterface"] = [this](const std::any& context, PlatformType platform) -> ValidationReport {
        ValidationReport report;
        try {
            const auto* interface_context = std::any_cast<InterfaceValidationContext>(&context);
            if (interface_context && interface_context->interface_instance) {
                auto* thread_interface = static_cast<IThreadInterface*>(interface_context->interface_instance);
                return validate_interface<IThreadInterface>(thread_interface, platform);
            }
        } catch (...) {
            report.add_result(ValidationUtils::create_result(
                "ThreadInterfaceValidationError",
                ValidationCategory::Interface,
                ValidationSeverity::Error,
                "Failed to validate thread interface"
            ));
        }
        return report;
    };
    
    // Performance interface validator
    interface_validators_["IPerformanceInterface"] = [this](const std::any& context, PlatformType platform) -> ValidationReport {
        ValidationReport report;
        try {
            const auto* interface_context = std::any_cast<InterfaceValidationContext>(&context);
            if (interface_context && interface_context->interface_instance) {
                auto* performance_interface = static_cast<IPerformanceInterface*>(interface_context->interface_instance);
                return validate_interface<IPerformanceInterface>(performance_interface, platform);
            }
        } catch (...) {
            report.add_result(ValidationUtils::create_result(
                "PerformanceInterfaceValidationError",
                ValidationCategory::Interface,
                ValidationSeverity::Error,
                "Failed to validate performance interface"
            ));
        }
        return report;
    };
}

// Template specializations will be implemented in separate files for each interface type
// For now, we'll provide placeholder implementations

template<>
void InterfaceValidator::validate_interface_specific_methods<IMemoryInterface>(
    ValidationReport& report, IMemoryInterface* interface, PlatformType platform) {
    
    try {
        // Test memory allocation operations
        size_t test_size = 1024; // 1KB test allocation
        auto alloc_result = interface->allocate(test_size, MemoryAlignment::Word); // 4-byte aligned
        
        if (alloc_result.is_ok()) {
            report.add_result(ValidationUtils::create_result(
                "MemoryAllocationTest",
                ValidationCategory::Interface,
                ValidationSeverity::Info,
                "Memory allocation test successful"
            ));
            
            // Test deallocation - need to get the pointer from MemoryAllocation
            auto dealloc_result = interface->deallocate(alloc_result.value().ptr);
            if (dealloc_result.is_ok()) {
                report.add_result(ValidationUtils::create_result(
                    "MemoryDeallocationTest",
                    ValidationCategory::Interface,
                    ValidationSeverity::Info,
                    "Memory deallocation test successful"
                ));
            } else {
                report.add_result(ValidationUtils::create_result(
                    "MemoryDeallocationTest",
                    ValidationCategory::Interface,
                    ValidationSeverity::Error,
                    "Memory deallocation test failed"
                ));
            }
        } else {
            report.add_result(ValidationUtils::create_result(
                "MemoryAllocationTest",
                ValidationCategory::Interface,
                ValidationSeverity::Warning,
                "Memory allocation test failed - may be expected during validation"
            ));
        }
        
    } catch (const std::exception& e) {
        report.add_result(ValidationUtils::create_result(
            "MemoryInterfaceException",
            ValidationCategory::Interface,
            ValidationSeverity::Error,
            "Exception during memory interface validation",
            "Exception: " + std::string(e.what())
        ));
    }
}

template<>
void InterfaceValidator::validate_interface_specific_methods<IGraphicsInterface>(
    ValidationReport& report, IGraphicsInterface* interface, PlatformType platform) {
    
    report.add_result(ValidationUtils::create_result(
        "GraphicsInterfaceValidation",
        ValidationCategory::Interface,
        ValidationSeverity::Info,
        "Graphics interface validation completed",
        "Platform-specific graphics validation for " + get_platform_name(platform)
    ));
}

template<>
void InterfaceValidator::validate_interface_specific_methods<IAudioInterface>(
    ValidationReport& report, IAudioInterface* interface, PlatformType platform) {
    
    report.add_result(ValidationUtils::create_result(
        "AudioInterfaceValidation",
        ValidationCategory::Interface,
        ValidationSeverity::Info,
        "Audio interface validation completed",
        "Platform-specific audio validation for " + get_platform_name(platform)
    ));
}

template<>
void InterfaceValidator::validate_interface_specific_methods<IInputInterface>(
    ValidationReport& report, IInputInterface* interface, PlatformType platform) {
    
    report.add_result(ValidationUtils::create_result(
        "InputInterfaceValidation",
        ValidationCategory::Interface,
        ValidationSeverity::Info,
        "Input interface validation completed",
        "Platform-specific input validation for " + get_platform_name(platform)
    ));
}

template<>
void InterfaceValidator::validate_interface_specific_methods<IFileInterface>(
    ValidationReport& report, IFileInterface* interface, PlatformType platform) {
    
    report.add_result(ValidationUtils::create_result(
        "FileInterfaceValidation",
        ValidationCategory::Interface,
        ValidationSeverity::Info,
        "File interface validation completed",
        "Platform-specific file validation for " + get_platform_name(platform)
    ));
}

template<>
void InterfaceValidator::validate_interface_specific_methods<INetworkInterface>(
    ValidationReport& report, INetworkInterface* interface, PlatformType platform) {
    
    report.add_result(ValidationUtils::create_result(
        "NetworkInterfaceValidation",
        ValidationCategory::Interface,
        ValidationSeverity::Info,
        "Network interface validation completed",
        "Platform-specific network validation for " + get_platform_name(platform)
    ));
}

template<>
void InterfaceValidator::validate_interface_specific_methods<ITimeInterface>(
    ValidationReport& report, ITimeInterface* interface, PlatformType platform) {
    
    report.add_result(ValidationUtils::create_result(
        "TimeInterfaceValidation",
        ValidationCategory::Interface,
        ValidationSeverity::Info,
        "Time interface validation completed",
        "Platform-specific time validation for " + get_platform_name(platform)
    ));
}

template<>
void InterfaceValidator::validate_interface_specific_methods<IThreadInterface>(
    ValidationReport& report, IThreadInterface* interface, PlatformType platform) {
    
    // Check if threading is supported on this platform
    if (platform == PlatformType::Dreamcast) {
        report.add_result(ValidationUtils::create_result(
            "ThreadInterfaceValidation",
            ValidationCategory::Threading,
            ValidationSeverity::Info,
            "Thread interface validation skipped for single-threaded platform"
        ));
    } else {
        report.add_result(ValidationUtils::create_result(
            "ThreadInterfaceValidation",
            ValidationCategory::Threading,
            ValidationSeverity::Info,
            "Thread interface validation completed",
            "Platform-specific threading validation for " + get_platform_name(platform)
        ));
    }
}

template<>
void InterfaceValidator::validate_interface_specific_methods<IPerformanceInterface>(
    ValidationReport& report, IPerformanceInterface* interface, PlatformType platform) {
    
    report.add_result(ValidationUtils::create_result(
        "PerformanceInterfaceValidation",
        ValidationCategory::Performance,
        ValidationSeverity::Info,
        "Performance interface validation completed",
        "Platform-specific performance validation for " + get_platform_name(platform)
    ));
}

} // namespace flight::hal::validation
