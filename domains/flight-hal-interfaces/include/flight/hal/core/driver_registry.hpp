/**
 * @file driver_registry.hpp
 * @brief Flight HAL Driver Registry Interface
 * 
 * Provides centralized driver registration and discovery for HAL interfaces.
 * Supports dynamic driver loading and interface resolution across platforms.
 */

#pragma once

#include <memory>
#include <typeinfo>
#include <unordered_map>
#include <vector>
#include <functional>
#include <string_view>
#include <atomic>
#include <shared_mutex>
#include "hal_result.hpp"
#include "hal_capabilities.hpp"
#include "hal_error.hpp"

namespace flight::hal {

// Forward declarations
class IHALInterface;

/**
 * @brief Version information for drivers
 */
struct Version {
    uint16_t major = 1;
    uint16_t minor = 0;
    uint16_t patch = 0;
    
    constexpr Version() = default;
    constexpr Version(uint16_t maj, uint16_t min, uint16_t pat = 0) 
        : major(maj), minor(min), patch(pat) {}
        
    std::string to_string() const {
        return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
    }
    
    bool operator<(const Version& other) const {
        if (major != other.major) return major < other.major;
        if (minor != other.minor) return minor < other.minor;
        return patch < other.patch;
    }
    
    bool operator==(const Version& other) const {
        return major == other.major && minor == other.minor && patch == other.patch;
    }
};

/**
 * @brief Priority levels for driver selection
 */
enum class DriverPriority : int {
    Fallback = 0,
    Low = 10,
    Normal = 50,
    High = 100,
    Critical = 200,
    Platform = 500,
    Override = 1000
};

/**
 * @brief Platform mask type for driver compatibility
 */
using PlatformMask = uint32_t;

/**
 * @brief Capability flags type for driver feature sets
 */
using CapabilityFlags = uint32_t;

/**
 * @brief Comprehensive driver metadata structure
 * 
 * Contains all information needed for driver selection, fallback management,
 * and capability matching across platforms.
 */
struct DriverInfo {
    std::string_view name;
    Version version;
    PlatformMask supported_platforms;
    CapabilityFlags capabilities;
    int priority;
    PerformanceTier performance_tier;
    std::function<std::unique_ptr<IHALInterface>()> factory;
    std::vector<std::string_view> dependencies;
    std::vector<std::string_view> fallback_drivers;
    size_t memory_overhead = 0;
    bool supports_hot_swap = false;
    std::string_view description;
    
    DriverInfo() = default;
    
    DriverInfo(std::string_view driver_name, 
               Version driver_version = Version{},
               int driver_priority = static_cast<int>(DriverPriority::Normal))
        : name(driver_name), version(driver_version), 
          supported_platforms(0), capabilities(0), priority(driver_priority),
          performance_tier(PerformanceTier::Standard) {}
};

/**
 * @brief Capability requirements for driver selection
 */
struct CapabilityRequirements {
    CapabilityFlags required_capabilities = 0;
    CapabilityFlags preferred_capabilities = 0;
    PerformanceTier minimum_performance = PerformanceTier::Minimal;
    size_t max_memory_overhead = SIZE_MAX;
    bool require_hot_swap = false;
};

/**
 * @brief Base interface for all HAL drivers
 * 
 * All HAL interface implementations must inherit from this base class
 * to participate in the driver registry system. Provides capability detection
 * and enhanced lifecycle management.
 */
class IHALInterface : public ICapabilityProvider {
public:
    virtual ~IHALInterface() = default;
    
    /**
     * @brief Get the interface name
     * @return String identifier for this interface type
     */
    virtual std::string_view get_interface_name() const = 0;
    
    /**
     * @brief Get the driver implementation name
     * @return String identifier for this specific driver implementation
     */
    virtual std::string_view get_driver_name() const = 0;
    
    /**
     * @brief Get driver priority for selection when multiple drivers available
     * @return Priority value (higher values take precedence)
     */
    virtual int get_priority() const { return 0; }
    
    /**
     * @brief Initialize the driver
     * @return HALResult indicating success or failure with error details
     */
    virtual HALResult<void> initialize() = 0;
    
    /**
     * @brief Shutdown the driver and release resources
     * @return HALResult indicating success or failure with error details
     */
    virtual HALResult<void> shutdown() = 0;
    
    /**
     * @brief Check if the driver is currently active
     * @return true if driver is initialized and ready for use
     */
    virtual bool is_active() const = 0;
    
    /**
     * @brief Check if this driver is available on the current platform
     * @return true if the driver can be used on this platform
     */
    virtual bool is_available() const = 0;
    
    /**
     * @brief Get driver version information
     * @return Version string (e.g., "1.0.0")
     */
    virtual std::string_view get_version() const { return "1.0.0"; }
};

/**
 * @brief Centralized registry for HAL interface drivers
 * 
 * Enhanced registry supporting multiple drivers per interface type,
 * priority-based selection, capability matching, and fallback chains.
 * Provides thread-safe access to registered drivers across the system.
 */
class DriverRegistry {
public:
    /**
     * @brief Get the singleton instance of the driver registry
     * @return Reference to the global driver registry
     */
    static DriverRegistry& instance();
    
    /**
     * @brief Register a driver implementation for an interface type
     * @tparam InterfaceType The HAL interface type
     * @param driver Unique pointer to the driver implementation
     * @param priority Driver priority (higher values take precedence)
     * @return true if registration succeeded
     */
    template<typename InterfaceType>
    bool register_driver(std::unique_ptr<InterfaceType> driver, int priority = 0);
    
    /**
     * @brief Register a driver with comprehensive metadata
     * @tparam InterfaceType The HAL interface type
     * @param info Complete driver information structure
     * @return HALResult indicating success or failure with error details
     */
    template<typename InterfaceType>
    HALResult<void> register_driver_with_info(const DriverInfo& info);
    
    /**
     * @brief Register a driver factory for lazy initialization
     * @tparam InterfaceType The HAL interface type
     * @param factory Function that creates the driver instance
     * @param info Driver metadata including priority and capabilities
     * @return HALResult indicating success or failure with error details
     */
    template<typename InterfaceType>
    HALResult<void> register_driver_factory(
        std::function<std::unique_ptr<InterfaceType>()> factory,
        const DriverInfo& info);
    
    /**
     * @brief Unregister a specific driver by name
     * @tparam InterfaceType The HAL interface type
     * @param driver_name Name of the driver to unregister
     * @return true if driver was found and removed
     */
    template<typename InterfaceType>
    bool unregister_driver(std::string_view driver_name);
    
    /**
     * @brief Unregister all drivers for an interface type
     * @tparam InterfaceType The HAL interface type
     * @return Number of drivers removed
     */
    template<typename InterfaceType>
    size_t unregister_all_drivers();
    
    /**
     * @brief Get the best available driver for an interface type
     * @tparam InterfaceType The HAL interface type
     * @return Shared pointer to the best driver, or nullptr if none available
     */
    template<typename InterfaceType>
    std::shared_ptr<InterfaceType> get_interface();
    
    /**
     * @brief Get a driver that meets specific capability requirements
     * @tparam InterfaceType The HAL interface type
     * @param requirements Capability and performance requirements
     * @return Shared pointer to matching driver, or nullptr if none found
     */
    template<typename InterfaceType>
    std::shared_ptr<InterfaceType> get_interface_with_requirements(
        const CapabilityRequirements& requirements);
    
    /**
     * @brief Get a specific driver by name
     * @tparam InterfaceType The HAL interface type
     * @param driver_name Name of the specific driver to retrieve
     * @return Shared pointer to the named driver, or nullptr if not found
     */
    template<typename InterfaceType>
    std::shared_ptr<InterfaceType> get_interface_by_name(std::string_view driver_name);
    
    /**
     * @brief Get all available drivers for an interface type
     * @tparam InterfaceType The HAL interface type
     * @return Vector of all registered drivers for this interface
     */
    template<typename InterfaceType>
    std::vector<std::shared_ptr<InterfaceType>> get_all_interfaces();
    
    /**
     * @brief Check if any driver is registered for an interface type
     * @tparam InterfaceType The HAL interface type
     * @return true if at least one driver is registered
     */
    template<typename InterfaceType>
    bool has_interface() const;
    
    /**
     * @brief Check if a specific driver is registered
     * @tparam InterfaceType The HAL interface type
     * @param driver_name Name of the driver to check
     * @return true if the named driver is registered
     */
    template<typename InterfaceType>
    bool has_interface(std::string_view driver_name) const;
    
    /**
     * @brief Get driver information for all registered drivers of an interface type
     * @tparam InterfaceType The HAL interface type
     * @return Vector of DriverInfo structures
     */
    template<typename InterfaceType>
    std::vector<DriverInfo> get_driver_info() const;
    
    /**
     * @brief Initialize all registered drivers
     * @return HALResult indicating success or failure with error details
     */
    HALResult<void> initialize_all();
    
    /**
     * @brief Initialize drivers for a specific interface type
     * @tparam InterfaceType The HAL interface type
     * @return HALResult indicating success or failure with error details
     */
    template<typename InterfaceType>
    HALResult<void> initialize_interface();
    
    /**
     * @brief Shutdown all registered drivers
     */
    void shutdown_all();
    
    /**
     * @brief Shutdown drivers for a specific interface type
     * @tparam InterfaceType The HAL interface type
     */
    template<typename InterfaceType>
    void shutdown_interface();
    
    /**
     * @brief Get the total number of registered drivers across all interfaces
     * @return Count of registered driver instances
     */
    size_t driver_count() const;
    
    /**
     * @brief Get the number of drivers for a specific interface type
     * @tparam InterfaceType The HAL interface type
     * @return Count of drivers for this interface
     */
    template<typename InterfaceType>
    size_t interface_driver_count() const;
    
    /**
     * @brief Get a list of all registered interface type names
     * @return Vector of interface type name strings
     */
    std::vector<std::string> get_registered_interfaces() const;

private:
    DriverRegistry() = default;
    ~DriverRegistry() = default;
    DriverRegistry(const DriverRegistry&) = delete;
    DriverRegistry& operator=(const DriverRegistry&) = delete;
    
    /**
     * @brief Enhanced driver entry with comprehensive metadata
     */
    struct DriverEntry {
        std::shared_ptr<IHALInterface> driver;
        DriverInfo info;
        std::function<std::unique_ptr<IHALInterface>()> factory;
        bool is_factory_based = false;
        bool is_initialized = false;
        
        DriverEntry() = default;
        
        DriverEntry(std::shared_ptr<IHALInterface> drv, const DriverInfo& driver_info)
            : driver(std::move(drv)), info(driver_info), is_factory_based(false) {}
            
        DriverEntry(std::function<std::unique_ptr<IHALInterface>()> fact, const DriverInfo& driver_info)
            : info(driver_info), factory(std::move(fact)), is_factory_based(true) {}
    };
    
    /**
     * @brief Driver selection algorithm
     */
    template<typename InterfaceType>
    std::shared_ptr<InterfaceType> select_best_driver(
        const std::vector<DriverEntry*>& candidates,
        const CapabilityRequirements* requirements = nullptr);
    
    /**
     * @brief Capability matching for driver selection
     */
    bool matches_requirements(const DriverEntry& entry, 
                             const CapabilityRequirements& requirements) const;
    
    /**
     * @brief Rank drivers by priority and capability match
     */
    void rank_drivers(std::vector<DriverEntry*>& drivers,
                     const CapabilityRequirements* requirements = nullptr) const;
    
    /**
     * @brief Create driver from factory if needed
     */
    std::shared_ptr<IHALInterface> ensure_driver_created(DriverEntry& entry);
    
    /**
     * @brief Get interface type key for storage
     */
    template<typename InterfaceType>
    std::string get_interface_key() const;
    
    mutable std::shared_mutex registry_mutex_;
    
    // Map from interface type name to list of drivers
    std::unordered_map<std::string, std::vector<DriverEntry>> drivers_;
    
    // Cache for quick lookups (interface type -> best driver)
    mutable std::unordered_map<std::string, std::weak_ptr<IHALInterface>> driver_cache_;
    
    // Performance metrics
    mutable std::atomic<size_t> selection_count_{0};
    mutable std::atomic<size_t> cache_hits_{0};
};

} // namespace flight::hal

// Template implementation
#include "driver_registry_impl.hpp"
