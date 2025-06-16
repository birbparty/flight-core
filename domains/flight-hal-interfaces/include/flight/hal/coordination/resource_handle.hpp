/**
 * @file resource_handle.hpp
 * @brief Flight HAL Resource Handle System
 * 
 * Provides unique resource identification and metadata for cross-driver
 * resource sharing and coordination.
 */

#pragma once

#include "../core/hal_result.hpp"
#include <cstdint>
#include <string>
#include <memory>
#include <atomic>
#include <unordered_map>
#include <chrono>

namespace flight::hal::coordination {

/**
 * @brief Types of resources managed by the coordination system
 */
enum class ResourceType : uint32_t {
    Hardware = 0x01,        ///< Physical hardware resources (timers, DMA channels)
    Memory = 0x02,          ///< Memory regions, pools, caches
    Performance = 0x04,     ///< CPU time, bandwidth limits
    Communication = 0x08,   ///< Message queues, event channels
    Platform = 0x10,        ///< Platform-specific resources
    Custom = 0x80000000     ///< Custom resource types
};

/**
 * @brief Resource access patterns for optimization
 */
enum class AccessPattern : uint8_t {
    ReadOnly,       ///< Resource is only read from
    WriteOnly,      ///< Resource is only written to
    ReadWrite,      ///< Resource is both read and written
    Streaming,      ///< Resource is accessed in streaming fashion
    Random          ///< Resource is accessed randomly
};

/**
 * @brief Resource priority levels for arbitration
 */
enum class ResourcePriority : uint8_t {
    Low = 0,        ///< Background operations
    Normal = 1,     ///< Standard operations
    High = 2,       ///< Time-critical operations
    Critical = 3    ///< System-critical operations
};

/**
 * @brief Resource flags for behavior control
 */
enum ResourceFlags : uint32_t {
    None = 0x00,
    Shareable = 0x01,           ///< Resource can be shared between drivers
    Exclusive = 0x02,           ///< Resource requires exclusive access
    Persistent = 0x04,          ///< Resource persists across driver restarts
    Cacheable = 0x08,           ///< Resource data can be cached
    GPUAccessible = 0x10,       ///< Resource is GPU-accessible
    DMACapable = 0x20,          ///< Resource supports DMA transfers
    MemoryMapped = 0x40,        ///< Resource is memory-mapped
    Synchronized = 0x80         ///< Resource requires synchronization
};

/**
 * @brief Resource metadata for coordination decisions
 */
struct ResourceMetadata {
    ResourceType type;
    AccessPattern access_pattern;
    ResourcePriority priority;
    uint32_t flags;
    size_t size_bytes;
    size_t alignment_bytes;
    std::chrono::milliseconds timeout;
    std::string description;
    
    // Platform-specific metadata
    void* platform_data = nullptr;
    size_t platform_data_size = 0;
    
    ResourceMetadata() 
        : type(ResourceType::Custom)
        , access_pattern(AccessPattern::ReadWrite)
        , priority(ResourcePriority::Normal)
        , flags(ResourceFlags::None)
        , size_bytes(0)
        , alignment_bytes(1)
        , timeout(std::chrono::milliseconds(5000))
    {}
};

/**
 * @brief Unique resource identifier with metadata
 */
class ResourceHandle {
public:
    /**
     * @brief Create an invalid resource handle
     */
    ResourceHandle() : id_(0), version_(0) {}
    
    /**
     * @brief Create a resource handle with metadata
     * @param name Human-readable resource name
     * @param metadata Resource metadata
     */
    ResourceHandle(const std::string& name, const ResourceMetadata& metadata);
    
    /**
     * @brief Copy constructor
     */
    ResourceHandle(const ResourceHandle& other) = default;
    
    /**
     * @brief Move constructor
     */
    ResourceHandle(ResourceHandle&& other) noexcept = default;
    
    /**
     * @brief Assignment operator
     */
    ResourceHandle& operator=(const ResourceHandle& other) = default;
    
    /**
     * @brief Move assignment operator
     */
    ResourceHandle& operator=(ResourceHandle&& other) noexcept = default;
    
    /**
     * @brief Check if handle is valid
     * @return true if handle represents a valid resource
     */
    bool is_valid() const noexcept { return id_ != 0; }
    
    /**
     * @brief Get unique resource ID
     * @return 64-bit unique identifier
     */
    uint64_t id() const noexcept { return id_; }
    
    /**
     * @brief Get resource version
     * @return Version number for resource updates
     */
    uint32_t version() const noexcept { return version_; }
    
    /**
     * @brief Get resource name
     * @return Human-readable resource name
     */
    const std::string& name() const noexcept { return name_; }
    
    /**
     * @brief Get resource metadata
     * @return Const reference to resource metadata
     */
    const ResourceMetadata& metadata() const noexcept { return metadata_; }
    
    /**
     * @brief Update resource metadata
     * @param new_metadata Updated metadata
     * @return HALResult indicating success or failure
     */
    HALResult<void> update_metadata(const ResourceMetadata& new_metadata);
    
    /**
     * @brief Increment resource version
     * @return New version number
     */
    uint32_t increment_version() noexcept { return ++version_; }
    
    /**
     * @brief Equality comparison
     */
    bool operator==(const ResourceHandle& other) const noexcept {
        return id_ == other.id_;
    }
    
    /**
     * @brief Inequality comparison
     */
    bool operator!=(const ResourceHandle& other) const noexcept {
        return !(*this == other);
    }
    
    /**
     * @brief Less-than comparison for ordering
     */
    bool operator<(const ResourceHandle& other) const noexcept {
        return id_ < other.id_;
    }
    
    /**
     * @brief Hash function for use in containers
     */
    struct Hash {
        size_t operator()(const ResourceHandle& handle) const noexcept {
            return std::hash<uint64_t>{}(handle.id_);
        }
    };

private:
    uint64_t id_;
    uint32_t version_;
    std::string name_;
    ResourceMetadata metadata_;
    
    static std::atomic<uint64_t> next_id_;
    
    friend class ResourceRegistry;
};

/**
 * @brief Registry for managing resource handles and metadata
 */
class ResourceRegistry {
public:
    /**
     * @brief Get singleton instance
     * @return Reference to global resource registry
     */
    static ResourceRegistry& instance();
    
    /**
     * @brief Register a new resource
     * @param name Human-readable resource name
     * @param metadata Resource metadata
     * @return HALResult containing the resource handle or error
     */
    HALResult<ResourceHandle> register_resource(const std::string& name,
                                               const ResourceMetadata& metadata);
    
    /**
     * @brief Unregister a resource
     * @param handle Resource handle to unregister
     * @return HALResult indicating success or failure
     */
    HALResult<void> unregister_resource(const ResourceHandle& handle);
    
    /**
     * @brief Find resource by name
     * @param name Resource name to search for
     * @return HALResult containing the resource handle or error
     */
    HALResult<ResourceHandle> find_resource(const std::string& name) const;
    
    /**
     * @brief Get all resources of a specific type
     * @param type Resource type to filter by
     * @return Vector of resource handles
     */
    std::vector<ResourceHandle> get_resources_by_type(ResourceType type) const;
    
    /**
     * @brief Get resource metadata
     * @param handle Resource handle
     * @return HALResult containing metadata or error
     */
    HALResult<ResourceMetadata> get_metadata(const ResourceHandle& handle) const;
    
    /**
     * @brief Update resource metadata
     * @param handle Resource handle
     * @param metadata New metadata
     * @return HALResult indicating success or failure
     */
    HALResult<void> update_metadata(const ResourceHandle& handle,
                                   const ResourceMetadata& metadata);
    
    /**
     * @brief Get total number of registered resources
     * @return Number of active resources
     */
    size_t get_resource_count() const;
    
    /**
     * @brief Clear all resources (for testing/shutdown)
     * @return HALResult indicating success or failure
     */
    HALResult<void> clear_all_resources();

private:
    ResourceRegistry() = default;
    ~ResourceRegistry() = default;
    ResourceRegistry(const ResourceRegistry&) = delete;
    ResourceRegistry& operator=(const ResourceRegistry&) = delete;
    
    mutable std::mutex registry_mutex_;
    std::unordered_map<uint64_t, ResourceHandle> resources_by_id_;
    std::unordered_map<std::string, uint64_t> resources_by_name_;
    std::unordered_map<ResourceType, std::vector<uint64_t>> resources_by_type_;
};

/**
 * @brief Helper functions for resource handle operations
 */
namespace resource_utils {

/**
 * @brief Check if two resource types are compatible for sharing
 * @param type1 First resource type
 * @param type2 Second resource type
 * @return true if types can share resources
 */
constexpr bool are_types_compatible(ResourceType type1, ResourceType type2) noexcept {
    // Same types are always compatible
    if (type1 == type2) return true;
    
    // Memory and Hardware can sometimes be compatible
    if ((type1 == ResourceType::Memory && type2 == ResourceType::Hardware) ||
        (type1 == ResourceType::Hardware && type2 == ResourceType::Memory)) {
        return true;
    }
    
    return false;
}

/**
 * @brief Calculate resource priority score for arbitration
 * @param priority Resource priority level
 * @param flags Resource flags
 * @return Numeric priority score (higher = more important)
 */
constexpr uint32_t calculate_priority_score(ResourcePriority priority, uint32_t flags) noexcept {
    uint32_t base_score = static_cast<uint32_t>(priority) * 1000;
    
    // Add bonuses for critical flags
    if (flags & ResourceFlags::Exclusive) base_score += 500;
    if (flags & ResourceFlags::Synchronized) base_score += 200;
    if (flags & ResourceFlags::DMACapable) base_score += 100;
    
    return base_score;
}

/**
 * @brief Check if resource requires synchronization
 * @param metadata Resource metadata
 * @return true if synchronization is needed
 */
constexpr bool requires_synchronization(const ResourceMetadata& metadata) noexcept {
    return (metadata.flags & ResourceFlags::Synchronized) ||
           (metadata.access_pattern == AccessPattern::ReadWrite) ||
           (metadata.priority >= ResourcePriority::High);
}

} // namespace resource_utils

} // namespace flight::hal::coordination
