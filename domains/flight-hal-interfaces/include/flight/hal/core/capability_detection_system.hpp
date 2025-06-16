/**
 * @file capability_detection_system.hpp
 * @brief Comprehensive Capability Detection System
 * 
 * Integrates hardware detection, performance benchmarking, capability caching,
 * and driver integration to provide a complete runtime capability detection
 * solution with graceful degradation and adaptive performance optimization.
 */

#pragma once

#include "hal_capabilities.hpp"
#include "hal_result.hpp"
#include "hardware_detection.hpp"
#include "performance_benchmarking.hpp"
#include "capability_cache.hpp"
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <chrono>
#include <unordered_map>

// Forward declaration
namespace flight::hal { class DriverRegistry; }

namespace flight::hal {

/**
 * @brief Capability detection configuration
 */
struct CapabilityDetectionConfig {
    // Detection modes
    bool enable_hardware_detection;    ///< Enable runtime hardware detection
    bool enable_performance_benchmarks; ///< Enable performance benchmarking
    bool enable_capability_caching;    ///< Enable capability result caching
    bool enable_driver_integration;    ///< Enable driver capability queries
    bool enable_background_updates;    ///< Enable background capability updates
    
    // Detection behavior
    bool force_detection_on_startup;   ///< Force full detection on startup
    bool use_quick_detection_mode;     ///< Use quick detection for faster startup
    bool validate_cached_results;      ///< Validate cached capability results
    bool enable_fallback_detection;    ///< Enable fallback capability detection
    
    // Performance settings
    std::chrono::milliseconds max_detection_time; ///< Maximum detection time
    std::chrono::milliseconds cache_update_interval; ///< Cache update interval
    double acceptable_error_rate;      ///< Acceptable detection error rate
    uint32_t max_concurrent_detections; ///< Maximum concurrent detections
    
    // Integration settings
    bool auto_update_on_driver_change; ///< Update when drivers change
    bool propagate_capability_changes; ///< Propagate changes to dependent systems
    bool enable_capability_prediction; ///< Enable capability prediction
    
    // Cache configuration
    CacheConfig cache_config;          ///< Capability cache configuration
    BenchmarkConfig benchmark_config;  ///< Benchmark configuration
};

/**
 * @brief Capability detection results
 */
struct CapabilityDetectionResults {
    // Detection metadata
    std::chrono::steady_clock::time_point detection_timestamp;
    std::chrono::milliseconds detection_duration;
    bool detection_successful;
    std::string detection_source;
    
    // Capability information
    uint32_t capability_mask;
    PerformanceTier performance_tier;
    PlatformInfo platform_info;
    std::unordered_map<HALCapability, bool> fallback_availability;
    
    // Hardware features
    CPUFeatures cpu_features;
    GPUFeatures gpu_features;
    MemoryFeatures memory_features;
    IOFeatures io_features;
    
    // Performance benchmarks
    SystemBenchmarkResults benchmark_results;
    
    // Driver capabilities
    std::unordered_map<std::string, uint32_t> driver_capabilities;
    
    // Confidence metrics
    double detection_confidence;       ///< Overall detection confidence (0.0-1.0)
    double hardware_confidence;        ///< Hardware detection confidence
    double benchmark_confidence;       ///< Benchmark confidence
    std::vector<std::string> warnings; ///< Detection warnings
    std::vector<std::string> errors;   ///< Detection errors
};

/**
 * @brief Capability change notification
 */
struct CapabilityChangeNotification {
    std::chrono::steady_clock::time_point timestamp;
    HALCapability changed_capability;
    bool was_supported;                ///< Previous support status
    bool now_supported;                ///< Current support status
    std::string change_reason;         ///< Reason for change
    PerformanceTier old_tier;          ///< Previous performance tier
    PerformanceTier new_tier;          ///< New performance tier
};

/**
 * @brief Capability detection event listener
 */
class ICapabilityDetectionEventListener {
public:
    virtual ~ICapabilityDetectionEventListener() = default;
    
    /**
     * @brief Called when capability detection starts
     * @param config Detection configuration being used
     */
    virtual void on_detection_started(const CapabilityDetectionConfig& config) = 0;
    
    /**
     * @brief Called when capability detection completes
     * @param results Detection results
     */
    virtual void on_detection_completed(const CapabilityDetectionResults& results) = 0;
    
    /**
     * @brief Called when capability detection fails
     * @param error_message Error description
     * @param partial_results Partial results if available
     */
    virtual void on_detection_failed(const std::string& error_message, 
                                   const CapabilityDetectionResults* partial_results) = 0;
    
    /**
     * @brief Called when capabilities change
     * @param notification Change notification details
     */
    virtual void on_capability_changed(const CapabilityChangeNotification& notification) = 0;
    
    /**
     * @brief Called when performance tier changes
     * @param old_tier Previous performance tier
     * @param new_tier New performance tier
     * @param reason Reason for change
     */
    virtual void on_performance_tier_changed(PerformanceTier old_tier, 
                                            PerformanceTier new_tier,
                                            const std::string& reason) = 0;
};

/**
 * @brief Main capability detection system interface
 */
class ICapabilityDetectionSystem : public ICapabilityProvider {
public:
    virtual ~ICapabilityDetectionSystem() = default;
    
    /**
     * @brief Initialize the capability detection system
     * @param config Detection configuration
     * @return Success result
     */
    virtual HALResult<void> initialize(const CapabilityDetectionConfig& config) = 0;
    
    /**
     * @brief Shutdown the capability detection system
     * @return Success result
     */
    virtual HALResult<void> shutdown() = 0;
    
    /**
     * @brief Perform capability detection
     * @param force_detection Force detection even if cached results exist
     * @return Detection results
     */
    virtual HALResult<CapabilityDetectionResults> detect_capabilities(bool force_detection = false) = 0;
    
    /**
     * @brief Get current capability detection results
     * @return Current detection results
     */
    virtual CapabilityDetectionResults get_current_results() const = 0;
    
    /**
     * @brief Check if capability detection is in progress
     * @return true if detection is running
     */
    virtual bool is_detection_in_progress() const = 0;
    
    /**
     * @brief Get detection progress
     * @return Progress percentage (0.0 to 1.0)
     */
    virtual double get_detection_progress() const = 0;
    
    /**
     * @brief Cancel ongoing capability detection
     * @return Success result
     */
    virtual HALResult<void> cancel_detection() = 0;
    
    /**
     * @brief Update capabilities based on driver changes
     * @param driver_name Name of changed driver
     * @return Success result
     */
    virtual HALResult<void> update_for_driver_change(const std::string& driver_name) = 0;
    
    /**
     * @brief Invalidate capability cache
     * @param reason Invalidation reason
     * @return Success result
     */
    virtual HALResult<void> invalidate_cache(InvalidationReason reason) = 0;
    
    /**
     * @brief Register event listener for capability changes
     * @param listener Event listener to register
     * @return Success result
     */
    virtual HALResult<void> register_event_listener(
        std::shared_ptr<ICapabilityDetectionEventListener> listener
    ) = 0;
    
    /**
     * @brief Unregister event listener
     * @param listener Event listener to unregister
     * @return Success result
     */
    virtual HALResult<void> unregister_event_listener(
        std::shared_ptr<ICapabilityDetectionEventListener> listener
    ) = 0;
    
    /**
     * @brief Configure the detection system
     * @param config New configuration
     * @return Success result
     */
    virtual HALResult<void> configure(const CapabilityDetectionConfig& config) = 0;
    
    /**
     * @brief Get current configuration
     * @return Current detection configuration
     */
    virtual CapabilityDetectionConfig get_configuration() const = 0;
    
    /**
     * @brief Get detection statistics
     * @return Detection system statistics
     */
    virtual std::unordered_map<std::string, uint64_t> get_statistics() const = 0;
    
    /**
     * @brief Reset detection statistics
     * @return Success result
     */
    virtual HALResult<void> reset_statistics() = 0;
    
    // Extended capability queries
    
    /**
     * @brief Get capability with confidence level
     * @param capability Capability to check
     * @return Capability support with confidence
     */
    virtual std::pair<bool, double> get_capability_with_confidence(HALCapability capability) const = 0;
    
    /**
     * @brief Get performance tier with reasoning
     * @param subsystem Specific subsystem ("cpu", "gpu", "memory", "io", "overall")
     * @return Performance tier with explanation
     */
    virtual std::pair<PerformanceTier, std::string> get_performance_tier_with_reasoning(
        const std::string& subsystem = "overall"
    ) const = 0;
    
    /**
     * @brief Get capability history
     * @param capability Capability to get history for
     * @param duration Time period to retrieve
     * @return Capability change history
     */
    virtual std::vector<CapabilityChangeNotification> get_capability_history(
        HALCapability capability, 
        std::chrono::hours duration
    ) const = 0;
    
    /**
     * @brief Predict capability support for hypothetical hardware
     * @param hardware_specs Hypothetical hardware specifications
     * @return Predicted capability support
     */
    virtual HALResult<uint32_t> predict_capabilities(
        const std::unordered_map<std::string, std::string>& hardware_specs
    ) = 0;
    
    /**
     * @brief Get recommended settings for current capabilities
     * @param use_case Target use case ("gaming", "productivity", "power_saving")
     * @return Recommended configuration settings
     */
    virtual std::unordered_map<std::string, std::string> get_recommended_settings(
        const std::string& use_case
    ) const = 0;
};

/**
 * @brief Fallback capability provider
 * 
 * Provides capability information when full detection fails,
 * using conservative estimates and compile-time information.
 */
class IFallbackCapabilityProvider : public ICapabilityProvider {
public:
    virtual ~IFallbackCapabilityProvider() = default;
    
    /**
     * @brief Initialize fallback provider with platform hints
     * @param platform_hints Known platform characteristics
     * @return Success result
     */
    virtual HALResult<void> initialize_with_hints(
        const std::unordered_map<std::string, std::string>& platform_hints
    ) = 0;
    
    /**
     * @brief Update fallback capabilities based on partial detection
     * @param partial_results Partial detection results
     * @return Success result
     */
    virtual HALResult<void> update_from_partial_results(
        const CapabilityDetectionResults& partial_results
    ) = 0;
    
    /**
     * @brief Get confidence level for fallback capabilities
     * @return Confidence level (0.0 to 1.0)
     */
    virtual double get_fallback_confidence() const = 0;
};

/**
 * @brief Create capability detection system
 * @param driver_registry Driver registry for driver integration
 * @return Capability detection system instance
 */
std::unique_ptr<ICapabilityDetectionSystem> create_capability_detection_system(
    std::shared_ptr<DriverRegistry> driver_registry = nullptr
);

/**
 * @brief Create fallback capability provider
 * @return Fallback capability provider instance
 */
std::unique_ptr<IFallbackCapabilityProvider> create_fallback_capability_provider();

/**
 * @brief Create default capability detection configuration
 * @param profile Configuration profile ("default", "quick", "comprehensive", "embedded")
 * @return Default configuration for the specified profile
 */
CapabilityDetectionConfig create_default_capability_detection_config(
    const std::string& profile = "default"
);

/**
 * @brief Create capability detection configuration for specific use case
 * @param use_case Use case ("gaming", "server", "embedded", "development")
 * @return Optimized configuration for the use case
 */
CapabilityDetectionConfig create_capability_detection_config_for_use_case(
    const std::string& use_case
);

/**
 * @brief Validate capability detection configuration
 * @param config Configuration to validate
 * @return Validation results with warnings/errors
 */
HALResult<std::vector<std::string>> validate_capability_detection_config(
    const CapabilityDetectionConfig& config
);

/**
 * @brief Convert detection results to JSON string
 * @param results Detection results to serialize
 * @return JSON representation
 */
std::string capability_detection_results_to_json(const CapabilityDetectionResults& results);

/**
 * @brief Parse detection results from JSON string
 * @param json JSON string to parse
 * @return Parsed detection results
 */
HALResult<CapabilityDetectionResults> capability_detection_results_from_json(
    const std::string& json
);

/**
 * @brief Compare two capability detection results
 * @param old_results Previous results
 * @param new_results New results
 * @return List of capability changes
 */
std::vector<CapabilityChangeNotification> compare_capability_results(
    const CapabilityDetectionResults& old_results,
    const CapabilityDetectionResults& new_results
);

} // namespace flight::hal
