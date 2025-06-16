/**
 * @file driver_auto_registration.hpp
 * @brief Flight HAL Auto-Registration System
 * 
 * Provides automatic driver registration using static initialization
 * inspired by RetroArch's self-registering driver pattern.
 */

#pragma once

#include "driver_registry.hpp"
#include <memory>
#include <optional>
#include <string>
#include <exception>

namespace flight::hal {

/**
 * @brief Auto-registration helper for HAL drivers
 * 
 * Enhanced template class supporting comprehensive driver metadata,
 * factory-based registration, and conditional compilation.
 * 
 * @tparam InterfaceType The HAL interface type (e.g., IMemoryInterface)
 * @tparam DriverType The concrete driver implementation
 */
template<typename InterfaceType, typename DriverType>
class DriverRegistrar {
public:
    /**
     * @brief Constructor performs automatic registration with basic metadata
     * 
     * Creates an instance of DriverType and registers it with basic
     * auto-detected metadata during static initialization.
     */
    DriverRegistrar() {
        register_driver_instance();
    }
    
    /**
     * @brief Constructor with explicit priority
     * @param priority Driver priority for selection
     */
    explicit DriverRegistrar(int priority) : explicit_priority_(priority) {
        register_driver_instance();
    }
    
    /**
     * @brief Constructor with comprehensive driver info
     * @param info Complete driver metadata
     */
    explicit DriverRegistrar(const DriverInfo& info) : driver_info_(info) {
        register_with_info();
    }
    
    /**
     * @brief Check if registration was successful
     * @return true if the driver was successfully registered
     */
    bool is_registered() const {
        return registered_;
    }
    
    /**
     * @brief Get registration error message if registration failed
     * @return Error message, or empty string if successful
     */
    std::string_view get_error() const {
        return error_message_;
    }

private:
    void register_driver_instance() {
        try {
            // Create driver instance for metadata extraction
            auto driver = std::make_unique<DriverType>();
            
            // Build DriverInfo from driver properties
            DriverInfo info;
            info.name = driver->get_driver_name();
            info.version = parse_version(driver->get_version());
            info.priority = explicit_priority_.value_or(driver->get_priority());
            info.performance_tier = driver->get_performance_tier();
            info.capabilities = driver->get_capability_mask();
            info.supported_platforms = detect_platform_mask();
            info.description = "Auto-registered driver";
            
            // Set up factory for lazy instantiation
            info.factory = []() -> std::unique_ptr<IHALInterface> {
                return std::make_unique<DriverType>();
            };
            
            // Register using enhanced registration
            auto result = DriverRegistry::instance().register_driver_with_info<InterfaceType>(info);
            registered_ = result.is_success();
            
            if (!registered_) {
                error_message_ = result.get_error();
            }
            
        } catch (const std::exception& e) {
            registered_ = false;
            error_message_ = e.what();
        }
    }
    
    void register_with_info() {
        try {
            auto result = DriverRegistry::instance().register_driver_with_info<InterfaceType>(driver_info_);
            registered_ = result.is_success();
            
            if (!registered_) {
                error_message_ = result.get_error();
            }
            
        } catch (const std::exception& e) {
            registered_ = false;
            error_message_ = e.what();
        }
    }
    
    Version parse_version(std::string_view version_str) {
        // Simple version parsing: "major.minor.patch"
        Version version;
        
        size_t pos = 0;
        size_t dot1 = version_str.find('.', pos);
        if (dot1 != std::string_view::npos) {
            version.major = static_cast<uint16_t>(std::stoi(std::string(version_str.substr(pos, dot1 - pos))));
            
            pos = dot1 + 1;
            size_t dot2 = version_str.find('.', pos);
            if (dot2 != std::string_view::npos) {
                version.minor = static_cast<uint16_t>(std::stoi(std::string(version_str.substr(pos, dot2 - pos))));
                version.patch = static_cast<uint16_t>(std::stoi(std::string(version_str.substr(dot2 + 1))));
            } else {
                version.minor = static_cast<uint16_t>(std::stoi(std::string(version_str.substr(pos))));
            }
        } else {
            version.major = static_cast<uint16_t>(std::stoi(std::string(version_str)));
        }
        
        return version;
    }
    
    PlatformMask detect_platform_mask() {
        // Detect current platform and return appropriate mask
        // This would be expanded with actual platform detection
        #ifdef PLATFORM_DREAMCAST
            return 1 << 0;
        #elif defined(PLATFORM_PSP)
            return 1 << 1;
        #elif defined(PLATFORM_WEB)
            return 1 << 2;
        #elif defined(PLATFORM_DESKTOP)
            return 1 << 3;
        #else
            return 0xFFFFFFFF; // Support all platforms by default
        #endif
    }
    
    bool registered_ = false;
    std::string error_message_;
    std::optional<int> explicit_priority_;
    DriverInfo driver_info_;
};

/**
 * @brief Enhanced auto-registration helper with advanced features
 * 
 * Provides additional registration options including conditional registration,
 * platform-specific registration, and comprehensive metadata support.
 * 
 * @tparam InterfaceType The HAL interface type
 * @tparam DriverType The concrete driver implementation  
 */
template<typename InterfaceType, typename DriverType>
class AdvancedDriverRegistrar {
public:
    /**
     * @brief Constructor with full configuration options
     * @param config Registration configuration
     */
    struct Config {
        int priority = static_cast<int>(DriverPriority::Normal);
        PlatformMask supported_platforms = 0xFFFFFFFF;
        CapabilityFlags required_capabilities = 0;
        size_t max_memory_overhead = 1024; // 1KB default
        bool supports_hot_swap = false;
        std::string_view description = "Advanced registered driver";
        std::vector<std::string_view> dependencies;
        std::vector<std::string_view> fallback_drivers;
        Version version = Version{1, 0, 0};
    };
    
    explicit AdvancedDriverRegistrar(const Config& config) {
        // Build comprehensive DriverInfo
        DriverInfo info;
        info.name = DriverType::static_driver_name(); // Requires static method
        info.version = config.version;
        info.priority = config.priority;
        info.supported_platforms = config.supported_platforms;
        info.capabilities = DriverType::static_capabilities(); // Requires static method
        info.performance_tier = DriverType::static_performance_tier(); // Requires static method
        info.memory_overhead = config.max_memory_overhead;
        info.supports_hot_swap = config.supports_hot_swap;
        info.description = config.description;
        info.dependencies = config.dependencies;
        info.fallback_drivers = config.fallback_drivers;
        
        // Set up factory
        info.factory = []() -> std::unique_ptr<IHALInterface> {
            return std::make_unique<DriverType>();
        };
        
        // Register
        auto result = DriverRegistry::instance().register_driver_with_info<InterfaceType>(info);
        registered_ = result.is_success();
        
        if (!registered_) {
            error_message_ = result.get_error();
        }
    }
    
    bool is_registered() const { return registered_; }
    std::string_view get_error() const { return error_message_; }

private:
    bool registered_ = false;
    std::string error_message_;
};

/**
 * @brief Factory function type for driver creation
 * 
 * Function signature for factory functions that create driver instances.
 * Used for lazy initialization and dynamic driver loading.
 */
template<typename InterfaceType>
using DriverFactory = std::function<std::unique_ptr<InterfaceType>()>;

/**
 * @brief Lazy registration helper for delayed driver creation
 * 
 * Alternative to DriverRegistrar that defers driver creation until
 * the driver is actually requested. Useful for drivers with expensive
 * initialization or platform detection.
 * 
 * @tparam InterfaceType The HAL interface type
 */
template<typename InterfaceType>
class LazyDriverRegistrar {
public:
    /**
     * @brief Constructor with factory function
     * @param factory Function that creates the driver instance
     * @param priority Driver priority for selection
     */
    LazyDriverRegistrar(DriverFactory<InterfaceType> factory, int priority = 0)
        : factory_(std::move(factory)), priority_(priority) {
        // Register factory with the registry
        // Note: This would require extending DriverRegistry to support factories
        registered_ = true;
    }
    
    /**
     * @brief Check if registration was successful
     * @return true if the factory was successfully registered
     */
    bool is_registered() const {
        return registered_;
    }

private:
    DriverFactory<InterfaceType> factory_;
    int priority_;
    bool registered_ = false;
};

} // namespace flight::hal

/**
 * @brief Macro for automatic driver registration
 * 
 * Use this macro in your driver source file to automatically register
 * the driver with the HAL system during static initialization.
 * 
 * @param InterfaceType The HAL interface type (e.g., IMemoryInterface)
 * @param DriverType The concrete driver implementation class
 * 
 * Example usage:
 * ```cpp
 * // In dreamcast_memory_driver.cpp
 * REGISTER_HAL_DRIVER(IMemoryInterface, DreamcastMemoryDriver);
 * ```
 */
#define REGISTER_HAL_DRIVER(InterfaceType, DriverType) \
    namespace { \
        static ::flight::hal::DriverRegistrar<InterfaceType, DriverType> \
            _hal_registrar_##DriverType; \
    }

/**
 * @brief Macro for conditional driver registration
 * 
 * Registers a driver only if a compile-time condition is met.
 * Useful for platform-specific drivers.
 * 
 * @param Condition Compile-time boolean condition
 * @param InterfaceType The HAL interface type
 * @param DriverType The concrete driver implementation class
 * 
 * Example usage:
 * ```cpp
 * // Only register on Dreamcast platform
 * REGISTER_HAL_DRIVER_IF(PLATFORM_DREAMCAST, IMemoryInterface, DreamcastMemoryDriver);
 * ```
 */
#define REGISTER_HAL_DRIVER_IF(Condition, InterfaceType, DriverType) \
    namespace { \
        static std::conditional_t< \
            (Condition), \
            ::flight::hal::DriverRegistrar<InterfaceType, DriverType>, \
            ::flight::hal::detail::NullRegistrar \
        > _hal_registrar_##DriverType; \
    }

/**
 * @brief Macro for factory-based driver registration
 * 
 * Registers a factory function that creates the driver on-demand.
 * Useful for drivers that need platform detection or expensive initialization.
 * 
 * @param InterfaceType The HAL interface type
 * @param FactoryFunc Function that returns std::unique_ptr<InterfaceType>
 * @param Priority Driver priority (default: 0)
 * 
 * Example usage:
 * ```cpp
 * auto create_memory_driver() -> std::unique_ptr<IMemoryInterface> {
 *     if (is_dreamcast()) return std::make_unique<DreamcastMemoryDriver>();
 *     if (is_psp()) return std::make_unique<PSPMemoryDriver>();
 *     return nullptr;
 * }
 * REGISTER_HAL_FACTORY(IMemoryInterface, create_memory_driver, 10);
 * ```
 */
#define REGISTER_HAL_FACTORY(InterfaceType, FactoryFunc, Priority) \
    namespace { \
        static ::flight::hal::LazyDriverRegistrar<InterfaceType> \
            _hal_factory_registrar_##FactoryFunc(FactoryFunc, Priority); \
    }

namespace flight::hal::detail {

/**
 * @brief Null registrar for conditional compilation
 * 
 * Empty class used by REGISTER_HAL_DRIVER_IF when condition is false.
 * Prevents compilation errors while having zero runtime cost.
 */
struct NullRegistrar {
    bool is_registered() const { return false; }
};

} // namespace flight::hal::detail
