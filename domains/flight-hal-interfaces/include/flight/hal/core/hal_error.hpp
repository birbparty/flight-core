/**
 * @file hal_error.hpp
 * @brief Flight HAL Error Types and Categories
 * 
 * Defines structured error types for comprehensive error handling
 * across the HAL system with zero-allocation guarantees.
 */

#pragma once

#include <cstdint>

namespace flight::hal {

/**
 * @brief HAL error categories for structured error classification
 */
enum class HALErrorCategory : uint8_t {
    Hardware = 1,      ///< Device failures, initialization errors
    Driver = 2,        ///< Driver compatibility, missing features
    Configuration = 3, ///< Invalid parameters, settings
    Resource = 4,      ///< Memory exhaustion, resource limits
    Platform = 5,      ///< Platform-specific constraints
    Network = 6,       ///< Connectivity issues
    Validation = 7,    ///< Input validation failures
    Internal = 8       ///< Internal HAL system errors
};

/**
 * @brief Structured HAL error with category, code, and context
 * 
 * Provides rich error information without dynamic allocation.
 * Uses static strings and fixed-size storage for embedded compatibility.
 */
class HALError {
public:
    /**
     * @brief Construct error with category and code
     * @param category Error category classification
     * @param code Specific error code within category
     * @param message Static error message (must be string literal)
     * @param context Additional context information (optional)
     */
    constexpr HALError(HALErrorCategory category, uint32_t code, 
                      const char* message, const char* context = nullptr) noexcept
        : category_(category), code_(code), message_(message), context_(context) {}
    
    /**
     * @brief Get error category
     * @return Error category enumeration
     */
    constexpr HALErrorCategory category() const noexcept { return category_; }
    
    /**
     * @brief Get error code within category
     * @return Specific error code
     */
    constexpr uint32_t code() const noexcept { return code_; }
    
    /**
     * @brief Get error message
     * @return Static error message string
     */
    constexpr const char* message() const noexcept { return message_; }
    
    /**
     * @brief Get additional context information
     * @return Context string or nullptr if none
     */
    constexpr const char* context() const noexcept { return context_; }
    
    /**
     * @brief Get combined error ID (category + code)
     * @return Unique error identifier
     */
    constexpr uint32_t error_id() const noexcept {
        return (static_cast<uint32_t>(category_) << 24) | (code_ & 0x00FFFFFF);
    }
    
    /**
     * @brief Equality comparison
     * @param other Error to compare against
     * @return true if category and code match
     */
    constexpr bool operator==(const HALError& other) const noexcept {
        return category_ == other.category_ && code_ == other.code_;
    }
    
    /**
     * @brief Inequality comparison
     * @param other Error to compare against
     * @return true if category or code differ
     */
    constexpr bool operator!=(const HALError& other) const noexcept {
        return !(*this == other);
    }

private:
    HALErrorCategory category_;
    uint32_t code_;
    const char* message_;
    const char* context_;
};

// Standard HAL error definitions with static strings
namespace errors {

// Hardware errors
constexpr HALError device_not_found(uint32_t code, const char* context = nullptr) {
    return HALError(HALErrorCategory::Hardware, code, "Device not found", context);
}

constexpr HALError initialization_failed(uint32_t code, const char* context = nullptr) {
    return HALError(HALErrorCategory::Hardware, code, "Hardware initialization failed", context);
}

constexpr HALError device_busy(uint32_t code, const char* context = nullptr) {
    return HALError(HALErrorCategory::Hardware, code, "Device is busy", context);
}

// Driver errors
constexpr HALError driver_incompatible(uint32_t code, const char* context = nullptr) {
    return HALError(HALErrorCategory::Driver, code, "Driver incompatible", context);
}

constexpr HALError feature_not_supported(uint32_t code, const char* context = nullptr) {
    return HALError(HALErrorCategory::Driver, code, "Feature not supported", context);
}

constexpr HALError driver_not_loaded(uint32_t code, const char* context = nullptr) {
    return HALError(HALErrorCategory::Driver, code, "Driver not loaded", context);
}

// Configuration errors
constexpr HALError invalid_parameter(uint32_t code, const char* context = nullptr) {
    return HALError(HALErrorCategory::Configuration, code, "Invalid parameter", context);
}

constexpr HALError configuration_missing(uint32_t code, const char* context = nullptr) {
    return HALError(HALErrorCategory::Configuration, code, "Configuration missing", context);
}

constexpr HALError parameter_out_of_range(uint32_t code, const char* context = nullptr) {
    return HALError(HALErrorCategory::Configuration, code, "Parameter out of range", context);
}

// Resource errors
constexpr HALError out_of_memory(uint32_t code, const char* context = nullptr) {
    return HALError(HALErrorCategory::Resource, code, "Out of memory", context);
}

constexpr HALError resource_exhausted(uint32_t code, const char* context = nullptr) {
    return HALError(HALErrorCategory::Resource, code, "Resource exhausted", context);
}

constexpr HALError resource_locked(uint32_t code, const char* context = nullptr) {
    return HALError(HALErrorCategory::Resource, code, "Resource locked", context);
}

// Platform errors
constexpr HALError platform_not_supported(uint32_t code, const char* context = nullptr) {
    return HALError(HALErrorCategory::Platform, code, "Platform not supported", context);
}

constexpr HALError platform_constraint(uint32_t code, const char* context = nullptr) {
    return HALError(HALErrorCategory::Platform, code, "Platform constraint violation", context);
}

// Network errors
constexpr HALError connection_failed(uint32_t code, const char* context = nullptr) {
    return HALError(HALErrorCategory::Network, code, "Connection failed", context);
}

constexpr HALError network_timeout(uint32_t code, const char* context = nullptr) {
    return HALError(HALErrorCategory::Network, code, "Network timeout", context);
}

// Validation errors
constexpr HALError validation_failed(uint32_t code, const char* context = nullptr) {
    return HALError(HALErrorCategory::Validation, code, "Validation failed", context);
}

constexpr HALError invalid_state(uint32_t code, const char* context = nullptr) {
    return HALError(HALErrorCategory::Validation, code, "Invalid state", context);
}

// Internal errors
constexpr HALError internal_error(uint32_t code, const char* context = nullptr) {
    return HALError(HALErrorCategory::Internal, code, "Internal HAL error", context);
}

constexpr HALError not_implemented(uint32_t code, const char* context = nullptr) {
    return HALError(HALErrorCategory::Internal, code, "Feature not implemented", context);
}

} // namespace errors

/**
 * @brief Get string representation of error category
 * @param category Error category to convert
 * @return Static string representation
 */
constexpr const char* category_to_string(HALErrorCategory category) noexcept {
    switch (category) {
        case HALErrorCategory::Hardware: return "Hardware";
        case HALErrorCategory::Driver: return "Driver";
        case HALErrorCategory::Configuration: return "Configuration";
        case HALErrorCategory::Resource: return "Resource";
        case HALErrorCategory::Platform: return "Platform";
        case HALErrorCategory::Network: return "Network";
        case HALErrorCategory::Validation: return "Validation";
        case HALErrorCategory::Internal: return "Internal";
        default: return "Unknown";
    }
}

} // namespace flight::hal
