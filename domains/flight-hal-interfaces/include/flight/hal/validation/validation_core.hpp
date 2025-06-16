/**
 * @file validation_core.hpp
 * @brief Core Validation Infrastructure for HAL Interface Validation
 * 
 * Provides the foundational framework for HAL validation tools including
 * validation result handling, metrics collection, and common validation
 * patterns used across all validation components.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <functional>
#include <optional>
#include <variant>
#include <sstream>
#include <any>
#include <algorithm>

#include "../core/hal_result.hpp"
#include "../core/hal_error.hpp"
#include "../core/hal_capabilities.hpp"
#include "../core/platform_detection.hpp"

namespace flight::hal::validation {

/**
 * @brief Platform type enumeration for validation
 */
enum class PlatformType {
    Unknown = 0,
    Dreamcast,
    PSP,
    Web,
    macOS,
    Windows,
    Linux
};

/**
 * @brief Severity levels for validation results
 */
enum class ValidationSeverity {
    Info = 0,      ///< Informational messages
    Warning,       ///< Warnings that don't prevent operation
    Error,         ///< Errors that prevent proper operation
    Critical       ///< Critical errors that indicate system failure
};

/**
 * @brief Categories of validation checks
 */
enum class ValidationCategory {
    Interface,          ///< Interface compliance validation
    Performance,        ///< Performance-related validation
    ResourceUsage,      ///< Resource usage validation
    ErrorHandling,      ///< Error handling validation
    Configuration,      ///< Configuration validation
    PlatformSupport,    ///< Platform-specific validation
    Threading,          ///< Thread safety validation
    Memory             ///< Memory management validation
};

/**
 * @brief Individual validation result
 */
struct ValidationResult {
    std::string check_name;                                 ///< Name of the validation check
    ValidationCategory category;                            ///< Category of validation
    ValidationSeverity severity;                           ///< Severity level
    std::string message;                                   ///< Human-readable message
    std::string details;                                   ///< Additional details
    std::chrono::system_clock::time_point timestamp;      ///< When validation occurred
    std::optional<std::string> file_location;             ///< Source file location if applicable
    std::optional<size_t> line_number;                    ///< Line number if applicable
    std::unordered_map<std::string, std::string> metadata; ///< Additional metadata
    
    /**
     * @brief Check if validation passed
     * @return true if severity is Info or Warning
     */
    bool passed() const {
        return severity == ValidationSeverity::Info || 
               severity == ValidationSeverity::Warning;
    }
    
    /**
     * @brief Check if validation failed
     * @return true if severity is Error or Critical
     */
    bool failed() const {
        return !passed();
    }
};

/**
 * @brief Collection of validation results with summary information
 */
class ValidationReport {
public:
    ValidationReport() = default;
    
    /**
     * @brief Add a validation result
     * @param result Validation result to add
     */
    void add_result(ValidationResult result) {
        results_.push_back(std::move(result));
        update_summary();
    }
    
    /**
     * @brief Add multiple validation results
     * @param results Vector of validation results
     */
    void add_results(const std::vector<ValidationResult>& results) {
        for (const auto& result : results) {
            add_result(result);
        }
    }
    
    /**
     * @brief Get all validation results
     * @return Vector of validation results
     */
    const std::vector<ValidationResult>& get_results() const {
        return results_;
    }
    
    /**
     * @brief Get results filtered by category
     * @param category Category to filter by
     * @return Vector of filtered results
     */
    std::vector<ValidationResult> get_results_by_category(ValidationCategory category) const {
        std::vector<ValidationResult> filtered;
        for (const auto& result : results_) {
            if (result.category == category) {
                filtered.push_back(result);
            }
        }
        return filtered;
    }
    
    /**
     * @brief Get results filtered by severity
     * @param severity Severity to filter by
     * @return Vector of filtered results
     */
    std::vector<ValidationResult> get_results_by_severity(ValidationSeverity severity) const {
        std::vector<ValidationResult> filtered;
        for (const auto& result : results_) {
            if (result.severity == severity) {
                filtered.push_back(result);
            }
        }
        return filtered;
    }
    
    /**
     * @brief Get count of results by severity
     * @param severity Severity to count
     * @return Number of results with specified severity
     */
    size_t count_by_severity(ValidationSeverity severity) const {
        size_t count = 0;
        for (const auto& result : results_) {
            if (result.severity == severity) {
                ++count;
            }
        }
        return count;
    }
    
    /**
     * @brief Check if overall validation passed
     * @return true if no errors or criticals
     */
    bool overall_passed() const {
        return count_by_severity(ValidationSeverity::Error) == 0 &&
               count_by_severity(ValidationSeverity::Critical) == 0;
    }
    
    /**
     * @brief Get summary statistics
     */
    struct Summary {
        size_t total_checks = 0;
        size_t info_count = 0;
        size_t warning_count = 0;
        size_t error_count = 0;
        size_t critical_count = 0;
        std::chrono::system_clock::time_point start_time;
        std::chrono::system_clock::time_point end_time;
        std::chrono::milliseconds duration{0};
        bool passed = false;
    };
    
    /**
     * @brief Get summary of validation results
     * @return Summary structure
     */
    const Summary& get_summary() const {
        return summary_;
    }
    
    /**
     * @brief Clear all results
     */
    void clear() {
        results_.clear();
        summary_ = Summary{};
    }
    
    /**
     * @brief Set validation start time
     * @param start_time Start time
     */
    void set_start_time(std::chrono::system_clock::time_point start_time) {
        summary_.start_time = start_time;
    }
    
    /**
     * @brief Set validation end time and calculate duration
     * @param end_time End time
     */
    void set_end_time(std::chrono::system_clock::time_point end_time) {
        summary_.end_time = end_time;
        summary_.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - summary_.start_time);
    }

private:
    std::vector<ValidationResult> results_;
    Summary summary_;
    
    void update_summary() {
        summary_.total_checks = results_.size();
        summary_.info_count = count_by_severity(ValidationSeverity::Info);
        summary_.warning_count = count_by_severity(ValidationSeverity::Warning);
        summary_.error_count = count_by_severity(ValidationSeverity::Error);
        summary_.critical_count = count_by_severity(ValidationSeverity::Critical);
        summary_.passed = overall_passed();
    }
};

/**
 * @brief Base class for all HAL validators
 */
class IValidator {
public:
    virtual ~IValidator() = default;
    
    /**
     * @brief Get validator name
     * @return Validator name
     */
    virtual std::string get_name() const = 0;
    
    /**
     * @brief Get validator description
     * @return Validator description
     */
    virtual std::string get_description() const = 0;
    
    /**
     * @brief Get supported validation categories
     * @return Vector of supported categories
     */
    virtual std::vector<ValidationCategory> get_supported_categories() const = 0;
    
    /**
     * @brief Run validation
     * @param context Validation context (can be any type)
     * @return Validation report
     */
    virtual ValidationReport validate(const std::any& context) = 0;
    
    /**
     * @brief Check if validator supports a specific platform
     * @param platform Platform to check
     * @return true if platform is supported
     */
    virtual bool supports_platform(PlatformType platform) const = 0;
};

/**
 * @brief Validation context for interface validation
 */
struct InterfaceValidationContext {
    std::string interface_name;                    ///< Name of interface being validated
    std::type_info const* interface_type = nullptr; ///< Type info of interface
    void* interface_instance = nullptr;            ///< Pointer to interface instance
    PlatformType target_platform;                 ///< Target platform
    std::unordered_map<std::string, std::any> properties; ///< Additional properties
};

/**
 * @brief Validation context for driver validation
 */
struct DriverValidationContext {
    std::string driver_name;                       ///< Name of driver being validated
    std::string driver_version;                    ///< Driver version
    void* driver_instance = nullptr;               ///< Pointer to driver instance
    PlatformType target_platform;                 ///< Target platform
    std::vector<std::string> supported_interfaces; ///< Supported interface names
    std::unordered_map<std::string, std::any> driver_config; ///< Driver configuration
};

/**
 * @brief Utility functions for validation
 */
class ValidationUtils {
public:
    /**
     * @brief Convert severity to string
     * @param severity Severity level
     * @return String representation
     */
    static std::string severity_to_string(ValidationSeverity severity) {
        switch (severity) {
            case ValidationSeverity::Info: return "INFO";
            case ValidationSeverity::Warning: return "WARNING";
            case ValidationSeverity::Error: return "ERROR";
            case ValidationSeverity::Critical: return "CRITICAL";
            default: return "UNKNOWN";
        }
    }
    
    /**
     * @brief Convert category to string
     * @param category Validation category
     * @return String representation
     */
    static std::string category_to_string(ValidationCategory category) {
        switch (category) {
            case ValidationCategory::Interface: return "Interface";
            case ValidationCategory::Performance: return "Performance";
            case ValidationCategory::ResourceUsage: return "Resource Usage";
            case ValidationCategory::ErrorHandling: return "Error Handling";
            case ValidationCategory::Configuration: return "Configuration";
            case ValidationCategory::PlatformSupport: return "Platform Support";
            case ValidationCategory::Threading: return "Threading";
            case ValidationCategory::Memory: return "Memory";
            default: return "Unknown";
        }
    }
    
    /**
     * @brief Create a validation result with current timestamp
     * @param check_name Name of the check
     * @param category Validation category
     * @param severity Severity level
     * @param message Human-readable message
     * @param details Additional details
     * @return ValidationResult structure
     */
    static ValidationResult create_result(
        const std::string& check_name,
        ValidationCategory category,
        ValidationSeverity severity,
        const std::string& message,
        const std::string& details = "") {
        
        ValidationResult result;
        result.check_name = check_name;
        result.category = category;
        result.severity = severity;
        result.message = message;
        result.details = details;
        result.timestamp = std::chrono::system_clock::now();
        return result;
    }
    
    /**
     * @brief Format validation result for display
     * @param result Validation result to format
     * @return Formatted string
     */
    static std::string format_result(const ValidationResult& result) {
        std::ostringstream oss;
        oss << "[" << severity_to_string(result.severity) << "] "
            << category_to_string(result.category) << ": "
            << result.check_name << " - " << result.message;
        
        if (!result.details.empty()) {
            oss << "\n  Details: " << result.details;
        }
        
        if (result.file_location && result.line_number) {
            oss << "\n  Location: " << *result.file_location << ":" << *result.line_number;
        }
        
        return oss.str();
    }
    
    /**
     * @brief Format validation report summary
     * @param report Validation report
     * @return Formatted summary string
     */
    static std::string format_summary(const ValidationReport& report) {
        const auto& summary = report.get_summary();
        
        std::ostringstream oss;
        oss << "Validation Summary:\n";
        oss << "  Total Checks: " << summary.total_checks << "\n";
        oss << "  Info: " << summary.info_count << "\n";
        oss << "  Warnings: " << summary.warning_count << "\n";
        oss << "  Errors: " << summary.error_count << "\n";
        oss << "  Critical: " << summary.critical_count << "\n";
        oss << "  Duration: " << summary.duration.count() << "ms\n";
        oss << "  Overall: " << (summary.passed ? "PASSED" : "FAILED");
        
        return oss.str();
    }
};

/**
 * @brief Registry for validation tools
 */
class ValidationRegistry {
public:
    /**
     * @brief Get singleton instance
     * @return Reference to singleton instance
     */
    static ValidationRegistry& instance() {
        static ValidationRegistry registry;
        return registry;
    }
    
    /**
     * @brief Register a validator
     * @param validator Unique pointer to validator
     */
    void register_validator(std::unique_ptr<IValidator> validator) {
        const std::string name = validator->get_name();
        validators_[name] = std::move(validator);
    }
    
    /**
     * @brief Get validator by name
     * @param name Validator name
     * @return Pointer to validator or nullptr if not found
     */
    IValidator* get_validator(const std::string& name) const {
        auto it = validators_.find(name);
        return (it != validators_.end()) ? it->second.get() : nullptr;
    }
    
    /**
     * @brief Get all validator names
     * @return Vector of validator names
     */
    std::vector<std::string> get_validator_names() const {
        std::vector<std::string> names;
        for (const auto& pair : validators_) {
            names.push_back(pair.first);
        }
        return names;
    }
    
    /**
     * @brief Get validators by category
     * @param category Validation category
     * @return Vector of validators supporting the category
     */
    std::vector<IValidator*> get_validators_by_category(ValidationCategory category) const {
        std::vector<IValidator*> result;
        for (const auto& pair : validators_) {
            const auto& supported = pair.second->get_supported_categories();
            if (std::find(supported.begin(), supported.end(), category) != supported.end()) {
                result.push_back(pair.second.get());
            }
        }
        return result;
    }
    
    /**
     * @brief Get validators by platform
     * @param platform Target platform
     * @return Vector of validators supporting the platform
     */
    std::vector<IValidator*> get_validators_by_platform(PlatformType platform) const {
        std::vector<IValidator*> result;
        for (const auto& pair : validators_) {
            if (pair.second->supports_platform(platform)) {
                result.push_back(pair.second.get());
            }
        }
        return result;
    }

private:
    std::unordered_map<std::string, std::unique_ptr<IValidator>> validators_;
};

} // namespace flight::hal::validation
