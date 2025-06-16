/**
 * @file platform_config.cpp
 * @brief Flight HAL Platform Configuration System Implementation
 */

#include "flight/hal/core/platform_config.hpp"
#include "flight/hal/core/hal_error.hpp"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <algorithm>

namespace flight::hal {

// MemoryBudget implementation
HALResult<void> MemoryBudget::validate() const {
    if (total_budget_mb == 0) {
        return HALResult<void>::error(errors::invalid_parameter(1, "Total memory budget cannot be zero"));
    }
    
    size_t allocated_budget = graphics_budget_mb + audio_budget_mb + network_buffer_mb + file_cache_mb;
    if (allocated_budget > total_budget_mb) {
        return HALResult<void>::error(errors::parameter_out_of_range(2, "Sum of allocated budgets exceeds total budget"));
    }
    
    // Warn if budgets are very small
    if (graphics_budget_mb < 16) {
        // Log warning but don't fail validation
    }
    
    return HALResult<void>::success();
}

// PlatformConfig static factory methods
PlatformConfig PlatformConfig::create_default() {
    PlatformConfig config;
    
    // Memory budget defaults
    config.memory_budget.total_budget_mb = 512;
    config.memory_budget.graphics_budget_mb = 256;
    config.memory_budget.audio_budget_mb = 64;
    config.memory_budget.network_buffer_mb = 32;
    config.memory_budget.file_cache_mb = 128;
    config.memory_budget.enforce_budgets = false;
    
    // Performance defaults
    config.performance.enable_telemetry = true;
    config.performance.enable_resource_tracking = true;
    config.performance.enable_driver_profiling = true;
    config.performance.enable_coordination_metrics = true;
    config.performance.telemetry_interval = std::chrono::milliseconds(1000);
    config.performance.resource_check_interval = std::chrono::milliseconds(500);
    config.performance.metric_report_interval = std::chrono::milliseconds(5000);
    config.performance.cpu_usage_warning_threshold = 80.0;
    config.performance.memory_usage_warning_threshold = 90.0;
    config.performance.latency_warning_threshold_ms = 16.0;
    
    // Resource coordination defaults
    config.resource_coordination.enable_cross_driver_sharing = true;
    config.resource_coordination.enable_priority_arbitration = true;
    config.resource_coordination.enable_deadlock_prevention = true;
    config.resource_coordination.resource_timeout = std::chrono::milliseconds(5000);
    config.resource_coordination.coordination_timeout = std::chrono::milliseconds(1000);
    config.resource_coordination.max_concurrent_resources = 32;
    config.resource_coordination.resource_retry_attempts = 3;
    
    // Driver initialization defaults
    config.driver_init.auto_discover_drivers = true;
    config.driver_init.fail_fast_on_critical_drivers = true;
    config.driver_init.enable_driver_fallbacks = true;
    config.driver_init.driver_init_timeout = std::chrono::milliseconds(10000);
    config.driver_init.platform_init_timeout = std::chrono::milliseconds(30000);
    config.driver_init.critical_drivers = {"memory", "time", "thread"};
    
    // Global defaults
    config.debug_level = LogLevel::Info;
    config.enable_debug_output = false;
    config.enable_validation_checks = true;
    
    return config;
}

PlatformConfig PlatformConfig::create_minimal() {
    PlatformConfig config = create_default();
    
    // Reduce memory budgets for resource-constrained environments
    config.memory_budget.total_budget_mb = 128;
    config.memory_budget.graphics_budget_mb = 64;
    config.memory_budget.audio_budget_mb = 16;
    config.memory_budget.network_buffer_mb = 8;
    config.memory_budget.file_cache_mb = 32;
    
    // Reduce monitoring overhead
    config.performance.enable_driver_profiling = false;
    config.performance.enable_coordination_metrics = false;
    config.performance.telemetry_interval = std::chrono::milliseconds(5000);
    config.performance.resource_check_interval = std::chrono::milliseconds(2000);
    config.performance.metric_report_interval = std::chrono::milliseconds(10000);
    
    // Reduce resource coordination overhead
    config.resource_coordination.max_concurrent_resources = 16;
    config.resource_coordination.resource_retry_attempts = 1;
    
    // Faster timeouts for minimal systems
    config.driver_init.driver_init_timeout = std::chrono::milliseconds(5000);
    config.driver_init.platform_init_timeout = std::chrono::milliseconds(15000);
    
    // Reduce validation overhead
    config.enable_validation_checks = false;
    config.debug_level = LogLevel::Warning;
    
    return config;
}

PlatformConfig PlatformConfig::create_high_performance() {
    PlatformConfig config = create_default();
    
    // Increase memory budgets for high performance
    config.memory_budget.total_budget_mb = 2048;
    config.memory_budget.graphics_budget_mb = 1024;
    config.memory_budget.audio_budget_mb = 256;
    config.memory_budget.network_buffer_mb = 128;
    config.memory_budget.file_cache_mb = 512;
    
    // Enhanced monitoring for performance analysis
    config.performance.enable_telemetry = true;
    config.performance.enable_resource_tracking = true;
    config.performance.enable_driver_profiling = true;
    config.performance.enable_coordination_metrics = true;
    config.performance.telemetry_interval = std::chrono::milliseconds(250);
    config.performance.resource_check_interval = std::chrono::milliseconds(100);
    config.performance.metric_report_interval = std::chrono::milliseconds(1000);
    
    // Stricter performance thresholds
    config.performance.cpu_usage_warning_threshold = 70.0;
    config.performance.memory_usage_warning_threshold = 85.0;
    config.performance.latency_warning_threshold_ms = 8.0;
    
    // Enhanced resource coordination
    config.resource_coordination.max_concurrent_resources = 128;
    config.resource_coordination.resource_retry_attempts = 5;
    config.resource_coordination.resource_timeout = std::chrono::milliseconds(1000);
    config.resource_coordination.coordination_timeout = std::chrono::milliseconds(500);
    
    // Longer timeouts for complex initialization
    config.driver_init.driver_init_timeout = std::chrono::milliseconds(15000);
    config.driver_init.platform_init_timeout = std::chrono::milliseconds(60000);
    
    // Enhanced validation and debugging
    config.enable_validation_checks = true;
    config.enable_debug_output = true;
    config.debug_level = LogLevel::Debug;
    
    return config;
}

HALResult<void> PlatformConfig::validate() const {
    // Validate memory budget
    auto memory_result = memory_budget.validate();
    if (!memory_result.is_success()) {
        return memory_result;
    }
    
    // Validate performance configuration
    if (performance.telemetry_interval.count() <= 0) {
        return HALResult<void>::error(errors::invalid_parameter(3, "Telemetry interval must be positive"));
    }
    
    if (performance.resource_check_interval.count() <= 0) {
        return HALResult<void>::error(errors::invalid_parameter(4, "Resource check interval must be positive"));
    }
    
    if (performance.cpu_usage_warning_threshold < 0.0 || performance.cpu_usage_warning_threshold > 100.0) {
        return HALResult<void>::error(errors::parameter_out_of_range(5, "CPU usage warning threshold must be between 0 and 100"));
    }
    
    if (performance.memory_usage_warning_threshold < 0.0 || performance.memory_usage_warning_threshold > 100.0) {
        return HALResult<void>::error(errors::parameter_out_of_range(6, "Memory usage warning threshold must be between 0 and 100"));
    }
    
    // Validate resource coordination configuration
    if (resource_coordination.resource_timeout.count() <= 0) {
        return HALResult<void>::error(errors::invalid_parameter(7, "Resource timeout must be positive"));
    }
    
    if (resource_coordination.coordination_timeout.count() <= 0) {
        return HALResult<void>::error(errors::invalid_parameter(8, "Coordination timeout must be positive"));
    }
    
    if (resource_coordination.max_concurrent_resources == 0) {
        return HALResult<void>::error(errors::invalid_parameter(9, "Max concurrent resources must be positive"));
    }
    
    // Validate driver initialization configuration
    if (driver_init.driver_init_timeout.count() <= 0) {
        return HALResult<void>::error(errors::invalid_parameter(10, "Driver initialization timeout must be positive"));
    }
    
    if (driver_init.platform_init_timeout.count() <= 0) {
        return HALResult<void>::error(errors::invalid_parameter(11, "Platform initialization timeout must be positive"));
    }
    
    // Ensure platform timeout is larger than driver timeout
    if (driver_init.platform_init_timeout <= driver_init.driver_init_timeout) {
        return HALResult<void>::error(errors::invalid_parameter(12, "Platform initialization timeout must be greater than driver initialization timeout"));
    }
    
    return HALResult<void>::success();
}

// PlatformConfigManager implementation
HALResult<PlatformConfig> PlatformConfigManager::load_from_file(const std::string& config_path) {
    std::ifstream file(config_path);
    if (!file.is_open()) {
        return HALResult<PlatformConfig>::error(errors::configuration_missing(13, "Could not open configuration file"));
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json_content = buffer.str();
    
    if (json_content.empty()) {
        return HALResult<PlatformConfig>::error(errors::validation_failed(14, "Configuration file is empty"));
    }
    
    return parse_json_config(json_content);
}

HALResult<void> PlatformConfigManager::save_to_file(const PlatformConfig& config, const std::string& config_path) {
    auto json_result = generate_json_config(config);
    if (!json_result.is_success()) {
        return HALResult<void>::error(json_result.error());
    }
    
    std::ofstream file(config_path);
    if (!file.is_open()) {
        return HALResult<void>::error(errors::configuration_missing(15, "Could not create configuration file"));
    }
    
    file << json_result.value();
    if (file.fail()) {
        return HALResult<void>::error(errors::internal_error(16, "Failed to write configuration file"));
    }
    
    return HALResult<void>::success();
}

HALResult<PlatformConfig> PlatformConfigManager::load_from_environment(const PlatformConfig& base_config) {
    PlatformConfig config = base_config;
    
    // Check for environment variable overrides
    const char* env_memory_budget = std::getenv("FLIGHT_HAL_MEMORY_BUDGET_MB");
    if (env_memory_budget) {
        try {
            config.memory_budget.total_budget_mb = std::stoull(env_memory_budget);
        } catch (const std::exception&) {
            return HALResult<PlatformConfig>::error(errors::validation_failed(17, "Invalid FLIGHT_HAL_MEMORY_BUDGET_MB environment variable"));
        }
    }
    
    const char* env_debug_level = std::getenv("FLIGHT_HAL_DEBUG_LEVEL");
    if (env_debug_level) {
        std::string level_str(env_debug_level);
        std::transform(level_str.begin(), level_str.end(), level_str.begin(), ::tolower);
        
        if (level_str == "debug") {
            config.debug_level = LogLevel::Debug;
        } else if (level_str == "info") {
            config.debug_level = LogLevel::Info;
        } else if (level_str == "warning") {
            config.debug_level = LogLevel::Warning;
        } else if (level_str == "error") {
            config.debug_level = LogLevel::Error;
        } else {
            return HALResult<PlatformConfig>::error(errors::validation_failed(18, "Invalid FLIGHT_HAL_DEBUG_LEVEL environment variable"));
        }
    }
    
    const char* env_enable_telemetry = std::getenv("FLIGHT_HAL_ENABLE_TELEMETRY");
    if (env_enable_telemetry) {
        std::string telemetry_str(env_enable_telemetry);
        std::transform(telemetry_str.begin(), telemetry_str.end(), telemetry_str.begin(), ::tolower);
        config.performance.enable_telemetry = (telemetry_str == "true" || telemetry_str == "1");
    }
    
    return HALResult<PlatformConfig>::success(std::move(config));
}

PlatformConfig PlatformConfigManager::merge_configs(const PlatformConfig& base, const PlatformConfig& overlay) {
    PlatformConfig merged = base;
    
    // Merge memory budget (overlay takes precedence for non-zero values)
    if (overlay.memory_budget.total_budget_mb != base.memory_budget.total_budget_mb) {
        merged.memory_budget.total_budget_mb = overlay.memory_budget.total_budget_mb;
    }
    if (overlay.memory_budget.graphics_budget_mb != base.memory_budget.graphics_budget_mb) {
        merged.memory_budget.graphics_budget_mb = overlay.memory_budget.graphics_budget_mb;
    }
    if (overlay.memory_budget.audio_budget_mb != base.memory_budget.audio_budget_mb) {
        merged.memory_budget.audio_budget_mb = overlay.memory_budget.audio_budget_mb;
    }
    if (overlay.memory_budget.network_buffer_mb != base.memory_budget.network_buffer_mb) {
        merged.memory_budget.network_buffer_mb = overlay.memory_budget.network_buffer_mb;
    }
    if (overlay.memory_budget.file_cache_mb != base.memory_budget.file_cache_mb) {
        merged.memory_budget.file_cache_mb = overlay.memory_budget.file_cache_mb;
    }
    merged.memory_budget.enforce_budgets = overlay.memory_budget.enforce_budgets;
    
    // Merge performance configuration
    merged.performance = overlay.performance;
    
    // Merge resource coordination configuration
    merged.resource_coordination = overlay.resource_coordination;
    
    // Merge driver initialization configuration
    merged.driver_init = overlay.driver_init;
    
    // Merge global settings
    merged.debug_level = overlay.debug_level;
    merged.enable_debug_output = overlay.enable_debug_output;
    merged.enable_validation_checks = overlay.enable_validation_checks;
    
    // Merge platform overrides (overlay takes precedence)
    for (const auto& [key, value] : overlay.platform_overrides) {
        merged.platform_overrides[key] = value;
    }
    
    // Merge driver configs (overlay takes precedence)
    for (const auto& [driver_name, config] : overlay.driver_configs) {
        merged.driver_configs[driver_name] = config;
    }
    
    return merged;
}

HALResult<void> PlatformConfigManager::validate_for_platform(const PlatformConfig& config, const std::string& platform_name) {
    // First, perform general validation
    auto general_validation = config.validate();
    if (!general_validation.is_success()) {
        return general_validation;
    }
    
    // Platform-specific validation
    if (platform_name == "dreamcast") {
        // Dreamcast has limited memory
        if (config.memory_budget.total_budget_mb > 32) {
            return HALResult<void>::error(errors::platform_constraint(19, "Memory budget too large for Dreamcast platform (max 32MB)"));
        }
        
        // Dreamcast doesn't support some features
        if (config.performance.enable_driver_profiling) {
            return HALResult<void>::error(errors::feature_not_supported(20, "Driver profiling not supported on Dreamcast"));
        }
    } else if (platform_name == "saturn") {
        // Saturn has even more limited memory
        if (config.memory_budget.total_budget_mb > 16) {
            return HALResult<void>::error(errors::platform_constraint(21, "Memory budget too large for Saturn platform (max 16MB)"));
        }
    } else if (platform_name == "windows" || platform_name == "linux" || platform_name == "macos") {
        // Modern platforms - minimal restrictions
        if (config.memory_budget.total_budget_mb < 64) {
            // Warning but not error for modern platforms
        }
    }
    
    return HALResult<void>::success();
}

// Private helper methods
HALResult<PlatformConfig> PlatformConfigManager::parse_json_config(const std::string& json_content) {
    // For now, return default config since we don't have JSON parsing dependencies
    // In a real implementation, this would use a JSON library like nlohmann/json
    
    // Basic validation - check if it looks like JSON
    if (json_content.find('{') == std::string::npos || json_content.find('}') == std::string::npos) {
        return HALResult<PlatformConfig>::error(errors::validation_failed(22, "Invalid JSON format"));
    }
    
    // Return default config for now - would parse actual JSON in full implementation
    return HALResult<PlatformConfig>::success(PlatformConfig::create_default());
}

HALResult<std::string> PlatformConfigManager::generate_json_config(const PlatformConfig& config) {
    // Simple JSON generation - in real implementation would use proper JSON library
    std::stringstream json;
    
    json << "{\n";
    json << "  \"memory_budget\": {\n";
    json << "    \"total_budget_mb\": " << config.memory_budget.total_budget_mb << ",\n";
    json << "    \"graphics_budget_mb\": " << config.memory_budget.graphics_budget_mb << ",\n";
    json << "    \"audio_budget_mb\": " << config.memory_budget.audio_budget_mb << ",\n";
    json << "    \"network_buffer_mb\": " << config.memory_budget.network_buffer_mb << ",\n";
    json << "    \"file_cache_mb\": " << config.memory_budget.file_cache_mb << ",\n";
    json << "    \"enforce_budgets\": " << (config.memory_budget.enforce_budgets ? "true" : "false") << "\n";
    json << "  },\n";
    
    json << "  \"performance\": {\n";
    json << "    \"enable_telemetry\": " << (config.performance.enable_telemetry ? "true" : "false") << ",\n";
    json << "    \"enable_resource_tracking\": " << (config.performance.enable_resource_tracking ? "true" : "false") << ",\n";
    json << "    \"enable_driver_profiling\": " << (config.performance.enable_driver_profiling ? "true" : "false") << ",\n";
    json << "    \"telemetry_interval_ms\": " << config.performance.telemetry_interval.count() << ",\n";
    json << "    \"resource_check_interval_ms\": " << config.performance.resource_check_interval.count() << ",\n";
    json << "    \"cpu_usage_warning_threshold\": " << config.performance.cpu_usage_warning_threshold << ",\n";
    json << "    \"memory_usage_warning_threshold\": " << config.performance.memory_usage_warning_threshold << "\n";
    json << "  },\n";
    
    json << "  \"debug_level\": " << static_cast<int>(config.debug_level) << ",\n";
    json << "  \"enable_debug_output\": " << (config.enable_debug_output ? "true" : "false") << ",\n";
    json << "  \"enable_validation_checks\": " << (config.enable_validation_checks ? "true" : "false") << "\n";
    
    json << "}\n";
    
    return HALResult<std::string>::success(json.str());
}

} // namespace flight::hal
