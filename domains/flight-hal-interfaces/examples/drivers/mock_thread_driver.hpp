/**
 * @file mock_thread_driver.hpp
 * @brief Mock Threading Driver for Flight HAL Testing
 * 
 * Platform-adaptive mock implementation of the threading interface supporting
 * all threading models from single-threaded Dreamcast simulation to full
 * preemptive multithreading. Provides realistic behavior simulation for
 * comprehensive testing and development.
 */

#pragma once

#include "../../include/flight/hal/interfaces/thread.hpp"
#include "../../include/flight/hal/core/platform_detection.hpp"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <queue>
#include <unordered_map>
#include <chrono>

namespace flight::hal::examples {

/**
 * @brief Mock work item implementation for testing
 */
class MockWorkItem : public IWorkItem {
private:
    WorkFunction work_func_;
    WorkPriority priority_;
    uint64_t estimated_time_us_;
    std::vector<WorkHandle> dependencies_;
    std::string name_;
    bool parallelizable_;

public:
    MockWorkItem(WorkFunction func, WorkPriority priority, std::string_view name,
                uint64_t estimated_time = 1000, bool parallel = true)
        : work_func_(std::move(func)), priority_(priority), 
          estimated_time_us_(estimated_time), name_(name), parallelizable_(parallel) {}

    HALResult<void> execute() override {
        if (work_func_) {
            work_func_();
            return HALResult<void>::success();
        }
        return HALResult<void>::error(errors::invalid_parameter(1, "No work function provided"));
    }

    WorkPriority get_priority() const override { return priority_; }
    uint64_t get_estimated_time_us() const override { return estimated_time_us_; }
    bool is_parallelizable() const override { return parallelizable_; }
    std::vector<WorkHandle> get_dependencies() const override { return dependencies_; }
    std::string_view get_name() const override { return name_; }

    void add_dependency(const WorkHandle& handle) {
        dependencies_.push_back(handle);
    }
};

/**
 * @brief Mock thread implementation
 */
class MockThread : public IThread {
private:
    uint32_t id_;
    std::string name_;
    ThreadPriority priority_;
    ThreadState state_;
    std::unique_ptr<std::thread> thread_;
    ThreadCallback completion_callback_;
    ThreadStats stats_;
    bool detached_;
    mutable std::mutex state_mutex_;

public:
    MockThread(uint32_t id, const ThreadConfig& config)
        : id_(id), name_(config.name), priority_(config.priority),
          state_(ThreadState::Created), detached_(config.detached) {
        stats_.reset();
    }

    ~MockThread() {
        if (thread_ && thread_->joinable()) {
            thread_->join();
        }
    }

    HALResult<void> start(ThreadFunction function) override {
        std::lock_guard<std::mutex> lock(state_mutex_);
        
        if (state_ != ThreadState::Created) {
            return HALResult<void>::error(errors::invalid_state(1, "Thread already started"));
        }

        try {
            thread_ = std::make_unique<std::thread>([this, function]() {
                {
                    std::lock_guard<std::mutex> lock(state_mutex_);
                    state_ = ThreadState::Running;
                }
                
                auto start_time = std::chrono::high_resolution_clock::now();
                
                try {
                    function();
                    
                    std::lock_guard<std::mutex> lock(state_mutex_);
                    state_ = ThreadState::Terminated;
                } catch (...) {
                    std::lock_guard<std::mutex> lock(state_mutex_);
                    state_ = ThreadState::Error;
                }
                
                auto end_time = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
                
                {
                    std::lock_guard<std::mutex> lock(state_mutex_);
                    stats_.wall_time_us = duration.count();
                    stats_.cpu_time_us = duration.count();  // Simplified for mock
                    stats_.cpu_utilization = 1.0;  // Assume full utilization for mock
                }
                
                if (completion_callback_) {
                    completion_callback_(id_, state_);
                }
            });
            
            if (detached_) {
                thread_->detach();
            }
            
            return HALResult<void>::success();
        } catch (const std::exception& e) {
            state_ = ThreadState::Error;
            return HALResult<void>::error(HALErrorCode::SystemError, e.what());
        }
    }

    HALResult<void> join() override {
        std::lock_guard<std::mutex> lock(state_mutex_);
        
        if (!thread_ || !thread_->joinable()) {
            return HALResult<void>::error(HALErrorCode::InvalidState, "Thread not joinable");
        }
        
        try {
            thread_->join();
            return HALResult<void>::success();
        } catch (const std::exception& e) {
            return HALResult<void>::error(HALErrorCode::SystemError, e.what());
        }
    }

    HALResult<void> detach() override {
        std::lock_guard<std::mutex> lock(state_mutex_);
        
        if (!thread_ || !thread_->joinable()) {
            return HALResult<void>::error(HALErrorCode::InvalidState, "Thread not detachable");
        }
        
        thread_->detach();
        detached_ = true;
        return HALResult<void>::success();
    }

    HALResult<void> terminate() override {
        std::lock_guard<std::mutex> lock(state_mutex_);
        state_ = ThreadState::Terminated;
        // Note: std::thread doesn't support forced termination
        return HALResult<void>::success();
    }

    HALResult<void> suspend() override {
        std::lock_guard<std::mutex> lock(state_mutex_);
        
        if (state_ == ThreadState::Running) {
            state_ = ThreadState::Suspended;
            return HALResult<void>::success();
        }
        
        return HALResult<void>::error(HALErrorCode::InvalidState, "Thread not running");
    }

    HALResult<void> resume() override {
        std::lock_guard<std::mutex> lock(state_mutex_);
        
        if (state_ == ThreadState::Suspended) {
            state_ = ThreadState::Running;
            return HALResult<void>::success();
        }
        
        return HALResult<void>::error(HALErrorCode::InvalidState, "Thread not suspended");
    }

    HALResult<void> set_priority(ThreadPriority priority) override {
        std::lock_guard<std::mutex> lock(state_mutex_);
        priority_ = priority;
        stats_.priority_changes++;
        return HALResult<void>::success();
    }

    ThreadState get_state() const override {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return state_;
    }

    uint32_t get_id() const override { return id_; }
    std::string_view get_name() const override { return name_; }
    ThreadStats get_stats() const override {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return stats_;
    }

    HALResult<void> set_completion_callback(ThreadCallback callback) override {
        std::lock_guard<std::mutex> lock(state_mutex_);
        completion_callback_ = std::move(callback);
        return HALResult<void>::success();
    }

    HALResult<void> yield() override {
        std::this_thread::yield();
        return HALResult<void>::success();
    }

    bool is_joinable() const override {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return thread_ && thread_->joinable();
    }
};

/**
 * @brief Mock mutex implementation
 */
class MockMutex : public IMutex {
private:
    std::string name_;
    mutable std::mutex mutex_;
    std::atomic<bool> locked_{false};
    std::atomic<uint32_t> owner_{0};

public:
    explicit MockMutex(std::string_view name) : name_(name) {}

    SyncPrimitiveType get_type() const override { return SyncPrimitiveType::Mutex; }
    std::string_view get_name() const override { return name_; }
    bool is_valid() const override { return true; }

    HALResult<void> reset() override {
        if (locked_.load()) {
            return HALResult<void>::error(HALErrorCode::InvalidState, "Cannot reset locked mutex");
        }
        return HALResult<void>::success();
    }

    HALResult<void> lock() override {
        try {
            mutex_.lock();
            locked_.store(true);
            // Note: In a real implementation, we'd track the actual thread ID
            owner_.store(1);  // Simplified for mock
            return HALResult<void>::success();
        } catch (const std::exception& e) {
            return HALResult<void>::error(HALErrorCode::SystemError, e.what());
        }
    }

    HALResult<bool> try_lock() override {
        bool acquired = mutex_.try_lock();
        if (acquired) {
            locked_.store(true);
            owner_.store(1);
        }
        return HALResult<bool>::success(acquired);
    }

    HALResult<bool> try_lock_for(uint64_t timeout_us) override {
        auto timeout = std::chrono::microseconds(timeout_us);
        bool acquired = mutex_.try_lock_for(timeout);
        if (acquired) {
            locked_.store(true);
            owner_.store(1);
        }
        return HALResult<bool>::success(acquired);
    }

    HALResult<void> unlock() override {
        try {
            locked_.store(false);
            owner_.store(0);
            mutex_.unlock();
            return HALResult<void>::success();
        } catch (const std::exception& e) {
            return HALResult<void>::error(HALErrorCode::SystemError, e.what());
        }
    }

    bool is_locked() const override {
        return locked_.load();
    }

    uint32_t get_owner() const override {
        return owner_.load();
    }
};

/**
 * @brief Mock work queue implementation with platform-adaptive behavior
 */
class MockWorkQueue : public IWorkQueue {
private:
    WorkQueueMode mode_;
    std::string name_;
    uint32_t max_items_;
    std::queue<std::pair<std::unique_ptr<IWorkItem>, WorkHandle>> work_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::atomic<uint64_t> next_work_id_{1};
    WorkQueueStats stats_;
    WorkCallback completion_callback_;
    std::atomic<bool> running_{true};

public:
    MockWorkQueue(WorkQueueMode mode, uint32_t max_items, std::string_view name)
        : mode_(mode), name_(name), max_items_(max_items) {
        stats_.reset();
    }

    HALResult<WorkHandle> submit(std::unique_ptr<IWorkItem> work, WorkPriority priority) override {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        
        if (max_items_ > 0 && work_queue_.size() >= max_items_) {
            return HALResult<WorkHandle>::error(HALErrorCode::ResourceExhausted, "Work queue full");
        }
        
        WorkHandle handle = thread::create_work_handle(next_work_id_++, priority);
        
        if (mode_ == WorkQueueMode::Immediate) {
            // Execute immediately in calling thread
            auto start_time = std::chrono::high_resolution_clock::now();
            auto result = work->execute();
            auto end_time = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
            stats_.total_submitted++;
            stats_.total_execution_time_us += duration.count();
            
            if (result.is_success()) {
                stats_.total_completed++;
            } else {
                stats_.total_failed++;
            }
            
            if (completion_callback_) {
                completion_callback_(handle, result.is_success());
            }
        } else {
            // Queue for later execution
            work_queue_.emplace(std::move(work), handle);
            stats_.total_submitted++;
            stats_.current_queue_size = work_queue_.size();
            stats_.max_queue_size = std::max(stats_.max_queue_size, stats_.current_queue_size);
            queue_cv_.notify_one();
        }
        
        return HALResult<WorkHandle>::success(handle);
    }

    HALResult<WorkHandle> submit(WorkFunction work_func, WorkPriority priority, std::string_view name) override {
        auto work_item = std::make_unique<MockWorkItem>(std::move(work_func), priority, name);
        return submit(std::move(work_item), priority);
    }

    HALResult<WorkHandle> submit_with_dependencies(std::unique_ptr<IWorkItem> work,
                                                  const std::vector<WorkHandle>& dependencies,
                                                  WorkPriority priority) override {
        // Simplified - ignore dependencies for mock
        return submit(std::move(work), priority);
    }

    HALResult<void> wait(const WorkHandle& handle) override {
        // Simplified wait implementation
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return HALResult<void>::success();
    }

    HALResult<std::vector<WorkHandle>> wait_multiple(const std::vector<WorkHandle>& handles, bool wait_all) override {
        // Simplified implementation
        return HALResult<std::vector<WorkHandle>>::success(handles);
    }

    HALResult<void> cancel(const WorkHandle& handle) override {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        stats_.total_cancelled++;
        return HALResult<void>::success();
    }

    HALResult<uint32_t> process_pending(uint32_t max_items) override {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        
        uint32_t processed = 0;
        uint32_t to_process = (max_items == 0) ? work_queue_.size() : std::min(max_items, static_cast<uint32_t>(work_queue_.size()));
        
        while (processed < to_process && !work_queue_.empty()) {
            auto [work, handle] = std::move(work_queue_.front());
            work_queue_.pop();
            
            auto start_time = std::chrono::high_resolution_clock::now();
            auto result = work->execute();
            auto end_time = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
            stats_.total_execution_time_us += duration.count();
            
            if (result.is_success()) {
                stats_.total_completed++;
            } else {
                stats_.total_failed++;
            }
            
            processed++;
            
            if (completion_callback_) {
                completion_callback_(handle, result.is_success());
            }
        }
        
        stats_.current_queue_size = work_queue_.size();
        return HALResult<uint32_t>::success(processed);
    }

    HALResult<WorkQueueStats> get_stats() override {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        
        if (stats_.total_completed > 0) {
            stats_.avg_execution_time_us = stats_.total_execution_time_us / stats_.total_completed;
        }
        
        return HALResult<WorkQueueStats>::success(stats_);
    }

    HALResult<void> set_completion_callback(WorkCallback callback) override {
        completion_callback_ = std::move(callback);
        return HALResult<void>::success();
    }

    HALResult<void> drain() override {
        return process_pending(0).map([](uint32_t) {});
    }

    HALResult<void> clear() override {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        
        size_t cleared = work_queue_.size();
        while (!work_queue_.empty()) {
            work_queue_.pop();
        }
        
        stats_.total_cancelled += cleared;
        stats_.current_queue_size = 0;
        
        return HALResult<void>::success();
    }

    uint32_t get_queue_size() const override {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return work_queue_.size();
    }

    bool is_empty() const override {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return work_queue_.empty();
    }

    WorkQueueMode get_mode() const override { return mode_; }
};

/**
 * @brief Mock threading interface with platform-adaptive behavior
 */
class MockThreadInterface : public IThreadInterface {
private:
    ThreadingCapabilities capabilities_;
    ThreadingModel threading_model_;
    std::atomic<uint32_t> next_thread_id_{1};
    std::unique_ptr<MockWorkQueue> global_work_queue_;
    mutable std::mutex interface_mutex_;
    bool active_ = false;
    uint64_t cooperative_quantum_us_ = 10000;  // 10ms default

    void init_capabilities() {
        // Detect platform and set capabilities accordingly
        if constexpr (FLIGHT_PLATFORM_DREAMCAST) {
            threading_model_ = ThreadingModel::SingleThreaded;
            capabilities_.max_threads = 1;
            capabilities_.supports_priorities = false;
            capabilities_.supports_affinity = false;
            capabilities_.supports_realtime = false;
            capabilities_.supports_detached_threads = false;
            capabilities_.supports_thread_local = false;
            capabilities_.supports_atomic_operations = false;
        } else if constexpr (FLIGHT_PLATFORM_PSP) {
            threading_model_ = ThreadingModel::Cooperative;
            capabilities_.max_threads = 4;
            capabilities_.supports_priorities = true;
            capabilities_.supports_affinity = false;
            capabilities_.supports_realtime = false;
            capabilities_.supports_detached_threads = true;
            capabilities_.supports_thread_local = false;
            capabilities_.supports_atomic_operations = true;
        } else if constexpr (FLIGHT_PLATFORM_WEB) {
            threading_model_ = ThreadingModel::WebWorkers;
            capabilities_.max_threads = 8;
            capabilities_.supports_priorities = false;
            capabilities_.supports_affinity = false;
            capabilities_.supports_realtime = false;
            capabilities_.supports_detached_threads = true;
            capabilities_.supports_thread_local = false;
            capabilities_.supports_atomic_operations = true;
        } else {
            // Desktop platforms
            threading_model_ = ThreadingModel::Preemptive;
            capabilities_.max_threads = std::thread::hardware_concurrency();
            capabilities_.supports_priorities = true;
            capabilities_.supports_affinity = true;
            capabilities_.supports_realtime = true;
            capabilities_.supports_detached_threads = true;
            capabilities_.supports_thread_local = true;
            capabilities_.supports_atomic_operations = true;
        }
        
        capabilities_.model = threading_model_;
        capabilities_.max_work_items = 10000;
        capabilities_.context_switch_overhead_ns = 1000;
        capabilities_.sync_overhead_ns = 100;
        capabilities_.default_stack_size = 1024 * 1024;  // 1MB
        capabilities_.min_stack_size = 64 * 1024;        // 64KB
        capabilities_.max_stack_size = 8 * 1024 * 1024;  // 8MB
    }

public:
    MockThreadInterface() {
        init_capabilities();
        
        // Create global work queue with adaptive mode
        global_work_queue_ = std::make_unique<MockWorkQueue>(
            WorkQueueMode::Adaptive, 0, "GlobalWorkQueue");
    }

    // === Platform Capabilities ===
    const ThreadingCapabilities& get_threading_capabilities() const override {
        return capabilities_;
    }

    ThreadingModel get_threading_model() const override {
        return threading_model_;
    }

    bool is_feature_supported(HALCapability feature) const override {
        switch (feature) {
            case HALCapability::Threading:
                return threading_model_ != ThreadingModel::SingleThreaded;
            case HALCapability::AtomicOperations:
                return capabilities_.supports_atomic_operations;
            default:
                return false;
        }
    }

    // === Thread Management ===
    HALResult<std::unique_ptr<IThread>> create_thread(const ThreadConfig& config) override {
        if (threading_model_ == ThreadingModel::SingleThreaded) {
            return HALResult<std::unique_ptr<IThread>>::error(
                HALErrorCode::NotSupported, "Threading not supported on this platform");
        }

        uint32_t thread_id = next_thread_id_++;
        auto thread = std::make_unique<MockThread>(thread_id, config);
        
        return HALResult<std::unique_ptr<IThread>>::success(std::move(thread));
    }

    uint32_t get_current_thread_id() const override {
        return 1;  // Simplified for mock
    }

    HALResult<void> yield_current_thread() override {
        if (threading_model_ != ThreadingModel::SingleThreaded) {
            std::this_thread::yield();
        }
        return HALResult<void>::success();
    }

    HALResult<void> sleep_current_thread(uint64_t microseconds) override {
        std::this_thread::sleep_for(std::chrono::microseconds(microseconds));
        return HALResult<void>::success();
    }

    ThreadStats get_current_thread_stats() const override {
        ThreadStats stats;
        stats.reset();
        return stats;
    }

    // === Synchronization Primitives ===
    HALResult<std::unique_ptr<IMutex>> create_mutex(std::string_view name, bool recursive) override {
        if (threading_model_ == ThreadingModel::SingleThreaded) {
            return HALResult<std::unique_ptr<IMutex>>::error(
                HALErrorCode::NotSupported, "Synchronization not needed in single-threaded mode");
        }

        auto mutex = std::make_unique<MockMutex>(name);
        return HALResult<std::unique_ptr<IMutex>>::success(std::move(mutex));
    }

    HALResult<std::unique_ptr<ISemaphore>> create_semaphore(uint32_t initial_count, uint32_t max_count, std::string_view name) override {
        return HALResult<std::unique_ptr<ISemaphore>>::error(
            HALErrorCode::NotImplemented, "Semaphore not implemented in mock");
    }

    HALResult<std::unique_ptr<IConditionVariable>> create_condition_variable(std::string_view name) override {
        return HALResult<std::unique_ptr<IConditionVariable>>::error(
            HALErrorCode::NotImplemented, "Condition variable not implemented in mock");
    }

    // === Work Queue Management ===
    HALResult<std::unique_ptr<IWorkQueue>> create_work_queue(WorkQueueMode mode, uint32_t max_items, std::string_view name) override {
        // Adapt mode based on platform
        WorkQueueMode actual_mode = mode;
        if (mode == WorkQueueMode::Adaptive) {
            switch (threading_model_) {
                case ThreadingModel::SingleThreaded:
                    actual_mode = WorkQueueMode::Immediate;
                    break;
                case ThreadingModel::Cooperative:
                    actual_mode = WorkQueueMode::Deferred;
                    break;
                case ThreadingModel::WebWorkers:
                    actual_mode = WorkQueueMode::Sequential;
                    break;
                case ThreadingModel::Preemptive:
                    actual_mode = WorkQueueMode::Parallel;
                    break;
            }
        }

        auto queue = std::make_unique<MockWorkQueue>(actual_mode, max_items, name);
        return HALResult<std::unique_ptr<IWorkQueue>>::success(std::move(queue));
    }

    HALResult<std::unique_ptr<IThreadPool>> create_thread_pool(uint32_t thread_count, const ThreadConfig& config, std::string_view name) override {
        return HALResult<std::unique_ptr<IThreadPool>>::error(
            HALErrorCode::NotImplemented, "Thread pool not implemented in mock");
    }

    IWorkQueue& get_global_work_queue() override {
        return *global_work_queue_;
    }

    // === Cooperative Scheduling Support ===
    HALResult<void> cooperative_tick() override {
        if (threading_model_ == ThreadingModel::Cooperative) {
            std::this_thread::yield();
        }
        return HALResult<void>::success();
    }

    HALResult<void> set_cooperative_quantum(uint64_t quantum_us) override {
        cooperative_quantum_us_ = quantum_us;
        return HALResult<void>::success();
    }

    uint64_t get_cooperative_quantum() const override {
        return cooperative_quantum_us_;
    }

    // === Utility Functions ===
    uint32_t get_optimal_thread_count() const override {
        return capabilities_.get_recommended_thread_count();
    }

    uint32_t get_system_thread_count() const override {
        return capabilities_.max_threads;
    }

    bool is_cooperative_scheduling() const override {
        return threading_model_ == ThreadingModel::Cooperative;
    }

    HALResult<void> set_cooperative_scheduling(bool enabled) override {
        // Mock implementation - would need platform-specific code in real implementation
        return HALResult<void>::success();
    }

    // === IHALInterface implementation ===
    std::string_view get_driver_name() const override {
        return "MockThreadDriver";
    }

    HALResult<void> initialize() override {
        std::lock_guard<std::mutex> lock(interface_mutex_);
        active_ = true;
        return HALResult<void>::success();
    }

    HALResult<void> shutdown() override {
        std::lock_guard<std::mutex> lock(interface_mutex_);
        active_ = false;
        return HALResult<void>::success();
    }

    bool is_active() const override {
        std::lock_guard<std::mutex> lock(interface_mutex_);
        return active_;
    }

    bool is_available() const override {
        return true;  // Mock is always available
    }
};

} // namespace flight::hal::examples
