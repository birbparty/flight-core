/**
 * @file platform.cpp
 * @brief Flight HAL Platform Coordination Implementation
 */

#include "flight/hal/core/platform.hpp"
#include "flight/hal/core/platform_config.hpp"
#include "flight/hal/core/hal_logging.hpp"
#include "flight/hal/coordination/cross_driver_messenger.hpp"
#include "flight/hal/coordination/deadlock_prevention.hpp"
#include <algorithm>
#include <chrono>

namespace flight::hal {

// ResourceCoordinator implementation
HALResult<void> ResourceCoordinator::request_resource(const std::string& resource_id, 
                                                     const std::string& requester_id,
                                                     AccessMode mode) {
    std::lock_guard<std::mutex> lock(resources_mutex_);
    
    auto it = resources_.find(resource_id);
    if (it == resources_.end()) {
        // Resource doesn't exist yet, create it
        ResourceInfo info;
        info.mode = mode;
        info.owners.insert(requester_id);
        resources_[resource_id] = std::move(info);
        return HALResult<void>::success();
    }
    
    ResourceInfo& info = it->second;
    
    // Check if resource is available
    if (mode == AccessMode::Exclusive) {
        if (!info.owners.empty()) {
            return HALResult<void>::error(errors::resource_locked(1, "Resource is already in use"));
        }
        // Clear any previous owners and set exclusive access
        info.owners.clear();
        info.owners.insert(requester_id);
        info.mode = AccessMode::Exclusive;
    } else { // Shared access
        if (info.mode == AccessMode::Exclusive && !info.owners.empty()) {
            return HALResult<void>::error(errors::resource_locked(2, "Resource is exclusively locked"));
        }
        // Add to shared owners
        info.owners.insert(requester_id);
        info.mode = AccessMode::Shared;
    }
    
    return HALResult<void>::success();
}

HALResult<void> ResourceCoordinator::release_resource(const std::string& resource_id,
                                                     const std::string& requester_id) {
    std::lock_guard<std::mutex> lock(resources_mutex_);
    
    auto it = resources_.find(resource_id);
    if (it == resources_.end()) {
        return HALResult<void>::error(errors::invalid_parameter(3, "Resource not found"));
    }
    
    ResourceInfo& info = it->second;
    auto owner_it = info.owners.find(requester_id);
    if (owner_it == info.owners.end()) {
        return HALResult<void>::error(errors::invalid_parameter(4, "Requester does not own this resource"));
    }
    
    // Remove the requester from owners
    info.owners.erase(owner_it);
    
    // If no more owners, remove the resource entry
    if (info.owners.empty()) {
        resources_.erase(it);
    }
    
    return HALResult<void>::success();
}

bool ResourceCoordinator::is_resource_available(const std::string& resource_id, 
                                               AccessMode mode) const {
    std::lock_guard<std::mutex> lock(resources_mutex_);
    
    auto it = resources_.find(resource_id);
    if (it == resources_.end()) {
        return true; // Resource doesn't exist, so it's available
    }
    
    const ResourceInfo& info = it->second;
    
    if (mode == AccessMode::Exclusive) {
        return info.owners.empty();
    } else { // Shared access
        return info.mode != AccessMode::Exclusive || info.owners.empty();
    }
}

std::unordered_set<std::string> ResourceCoordinator::get_resource_owners(const std::string& resource_id) const {
    std::lock_guard<std::mutex> lock(resources_mutex_);
    
    auto it = resources_.find(resource_id);
    if (it == resources_.end()) {
        return {};
    }
    
    return it->second.owners;
}

// Platform implementation
Platform::Platform() 
    : registry_(DriverRegistry::instance())
    , initialized_(false) {
    // Initialize performance stats
    stats_.total_interfaces = 0;
    stats_.active_interfaces = 0;
    stats_.failed_interfaces = 0;
    stats_.initialization_time_ms = 0.0;
}

Platform::~Platform() {
    if (initialized_) {
        auto result = shutdown();
        if (!result.is_success()) {
            // Log error but can't throw from destructor
        }
    }
}

Platform& Platform::instance() {
    static Platform instance;
    return instance;
}

HALResult<void> Platform::initialize() {
    std::lock_guard<std::mutex> lock(platform_mutex_);
    
    if (initialized_) {
        return HALResult<void>::success(); // Already initialized
    }
    
    init_start_time_ = std::chrono::steady_clock::now();
    
    // Load default configuration
    PlatformConfig config = PlatformConfig::create_default();
    
    // Try to load configuration from environment
    auto env_config_result = PlatformConfigManager::load_from_environment(config);
    if (env_config_result.is_success()) {
        config = env_config_result.value();
    }
    
    // Validate configuration
    auto validation_result = config.validate();
    if (!validation_result.is_success()) {
        return HALResult<void>::error(errors::invalid_parameter(5, "Platform configuration validation failed"));
    }
    
    // Initialize platform capability detection
    detect_platform_capabilities();
    
    if (!capability_provider_) {
        return HALResult<void>::error(errors::initialization_failed(6, "Failed to initialize platform capability provider"));
    }
    
    // Initialize cross-driver messaging if enabled
    if (config.resource_coordination.enable_cross_driver_sharing) {
        auto& messenger = coordination::CrossDriverMessenger::instance();
        auto messenger_result = messenger.initialize();
        if (!messenger_result.is_success()) {
            return HALResult<void>::error(errors::initialization_failed(7, "Failed to initialize cross-driver messenger"));
        }
    }
    
    // Initialize all registered drivers
    auto init_result = registry_.initialize_all();
    if (!init_result.is_success()) {
        if (config.driver_init.fail_fast_on_critical_drivers) {
            return HALResult<void>::error(errors::initialization_failed(8, "Failed to initialize drivers"));
        }
        // Continue with partial initialization
    }
    
    // Get statistics from registry
    auto interfaces = registry_.get_registered_interfaces();
    stats_.total_interfaces = interfaces.size();
    stats_.active_interfaces = 0;
    stats_.failed_interfaces = 0;
    
    // For now, assume all interfaces initialized successfully
    // In a real implementation, would query each interface's status
    stats_.active_interfaces = stats_.total_interfaces;
    for (const auto& interface_name : interfaces) {
        stats_.interface_status[interface_name] = true;
    }
    
    // Calculate initialization time
    auto init_end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(init_end_time - init_start_time_);
    stats_.initialization_time_ms = duration.count() / 1000.0;
    
    initialized_ = true;
    
    return HALResult<void>::success();
}

HALResult<void> Platform::shutdown() {
    std::lock_guard<std::mutex> lock(platform_mutex_);
    
    if (!initialized_) {
        return HALResult<void>::success(); // Already shutdown
    }
    
    // Shutdown all registered drivers
    registry_.shutdown_all();
    
    // Shutdown cross-driver messaging
    auto& messenger = coordination::CrossDriverMessenger::instance();
    auto messenger_result = messenger.shutdown();
    if (!messenger_result.is_success()) {
        // Log error but continue shutdown
    }
    
    // Clear statistics
    stats_ = PerformanceStats{};
    
    initialized_ = false;
    
    return HALResult<void>::success();
}

bool Platform::is_initialized() const {
    std::lock_guard<std::mutex> lock(platform_mutex_);
    return initialized_;
}

const ICapabilityProvider& Platform::get_capabilities() const {
    if (!capability_provider_) {
        static PlatformCapabilityProvider default_provider;
        return default_provider;
    }
    return *capability_provider_;
}

const PlatformInfo& Platform::get_platform_info() const {
    if (!platform_info_) {
        static PlatformInfo default_info{};
        return default_info;
    }
    return *platform_info_;
}

HALResult<void> Platform::request_exclusive_resource(const std::string& resource_id,
                                                    const std::string& requester_id) {
    return resource_coordinator_.request_resource(resource_id, requester_id, 
                                                 ResourceCoordinator::AccessMode::Exclusive);
}

HALResult<void> Platform::request_shared_resource(const std::string& resource_id,
                                                 const std::string& requester_id) {
    return resource_coordinator_.request_resource(resource_id, requester_id, 
                                                 ResourceCoordinator::AccessMode::Shared);
}

HALResult<void> Platform::release_resource(const std::string& resource_id,
                                          const std::string& requester_id) {
    return resource_coordinator_.release_resource(resource_id, requester_id);
}

std::vector<std::string> Platform::get_active_interfaces() const {
    std::lock_guard<std::mutex> lock(platform_mutex_);
    
    std::vector<std::string> active_interfaces;
    for (const auto& [interface_name, is_active] : stats_.interface_status) {
        if (is_active) {
            active_interfaces.push_back(interface_name);
        }
    }
    
    return active_interfaces;
}

Platform::PerformanceStats Platform::get_performance_stats() const {
    std::lock_guard<std::mutex> lock(platform_mutex_);
    return stats_;
}

// PlatformCapabilityProvider implementation
PlatformCapabilityProvider::PlatformCapabilityProvider() 
    : capability_mask_(0) {
    detect_capabilities();
    detect_platform_info();
}

bool PlatformCapabilityProvider::supports_capability(HALCapability capability) const {
    return (capability_mask_ & static_cast<uint32_t>(capability)) != 0;
}

uint32_t PlatformCapabilityProvider::get_capability_mask() const {
    return capability_mask_;
}

std::vector<HALCapability> PlatformCapabilityProvider::get_capabilities() const {
    std::vector<HALCapability> capabilities;
    
    for (uint32_t i = 0; i < 32; ++i) {
        uint32_t flag = 1u << i;
        if (capability_mask_ & flag) {
            capabilities.push_back(static_cast<HALCapability>(flag));
        }
    }
    
    return capabilities;
}

PerformanceTier PlatformCapabilityProvider::get_performance_tier() const {
    // Determine performance tier based on platform capabilities
    if (supports_capability(HALCapability::Hardware3D) &&
        supports_capability(HALCapability::Threading) &&
        supports_capability(HALCapability::VirtualMemory)) {
        return PerformanceTier::High;
    } else if (supports_capability(HALCapability::Hardware2D) &&
               supports_capability(HALCapability::DMA)) {
        return PerformanceTier::Standard;
    } else {
        return PerformanceTier::Minimal;
    }
}

const PlatformInfo& PlatformCapabilityProvider::get_platform_info() const {
    return platform_info_;
}

bool PlatformCapabilityProvider::has_fallback(HALCapability capability) const {
    auto it = fallback_availability_.find(capability);
    return it != fallback_availability_.end() && it->second;
}

void PlatformCapabilityProvider::detect_capabilities() {
    // Platform-specific capability detection
    capability_mask_ = 0;
    
    // Basic capabilities available on all platforms
    capability_mask_ |= static_cast<uint32_t>(HALCapability::PersistentStorage);
    capability_mask_ |= static_cast<uint32_t>(HALCapability::HighPrecisionTimer);
    
    // Platform-specific detection
#ifdef PLATFORM_WINDOWS
    capability_mask_ |= static_cast<uint32_t>(HALCapability::Hardware3D);
    capability_mask_ |= static_cast<uint32_t>(HALCapability::Threading);
    capability_mask_ |= static_cast<uint32_t>(HALCapability::VirtualMemory);
    capability_mask_ |= static_cast<uint32_t>(HALCapability::Networking);
#elif defined(PLATFORM_LINUX)
    capability_mask_ |= static_cast<uint32_t>(HALCapability::Hardware3D);
    capability_mask_ |= static_cast<uint32_t>(HALCapability::Threading);
    capability_mask_ |= static_cast<uint32_t>(HALCapability::VirtualMemory);
    capability_mask_ |= static_cast<uint32_t>(HALCapability::Networking);
#elif defined(PLATFORM_MACOS)
    capability_mask_ |= static_cast<uint32_t>(HALCapability::Hardware3D);
    capability_mask_ |= static_cast<uint32_t>(HALCapability::Threading);
    capability_mask_ |= static_cast<uint32_t>(HALCapability::VirtualMemory);
    capability_mask_ |= static_cast<uint32_t>(HALCapability::Networking);
#elif defined(PLATFORM_DREAMCAST)
    capability_mask_ |= static_cast<uint32_t>(HALCapability::Hardware3D);
    capability_mask_ |= static_cast<uint32_t>(HALCapability::DMA);
    // Dreamcast has limited capabilities
#elif defined(PLATFORM_SATURN)
    capability_mask_ |= static_cast<uint32_t>(HALCapability::Hardware2D);
    capability_mask_ |= static_cast<uint32_t>(HALCapability::DMA);
    // Saturn has very limited capabilities
#else
    // Unknown platform - minimal capabilities
    capability_mask_ |= static_cast<uint32_t>(HALCapability::Hardware2D);
#endif
    
    // Set up fallback availability
    fallback_availability_[HALCapability::Hardware3D] = supports_capability(HALCapability::Hardware2D);
    fallback_availability_[HALCapability::Threading] = true; // Can always fall back to single-threaded
    fallback_availability_[HALCapability::VirtualMemory] = supports_capability(HALCapability::DMA);
}

void PlatformCapabilityProvider::detect_platform_info() {
    // Initialize platform info based on compile-time detection
#ifdef PLATFORM_WINDOWS
    platform_info_.platform_name = "Windows";
    platform_info_.architecture = "x86_64";
    platform_info_.performance_tier = PerformanceTier::High;
    platform_info_.total_memory = 8ULL * 1024 * 1024 * 1024; // 8GB default
    platform_info_.cpu_cores = 8;
    platform_info_.has_fpu = true;
    platform_info_.has_simd = true;
#elif defined(PLATFORM_LINUX)
    platform_info_.platform_name = "Linux";
    platform_info_.architecture = "x86_64";
    platform_info_.performance_tier = PerformanceTier::High;
    platform_info_.total_memory = 8ULL * 1024 * 1024 * 1024; // 8GB default
    platform_info_.cpu_cores = 8;
    platform_info_.has_fpu = true;
    platform_info_.has_simd = true;
#elif defined(PLATFORM_MACOS)
    platform_info_.platform_name = "macOS";
    platform_info_.architecture = "x86_64";
    platform_info_.performance_tier = PerformanceTier::High;
    platform_info_.total_memory = 8ULL * 1024 * 1024 * 1024; // 8GB default
    platform_info_.cpu_cores = 8;
    platform_info_.has_fpu = true;
    platform_info_.has_simd = true;
#elif defined(PLATFORM_DREAMCAST)
    platform_info_.platform_name = "Dreamcast";
    platform_info_.architecture = "SH-4";
    platform_info_.performance_tier = PerformanceTier::Minimal;
    platform_info_.total_memory = 16ULL * 1024 * 1024; // 16MB
    platform_info_.cpu_cores = 1;
    platform_info_.has_fpu = true;
    platform_info_.has_simd = false;
#elif defined(PLATFORM_SATURN)
    platform_info_.platform_name = "Saturn";
    platform_info_.architecture = "SH-2";
    platform_info_.performance_tier = PerformanceTier::Minimal;
    platform_info_.total_memory = 4ULL * 1024 * 1024; // 4MB
    platform_info_.cpu_cores = 2;
    platform_info_.has_fpu = false;
    platform_info_.has_simd = false;
#else
    platform_info_.platform_name = "Unknown";
    platform_info_.architecture = "Unknown";
    platform_info_.performance_tier = PerformanceTier::Limited;
    platform_info_.total_memory = 64ULL * 1024 * 1024; // 64MB default
    platform_info_.cpu_cores = 1;
    platform_info_.has_fpu = false;
    platform_info_.has_simd = false;
#endif
}

} // namespace flight::hal
