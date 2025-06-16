/**
 * @file mock_behavior_config.hpp
 * @brief Mock Driver Behavior Configuration System
 * 
 * Comprehensive configuration system for controlling mock driver behavior,
 * including success/failure patterns, latency simulation, and error injection.
 */

#pragma once

#include "../../include/flight/hal/core/hal_result.hpp"
#include "../../include/flight/hal/core/hal_error.hpp"
#include <functional>
#include <unordered_map>
#include <string>
#include <chrono>
#include <random>
#include <memory>

namespace flight::hal::test {

/**
 * @brief Failure injection mode
 */
enum class FailureMode {
    Never,           ///< Never fail
    Always,          ///< Always fail
    RandomPercent,   ///< Fail with specified percentage
    AfterNCalls,     ///< Fail after N successful calls
    OnSpecificCall,  ///< Fail on specific call number
    Pattern,         ///< Follow a specific pattern
    Custom           ///< Use custom failure function
};

/**
 * @brief Performance simulation mode
 */
enum class PerformanceMode {
    Instant,         ///< No delay
    Fixed,           ///< Fixed delay
    Random,          ///< Random delay within range
    Realistic,       ///< Platform-realistic simulation
    Custom           ///< Use custom timing function
};

/**
 * @brief Resource usage simulation mode
 */
enum class ResourceMode {
    Unlimited,       ///< No resource constraints
    Limited,         ///< Simulate limited resources
    Exhaustible,     ///< Resources can be exhausted
    Custom           ///< Use custom resource function
};

/**
 * @brief Mock behavior configuration for a specific method
 */
struct MethodBehaviorConfig {
    // Failure configuration
    FailureMode failure_mode = FailureMode::Never;
    double failure_rate = 0.0;                    ///< Percentage for RandomPercent mode
    uint32_t failure_after_calls = 0;             ///< N calls for AfterNCalls mode
    uint32_t failure_on_call = 0;                 ///< Specific call for OnSpecificCall mode
    std::vector<bool> failure_pattern;            ///< Pattern for Pattern mode
    std::function<bool(uint32_t)> custom_failure_fn; ///< Custom failure function
    
    // Performance configuration
    PerformanceMode performance_mode = PerformanceMode::Instant;
    std::chrono::microseconds fixed_delay{0};
    std::chrono::microseconds min_delay{0};
    std::chrono::microseconds max_delay{0};
    std::function<std::chrono::microseconds(uint32_t)> custom_timing_fn;
    
    // Resource usage configuration
    ResourceMode resource_mode = ResourceMode::Unlimited;
    size_t max_resources = SIZE_MAX;
    size_t resource_per_call = 1;
    std::function<size_t(uint32_t)> custom_resource_fn;
    
    // Error configuration
    HALErrorCategory error_category = HALErrorCategory::Internal;
    uint32_t error_code = 1;
    std::string error_message = "Mock failure";
    std::string error_context = "";
};

/**
 * @brief Global mock behavior configuration
 */
class MockBehaviorConfig {
public:
    MockBehaviorConfig() : rng_(std::random_device{}()) {}
    
    /**
     * @brief Set behavior for a specific method
     */
    void set_method_behavior(const std::string& method_name, const MethodBehaviorConfig& config) {
        method_configs_[method_name] = config;
    }
    
    /**
     * @brief Get behavior for a specific method
     */
    const MethodBehaviorConfig& get_method_behavior(const std::string& method_name) const {
        auto it = method_configs_.find(method_name);
        if (it != method_configs_.end()) {
            return it->second;
        }
        return default_config_;
    }
    
    /**
     * @brief Set default behavior for all methods
     */
    void set_default_behavior(const MethodBehaviorConfig& config) {
        default_config_ = config;
    }
    
    /**
     * @brief Check if method should fail on this call
     */
    bool should_fail(const std::string& method_name, uint32_t call_count) {
        const auto& config = get_method_behavior(method_name);
        
        switch (config.failure_mode) {
            case FailureMode::Never:
                return false;
                
            case FailureMode::Always:
                return true;
                
            case FailureMode::RandomPercent: {
                std::uniform_real_distribution<double> dist(0.0, 100.0);
                return dist(rng_) < config.failure_rate;
            }
            
            case FailureMode::AfterNCalls:
                return call_count > config.failure_after_calls;
                
            case FailureMode::OnSpecificCall:
                return call_count == config.failure_on_call;
                
            case FailureMode::Pattern:
                if (!config.failure_pattern.empty()) {
                    size_t index = (call_count - 1) % config.failure_pattern.size();
                    return config.failure_pattern[index];
                }
                return false;
                
            case FailureMode::Custom:
                if (config.custom_failure_fn) {
                    return config.custom_failure_fn(call_count);
                }
                return false;
        }
        
        return false;
    }
    
    /**
     * @brief Get simulated delay for method call
     */
    std::chrono::microseconds get_delay(const std::string& method_name, uint32_t call_count) {
        const auto& config = get_method_behavior(method_name);
        
        switch (config.performance_mode) {
            case PerformanceMode::Instant:
                return std::chrono::microseconds{0};
                
            case PerformanceMode::Fixed:
                return config.fixed_delay;
                
            case PerformanceMode::Random: {
                std::uniform_int_distribution<int64_t> dist(
                    config.min_delay.count(), 
                    config.max_delay.count()
                );
                return std::chrono::microseconds{dist(rng_)};
            }
            
            case PerformanceMode::Realistic:
                // Simulate realistic hardware timing based on method
                return get_realistic_timing(method_name);
                
            case PerformanceMode::Custom:
                if (config.custom_timing_fn) {
                    return config.custom_timing_fn(call_count);
                }
                return std::chrono::microseconds{0};
        }
        
        return std::chrono::microseconds{0};
    }
    
    /**
     * @brief Get resource usage for method call
     */
    size_t get_resource_usage(const std::string& method_name, uint32_t call_count) {
        const auto& config = get_method_behavior(method_name);
        
        switch (config.resource_mode) {
            case ResourceMode::Unlimited:
                return 0;
                
            case ResourceMode::Limited:
            case ResourceMode::Exhaustible:
                return config.resource_per_call;
                
            case ResourceMode::Custom:
                if (config.custom_resource_fn) {
                    return config.custom_resource_fn(call_count);
                }
                return 0;
        }
        
        return 0;
    }
    
    /**
     * @brief Check if resources are available
     */
    bool has_resources(const std::string& method_name, size_t requested) const {
        const auto& config = get_method_behavior(method_name);
        
        if (config.resource_mode == ResourceMode::Unlimited) {
            return true;
        }
        
        auto it = resource_usage_.find(method_name);
        size_t current_usage = (it != resource_usage_.end()) ? it->second : 0;
        
        return (current_usage + requested) <= config.max_resources;
    }
    
    /**
     * @brief Consume resources
     */
    void consume_resources(const std::string& method_name, size_t amount) {
        resource_usage_[method_name] += amount;
    }
    
    /**
     * @brief Release resources
     */
    void release_resources(const std::string& method_name, size_t amount) {
        auto it = resource_usage_.find(method_name);
        if (it != resource_usage_.end()) {
            it->second = (it->second > amount) ? (it->second - amount) : 0;
        }
    }
    
    /**
     * @brief Get error for failed method call
     */
    HALResult<void> get_error(const std::string& method_name) const {
        const auto& config = get_method_behavior(method_name);
        return HALResult<void>::error(HALError{
            config.error_category,
            config.error_code,
            config.error_message.c_str(),
            config.error_context.empty() ? nullptr : config.error_context.c_str()
        });
    }
    
    /**
     * @brief Reset all state
     */
    void reset() {
        resource_usage_.clear();
        call_counts_.clear();
    }
    
    /**
     * @brief Get call count for method
     */
    uint32_t get_call_count(const std::string& method_name) const {
        auto it = call_counts_.find(method_name);
        return (it != call_counts_.end()) ? it->second : 0;
    }
    
    /**
     * @brief Increment call count for method
     */
    void increment_call_count(const std::string& method_name) {
        call_counts_[method_name]++;
    }

private:
    /**
     * @brief Get realistic timing for hardware simulation
     */
    std::chrono::microseconds get_realistic_timing(const std::string& method_name) {
        // Simulate realistic hardware timing based on operation type
        if (method_name.find("allocate") != std::string::npos) {
            return std::chrono::microseconds{10}; // Memory allocation
        } else if (method_name.find("texture") != std::string::npos) {
            return std::chrono::microseconds{100}; // Texture operations
        } else if (method_name.find("buffer") != std::string::npos) {
            return std::chrono::microseconds{50}; // Buffer operations
        } else if (method_name.find("draw") != std::string::npos) {
            return std::chrono::microseconds{200}; // Draw operations
        } else if (method_name.find("present") != std::string::npos) {
            return std::chrono::microseconds{16667}; // Present (60 FPS)
        } else if (method_name.find("file") != std::string::npos) {
            return std::chrono::microseconds{1000}; // File I/O
        } else if (method_name.find("network") != std::string::npos) {
            return std::chrono::microseconds{5000}; // Network operations
        }
        
        return std::chrono::microseconds{1}; // Default minimal timing
    }
    
    std::unordered_map<std::string, MethodBehaviorConfig> method_configs_;
    MethodBehaviorConfig default_config_;
    std::unordered_map<std::string, size_t> resource_usage_;
    std::unordered_map<std::string, uint32_t> call_counts_;
    mutable std::mt19937 rng_;
};

/**
 * @brief Helper functions for creating common behavior configurations
 */
namespace behavior_presets {

/**
 * @brief Create a failure configuration that fails after N calls
 */
inline MethodBehaviorConfig fail_after_calls(uint32_t n, HALErrorCategory error_category = HALErrorCategory::Resource,
                                             uint32_t error_code = 1,
                                             const std::string& message = "Resource exhausted after N calls") {
    MethodBehaviorConfig config;
    config.failure_mode = FailureMode::AfterNCalls;
    config.failure_after_calls = n;
    config.error_category = error_category;
    config.error_code = error_code;
    config.error_message = message;
    return config;
}

/**
 * @brief Create a failure configuration with random percentage
 */
inline MethodBehaviorConfig fail_randomly(double percentage, HALErrorCategory error_category = HALErrorCategory::Hardware,
                                         uint32_t error_code = 1,
                                         const std::string& message = "Random failure") {
    MethodBehaviorConfig config;
    config.failure_mode = FailureMode::RandomPercent;
    config.failure_rate = percentage;
    config.error_category = error_category;
    config.error_code = error_code;
    config.error_message = message;
    return config;
}

/**
 * @brief Create a timing configuration with fixed delay
 */
inline MethodBehaviorConfig fixed_timing(std::chrono::microseconds delay) {
    MethodBehaviorConfig config;
    config.performance_mode = PerformanceMode::Fixed;
    config.fixed_delay = delay;
    return config;
}

/**
 * @brief Create a timing configuration with random delay
 */
inline MethodBehaviorConfig random_timing(std::chrono::microseconds min_delay, 
                                         std::chrono::microseconds max_delay) {
    MethodBehaviorConfig config;
    config.performance_mode = PerformanceMode::Random;
    config.min_delay = min_delay;
    config.max_delay = max_delay;
    return config;
}

/**
 * @brief Create a resource-limited configuration
 */
inline MethodBehaviorConfig limited_resources(size_t max_resources, size_t per_call = 1) {
    MethodBehaviorConfig config;
    config.resource_mode = ResourceMode::Limited;
    config.max_resources = max_resources;
    config.resource_per_call = per_call;
    return config;
}

/**
 * @brief Create realistic hardware simulation
 */
inline MethodBehaviorConfig realistic_hardware() {
    MethodBehaviorConfig config;
    config.performance_mode = PerformanceMode::Realistic;
    return config;
}

} // namespace behavior_presets

} // namespace flight::hal::test
