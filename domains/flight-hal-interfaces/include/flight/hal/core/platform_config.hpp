/**
 * @file platform_config.hpp
 * @brief Flight HAL Platform Configuration System
 * 
 * Provides initialization-time configuration for platform coordination,
 * driver management, and performance monitoring settings.
 */

#pragma once

#include "hal_result.hpp"
#include "hal_logging.hpp"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <any>
#include <chrono>
#include <vector>

namespace flight::hal {

/**
 * @brief Memory budget configuration
 */
struct MemoryBudget {
    size_t total_budget_mb = 512;           ///< Total memory budget in MB
    size_t graphics_budget_mb = 256;        ///< Graphics memory budget in MB
    size_t audio_budget_mb = 64;            ///< Audio memory budget in MB
    size_t network_buffer_mb = 32;          ///< Network buffer budget in MB
    size_t file_cache_mb = 128;             ///< File cache budget in MB
    bool enforce_budgets = false;           ///< Whether to enforce budgets (monitoring only for now)
    
    /**
     * @brief Validate memory budget configuration
     * @return HALResult indicating validity
     */
    HALResult<void> validate() const;
};

/**
 * @brief Performance monitoring configuration
 */
struct PerformanceConfig {
    bool enable_telemetry = true;           ///< Enable performance telemetry
    bool enable_resource_tracking = true;   ///< Track resource usage
    bool enable_driver_profiling = true;    ///< Profile individual drivers
    bool enable_coordination_metrics = true; ///< Track cross-driver coordination
    
    // Monitoring intervals
    std::chrono::milliseconds telemetry_interval{1000};     ///< Telemetry collection interval
    std::chrono::milliseconds resource_check_interval{500}; ///< Resource usage check interval
    std::chrono::milliseconds metric_report_interval{5000}; ///< Metric reporting interval
    
    // Performance thresholds (for reporting, not enforcement)
    double cpu_usage_warning_threshold = 80.0;     ///< CPU usage warning %
    double memory_usage_warning_threshold = 90.0;  ///< Memory usage warning %
    double latency_warning_threshold_ms = 16.0;    ///< Frame latency warning in ms
};

/**
 * @brief Resource coordination configuration
 */
struct ResourceCoordinationConfig {
    bool enable_cross_driver_sharing = true;       ///< Enable resource sharing between drivers
    bool enable_priority_arbitration = true;       ///< Enable priority-based resource arbitration
    bool enable_deadlock_prevention = true;        ///< Enable deadlock prevention mechanisms
    
    std::chrono::milliseconds resource_timeout{5000};      ///< Default resource acquisition timeout
    std::chrono::milliseconds coordination_timeout{1000};  ///< Cross-driver coordination timeout
    
    uint32_t max_concurrent_resources = 32;        ///< Maximum concurrent resource handles
    uint32_t resource_retry_attempts = 3;          ///< Number of retry attempts for resource acquisition
};

/**
 * @brief Driver initialization configuration
 */
struct DriverInitConfig {
    bool auto_discover_drivers = true;             ///< Automatically discover available drivers
    bool fail_fast_on_critical_drivers = true;    ///< Fail initialization if critical drivers fail
    bool enable_driver_fallbacks = true;          ///< Enable fallback drivers when primary fails
    
    std::chrono::milliseconds driver_init_timeout{10000};  ///< Per-driver initialization timeout
    std::chrono::milliseconds platform_init_timeout{30000}; ///< Total platform initialization timeout
    
    // Critical driver types that must initialize successfully
    std::vector<std::string> critical_drivers{"memory", "time", "thread"};
    
    // Driver-specific initialization order (optional)
    std::vector<std::string> initialization_order;
};

/**
 * @brief Comprehensive platform configuration
 */
struct PlatformConfig {
    // Core configuration sections
    MemoryBudget memory_budget;
    PerformanceConfig performance;
    ResourceCoordinationConfig resource_coordination;
    DriverInitConfig driver_init;
    
    // Global settings
    LogLevel debug_level = LogLevel::Info;
    bool enable_debug_output = false;
    bool enable_validation_checks = true;
    
    // Platform-specific overrides
    std::unordered_map<std::string, std::any> platform_overrides;
    
    // Driver-specific configurations
    std::unordered_map<std::string, std::any> driver_configs;
    
    /**
     * @brief Create default configuration
     * @return Default platform configuration
     */
    static PlatformConfig create_default();
    
    /**
     * @brief Create minimal configuration for resource-constrained platforms
     * @return Minimal platform configuration
     */
    static PlatformConfig create_minimal();
    
    /**
     * @brief Create high-performance configuration
     * @return High-performance platform configuration
     */
    static PlatformConfig create_high_performance();
    
    /**
     * @brief Validate the entire configuration
     * @return HALResult indicating validity with detailed error information
     */
    HALResult<void> validate() const;
    
    /**
     * @brief Set driver-specific configuration
     * @tparam ConfigType Type of the driver configuration
     * @param driver_name Name of the driver
     * @param config Driver configuration object
     */
    template<typename ConfigType>
    void set_driver_config(const std::string& driver_name, const ConfigType& config);
    
    /**
     * @brief Get driver-specific configuration
     * @tparam ConfigType Type of the driver configuration
     * @param driver_name Name of the driver
     * @return Pointer to driver configuration or nullptr if not found
     */
    template<typename ConfigType>
    const ConfigType* get_driver_config(const std::string& driver_name) const;
    
    /**
     * @brief Set platform-specific override
     * @tparam ValueType Type of the override value
     * @param key Override key
     * @param value Override value
     */
    template<typename ValueType>
    void set_platform_override(const std::string& key, const ValueType& value);
    
    /**
     * @brief Get platform-specific override
     * @tparam ValueType Type of the override value
     * @param key Override key
     * @return Pointer to override value or nullptr if not found
     */
    template<typename ValueType>
    const ValueType* get_platform_override(const std::string& key) const;
};

/**
 * @brief Platform configuration loader and validator
 */
class PlatformConfigManager {
public:
    /**
     * @brief Load configuration from JSON file
     * @param config_path Path to configuration file
     * @return HALResult containing loaded configuration or error
     */
    static HALResult<PlatformConfig> load_from_file(const std::string& config_path);
    
    /**
     * @brief Save configuration to JSON file
     * @param config Configuration to save
     * @param config_path Path to save configuration
     * @return HALResult indicating success or failure
     */
    static HALResult<void> save_to_file(const PlatformConfig& config, const std::string& config_path);
    
    /**
     * @brief Load configuration from environment variables
     * @param base_config Base configuration to override
     * @return HALResult containing configuration with environment overrides
     */
    static HALResult<PlatformConfig> load_from_environment(const PlatformConfig& base_config = PlatformConfig::create_default());
    
    /**
     * @brief Merge two configurations (overlay takes precedence)
     * @param base Base configuration
     * @param overlay Configuration to overlay
     * @return Merged configuration
     */
    static PlatformConfig merge_configs(const PlatformConfig& base, const PlatformConfig& overlay);
    
    /**
     * @brief Validate configuration for specific platform
     * @param config Configuration to validate
     * @param platform_name Target platform name
     * @return HALResult indicating validity
     */
    static HALResult<void> validate_for_platform(const PlatformConfig& config, const std::string& platform_name);

private:
    /**
     * @brief Parse JSON configuration
     */
    static HALResult<PlatformConfig> parse_json_config(const std::string& json_content);
    
    /**
     * @brief Generate JSON from configuration
     */
    static HALResult<std::string> generate_json_config(const PlatformConfig& config);
};

// Template implementations
template<typename ConfigType>
void PlatformConfig::set_driver_config(const std::string& driver_name, const ConfigType& config) {
    driver_configs[driver_name] = config;
}

template<typename ConfigType>
const ConfigType* PlatformConfig::get_driver_config(const std::string& driver_name) const {
    auto it = driver_configs.find(driver_name);
    if (it != driver_configs.end()) {
        try {
            return &std::any_cast<const ConfigType&>(it->second);
        } catch (const std::bad_any_cast&) {
            return nullptr;
        }
    }
    return nullptr;
}

template<typename ValueType>
void PlatformConfig::set_platform_override(const std::string& key, const ValueType& value) {
    platform_overrides[key] = value;
}

template<typename ValueType>
const ValueType* PlatformConfig::get_platform_override(const std::string& key) const {
    auto it = platform_overrides.find(key);
    if (it != platform_overrides.end()) {
        try {
            return &std::any_cast<const ValueType&>(it->second);
        } catch (const std::bad_any_cast&) {
            return nullptr;
        }
    }
    return nullptr;
}

} // namespace flight::hal
