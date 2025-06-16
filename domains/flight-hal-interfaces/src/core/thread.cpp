/**
 * @file thread.cpp
 * @brief Flight HAL Threading Interface Implementation
 * 
 * Core implementation of threading interface functionality including
 * utility functions, platform-specific adaptations, and common
 * threading operations.
 */

#include "../../include/flight/hal/interfaces/thread.hpp"
#include "../../include/flight/hal/core/platform_detection.hpp"
#include <algorithm>
#include <cstring>

namespace flight::hal {

// ============================================================================
// ThreadStats Implementation
// ============================================================================

void ThreadStats::reset() {
    context_switches = 0;
    cpu_time_us = 0;
    wall_time_us = 0;
    memory_usage_bytes = 0;
    cpu_utilization = 0.0;
    priority_changes = 0;
    blocking_operations = 0;
}

double ThreadStats::get_efficiency() const {
    if (wall_time_us == 0) return 0.0;
    return static_cast<double>(cpu_time_us) / static_cast<double>(wall_time_us);
}

// ============================================================================
// ThreadingCapabilities Implementation
// ============================================================================

bool ThreadingCapabilities::supports_sync_primitive(SyncPrimitiveType type) const {
    // Single-threaded model doesn't support any synchronization primitives
    if (model == ThreadingModel::SingleThreaded) {
        return false;
    }
    
    // Web Workers have limited synchronization support
    if (model == ThreadingModel::WebWorkers) {
        switch (type) {
            case SyncPrimitiveType::Mutex:
            case SyncPrimitiveType::BinarySemaphore:
                return true;
            default:
                return false;
        }
    }
    
    // Cooperative and Preemptive models support most primitives
    switch (type) {
        case SyncPrimitiveType::Mutex:
        case SyncPrimitiveType::RecursiveMutex:
        case SyncPrimitiveType::Semaphore:
        case SyncPrimitiveType::BinarySemaphore:
        case SyncPrimitiveType::ConditionVariable:
            return true;
        case SyncPrimitiveType::Event:
        case SyncPrimitiveType::Barrier:
        case SyncPrimitiveType::ReadWriteLock:
            return model == ThreadingModel::Preemptive;
        default:
            return false;
    }
}

uint32_t ThreadingCapabilities::get_recommended_thread_count() const {
    switch (model) {
        case ThreadingModel::SingleThreaded:
            return 1;
        case ThreadingModel::Cooperative:
            return std::min(max_threads, 4u);  // PSP-like systems
        case ThreadingModel::WebWorkers:
            return std::min(max_threads, 8u);  // Web platform reasonable limit
        case ThreadingModel::Preemptive:
            return std::min(max_threads, 
                           std::max(2u, max_threads / 2));  // Half of available cores
        default:
            return 1;
    }
}

// ============================================================================
// ThreadConfig Implementation
// ============================================================================

ThreadConfig ThreadConfig::create_default(std::string_view thread_name) {
    ThreadConfig config;
    config.name = thread_name;
    config.stack_size = 0;  // Use system default
    config.priority = ThreadPriority::Normal;
    config.detached = false;
    config.processor_affinity = 0;  // Any processor
    return config;
}

ThreadConfig ThreadConfig::create_high_priority(std::string_view thread_name) {
    ThreadConfig config = create_default(thread_name);
    config.priority = ThreadPriority::High;
    return config;
}

ThreadConfig ThreadConfig::create_realtime(std::string_view thread_name) {
    ThreadConfig config = create_default(thread_name);
    config.priority = ThreadPriority::Realtime;
    config.stack_size = 1024 * 1024;  // 1MB stack for real-time threads
    return config;
}

// ============================================================================
// WorkQueueStats Implementation
// ============================================================================

void WorkQueueStats::reset() {
    total_submitted = 0;
    total_completed = 0;
    total_cancelled = 0;
    total_failed = 0;
    current_queue_size = 0;
    max_queue_size = 0;
    total_execution_time_us = 0;
    avg_execution_time_us = 0;
    throughput_per_second = 0.0;
}

double WorkQueueStats::get_completion_rate() const {
    if (total_submitted == 0) return 1.0;
    return static_cast<double>(total_completed) / static_cast<double>(total_submitted);
}

// ============================================================================
// ThreadPoolStats Implementation
// ============================================================================

void ThreadPoolStats::reset() {
    thread_count = 0;
    active_threads = 0;
    idle_threads = 0;
    total_context_switches = 0;
    total_cpu_time_us = 0;
    avg_cpu_utilization = 0.0;
    work_queue_stats.reset();
}

double ThreadPoolStats::get_efficiency() const {
    if (thread_count == 0) return 0.0;
    return static_cast<double>(active_threads) / static_cast<double>(thread_count);
}

// ============================================================================
// Utility Functions Implementation
// ============================================================================

namespace thread {

const char* to_string(ThreadingModel model) {
    switch (model) {
        case ThreadingModel::SingleThreaded: return "SingleThreaded";
        case ThreadingModel::Cooperative: return "Cooperative";
        case ThreadingModel::WebWorkers: return "WebWorkers";
        case ThreadingModel::Preemptive: return "Preemptive";
        default: return "Unknown";
    }
}

const char* to_string(ThreadState state) {
    switch (state) {
        case ThreadState::Created: return "Created";
        case ThreadState::Running: return "Running";
        case ThreadState::Suspended: return "Suspended";
        case ThreadState::Blocked: return "Blocked";
        case ThreadState::Terminated: return "Terminated";
        case ThreadState::Error: return "Error";
        default: return "Unknown";
    }
}

const char* to_string(ThreadPriority priority) {
    switch (priority) {
        case ThreadPriority::Idle: return "Idle";
        case ThreadPriority::Low: return "Low";
        case ThreadPriority::Normal: return "Normal";
        case ThreadPriority::High: return "High";
        case ThreadPriority::Critical: return "Critical";
        case ThreadPriority::Realtime: return "Realtime";
        default: return "Unknown";
    }
}

const char* to_string(WorkPriority priority) {
    switch (priority) {
        case WorkPriority::Deferred: return "Deferred";
        case WorkPriority::Background: return "Background";
        case WorkPriority::Normal: return "Normal";
        case WorkPriority::High: return "High";
        case WorkPriority::Urgent: return "Urgent";
        case WorkPriority::Immediate: return "Immediate";
        default: return "Unknown";
    }
}

const char* to_string(SyncPrimitiveType type) {
    switch (type) {
        case SyncPrimitiveType::Mutex: return "Mutex";
        case SyncPrimitiveType::RecursiveMutex: return "RecursiveMutex";
        case SyncPrimitiveType::Semaphore: return "Semaphore";
        case SyncPrimitiveType::BinarySemaphore: return "BinarySemaphore";
        case SyncPrimitiveType::ConditionVariable: return "ConditionVariable";
        case SyncPrimitiveType::Event: return "Event";
        case SyncPrimitiveType::Barrier: return "Barrier";
        case SyncPrimitiveType::ReadWriteLock: return "ReadWriteLock";
        default: return "Unknown";
    }
}

const char* to_string(WorkQueueMode mode) {
    switch (mode) {
        case WorkQueueMode::Immediate: return "Immediate";
        case WorkQueueMode::Deferred: return "Deferred";
        case WorkQueueMode::Parallel: return "Parallel";
        case WorkQueueMode::Sequential: return "Sequential";
        case WorkQueueMode::Adaptive: return "Adaptive";
        default: return "Unknown";
    }
}

uint32_t calculate_optimal_thread_count(ThreadingModel threading_model, uint32_t available_cores) {
    switch (threading_model) {
        case ThreadingModel::SingleThreaded:
            return 1;
            
        case ThreadingModel::Cooperative:
            // Cooperative threading benefits from limited threads to avoid overhead
            return std::min(available_cores, 4u);
            
        case ThreadingModel::WebWorkers:
            // Web Workers have overhead, so use fewer threads
            return std::min(available_cores, 8u);
            
        case ThreadingModel::Preemptive:
            // Preemptive threading can use more threads effectively
            if (available_cores <= 2) return available_cores;
            if (available_cores <= 4) return available_cores;
            if (available_cores <= 8) return available_cores - 1;  // Leave one for system
            return available_cores - 2;  // Leave some headroom for high core counts
            
        default:
            return 1;
    }
}

bool threading_model_supports(ThreadingModel model, SyncPrimitiveType primitive_type) {
    switch (model) {
        case ThreadingModel::SingleThreaded:
            return false;  // No synchronization needed in single-threaded
            
        case ThreadingModel::Cooperative:
            // Cooperative threading supports basic synchronization
            switch (primitive_type) {
                case SyncPrimitiveType::Mutex:
                case SyncPrimitiveType::RecursiveMutex:
                case SyncPrimitiveType::Semaphore:
                case SyncPrimitiveType::BinarySemaphore:
                case SyncPrimitiveType::ConditionVariable:
                    return true;
                default:
                    return false;
            }
            
        case ThreadingModel::WebWorkers:
            // Web Workers have limited synchronization
            switch (primitive_type) {
                case SyncPrimitiveType::Mutex:
                case SyncPrimitiveType::BinarySemaphore:
                    return true;
                default:
                    return false;
            }
            
        case ThreadingModel::Preemptive:
            // Preemptive threading supports all primitives
            return true;
            
        default:
            return false;
    }
}

ThreadConfig create_platform_thread_config(ThreadingModel model, std::string_view name) {
    ThreadConfig config = ThreadConfig::create_default(name);
    
    switch (model) {
        case ThreadingModel::SingleThreaded:
            // Not applicable, but provide safe defaults
            config.priority = ThreadPriority::Normal;
            config.stack_size = 0;
            break;
            
        case ThreadingModel::Cooperative:
            // PSP-like systems with limited memory
            config.stack_size = 64 * 1024;  // 64KB stack
            config.priority = ThreadPriority::Normal;
            break;
            
        case ThreadingModel::WebWorkers:
            // Web platforms with restricted environment
            config.stack_size = 0;  // Browser managed
            config.priority = ThreadPriority::Normal;
            config.detached = true;  // Web Workers are detached by nature
            break;
            
        case ThreadingModel::Preemptive:
            // Desktop systems with abundant resources
            config.stack_size = 1024 * 1024;  // 1MB stack
            config.priority = ThreadPriority::Normal;
            break;
            
        default:
            break;
    }
    
    return config;
}

WorkHandle create_work_handle(uint64_t id, WorkPriority priority) {
    WorkHandle handle;
    handle.id = id;
    handle.priority = priority;
    handle.is_valid = true;
    return handle;
}

size_t calculate_thread_overhead(ThreadingModel model, size_t stack_size) {
    size_t base_overhead = 0;
    
    switch (model) {
        case ThreadingModel::SingleThreaded:
            return 0;  // No threading overhead
            
        case ThreadingModel::Cooperative:
            // Minimal overhead for cooperative threads
            base_overhead = 512;  // Control structure
            break;
            
        case ThreadingModel::WebWorkers:
            // Higher overhead for Web Workers
            base_overhead = 8192;  // 8KB overhead
            break;
            
        case ThreadingModel::Preemptive:
            // Moderate overhead for preemptive threads
            base_overhead = 2048;  // 2KB control structure
            break;
            
        default:
            base_overhead = 1024;
            break;
    }
    
    // Add stack size to overhead
    size_t effective_stack_size = stack_size;
    if (effective_stack_size == 0) {
        // Use platform defaults
        switch (model) {
            case ThreadingModel::Cooperative:
                effective_stack_size = 64 * 1024;  // 64KB
                break;
            case ThreadingModel::WebWorkers:
                effective_stack_size = 0;  // Browser managed
                break;
            case ThreadingModel::Preemptive:
                effective_stack_size = 1024 * 1024;  // 1MB
                break;
            default:
                effective_stack_size = 256 * 1024;  // 256KB
                break;
        }
    }
    
    return base_overhead + effective_stack_size;
}

} // namespace thread

} // namespace flight::hal
