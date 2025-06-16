/**
 * @file platform.hpp
 * @brief Flight HAL Platform Coordination Interface
 * 
 * Provides unified platform management and driver coordination across
 * all HAL interfaces. Handles resource sharing and platform lifecycle.
 */

#pragma once

#include "driver_registry.hpp"
#include "hal_result.hpp"
#include "hal_capabilities.hpp"
#include "platform_detection.hpp"
#include "platform_capabilities.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <chrono>

namespace flight::hal {

/**
 * @brief Resource coordination for cross-driver resource sharing
 * 
 * Manages exclusive and shared resources between different HAL drivers
 * to prevent conflicts and ensure proper resource usage.
 */
class ResourceCoordinator {
public:
    /**
     * @brief Resource access modes
     */
    enum class AccessMode {
        Exclusive,  ///< Only one driver can access the resource
        Shared      ///< Multiple drivers can access the resource
    };
    
    /**
     * @brief Request access to a resource
     * @param resource_id Unique identifier for the resource
     * @param requester_id ID of the requesting driver
     * @param mode Access mode (exclusive or shared)
     * @return HALResult indicating success or failure
     */
    HALResult<void> request_resource(const std::string& resource_id, 
                                    const std::string& requester_id,
                                    AccessMode mode = AccessMode::Exclusive);
    
    /**
     * @brief Release a previously acquired resource
     * @param resource_id Unique identifier for the resource
     * @param requester_id ID of the driver releasing the resource
     * @return HALResult indicating success or failure
     */
    HALResult<void> release_resource(const std::string& resource_id,
                                    const std::string& requester_id);
    
    /**
     * @brief Check if a resource is available for the requested access mode
     * @param resource_id Unique identifier for the resource
     * @param mode Desired access mode
     * @return true if the resource is available
     */
    bool is_resource_available(const std::string& resource_id, 
                              AccessMode mode = AccessMode::Exclusive) const;
    
    /**
     * @brief Get the current owner(s) of a resource
     * @param resource_id Unique identifier for the resource
     * @return Set of driver IDs that currently own the resource
     */
    std::unordered_set<std::string> get_resource_owners(const std::string& resource_id) const;

private:
    struct ResourceInfo {
        AccessMode mode;
        std::unordered_set<std::string> owners;
    };
    
    mutable std::mutex resources_mutex_;
    std::unordered_map<std::string, ResourceInfo> resources_;
};

/**
 * @brief Platform coordination and management interface
 * 
 * Central hub for managing all HAL drivers and coordinating platform-wide
 * operations. Provides unified access to all HAL interfaces and handles
 * cross-driver resource coordination.
 */
class Platform {
public:
    /**
     * @brief Get the singleton platform instance
     * @return Reference to the global platform coordinator
     */
    static Platform& instance();
    
    /**
     * @brief Initialize the platform and all registered drivers
     * @return HALResult indicating success or failure with error details
     */
    HALResult<void> initialize();
    
    /**
     * @brief Shutdown the platform and all active drivers
     * @return HALResult indicating success or failure with error details
     */
    HALResult<void> shutdown();
    
    /**
     * @brief Check if the platform is initialized and ready
     * @return true if platform is initialized
     */
    bool is_initialized() const;
    
    /**
     * @brief Get a HAL interface driver
     * @tparam InterfaceType The HAL interface type to retrieve
     * @return Shared pointer to the interface, or nullptr if not available
     */
    template<typename InterfaceType>
    std::shared_ptr<InterfaceType> get_interface();
    
    /**
     * @brief Check if a HAL interface is available
     * @tparam InterfaceType The HAL interface type to check
     * @return true if the interface is available and active
     */
    template<typename InterfaceType>
    bool has_interface() const;
    
    /**
     * @brief Get platform capability information
     * @return Reference to the platform's capability provider
     */
    const ICapabilityProvider& get_capabilities() const;
    
    /**
     * @brief Get platform information
     * @return Structure containing detailed platform information
     */
    const PlatformInfo& get_platform_info() const;
    
    /**
     * @brief Request exclusive access to a platform resource
     * @param resource_id Unique identifier for the resource
     * @param requester_id ID of the requesting component
     * @return HALResult indicating success or failure
     */
    HALResult<void> request_exclusive_resource(const std::string& resource_id,
                                              const std::string& requester_id);
    
    /**
     * @brief Request shared access to a platform resource
     * @param resource_id Unique identifier for the resource
     * @param requester_id ID of the requesting component
     * @return HALResult indicating success or failure
     */
    HALResult<void> request_shared_resource(const std::string& resource_id,
                                           const std::string& requester_id);
    
    /**
     * @brief Release a previously acquired resource
     * @param resource_id Unique identifier for the resource
     * @param requester_id ID of the component releasing the resource
     * @return HALResult indicating success or failure
     */
    HALResult<void> release_resource(const std::string& resource_id,
                                    const std::string& requester_id);
    
    /**
     * @brief Get list of all active interface names
     * @return Vector of interface name strings
     */
    std::vector<std::string> get_active_interfaces() const;
    
    /**
     * @brief Get platform performance statistics
     * @return Structure containing performance metrics
     */
    struct PerformanceStats {
        size_t total_interfaces;
        size_t active_interfaces;
        size_t failed_interfaces;
        double initialization_time_ms;
        std::unordered_map<std::string, bool> interface_status;
    };
    
    PerformanceStats get_performance_stats() const;

private:
    Platform();
    ~Platform();
    Platform(const Platform&) = delete;
    Platform& operator=(const Platform&) = delete;
    
    /**
     * @brief Detect and initialize platform capabilities
     */
    void detect_platform_capabilities() {
        // Create platform-specific capability provider using factory
        capability_provider_ = create_platform_capability_provider();
        
        // Get platform info from the provider
        if (capability_provider_) {
            platform_info_ = std::make_unique<PlatformInfo>(capability_provider_->get_platform_info());
        }
    }
    
    /**
     * @brief Create platform-specific capability provider
     */
    void create_capability_provider() {
        // This is now handled by detect_platform_capabilities()
        if (!capability_provider_) {
            detect_platform_capabilities();
        }
    }
    
    DriverRegistry& registry_;
    ResourceCoordinator resource_coordinator_;
    std::unique_ptr<ICapabilityProvider> capability_provider_;
    std::unique_ptr<PlatformInfo> platform_info_;
    
    mutable std::mutex platform_mutex_;
    bool initialized_ = false;
    
    // Performance tracking
    mutable PerformanceStats stats_;
    std::chrono::steady_clock::time_point init_start_time_;
};

/**
 * @brief Platform-specific capability provider implementation
 * 
 * Provides capability detection for the current platform based on
 * compile-time and runtime platform detection.
 */
class PlatformCapabilityProvider : public ICapabilityProvider {
public:
    PlatformCapabilityProvider();
    
    // ICapabilityProvider implementation
    bool supports_capability(HALCapability capability) const override;
    uint32_t get_capability_mask() const override;
    std::vector<HALCapability> get_capabilities() const override;
    PerformanceTier get_performance_tier() const override;
    const PlatformInfo& get_platform_info() const override;
    bool has_fallback(HALCapability capability) const override;

private:
    void detect_capabilities();
    void detect_platform_info();
    
    uint32_t capability_mask_;
    PlatformInfo platform_info_;
    std::unordered_map<HALCapability, bool> fallback_availability_;
};

// Template implementation
template<typename InterfaceType>
std::shared_ptr<InterfaceType> Platform::get_interface() {
    std::lock_guard<std::mutex> lock(platform_mutex_);
    
    if (!initialized_) {
        return nullptr;
    }
    
    return registry_.get_interface<InterfaceType>();
}

template<typename InterfaceType>
bool Platform::has_interface() const {
    std::lock_guard<std::mutex> lock(platform_mutex_);
    
    if (!initialized_) {
        return false;
    }
    
    auto interface = registry_.get_interface<InterfaceType>();
    return interface && interface->is_active();
}

} // namespace flight::hal
