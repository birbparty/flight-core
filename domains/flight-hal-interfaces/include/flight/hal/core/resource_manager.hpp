/**
 * @file resource_manager.hpp
 * @brief Flight HAL Resource Management System
 * 
 * Comprehensive resource management with RAII patterns, budget enforcement,
 * pool management, and cross-driver resource sharing for all platforms
 * from Dreamcast's constrained 16MB to modern systems.
 */

#pragma once

#include "hal_result.hpp"
#include "hal_error.hpp"
#include "../coordination/resource_handle.hpp"
#include "../allocators/pool_allocator.hpp"
#include "../interfaces/memory.hpp"
#include <memory>
#include <unordered_map>
#include <chrono>
#include <functional>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>

// Hash specialization for ResourceType, size_t pair
namespace std {
template<>
struct hash<std::pair<flight::hal::coordination::ResourceType, size_t>> {
    size_t operator()(const std::pair<flight::hal::coordination::ResourceType, size_t>& p) const noexcept {
        size_t h1 = std::hash<int>{}(static_cast<int>(p.first));
        size_t h2 = std::hash<size_t>{}(p.second);
        return h1 ^ (h2 << 1);
    }
};
}

namespace flight::hal::core {

// Forward declarations
class ResourceManager;
class ResourceBudgetManager;
class PoolManager;
template<typename T> class ResourceRef;
class ScopedResource;

/**
 * @brief Resource acquisition modes
 */
enum class AcquisitionMode : uint8_t {
    Blocking,       ///< Block until resource is available
    NonBlocking,    ///< Return immediately if resource unavailable
    Timeout,        ///< Block with timeout
    Emergency       ///< Emergency allocation (may trigger reclamation)
};

/**
 * @brief Resource pressure levels
 */
enum class ResourcePressure : uint8_t {
    None = 0,       ///< Abundant resources available
    Low = 1,        ///< Some pressure, minor optimizations
    Medium = 2,     ///< Moderate pressure, active management
    High = 3,       ///< High pressure, aggressive reclamation
    Critical = 4    ///< Critical pressure, emergency measures
};

/**
 * @brief Resource budget configuration
 */
struct ResourceBudget {
    size_t max_bytes = 0;           ///< Maximum bytes for this resource type
    size_t reserved_bytes = 0;      ///< Reserved bytes (cannot be reclaimed)
    size_t warning_threshold = 0;   ///< Warning threshold (percentage)
    size_t critical_threshold = 0;  ///< Critical threshold (percentage)
    bool enable_reclamation = true; ///< Allow automatic reclamation
    std::chrono::milliseconds timeout{5000}; ///< Acquisition timeout
    
    ResourceBudget() = default;
    ResourceBudget(size_t max, size_t reserved = 0, size_t warn = 80, size_t crit = 95)
        : max_bytes(max), reserved_bytes(reserved), 
          warning_threshold(warn), critical_threshold(crit) {}
};

/**
 * @brief Resource statistics
 */
struct ResourceStats {
    size_t total_allocated = 0;     ///< Total bytes allocated
    size_t peak_usage = 0;          ///< Peak usage in bytes
    size_t current_usage = 0;       ///< Current usage in bytes
    size_t allocation_count = 0;    ///< Number of allocations
    size_t deallocation_count = 0;  ///< Number of deallocations
    size_t reclamation_count = 0;   ///< Number of reclamations
    ResourcePressure pressure = ResourcePressure::None;
    std::chrono::steady_clock::time_point last_updated;
    
    ResourceStats() : last_updated(std::chrono::steady_clock::now()) {}
};

/**
 * @brief Resource pressure callback
 */
using PressureCallback = std::function<void(coordination::ResourceType, ResourcePressure, const ResourceStats&)>;

/**
 * @brief Resource reclamation callback
 */
using ReclamationCallback = std::function<size_t(coordination::ResourceType, size_t requested_bytes)>;

/**
 * @brief RAII-based resource reference with automatic cleanup
 */
template<typename T>
class ResourceRef {
public:
    /**
     * @brief Default constructor (empty reference)
     */
    ResourceRef() : resource_(nullptr), handle_(), manager_(nullptr) {}
    
    /**
     * @brief Constructor from resource pointer and handle
     */
    ResourceRef(T* resource, const coordination::ResourceHandle& handle, ResourceManager* manager)
        : resource_(resource), handle_(handle), manager_(manager) {}
    
    /**
     * @brief Move constructor
     */
    ResourceRef(ResourceRef&& other) noexcept
        : resource_(other.resource_), handle_(std::move(other.handle_)), manager_(other.manager_) {
        other.resource_ = nullptr;
        other.manager_ = nullptr;
    }
    
    /**
     * @brief Move assignment
     */
    ResourceRef& operator=(ResourceRef&& other) noexcept {
        if (this != &other) {
            reset();
            resource_ = other.resource_;
            handle_ = std::move(other.handle_);
            manager_ = other.manager_;
            other.resource_ = nullptr;
            other.manager_ = nullptr;
        }
        return *this;
    }
    
    /**
     * @brief Destructor - automatic cleanup
     */
    ~ResourceRef() {
        reset();
    }
    
    // Disable copy operations
    ResourceRef(const ResourceRef&) = delete;
    ResourceRef& operator=(const ResourceRef&) = delete;
    
    /**
     * @brief Get resource pointer
     */
    T* get() const noexcept { return resource_; }
    
    /**
     * @brief Dereference operator
     */
    T& operator*() const noexcept { return *resource_; }
    
    /**
     * @brief Arrow operator
     */
    T* operator->() const noexcept { return resource_; }
    
    /**
     * @brief Check if reference is valid
     */
    bool is_valid() const noexcept { return resource_ != nullptr; }
    
    /**
     * @brief Boolean conversion
     */
    explicit operator bool() const noexcept { return is_valid(); }
    
    /**
     * @brief Get resource handle
     */
    const coordination::ResourceHandle& handle() const noexcept { return handle_; }
    
    /**
     * @brief Release resource manually
     */
    void reset();
    
    /**
     * @brief Release and return raw pointer (transfers ownership)
     */
    T* release() noexcept {
        T* result = resource_;
        resource_ = nullptr;
        manager_ = nullptr;
        return result;
    }

private:
    T* resource_;
    coordination::ResourceHandle handle_;
    ResourceManager* manager_;
};

/**
 * @brief Scoped resource for stack-based RAII management
 */
class ScopedResource {
public:
    /**
     * @brief Constructor
     */
    ScopedResource(const coordination::ResourceHandle& handle, ResourceManager* manager);
    
    /**
     * @brief Move constructor
     */
    ScopedResource(ScopedResource&& other) noexcept;
    
    /**
     * @brief Move assignment
     */
    ScopedResource& operator=(ScopedResource&& other) noexcept;
    
    /**
     * @brief Destructor - automatic release
     */
    ~ScopedResource();
    
    // Disable copy operations
    ScopedResource(const ScopedResource&) = delete;
    ScopedResource& operator=(const ScopedResource&) = delete;
    
    /**
     * @brief Get resource handle
     */
    const coordination::ResourceHandle& handle() const noexcept { return handle_; }
    
    /**
     * @brief Check if resource is valid
     */
    bool is_valid() const noexcept { return handle_.is_valid() && manager_ != nullptr; }
    
    /**
     * @brief Release resource manually
     */
    void release();

private:
    coordination::ResourceHandle handle_;
    ResourceManager* manager_;
};

/**
 * @brief Resource budget manager
 */
class ResourceBudgetManager {
public:
    /**
     * @brief Constructor
     */
    ResourceBudgetManager();
    
    /**
     * @brief Set budget for resource type
     */
    HALResult<void> set_budget(coordination::ResourceType type, const ResourceBudget& budget);
    
    /**
     * @brief Get budget for resource type
     */
    HALResult<ResourceBudget> get_budget(coordination::ResourceType type) const;
    
    /**
     * @brief Check if allocation is within budget
     */
    HALResult<bool> can_allocate(coordination::ResourceType type, size_t bytes) const;
    
    /**
     * @brief Record allocation
     */
    HALResult<void> record_allocation(coordination::ResourceType type, size_t bytes);
    
    /**
     * @brief Record deallocation
     */
    HALResult<void> record_deallocation(coordination::ResourceType type, size_t bytes);
    
    /**
     * @brief Get resource statistics
     */
    HALResult<ResourceStats> get_stats(coordination::ResourceType type) const;
    
    /**
     * @brief Get current pressure level
     */
    ResourcePressure get_pressure(coordination::ResourceType type) const;
    
    /**
     * @brief Set pressure callback
     */
    void set_pressure_callback(PressureCallback callback);
    
    /**
     * @brief Set reclamation callback
     */
    void set_reclamation_callback(coordination::ResourceType type, ReclamationCallback callback);
    
    /**
     * @brief Trigger emergency reclamation
     */
    HALResult<size_t> emergency_reclamation(coordination::ResourceType type, size_t requested_bytes);
    
    /**
     * @brief Update pressure levels
     */
    void update_pressure_levels();

private:
    mutable std::shared_mutex budgets_mutex_;
    std::unordered_map<coordination::ResourceType, ResourceBudget> budgets_;
    std::unordered_map<coordination::ResourceType, ResourceStats> stats_;
    std::unordered_map<coordination::ResourceType, ReclamationCallback> reclamation_callbacks_;
    PressureCallback pressure_callback_;
    
    void notify_pressure_change(coordination::ResourceType type, ResourcePressure old_pressure, ResourcePressure new_pressure);
    ResourcePressure calculate_pressure(const ResourceBudget& budget, const ResourceStats& stats) const;
};

/**
 * @brief Pool manager for efficient resource pooling
 */
class PoolManager { 
public:
    /**
     * @brief Pool configuration
     */
    struct PoolConfig {
        coordination::ResourceType type;
        size_t block_size;
        size_t initial_count;
        size_t max_count;
        size_t alignment;
        bool thread_safe;
        std::string name;
    };
    
    /**
     * @brief Constructor
     */
    PoolManager();
    
    /**
     * @brief Destructor
     */
    ~PoolManager();
    
    /**
     * @brief Create pool for resource type
     */
    HALResult<void> create_pool(const PoolConfig& config);
    
    /**
     * @brief Get pool for resource type and size
     */
    allocators::PoolAllocator* get_pool(coordination::ResourceType type, size_t size);
    
    /**
     * @brief Get pool statistics
     */
    HALResult<AllocatorStats> get_pool_stats(coordination::ResourceType type, size_t size) const;
    
    /**
     * @brief Resize pool
     */
    HALResult<void> resize_pool(coordination::ResourceType type, size_t size, size_t new_count);
    
    /**
     * @brief Reset all pools
     */
    HALResult<void> reset_all_pools();

private:
    std::mutex pools_mutex_;
    using PoolKey = std::pair<coordination::ResourceType, size_t>;
    std::unordered_map<PoolKey, std::unique_ptr<allocators::ThreadSafePoolAllocator>> pools_;
    std::unordered_map<PoolKey, PoolConfig> configs_;
};

/**
 * @brief Main resource manager
 */
class ResourceManager {
public:
    /**
     * @brief Get singleton instance
     */
    static ResourceManager& instance();
    
    /**
     * @brief Initialize resource manager
     */
    HALResult<void> initialize();
    
    /**
     * @brief Shutdown resource manager
     */
    HALResult<void> shutdown();
    
    /**
     * @brief Acquire resource with RAII
     */
    template<typename T>
    HALResult<ResourceRef<T>> acquire_resource(const std::string& name, 
                                              const coordination::ResourceMetadata& metadata,
                                              AcquisitionMode mode = AcquisitionMode::Blocking);
    
    /**
     * @brief Acquire scoped resource
     */
    HALResult<ScopedResource> acquire_scoped_resource(const std::string& name,
                                                     const coordination::ResourceMetadata& metadata,
                                                     AcquisitionMode mode = AcquisitionMode::Blocking);
    
    /**
     * @brief Release resource
     */
    HALResult<void> release_resource(const coordination::ResourceHandle& handle);
    
    /**
     * @brief Share resource with another driver
     */
    HALResult<coordination::ResourceHandle> share_resource(const coordination::ResourceHandle& handle,
                                                          const std::string& target_driver);
    
    /**
     * @brief Get resource statistics
     */
    HALResult<ResourceStats> get_resource_stats(coordination::ResourceType type) const;
    
    /**
     * @brief Set resource budget
     */
    HALResult<void> set_budget(coordination::ResourceType type, const ResourceBudget& budget);
    
    /**
     * @brief Get budget manager
     */
    ResourceBudgetManager& budget_manager() { return budget_manager_; }
    
    /**
     * @brief Get pool manager
     */
    PoolManager& pool_manager() { return pool_manager_; }
    
    /**
     * @brief Register pressure callback
     */
    void register_pressure_callback(PressureCallback callback);
    
    /**
     * @brief Register reclamation callback
     */
    void register_reclamation_callback(coordination::ResourceType type, ReclamationCallback callback);

private:
    ResourceManager() = default;
    ~ResourceManager() = default;
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    
    bool initialized_ = false;
    ResourceBudgetManager budget_manager_;
    PoolManager pool_manager_;
    
    // Resource acquisition internals
    template<typename T>
    HALResult<T*> allocate_resource(const coordination::ResourceMetadata& metadata);
    
    template<typename T>
    void deallocate_resource(T* resource, const coordination::ResourceMetadata& metadata);
    
    HALResult<void> wait_for_resource(const coordination::ResourceMetadata& metadata, 
                                     AcquisitionMode mode);
};

// Template implementations
template<typename T>
void ResourceRef<T>::reset() {
    if (resource_ && manager_) {
        manager_->release_resource(handle_);
        resource_ = nullptr;
        manager_ = nullptr;
    }
}

template<typename T>
HALResult<ResourceRef<T>> ResourceManager::acquire_resource(const std::string& name,
                                                           const coordination::ResourceMetadata& metadata,
                                                           AcquisitionMode mode) {
    if (!initialized_) {
        return HALResult<ResourceRef<T>>::error(
            HALError(HALErrorCategory::Resource, 1, "ResourceManager not initialized", nullptr));
    }
    
    // Check budget constraints
    auto can_allocate = budget_manager_.can_allocate(metadata.type, metadata.size_bytes);
    if (!can_allocate || !can_allocate.value()) {
        if (mode == AcquisitionMode::NonBlocking) {
            return HALResult<ResourceRef<T>>::error(
                HALError(HALErrorCategory::Resource, 2, "Resource budget exceeded", name.c_str()));
        }
        
        // Try to wait or trigger reclamation
        auto wait_result = wait_for_resource(metadata, mode);
        if (!wait_result) {
            return HALResult<ResourceRef<T>>::error(wait_result.error());
        }
    }
    
    // Register resource handle
    auto handle_result = coordination::ResourceRegistry::instance().register_resource(name, metadata);
    if (!handle_result) {
        return HALResult<ResourceRef<T>>::error(handle_result.error());
    }
    
    // Allocate actual resource
    auto resource_result = allocate_resource<T>(metadata);
    if (!resource_result) {
        coordination::ResourceRegistry::instance().unregister_resource(handle_result.value());
        return HALResult<ResourceRef<T>>::error(resource_result.error());
    }
    
    // Record allocation in budget
    budget_manager_.record_allocation(metadata.type, metadata.size_bytes);
    
    return HALResult<ResourceRef<T>>::success(
        ResourceRef<T>(resource_result.value(), handle_result.value(), this));
}

} // namespace flight::hal::core
