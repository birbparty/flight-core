/**
 * @file thread.hpp
 * @brief Flight HAL Elite Threading and Concurrency Interface
 * 
 * Unified threading abstraction providing graceful degradation from single-core
 * Dreamcast to modern multi-core systems. Supports thread creation, synchronization
 * primitives, work queues, and cooperative threading models with platform-adaptive
 * execution strategies.
 */

#pragma once

#include "../core/driver_registry.hpp"
#include "../core/hal_result.hpp"
#include "../core/hal_capabilities.hpp"
#include "../core/platform_detection.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>
#include <string_view>
#include <chrono>

namespace flight::hal {

/**
 * @brief Threading execution models for different platform capabilities
 */
enum class ThreadingModel : uint8_t {
    SingleThreaded = 0,     ///< Single-threaded cooperative (Dreamcast)
    Cooperative,            ///< Limited cooperative threading (PSP)
    WebWorkers,            ///< Web Workers model (Web platforms)
    Preemptive             ///< Full preemptive multithreading (Desktop)
};

/**
 * @brief Thread execution states
 */
enum class ThreadState : uint8_t {
    Created = 0,           ///< Thread created but not started
    Running,               ///< Thread is actively running
    Suspended,             ///< Thread is suspended/paused
    Blocked,               ///< Thread is blocked on synchronization
    Terminated,            ///< Thread has completed execution
    Error                  ///< Thread encountered an error
};

/**
 * @brief Thread priority levels with platform-adaptive mapping
 */
enum class ThreadPriority : uint8_t {
    Idle = 0,              ///< Lowest priority, runs when nothing else can
    Low,                   ///< Low priority background tasks
    Normal,                ///< Normal priority for most tasks
    High,                  ///< High priority for time-sensitive tasks
    Critical,              ///< Critical priority for real-time tasks
    Realtime               ///< Highest priority, platform-dependent availability
};

/**
 * @brief Work item priority for task scheduling
 */
enum class WorkPriority : uint8_t {
    Deferred = 0,          ///< Can be delayed significantly
    Background,            ///< Background processing
    Normal,                ///< Normal priority work
    High,                  ///< High priority, should run soon
    Urgent,                ///< Urgent work, run as soon as possible
    Immediate              ///< Immediate execution required
};

/**
 * @brief Synchronization primitive types
 */
enum class SyncPrimitiveType : uint8_t {
    Mutex = 0,             ///< Mutual exclusion lock
    RecursiveMutex,        ///< Recursive mutex (same thread can lock multiple times)
    Semaphore,             ///< Counting semaphore
    BinarySemaphore,       ///< Binary semaphore (0 or 1)
    ConditionVariable,     ///< Condition variable for thread coordination
    Event,                 ///< Event signaling mechanism
    Barrier,               ///< Thread barrier for synchronization points
    ReadWriteLock          ///< Reader-writer lock for shared data
};

/**
 * @brief Work queue execution modes
 */
enum class WorkQueueMode : uint8_t {
    Immediate = 0,         ///< Execute immediately (single-threaded)
    Deferred,              ///< Execute when process_pending() is called
    Parallel,              ///< Execute in parallel using thread pool
    Sequential,            ///< Execute sequentially in background thread
    Adaptive               ///< Adaptive mode based on platform capabilities
};

/**
 * @brief Thread configuration parameters
 */
struct ThreadConfig {
    std::string_view name;          ///< Thread name for debugging
    size_t stack_size;              ///< Stack size in bytes (0 = default)
    ThreadPriority priority;        ///< Thread priority
    bool detached;                  ///< Whether thread runs detached
    uint32_t processor_affinity;    ///< Processor affinity mask (0 = any)
    
    /**
     * @brief Create default thread configuration
     * @param thread_name Name for the thread
     * @return Default thread configuration
     */
    static ThreadConfig create_default(std::string_view thread_name = "WorkerThread");
    
    /**
     * @brief Create high priority thread configuration
     * @param thread_name Name for the thread
     * @return High priority thread configuration
     */
    static ThreadConfig create_high_priority(std::string_view thread_name = "HighPriorityThread");
    
    /**
     * @brief Create real-time thread configuration
     * @param thread_name Name for the thread
     * @return Real-time thread configuration
     */
    static ThreadConfig create_realtime(std::string_view thread_name = "RealtimeThread");
};

/**
 * @brief Work item handle for tracking submitted work
 */
struct WorkHandle {
    uint64_t id;                    ///< Unique work item identifier
    WorkPriority priority;          ///< Work priority level
    bool is_valid;                  ///< Whether handle is valid
    
    /**
     * @brief Check if handle is valid
     * @return true if handle represents valid work
     */
    bool valid() const { return is_valid; }
    
    /**
     * @brief Invalidate the handle
     */
    void invalidate() { is_valid = false; }
};

/**
 * @brief Thread statistics and performance metrics
 */
struct ThreadStats {
    uint64_t context_switches;      ///< Number of context switches
    uint64_t cpu_time_us;          ///< CPU time in microseconds
    uint64_t wall_time_us;         ///< Wall clock time in microseconds
    uint64_t memory_usage_bytes;   ///< Memory usage in bytes
    double cpu_utilization;        ///< CPU utilization percentage (0.0-1.0)
    uint32_t priority_changes;     ///< Number of priority changes
    uint32_t blocking_operations;  ///< Number of blocking operations
    
    /**
     * @brief Reset all statistics
     */
    void reset();
    
    /**
     * @brief Calculate efficiency ratio
     * @return Efficiency as CPU time / wall time
     */
    double get_efficiency() const;
};

/**
 * @brief Platform threading capabilities
 */
struct ThreadingCapabilities {
    ThreadingModel model;               ///< Current threading model
    uint32_t max_threads;              ///< Maximum number of threads supported
    uint32_t max_work_items;           ///< Maximum work items in queue
    bool supports_priorities;          ///< Whether thread priorities are supported
    bool supports_affinity;            ///< Whether processor affinity is supported
    bool supports_realtime;            ///< Whether real-time scheduling is available
    bool supports_detached_threads;    ///< Whether detached threads are supported
    bool supports_thread_local;        ///< Whether thread-local storage is supported
    bool supports_atomic_operations;   ///< Whether atomic operations are available
    uint32_t context_switch_overhead_ns; ///< Context switch overhead in nanoseconds
    uint32_t sync_overhead_ns;         ///< Synchronization overhead in nanoseconds
    size_t default_stack_size;         ///< Default thread stack size
    size_t min_stack_size;             ///< Minimum thread stack size
    size_t max_stack_size;             ///< Maximum thread stack size
    
    /**
     * @brief Check if specific synchronization primitive is supported
     * @param type Synchronization primitive type
     * @return true if supported
     */
    bool supports_sync_primitive(SyncPrimitiveType type) const;
    
    /**
     * @brief Get recommended thread count for parallel work
     * @return Recommended number of threads
     */
    uint32_t get_recommended_thread_count() const;
};

// Forward declarations
class IThread;
class IThreadPool;
class ISyncPrimitive;
class IWorkQueue;
class IWorkItem;

/**
 * @brief Thread execution function type
 */
using ThreadFunction = std::function<void()>;

/**
 * @brief Work item execution function type
 */
using WorkFunction = std::function<void()>;

/**
 * @brief Thread completion callback type
 */
using ThreadCallback = std::function<void(uint32_t thread_id, ThreadState final_state)>;

/**
 * @brief Work completion callback type
 */
using WorkCallback = std::function<void(const WorkHandle& handle, bool success)>;

/**
 * @brief Thread monitoring callback type
 */
using ThreadMonitorCallback = std::function<void(uint32_t thread_id, const ThreadStats& stats)>;

/**
 * @brief Individual work item interface
 */
class IWorkItem {
public:
    virtual ~IWorkItem() = default;
    
    /**
     * @brief Execute the work item
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> execute() = 0;
    
    /**
     * @brief Get work item priority
     * @return Work priority level
     */
    virtual WorkPriority get_priority() const = 0;
    
    /**
     * @brief Get estimated execution time
     * @return Estimated execution time in microseconds
     */
    virtual uint64_t get_estimated_time_us() const = 0;
    
    /**
     * @brief Check if work item can be executed in parallel
     * @return true if parallelizable
     */
    virtual bool is_parallelizable() const = 0;
    
    /**
     * @brief Get work item dependencies
     * @return List of work handles this item depends on
     */
    virtual std::vector<WorkHandle> get_dependencies() const = 0;
    
    /**
     * @brief Get work item name for debugging
     * @return Work item name
     */
    virtual std::string_view get_name() const = 0;
};

/**
 * @brief Individual thread interface
 */
class IThread {
public:
    virtual ~IThread() = default;
    
    /**
     * @brief Start thread execution
     * @param function Function to execute in thread
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> start(ThreadFunction function) = 0;
    
    /**
     * @brief Join with thread (wait for completion)
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> join() = 0;
    
    /**
     * @brief Detach thread (run independently)
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> detach() = 0;
    
    /**
     * @brief Request thread termination
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> terminate() = 0;
    
    /**
     * @brief Suspend thread execution
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> suspend() = 0;
    
    /**
     * @brief Resume thread execution
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> resume() = 0;
    
    /**
     * @brief Set thread priority
     * @param priority New thread priority
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_priority(ThreadPriority priority) = 0;
    
    /**
     * @brief Get current thread state
     * @return Current thread state
     */
    virtual ThreadState get_state() const = 0;
    
    /**
     * @brief Get thread ID
     * @return Thread identifier
     */
    virtual uint32_t get_id() const = 0;
    
    /**
     * @brief Get thread name
     * @return Thread name
     */
    virtual std::string_view get_name() const = 0;
    
    /**
     * @brief Get thread statistics
     * @return Thread performance statistics
     */
    virtual ThreadStats get_stats() const = 0;
    
    /**
     * @brief Set thread completion callback
     * @param callback Callback to invoke when thread completes
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_completion_callback(ThreadCallback callback) = 0;
    
    /**
     * @brief Yield thread execution (cooperative)
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> yield() = 0;
    
    /**
     * @brief Check if thread is joinable
     * @return true if thread can be joined
     */
    virtual bool is_joinable() const = 0;
};

/**
 * @brief Synchronization primitive interface
 */
class ISyncPrimitive {
public:
    virtual ~ISyncPrimitive() = default;
    
    /**
     * @brief Get synchronization primitive type
     * @return Synchronization primitive type
     */
    virtual SyncPrimitiveType get_type() const = 0;
    
    /**
     * @brief Get primitive name
     * @return Primitive name for debugging
     */
    virtual std::string_view get_name() const = 0;
    
    /**
     * @brief Check if primitive is valid
     * @return true if primitive is valid
     */
    virtual bool is_valid() const = 0;
    
    /**
     * @brief Reset primitive to initial state
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> reset() = 0;
};

/**
 * @brief Mutex interface for mutual exclusion
 */
class IMutex : public ISyncPrimitive {
public:
    /**
     * @brief Lock the mutex
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> lock() = 0;
    
    /**
     * @brief Try to lock the mutex without blocking
     * @return HALResult with true if lock acquired, false if not
     */
    virtual HALResult<bool> try_lock() = 0;
    
    /**
     * @brief Try to lock the mutex with timeout
     * @param timeout_us Timeout in microseconds
     * @return HALResult with true if lock acquired, false if timeout
     */
    virtual HALResult<bool> try_lock_for(uint64_t timeout_us) = 0;
    
    /**
     * @brief Unlock the mutex
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> unlock() = 0;
    
    /**
     * @brief Check if mutex is locked
     * @return true if mutex is currently locked
     */
    virtual bool is_locked() const = 0;
    
    /**
     * @brief Get lock owner thread ID
     * @return Thread ID of lock owner, 0 if unlocked
     */
    virtual uint32_t get_owner() const = 0;
};

/**
 * @brief Semaphore interface for resource counting
 */
class ISemaphore : public ISyncPrimitive {
public:
    /**
     * @brief Acquire semaphore (decrement count)
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> acquire() = 0;
    
    /**
     * @brief Try to acquire semaphore without blocking
     * @return HALResult with true if acquired, false if not
     */
    virtual HALResult<bool> try_acquire() = 0;
    
    /**
     * @brief Try to acquire semaphore with timeout
     * @param timeout_us Timeout in microseconds
     * @return HALResult with true if acquired, false if timeout
     */
    virtual HALResult<bool> try_acquire_for(uint64_t timeout_us) = 0;
    
    /**
     * @brief Release semaphore (increment count)
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> release() = 0;
    
    /**
     * @brief Release multiple semaphore counts
     * @param count Number of counts to release
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> release(uint32_t count) = 0;
    
    /**
     * @brief Get current semaphore count
     * @return Current semaphore count
     */
    virtual uint32_t get_count() const = 0;
    
    /**
     * @brief Get maximum semaphore count
     * @return Maximum semaphore count
     */
    virtual uint32_t get_max_count() const = 0;
};

/**
 * @brief Condition variable interface for thread coordination
 */
class IConditionVariable : public ISyncPrimitive {
public:
    /**
     * @brief Wait for condition to be signaled
     * @param mutex Mutex to unlock while waiting
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> wait(IMutex& mutex) = 0;
    
    /**
     * @brief Wait for condition with timeout
     * @param mutex Mutex to unlock while waiting
     * @param timeout_us Timeout in microseconds
     * @return HALResult with true if signaled, false if timeout
     */
    virtual HALResult<bool> wait_for(IMutex& mutex, uint64_t timeout_us) = 0;
    
    /**
     * @brief Signal one waiting thread
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> notify_one() = 0;
    
    /**
     * @brief Signal all waiting threads
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> notify_all() = 0;
    
    /**
     * @brief Get number of waiting threads
     * @return Number of threads waiting on this condition variable
     */
    virtual uint32_t get_waiting_count() const = 0;
};

/**
 * @brief Work queue interface for task scheduling
 */
class IWorkQueue {
public:
    virtual ~IWorkQueue() = default;
    
    /**
     * @brief Submit work item to queue
     * @param work Work item to execute
     * @param priority Work priority level
     * @return HALResult containing work handle
     */
    virtual HALResult<WorkHandle> submit(std::unique_ptr<IWorkItem> work, 
                                        WorkPriority priority = WorkPriority::Normal) = 0;
    
    /**
     * @brief Submit work function to queue
     * @param work_func Function to execute
     * @param priority Work priority level
     * @param name Work name for debugging
     * @return HALResult containing work handle
     */
    virtual HALResult<WorkHandle> submit(WorkFunction work_func, 
                                        WorkPriority priority = WorkPriority::Normal,
                                        std::string_view name = "AnonymousWork") = 0;
    
    /**
     * @brief Submit work with dependencies
     * @param work Work item to execute
     * @param dependencies Work handles this item depends on
     * @param priority Work priority level
     * @return HALResult containing work handle
     */
    virtual HALResult<WorkHandle> submit_with_dependencies(std::unique_ptr<IWorkItem> work,
                                                          const std::vector<WorkHandle>& dependencies,
                                                          WorkPriority priority = WorkPriority::Normal) = 0;
    
    /**
     * @brief Wait for work item completion
     * @param handle Work handle to wait for
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> wait(const WorkHandle& handle) = 0;
    
    /**
     * @brief Wait for multiple work items
     * @param handles Work handles to wait for
     * @param wait_all Whether to wait for all (true) or any (false)
     * @return HALResult with handles of completed work
     */
    virtual HALResult<std::vector<WorkHandle>> wait_multiple(const std::vector<WorkHandle>& handles,
                                                            bool wait_all = true) = 0;
    
    /**
     * @brief Cancel work item
     * @param handle Work handle to cancel
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> cancel(const WorkHandle& handle) = 0;
    
    /**
     * @brief Process pending work (for single-threaded/cooperative mode)
     * @param max_items Maximum number of items to process (0 = all)
     * @return HALResult with number of items processed
     */
    virtual HALResult<uint32_t> process_pending(uint32_t max_items = 0) = 0;
    
    /**
     * @brief Get work queue statistics
     * @return Work queue statistics
     */
    virtual HALResult<struct WorkQueueStats> get_stats() = 0;
    
    /**
     * @brief Set work completion callback
     * @param callback Callback to invoke when work completes
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_completion_callback(WorkCallback callback) = 0;
    
    /**
     * @brief Drain work queue (complete all pending work)
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> drain() = 0;
    
    /**
     * @brief Clear work queue (cancel all pending work)
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> clear() = 0;
    
    /**
     * @brief Get current queue size
     * @return Number of items in queue
     */
    virtual uint32_t get_queue_size() const = 0;
    
    /**
     * @brief Check if work queue is empty
     * @return true if queue is empty
     */
    virtual bool is_empty() const = 0;
    
    /**
     * @brief Get work queue execution mode
     * @return Current execution mode
     */
    virtual WorkQueueMode get_mode() const = 0;
};

/**
 * @brief Thread pool interface for managing worker threads
 */
class IThreadPool {
public:
    virtual ~IThreadPool() = default;
    
    /**
     * @brief Start thread pool with specified number of threads
     * @param thread_count Number of worker threads
     * @param config Thread configuration for workers
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> start(uint32_t thread_count, const ThreadConfig& config) = 0;
    
    /**
     * @brief Stop thread pool
     * @param drain_queue Whether to complete pending work before stopping
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> stop(bool drain_queue = true) = 0;
    
    /**
     * @brief Resize thread pool
     * @param new_thread_count New number of worker threads
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> resize(uint32_t new_thread_count) = 0;
    
    /**
     * @brief Get work queue associated with thread pool
     * @return Work queue interface
     */
    virtual IWorkQueue& get_work_queue() = 0;
    
    /**
     * @brief Get thread pool statistics
     * @return Thread pool statistics
     */
    virtual HALResult<struct ThreadPoolStats> get_stats() = 0;
    
    /**
     * @brief Get current thread count
     * @return Number of worker threads
     */
    virtual uint32_t get_thread_count() const = 0;
    
    /**
     * @brief Check if thread pool is running
     * @return true if thread pool is active
     */
    virtual bool is_running() const = 0;
    
    /**
     * @brief Set thread monitoring callback
     * @param callback Callback for thread monitoring
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_monitor_callback(ThreadMonitorCallback callback) = 0;
};

/**
 * @brief Work queue statistics
 */
struct WorkQueueStats {
    uint64_t total_submitted;       ///< Total work items submitted
    uint64_t total_completed;       ///< Total work items completed
    uint64_t total_cancelled;       ///< Total work items cancelled
    uint64_t total_failed;          ///< Total work items failed
    uint32_t current_queue_size;    ///< Current queue size
    uint32_t max_queue_size;        ///< Maximum queue size reached
    uint64_t total_execution_time_us; ///< Total execution time in microseconds
    uint64_t avg_execution_time_us; ///< Average execution time per item
    double throughput_per_second;   ///< Work items completed per second
    
    /**
     * @brief Reset statistics
     */
    void reset();
    
    /**
     * @brief Get completion rate
     * @return Completion rate as percentage (0.0-1.0)
     */
    double get_completion_rate() const;
};

/**
 * @brief Thread pool statistics
 */
struct ThreadPoolStats {
    uint32_t thread_count;          ///< Current number of threads
    uint32_t active_threads;        ///< Number of threads currently working
    uint32_t idle_threads;          ///< Number of idle threads
    uint64_t total_context_switches; ///< Total context switches across all threads
    uint64_t total_cpu_time_us;     ///< Total CPU time across all threads
    double avg_cpu_utilization;     ///< Average CPU utilization across threads
    WorkQueueStats work_queue_stats; ///< Associated work queue statistics
    
    /**
     * @brief Reset statistics
     */
    void reset();
    
    /**
     * @brief Get thread efficiency
     * @return Efficiency as active threads / total threads
     */
    double get_efficiency() const;
};

/**
 * @brief Enhanced threading interface with platform-adaptive execution
 * 
 * Comprehensive threading system providing graceful degradation from single-core
 * systems to modern multi-core platforms. Supports thread management, synchronization
 * primitives, work queues, and cooperative scheduling with automatic platform
 * detection and capability adaptation.
 */
class IThreadInterface : public IHALInterface {
public:
    virtual ~IThreadInterface() = default;
    
    // === Platform Capabilities ===
    
    /**
     * @brief Get platform threading capabilities
     * @return Threading capabilities for current platform
     */
    virtual const ThreadingCapabilities& get_threading_capabilities() const = 0;
    
    /**
     * @brief Get current threading model
     * @return Active threading model
     */
    virtual ThreadingModel get_threading_model() const = 0;
    
    /**
     * @brief Check if specific feature is supported
     * @param feature Feature to check
     * @return true if feature is supported
     */
    virtual bool is_feature_supported(HALCapability feature) const = 0;
    
    // === Thread Management ===
    
    /**
     * @brief Create a new thread
     * @param config Thread configuration
     * @return HALResult containing thread interface
     */
    virtual HALResult<std::unique_ptr<IThread>> create_thread(const ThreadConfig& config) = 0;
    
    /**
     * @brief Get current thread ID
     * @return Current thread identifier
     */
    virtual uint32_t get_current_thread_id() const = 0;
    
    /**
     * @brief Yield current thread execution
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> yield_current_thread() = 0;
    
    /**
     * @brief Sleep current thread
     * @param microseconds Sleep duration in microseconds
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> sleep_current_thread(uint64_t microseconds) = 0;
    
    /**
     * @brief Get current thread statistics
     * @return Statistics for current thread
     */
    virtual ThreadStats get_current_thread_stats() const = 0;
    
    // === Synchronization Primitives ===
    
    /**
     * @brief Create a mutex
     * @param name Mutex name for debugging
     * @param recursive Whether mutex should be recursive
     * @return HALResult containing mutex interface
     */
    virtual HALResult<std::unique_ptr<IMutex>> create_mutex(std::string_view name = "Mutex",
                                                           bool recursive = false) = 0;
    
    /**
     * @brief Create a semaphore
     * @param initial_count Initial semaphore count
     * @param max_count Maximum semaphore count
     * @param name Semaphore name for debugging
     * @return HALResult containing semaphore interface
     */
    virtual HALResult<std::unique_ptr<ISemaphore>> create_semaphore(uint32_t initial_count,
                                                                   uint32_t max_count,
                                                                   std::string_view name = "Semaphore") = 0;
    
    /**
     * @brief Create a condition variable
     * @param name Condition variable name for debugging
     * @return HALResult containing condition variable interface
     */
    virtual HALResult<std::unique_ptr<IConditionVariable>> create_condition_variable(std::string_view name = "ConditionVariable") = 0;
    
    // === Work Queue Management ===
    
    /**
     * @brief Create a work queue
     * @param mode Work queue execution mode
     * @param max_items Maximum number of items in queue (0 = unlimited)
     * @param name Work queue name for debugging
     * @return HALResult containing work queue interface
     */
    virtual HALResult<std::unique_ptr<IWorkQueue>> create_work_queue(WorkQueueMode mode = WorkQueueMode::Adaptive,
                                                                    uint32_t max_items = 0,
                                                                    std::string_view name = "WorkQueue") = 0;
    
    /**
     * @brief Create a thread pool
     * @param thread_count Number of worker threads (0 = auto-detect)
     * @param config Thread configuration for workers
     * @param name Thread pool name for debugging
     * @return HALResult containing thread pool interface
     */
    virtual HALResult<std::unique_ptr<IThreadPool>> create_thread_pool(uint32_t thread_count = 0,
                                                                      const ThreadConfig& config = ThreadConfig::create_default(),
                                                                      std::string_view name = "ThreadPool") = 0;
    
    /**
     * @brief Get global work queue (shared across application)
     * @return Global work queue interface
     */
    virtual IWorkQueue& get_global_work_queue() = 0;
    
    // === Cooperative Scheduling Support ===
    
    /**
     * @brief Register cooperative scheduler tick
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> cooperative_tick() = 0;
    
    /**
     * @brief Set cooperative scheduler quantum
     * @param quantum_us Time quantum in microseconds
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_cooperative_quantum(uint64_t quantum_us) = 0;
    
    /**
     * @brief Get cooperative scheduler quantum
     * @return Current time quantum in microseconds
     */
    virtual uint64_t get_cooperative_quantum() const = 0;
    
    // === Utility Functions ===
    
    /**
     * @brief Get optimal thread count for parallel work
     * @return Recommended number of threads
     */
    virtual uint32_t get_optimal_thread_count() const = 0;
    
    /**
     * @brief Get system thread count
     * @return Number of threads supported by system
     */
    virtual uint32_t get_system_thread_count() const = 0;
    
    /**
     * @brief Check if cooperative scheduling is active
     * @return true if using cooperative scheduling
     */
    virtual bool is_cooperative_scheduling() const = 0;
    
    /**
     * @brief Enable or disable cooperative scheduling
     * @param enabled Whether to enable cooperative scheduling
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_cooperative_scheduling(bool enabled) = 0;
    
    // === IHALInterface implementation ===
    std::string_view get_interface_name() const override {
        return "thread";
    }
    
    /**
     * @brief Get driver name (implemented by concrete drivers)
     * @return Driver implementation name
     */
    virtual std::string_view get_driver_name() const = 0;
    
    /**
     * @brief Initialize the threading interface
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> initialize() = 0;
    
    /**
     * @brief Shutdown the threading interface
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> shutdown() = 0;
    
    /**
     * @brief Check if interface is active
     * @return true if initialized and ready
     */
    virtual bool is_active() const = 0;
    
    /**
     * @brief Check if interface is available on this platform
     * @return true if available
     */
    virtual bool is_available() const = 0;
    
    /**
     * @brief Get supported HAL capabilities
     * @return Vector of supported capabilities
     */
    std::vector<HALCapability> get_capabilities() const override {
        std::vector<HALCapability> caps;
        const auto& thread_caps = get_threading_capabilities();
        
        if (thread_caps.supports_atomic_operations) {
            caps.push_back(HALCapability::AtomicOperations);
        }
        if (thread_caps.model != ThreadingModel::SingleThreaded) {
            caps.push_back(HALCapability::Threading);
        }
        
        return caps;
    }
};

// === Utility Functions ===

/**
 * @brief Threading utility functions
 */
namespace thread {

/**
 * @brief Convert threading model to string
 * @param model Threading model to convert
 * @return String representation
 */
const char* to_string(ThreadingModel model);

/**
 * @brief Convert thread state to string
 * @param state Thread state to convert
 * @return String representation
 */
const char* to_string(ThreadState state);

/**
 * @brief Convert thread priority to string
 * @param priority Thread priority to convert
 * @return String representation
 */
const char* to_string(ThreadPriority priority);

/**
 * @brief Convert work priority to string
 * @param priority Work priority to convert
 * @return String representation
 */
const char* to_string(WorkPriority priority);

/**
 * @brief Convert synchronization primitive type to string
 * @param type Sync primitive type to convert
 * @return String representation
 */
const char* to_string(SyncPrimitiveType type);

/**
 * @brief Convert work queue mode to string
 * @param mode Work queue mode to convert
 * @return String representation
 */
const char* to_string(WorkQueueMode mode);

/**
 * @brief Get optimal thread count based on platform
 * @param threading_model Current threading model
 * @param available_cores Number of available CPU cores
 * @return Recommended thread count
 */
uint32_t calculate_optimal_thread_count(ThreadingModel threading_model, uint32_t available_cores);

/**
 * @brief Check if threading model supports specific feature
 * @param model Threading model to check
 * @param primitive_type Synchronization primitive type
 * @return true if supported
 */
bool threading_model_supports(ThreadingModel model, SyncPrimitiveType primitive_type);

/**
 * @brief Create default thread configuration for platform
 * @param model Threading model
 * @param name Thread name
 * @return Platform-appropriate thread configuration
 */
ThreadConfig create_platform_thread_config(ThreadingModel model, std::string_view name);

/**
 * @brief Create work handle
 * @param id Work item identifier
 * @param priority Work priority
 * @return Valid work handle
 */
WorkHandle create_work_handle(uint64_t id, WorkPriority priority);

/**
 * @brief Calculate thread overhead for platform
 * @param model Threading model
 * @param stack_size Thread stack size
 * @return Memory overhead in bytes
 */
size_t calculate_thread_overhead(ThreadingModel model, size_t stack_size);

} // namespace thread

} // namespace flight::hal
