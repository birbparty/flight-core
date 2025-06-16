/**
 * @file deadlock_prevention.hpp
 * @brief Flight HAL Deadlock Prevention System
 * 
 * Provides deadlock detection, prevention, and recovery mechanisms for
 * safe cross-driver resource coordination.
 */

#pragma once

#include "../core/hal_result.hpp"
#include "resource_handle.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <atomic>
#include <chrono>
#include <memory>
#include <queue>

namespace flight::hal::coordination {

/**
 * @brief Resource acquisition request with timing and priority information
 */
struct ResourceRequest {
    std::string requester_id;           ///< ID of requesting driver/component
    ResourceHandle resource_handle;     ///< Resource being requested
    ResourcePriority priority;          ///< Request priority level
    std::chrono::steady_clock::time_point request_time; ///< When request was made
    std::chrono::milliseconds timeout;  ///< Maximum time to wait
    bool exclusive;                     ///< Whether exclusive access is needed
    
    ResourceRequest(const std::string& id, const ResourceHandle& handle, 
                   ResourcePriority prio = ResourcePriority::Normal,
                   std::chrono::milliseconds to = std::chrono::milliseconds(5000),
                   bool excl = true)
        : requester_id(id)
        , resource_handle(handle)
        , priority(prio)
        , request_time(std::chrono::steady_clock::now())
        , timeout(to)
        , exclusive(excl)
    {}
};

/**
 * @brief Dependency edge in the resource dependency graph
 */
struct ResourceDependency {
    std::string from_requester;         ///< Requester that owns resource
    std::string to_requester;           ///< Requester waiting for resource
    ResourceHandle resource_handle;     ///< Resource causing the dependency
    std::chrono::steady_clock::time_point created_time; ///< When dependency was created
    
    ResourceDependency(const std::string& from, const std::string& to, 
                      const ResourceHandle& handle)
        : from_requester(from)
        , to_requester(to)
        , resource_handle(handle)
        , created_time(std::chrono::steady_clock::now())
    {}
};

/**
 * @brief Resource ordering constraint for deadlock prevention
 */
struct ResourceOrder {
    ResourceType type;                  ///< Resource type
    uint32_t order_value;              ///< Ordering value (lower = acquire first)
    std::string description;           ///< Human-readable description
    
    // Default constructor for unordered_map compatibility
    ResourceOrder() : type(ResourceType::Custom), order_value(999), description("Default") {}
    
    ResourceOrder(ResourceType t, uint32_t order, const std::string& desc)
        : type(t), order_value(order), description(desc) {}
};

/**
 * @brief Deadlock detection and prevention engine
 */
class DeadlockPrevention {
public:
    /**
     * @brief Get singleton instance
     * @return Reference to global deadlock prevention system
     */
    static DeadlockPrevention& instance();
    
    /**
     * @brief Initialize deadlock prevention system
     * @return HALResult indicating success or failure
     */
    HALResult<void> initialize();
    
    /**
     * @brief Shutdown deadlock prevention system
     * @return HALResult indicating success or failure
     */
    HALResult<void> shutdown();
    
    /**
     * @brief Register a resource ordering constraint
     * @param order Resource ordering constraint
     * @return HALResult indicating success or failure
     */
    HALResult<void> register_resource_order(const ResourceOrder& order);
    
    /**
     * @brief Check if a resource acquisition would cause deadlock
     * @param request Resource acquisition request
     * @return HALResult indicating if acquisition is safe
     */
    HALResult<bool> is_acquisition_safe(const ResourceRequest& request);
    
    /**
     * @brief Request resource acquisition with deadlock prevention
     * @param request Resource acquisition request
     * @return HALResult indicating success or failure with reason
     */
    HALResult<void> request_resource_acquisition(const ResourceRequest& request);
    
    /**
     * @brief Release a previously acquired resource
     * @param requester_id ID of requester releasing resource
     * @param resource_handle Resource being released
     * @return HALResult indicating success or failure
     */
    HALResult<void> release_resource(const std::string& requester_id,
                                    const ResourceHandle& resource_handle);
    
    /**
     * @brief Detect if there are any circular dependencies (deadlocks)
     * @return HALResult containing deadlock detection results
     */
    struct DeadlockInfo {
        bool deadlock_detected;
        std::vector<std::string> cycle_participants;
        std::vector<ResourceHandle> involved_resources;
        std::string description;
    };
    
    HALResult<DeadlockInfo> detect_deadlock();
    
    /**
     * @brief Resolve detected deadlock using priority-based preemption
     * @param deadlock_info Information about the detected deadlock
     * @return HALResult indicating success or failure of resolution
     */
    HALResult<void> resolve_deadlock(const DeadlockInfo& deadlock_info);
    
    /**
     * @brief Get current resource ownership information
     * @return Map of requester IDs to owned resource handles
     */
    std::unordered_map<std::string, std::vector<ResourceHandle>> get_resource_ownership() const;
    
    /**
     * @brief Get current waiting requests
     * @return Vector of pending resource requests
     */
    std::vector<ResourceRequest> get_waiting_requests() const;
    
    /**
     * @brief Get dependency graph edges
     * @return Vector of current dependencies
     */
    std::vector<ResourceDependency> get_dependencies() const;
    
    /**
     * @brief Clear expired requests and dependencies
     * @return Number of expired items cleaned up
     */
    size_t cleanup_expired_items();
    
    /**
     * @brief Statistics about deadlock prevention system
     */
    struct DeadlockStats {
        uint64_t requests_processed;
        uint64_t requests_denied;
        uint64_t deadlocks_detected;
        uint64_t deadlocks_resolved;
        uint64_t timeouts_occurred;
        uint64_t preemptions_performed;
        double average_wait_time_ms;
    };
    
    /**
     * @brief Get deadlock prevention statistics
     * @return Current statistics
     */
    DeadlockStats get_stats() const;
    
    /**
     * @brief Clear statistics
     */
    void clear_stats();

private:
    DeadlockPrevention() = default;
    ~DeadlockPrevention() = default;
    DeadlockPrevention(const DeadlockPrevention&) = delete;
    DeadlockPrevention& operator=(const DeadlockPrevention&) = delete;
    
    /**
     * @brief Check resource ordering constraints
     * @param requester_id ID of requester
     * @param new_resource Resource being requested
     * @return true if ordering is valid
     */
    bool check_resource_ordering(const std::string& requester_id,
                                const ResourceHandle& new_resource) const;
    
    /**
     * @brief Perform depth-first search for cycle detection
     * @param start_node Starting node for DFS
     * @param visited Set of visited nodes
     * @param recursion_stack Current recursion stack
     * @param cycle_path Path of nodes in detected cycle
     * @return true if cycle detected
     */
    bool dfs_cycle_detection(const std::string& start_node,
                            std::unordered_set<std::string>& visited,
                            std::unordered_set<std::string>& recursion_stack,
                            std::vector<std::string>& cycle_path) const;
    
    /**
     * @brief Calculate priority score for preemption decisions
     * @param requester_id ID of requester
     * @param resource_handle Resource being considered
     * @return Priority score (higher = more important)
     */
    uint32_t calculate_preemption_priority(const std::string& requester_id,
                                          const ResourceHandle& resource_handle) const;
    
    /**
     * @brief Get ordering value for a resource type
     * @param type Resource type
     * @return Ordering value (lower = acquire first)
     */
    uint32_t get_resource_order(ResourceType type) const;
    
    /**
     * @brief Add dependency edge to the graph
     * @param from_requester Requester that owns resource
     * @param to_requester Requester waiting for resource
     * @param resource_handle Resource causing dependency
     */
    void add_dependency(const std::string& from_requester,
                       const std::string& to_requester,
                       const ResourceHandle& resource_handle);
    
    /**
     * @brief Remove dependency edge from the graph
     * @param from_requester Requester that owned resource
     * @param to_requester Requester that was waiting
     * @param resource_handle Resource that caused dependency
     */
    void remove_dependency(const std::string& from_requester,
                          const std::string& to_requester,
                          const ResourceHandle& resource_handle);
    
    // Synchronization
    mutable std::mutex system_mutex_;
    std::atomic<bool> initialized_{false};
    
    // Resource ownership tracking
    std::unordered_map<std::string, std::vector<ResourceHandle>> owned_resources_;
    std::unordered_map<uint64_t, std::string> resource_owners_; // resource_id -> owner_id
    
    // Waiting requests
    std::queue<ResourceRequest> waiting_requests_;
    std::unordered_map<std::string, std::vector<ResourceRequest>> requests_by_requester_;
    
    // Dependency graph
    std::vector<ResourceDependency> dependencies_;
    std::unordered_map<std::string, std::vector<std::string>> dependency_graph_; // adjacency list
    
    // Resource ordering constraints
    std::unordered_map<ResourceType, ResourceOrder> resource_orders_;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    DeadlockStats stats_{};
    
    // Default resource ordering
    void initialize_default_resource_orders();
};

/**
 * @brief RAII resource lock with automatic deadlock prevention
 */
class ResourceLock {
public:
    /**
     * @brief Acquire resource lock with deadlock prevention
     * @param requester_id ID of requesting component
     * @param resource_handle Resource to acquire
     * @param priority Request priority
     * @param timeout Maximum time to wait for acquisition
     * @param exclusive Whether exclusive access is needed
     */
    ResourceLock(const std::string& requester_id,
                 const ResourceHandle& resource_handle,
                 ResourcePriority priority = ResourcePriority::Normal,
                 std::chrono::milliseconds timeout = std::chrono::milliseconds(5000),
                 bool exclusive = true);
    
    /**
     * @brief Release resource lock
     */
    ~ResourceLock();
    
    /**
     * @brief Move constructor
     */
    ResourceLock(ResourceLock&& other) noexcept;
    
    /**
     * @brief Move assignment operator
     */
    ResourceLock& operator=(ResourceLock&& other) noexcept;
    
    /**
     * @brief Deleted copy constructor
     */
    ResourceLock(const ResourceLock&) = delete;
    
    /**
     * @brief Deleted copy assignment operator
     */
    ResourceLock& operator=(const ResourceLock&) = delete;
    
    /**
     * @brief Check if lock was successfully acquired
     * @return true if resource is locked
     */
    bool is_locked() const noexcept { return locked_; }
    
    /**
     * @brief Get the locked resource handle
     * @return Resource handle (only valid if is_locked() returns true)
     */
    const ResourceHandle& resource() const noexcept { return resource_handle_; }
    
    /**
     * @brief Get lock acquisition result
     * @return HALResult with acquisition status
     */
    const HALResult<void>& result() const noexcept { return result_; }
    
    /**
     * @brief Manually release the lock (before destructor)
     * @return HALResult indicating success or failure
     */
    HALResult<void> release();

private:
    std::string requester_id_;
    ResourceHandle resource_handle_;
    bool locked_;
    HALResult<void> result_;
};

/**
 * @brief Helper macros for resource locking
 */
#define HAL_LOCK_RESOURCE(requester_id, resource_handle) \
    ResourceLock __hal_lock_##__LINE__(requester_id, resource_handle); \
    if (!__hal_lock_##__LINE__.is_locked()) { \
        return __hal_lock_##__LINE__.result().error(); \
    }

#define HAL_LOCK_RESOURCE_WITH_PRIORITY(requester_id, resource_handle, priority) \
    ResourceLock __hal_lock_##__LINE__(requester_id, resource_handle, priority); \
    if (!__hal_lock_##__LINE__.is_locked()) { \
        return __hal_lock_##__LINE__.result().error(); \
    }

#define HAL_LOCK_RESOURCE_WITH_TIMEOUT(requester_id, resource_handle, timeout) \
    ResourceLock __hal_lock_##__LINE__(requester_id, resource_handle, \
                                      ResourcePriority::Normal, timeout); \
    if (!__hal_lock_##__LINE__.is_locked()) { \
        return __hal_lock_##__LINE__.result().error(); \
    }

} // namespace flight::hal::coordination
