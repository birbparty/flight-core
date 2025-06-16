/**
 * @file resource_manager.cpp
 * @brief Flight HAL Resource Management System Implementation
 */

#include "flight/hal/core/resource_manager.hpp" 
#include "flight/hal/core/hal_logging.hpp"
#include "flight/hal/core/platform_detection.hpp"
#include <algorithm>
#include <thread>
#include <cstring>
#include <cstdlib>

namespace flight::hal::core {

// =====================================================================
// ScopedResource Implementation
// =====================================================================

ScopedResource::ScopedResource(const coordination::ResourceHandle& handle, ResourceManager* manager)
    : handle_(handle), manager_(manager) {}

ScopedResource::ScopedResource(ScopedResource&& other) noexcept
    : handle_(std::move(other.handle_)), manager_(other.manager_) {
    other.manager_ = nullptr;
}

ScopedResource& ScopedResource::operator=(ScopedResource&& other) noexcept {
    if (this != &other) {
        release();
        handle_ = std::move(other.handle_);
        manager_ = other.manager_;
        other.manager_ = nullptr;
    }
    return *this;
}

ScopedResource::~ScopedResource() {
    release();
}

void ScopedResource::release() {
    if (manager_ && handle_.is_valid()) {
        manager_->release_resource(handle_);
        manager_ = nullptr;
    }
}

// =====================================================================
// ResourceBudgetManager Implementation
// =====================================================================

ResourceBudgetManager::ResourceBudgetManager() {
    // Set default budgets based on reasonable defaults
    // TODO: Use actual platform detection when API is stable
    size_t total_memory = 1024 * 1024 * 1024; // Default to 1GB
    
    // Default budgets (as percentage of total memory)
    set_budget(coordination::ResourceType::Memory, 
              ResourceBudget(total_memory * 0.6, total_memory * 0.1, 75, 90));
    
    set_budget(coordination::ResourceType::Hardware,
              ResourceBudget(total_memory * 0.2, total_memory * 0.05, 80, 95));
    
    set_budget(coordination::ResourceType::Performance,
              ResourceBudget(total_memory * 0.1, 0, 70, 85));
    
    set_budget(coordination::ResourceType::Communication,
              ResourceBudget(total_memory * 0.05, 0, 80, 90));
    
    set_budget(coordination::ResourceType::Platform,
              ResourceBudget(total_memory * 0.05, 0, 85, 95));
}

HALResult<void> ResourceBudgetManager::set_budget(coordination::ResourceType type, const ResourceBudget& budget) {
    std::unique_lock<std::shared_mutex> lock(budgets_mutex_);
    
    budgets_[type] = budget;
    
    // Initialize stats if not present
    if (stats_.find(type) == stats_.end()) {
        stats_[type] = ResourceStats{};
    }
    
    return HALResult<void>::success();
}

HALResult<ResourceBudget> ResourceBudgetManager::get_budget(coordination::ResourceType type) const {
    std::shared_lock<std::shared_mutex> lock(budgets_mutex_);
    
    auto it = budgets_.find(type);
    if (it == budgets_.end()) {
        return HALResult<ResourceBudget>::error(
            HALError(HALErrorCategory::Resource, 1, "Budget not found for resource type", nullptr));
    }
    
    return HALResult<ResourceBudget>::success(it->second);
}

HALResult<bool> ResourceBudgetManager::can_allocate(coordination::ResourceType type, size_t bytes) const {
    std::shared_lock<std::shared_mutex> lock(budgets_mutex_);
    
    auto budget_it = budgets_.find(type);
    auto stats_it = stats_.find(type);
    
    if (budget_it == budgets_.end() || stats_it == stats_.end()) {
        // No budget set - allow allocation but warn
        HAL_LOG_MESSAGE(flight::hal::LogLevel::Warning, "No budget set for resource type, allowing allocation");
        return HALResult<bool>::success(true);
    }
    
    const auto& budget = budget_it->second;
    const auto& stats = stats_it->second;
    
    // Check if allocation would exceed budget
    size_t projected_usage = stats.current_usage + bytes;
    
    if (projected_usage > budget.max_bytes) {
        return HALResult<bool>::success(false);
    }
    
    return HALResult<bool>::success(true);
}

HALResult<void> ResourceBudgetManager::record_allocation(coordination::ResourceType type, size_t bytes) {
    std::unique_lock<std::shared_mutex> lock(budgets_mutex_);
    
    auto& stats = stats_[type];
    stats.current_usage += bytes;
    stats.total_allocated += bytes;
    stats.allocation_count++;
    stats.last_updated = std::chrono::steady_clock::now();
    
    if (stats.current_usage > stats.peak_usage) {
        stats.peak_usage = stats.current_usage;
    }
    
    // Update pressure level
    auto budget_it = budgets_.find(type);
    if (budget_it != budgets_.end()) {
        ResourcePressure old_pressure = stats.pressure;
        stats.pressure = calculate_pressure(budget_it->second, stats);
        
        if (stats.pressure != old_pressure) {
            lock.unlock(); // Release lock before callback
            notify_pressure_change(type, old_pressure, stats.pressure);
        }
    }
    
    return HALResult<void>::success();
}

HALResult<void> ResourceBudgetManager::record_deallocation(coordination::ResourceType type, size_t bytes) {
    std::unique_lock<std::shared_mutex> lock(budgets_mutex_);
    
    auto& stats = stats_[type];
    stats.current_usage = (stats.current_usage >= bytes) ? stats.current_usage - bytes : 0;
    stats.deallocation_count++;
    stats.last_updated = std::chrono::steady_clock::now();
    
    // Update pressure level
    auto budget_it = budgets_.find(type);
    if (budget_it != budgets_.end()) {
        ResourcePressure old_pressure = stats.pressure;
        stats.pressure = calculate_pressure(budget_it->second, stats);
        
        if (stats.pressure != old_pressure) {
            lock.unlock(); // Release lock before callback
            notify_pressure_change(type, old_pressure, stats.pressure);
        }
    }
    
    return HALResult<void>::success();
}

HALResult<ResourceStats> ResourceBudgetManager::get_stats(coordination::ResourceType type) const {
    std::shared_lock<std::shared_mutex> lock(budgets_mutex_);
    
    auto it = stats_.find(type);
    if (it == stats_.end()) {
        return HALResult<ResourceStats>::error(
            HALError(HALErrorCategory::Resource, 2, "Stats not found for resource type", nullptr));
    }
    
    return HALResult<ResourceStats>::success(it->second);
}

ResourcePressure ResourceBudgetManager::get_pressure(coordination::ResourceType type) const {
    std::shared_lock<std::shared_mutex> lock(budgets_mutex_);
    
    auto stats_it = stats_.find(type);
    if (stats_it == stats_.end()) {
        return ResourcePressure::None;
    }
    
    return stats_it->second.pressure;
}

void ResourceBudgetManager::set_pressure_callback(PressureCallback callback) {
    std::unique_lock<std::shared_mutex> lock(budgets_mutex_);
    pressure_callback_ = std::move(callback);
}

void ResourceBudgetManager::set_reclamation_callback(coordination::ResourceType type, ReclamationCallback callback) {
    std::unique_lock<std::shared_mutex> lock(budgets_mutex_);
    reclamation_callbacks_[type] = std::move(callback);
}

HALResult<size_t> ResourceBudgetManager::emergency_reclamation(coordination::ResourceType type, size_t requested_bytes) {
    std::shared_lock<std::shared_mutex> read_lock(budgets_mutex_);
    
    auto callback_it = reclamation_callbacks_.find(type);
    if (callback_it == reclamation_callbacks_.end()) {
        return HALResult<size_t>::success(0); // No reclamation callback available
    }
    
    auto callback = callback_it->second; // Copy callback
    read_lock.unlock();
    
    // Execute reclamation callback
    size_t reclaimed_bytes = callback(type, requested_bytes);
    
    // Update stats
    std::unique_lock<std::shared_mutex> write_lock(budgets_mutex_);
    auto& stats = stats_[type];
    stats.reclamation_count++;
    stats.last_updated = std::chrono::steady_clock::now();
    
    return HALResult<size_t>::success(reclaimed_bytes);
}

void ResourceBudgetManager::update_pressure_levels() {
    std::unique_lock<std::shared_mutex> lock(budgets_mutex_);
    
    for (auto& [type, stats] : stats_) {
        auto budget_it = budgets_.find(type);
        if (budget_it != budgets_.end()) {
            ResourcePressure old_pressure = stats.pressure;
            stats.pressure = calculate_pressure(budget_it->second, stats);
            
            if (stats.pressure != old_pressure) {
                lock.unlock(); // Release lock before callback
                notify_pressure_change(type, old_pressure, stats.pressure);
                lock.lock(); // Re-acquire lock
            }
        }
    }
}

void ResourceBudgetManager::notify_pressure_change(coordination::ResourceType type, 
                                                  ResourcePressure old_pressure, 
                                                  ResourcePressure new_pressure) {
    if (pressure_callback_) {
        std::shared_lock<std::shared_mutex> lock(budgets_mutex_);
        auto stats_it = stats_.find(type);
        if (stats_it != stats_.end()) {
            pressure_callback_(type, new_pressure, stats_it->second);
        }
    }
}

ResourcePressure ResourceBudgetManager::calculate_pressure(const ResourceBudget& budget, const ResourceStats& stats) const {
    if (budget.max_bytes == 0) {
        return ResourcePressure::None;
    }
    
    double usage_ratio = static_cast<double>(stats.current_usage) / budget.max_bytes * 100.0;
    
    if (usage_ratio >= budget.critical_threshold) {
        return ResourcePressure::Critical;
    } else if (usage_ratio >= budget.warning_threshold) {
        return ResourcePressure::High;
    } else if (usage_ratio >= 50.0) {
        return ResourcePressure::Medium;
    } else if (usage_ratio >= 25.0) {
        return ResourcePressure::Low;
    }
    
    return ResourcePressure::None;
}

// =====================================================================
// PoolManager Implementation
// =====================================================================

PoolManager::PoolManager() = default;

PoolManager::~PoolManager() = default;

HALResult<void> PoolManager::create_pool(const PoolConfig& config) {
    std::lock_guard<std::mutex> lock(pools_mutex_);
    
    PoolKey key = std::make_pair(config.type, config.block_size);
    
    // Check if pool already exists
    if (pools_.find(key) != pools_.end()) {
        return HALResult<void>::error(
            HALError(HALErrorCategory::Resource, 3, "Pool already exists", config.name.c_str()));
    }
    
    // Create new pool
    auto pool = std::make_unique<allocators::ThreadSafePoolAllocator>(
        config.initial_count, config.block_size, config.alignment, config.name);
    
    pools_[key] = std::move(pool);
    configs_[key] = config;
    
    HAL_LOG_MESSAGE(flight::hal::LogLevel::Info, "Created resource pool");
    
    return HALResult<void>::success();
}

allocators::PoolAllocator* PoolManager::get_pool(coordination::ResourceType type, size_t size) {
    std::lock_guard<std::mutex> lock(pools_mutex_);
    
    PoolKey key = std::make_pair(type, size);
    auto it = pools_.find(key);
    
    if (it != pools_.end()) {
        return it->second.get();
    }
    
    return nullptr;
}

HALResult<AllocatorStats> PoolManager::get_pool_stats(coordination::ResourceType type, size_t size) const {
    std::lock_guard<std::mutex> lock(pools_mutex_);
    
    PoolKey key = std::make_pair(type, size);
    auto it = pools_.find(key);
    
    if (it == pools_.end()) {
        return HALResult<AllocatorStats>::error(
            HALError(HALErrorCategory::Resource, 4, "Pool not found", nullptr));
    }
    
    return HALResult<AllocatorStats>::success(it->second->get_stats());
}

HALResult<void> PoolManager::resize_pool(coordination::ResourceType type, size_t size, size_t new_count) {
    std::lock_guard<std::mutex> lock(pools_mutex_);
    
    PoolKey key = std::make_pair(type, size);
    auto config_it = configs_.find(key);
    
    if (config_it == configs_.end()) {
        return HALResult<void>::error(
            HALError(HALErrorCategory::Resource, 5, "Pool not found for resize", nullptr));
    }
    
    // Update configuration
    config_it->second.max_count = new_count;
    
    // Note: Actual pool resizing would require more complex implementation
    // For now, we just update the configuration
    
    return HALResult<void>::success();
}

HALResult<void> PoolManager::reset_all_pools() {
    std::lock_guard<std::mutex> lock(pools_mutex_);
    
    for (auto& [key, pool] : pools_) {
        auto result = pool->reset();
        if (!result) {
            HAL_LOG_MESSAGE(flight::hal::LogLevel::Warning, "Failed to reset pool");
        }
    }
    
    return HALResult<void>::success();
}

// =====================================================================
// ResourceManager Implementation
// =====================================================================

ResourceManager& ResourceManager::instance() {
    static ResourceManager instance;
    return instance;
}

HALResult<void> ResourceManager::initialize() {
    if (initialized_) {
        return HALResult<void>::success();
    }
    
    HAL_LOG_MESSAGE(flight::hal::LogLevel::Info, "Initializing Resource Manager...");
    
    // Create default resource pools based on platform
    auto platform_info = flight::hal::platform_detection::RuntimePlatformDetector::detect_platform_info();
    
    if (platform_info.has_value()) {
        const auto& info = platform_info.value();
        
        // Create memory pools
        PoolManager::PoolConfig memory_pool{
            .type = coordination::ResourceType::Memory,
            .block_size = 1024,
            .initial_count = 100,
            .max_count = 1000,
            .alignment = alignof(std::max_align_t),
            .thread_safe = true,
            .name = "DefaultMemoryPool"
        };
        pool_manager_.create_pool(memory_pool);
        
        // Create smaller memory pool for frequent small allocations
        PoolManager::PoolConfig small_memory_pool{
            .type = coordination::ResourceType::Memory,
            .block_size = 64,
            .initial_count = 200,
            .max_count = 2000,
            .alignment = alignof(std::max_align_t),
            .thread_safe = true,
            .name = "SmallMemoryPool"
        };
        pool_manager_.create_pool(small_memory_pool);
        
        // Create hardware resource pool
        PoolManager::PoolConfig hardware_pool{
            .type = coordination::ResourceType::Hardware,
            .block_size = 256,
            .initial_count = 50,
            .max_count = 500,
            .alignment = alignof(std::max_align_t),
            .thread_safe = true,
            .name = "HardwareResourcePool"
        };
        pool_manager_.create_pool(hardware_pool);
    }
    
    // Set up pressure monitoring
    budget_manager_.set_pressure_callback([this](coordination::ResourceType type, 
                                                 ResourcePressure pressure, 
                                                 const ResourceStats& stats) {
        HAL_LOG_MESSAGE(flight::hal::LogLevel::Info, "Resource pressure changed");
        
        if (pressure >= ResourcePressure::High) {
            // Trigger emergency reclamation
            budget_manager_.emergency_reclamation(type, stats.current_usage * 0.2); // Try to reclaim 20%
        }
    });
    
    initialized_ = true;
    HAL_LOG_MESSAGE(flight::hal::LogLevel::Info, "Resource Manager initialized successfully");
    
    return HALResult<void>::success();
}

HALResult<void> ResourceManager::shutdown() {
    if (!initialized_) {
        return HALResult<void>::success();
    }
    
    HAL_LOG_MESSAGE(flight::hal::LogLevel::Info, "Shutting down Resource Manager...");
    
    // Reset all pools
    pool_manager_.reset_all_pools();
    
    initialized_ = false;
    HAL_LOG_MESSAGE(flight::hal::LogLevel::Info, "Resource Manager shutdown complete");
    
    return HALResult<void>::success();
}

HALResult<ScopedResource> ResourceManager::acquire_scoped_resource(const std::string& name,
                                                                  const coordination::ResourceMetadata& metadata,
                                                                  AcquisitionMode mode) {
    if (!initialized_) {
        return HALResult<ScopedResource>::error(
            HALError(HALErrorCategory::Resource, 6, "ResourceManager not initialized", nullptr));
    }
    
    // Check budget constraints
    auto can_allocate = budget_manager_.can_allocate(metadata.type, metadata.size_bytes);
    if (!can_allocate || !can_allocate.value()) {
        if (mode == AcquisitionMode::NonBlocking) {
            return HALResult<ScopedResource>::error(
                HALError(HALErrorCategory::Resource, 7, "Resource budget exceeded", name.c_str()));
        }
        
        // Try to wait or trigger reclamation
        auto wait_result = wait_for_resource(metadata, mode);
        if (!wait_result) {
            return HALResult<ScopedResource>::error(wait_result.error());
        }
    }
    
    // Register resource handle
    auto handle_result = coordination::ResourceRegistry::instance().register_resource(name, metadata);
    if (!handle_result) {
        return HALResult<ScopedResource>::error(handle_result.error());
    }
    
    // Record allocation in budget
    budget_manager_.record_allocation(metadata.type, metadata.size_bytes);
    
    return HALResult<ScopedResource>::success(ScopedResource(handle_result.value(), this));
}

HALResult<void> ResourceManager::release_resource(const coordination::ResourceHandle& handle) {
    if (!initialized_) {
        return HALResult<void>::error(
            HALError(HALErrorCategory::Resource, 8, "ResourceManager not initialized", nullptr));
    }
    
    // Get metadata for budget tracking
    const auto& metadata = handle.metadata();
    
    // Record deallocation
    budget_manager_.record_deallocation(metadata.type, metadata.size_bytes);
    
    // Unregister from resource registry
    auto result = coordination::ResourceRegistry::instance().unregister_resource(handle);
    if (!result) {
        HAL_LOG_MESSAGE(flight::hal::LogLevel::Warning, "Failed to unregister resource");
    }
    
    return HALResult<void>::success();
}

HALResult<coordination::ResourceHandle> ResourceManager::share_resource(const coordination::ResourceHandle& handle,
                                                                       const std::string& target_driver) {
    if (!initialized_) {
        return HALResult<coordination::ResourceHandle>::error(
            HALError(HALErrorCategory::Resource, 9, "ResourceManager not initialized", nullptr));
    }
    
    // Create shared resource with modified metadata
    auto metadata = handle.metadata();
    metadata.flags |= coordination::ResourceFlags::Shareable;
    
    std::string shared_name = handle.name() + "_shared_" + target_driver;
    
    auto shared_result = coordination::ResourceRegistry::instance().register_resource(shared_name, metadata);
    if (!shared_result) {
        return HALResult<coordination::ResourceHandle>::error(shared_result.error());
    }
    
    HAL_LOG_MESSAGE(flight::hal::LogLevel::Info, "Resource shared");
    
    return shared_result;
}

HALResult<ResourceStats> ResourceManager::get_resource_stats(coordination::ResourceType type) const {
    return budget_manager_.get_stats(type);
}

HALResult<void> ResourceManager::set_budget(coordination::ResourceType type, const ResourceBudget& budget) {
    return budget_manager_.set_budget(type, budget);
}

void ResourceManager::register_pressure_callback(PressureCallback callback) {
    budget_manager_.set_pressure_callback(std::move(callback));
}

void ResourceManager::register_reclamation_callback(coordination::ResourceType type, ReclamationCallback callback) {
    budget_manager_.set_reclamation_callback(type, std::move(callback));
}

// Template specializations would go here for specific resource types
template<>
HALResult<void*> ResourceManager::allocate_resource<void>(const coordination::ResourceMetadata& metadata) {
    // Try to get resource from appropriate pool first
    auto* pool = pool_manager_.get_pool(metadata.type, metadata.size_bytes);
    if (pool) {
        auto result = pool->allocate(metadata.size_bytes, metadata.alignment_bytes);
        if (result) {
            return result;
        }
    }
    
    // Fall back to system allocation
    void* ptr = nullptr;
    
    if (metadata.alignment_bytes > alignof(std::max_align_t)) {
        // Use aligned allocation
        ptr = std::aligned_alloc(metadata.alignment_bytes, metadata.size_bytes);
    } else {
        ptr = std::malloc(metadata.size_bytes);
    }
    
    if (!ptr) {
        return HALResult<void*>::error(
            HALError(HALErrorCategory::Resource, 10, "Failed to allocate resource", nullptr));
    }
    
    // Zero-initialize if requested
    if (metadata.flags & coordination::ResourceFlags::None) { // Would check for Zero flag if it existed
        std::memset(ptr, 0, metadata.size_bytes);
    }
    
    return HALResult<void*>::success(ptr);
}

template<>
void ResourceManager::deallocate_resource<void>(void* resource, const coordination::ResourceMetadata& metadata) {
    // Try to return to pool first
    auto* pool = pool_manager_.get_pool(metadata.type, metadata.size_bytes);
    if (pool && pool->owns_pointer(resource)) {
        pool->deallocate(resource);
        return;
    }
    
    // Fall back to system deallocation
    std::free(resource);
}

HALResult<void> ResourceManager::wait_for_resource(const coordination::ResourceMetadata& metadata, 
                                                  AcquisitionMode mode) {
    if (mode == AcquisitionMode::NonBlocking) {
        return HALResult<void>::error(
            HALError(HALErrorCategory::Resource, 11, "Resource not available (non-blocking)", nullptr));
    }
    
    // Try emergency reclamation first
    auto reclaimed = budget_manager_.emergency_reclamation(metadata.type, metadata.size_bytes);
    if (reclaimed && reclaimed.value() >= metadata.size_bytes) {
        return HALResult<void>::success();
    }
    
    if (mode == AcquisitionMode::Timeout) {
        // Wait with timeout
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Check if resource is now available
        auto can_allocate = budget_manager_.can_allocate(metadata.type, metadata.size_bytes);
        if (can_allocate && can_allocate.value()) {
            return HALResult<void>::success();
        }
        
        return HALResult<void>::error(
            HALError(HALErrorCategory::Resource, 12, "Resource allocation timeout", nullptr));
    }
    
    if (mode == AcquisitionMode::Emergency) {
        // Emergency mode - always allow allocation
        HAL_LOG_MESSAGE(flight::hal::LogLevel::Warning, "Emergency resource allocation");
        return HALResult<void>::success();
    }
    
    // Blocking mode - wait for resource to become available
    while (true) {
        auto can_allocate = budget_manager_.can_allocate(metadata.type, metadata.size_bytes);
        if (can_allocate && can_allocate.value()) {
            return HALResult<void>::success();
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

} // namespace flight::hal::core
