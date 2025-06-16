/**
 * @file stress_metrics_collector.hpp
 * @brief Stress Test Metrics Collection System
 * 
 * Provides comprehensive metrics collection for stress testing including
 * resource usage monitoring, performance tracking, and system health analysis.
 */

#pragma once

#include "../../../include/flight/hal/core/hal_result.hpp"
#include <chrono>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <memory>
#include <algorithm>
#include <numeric>
#include <unordered_map>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#elif defined(__linux__)
#include <sys/resource.h>
#include <unistd.h>
#include <fstream>
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace flight::hal::test::stress {

/**
 * @brief Comprehensive stress testing metrics
 */
struct StressMetrics {
    // Timestamp
    std::chrono::steady_clock::time_point timestamp;
    
    // Memory metrics
    size_t memory_usage = 0;        ///< Current memory usage in bytes
    size_t peak_memory_usage = 0;   ///< Peak memory usage in bytes
    size_t memory_allocations = 0;  ///< Total allocations
    size_t memory_deallocations = 0; ///< Total deallocations
    
    // CPU metrics
    double cpu_usage = 0.0;         ///< CPU usage percentage (0-100)
    uint32_t thread_count = 0;      ///< Current thread count
    
    // Performance metrics
    std::chrono::microseconds avg_response_time{0};  ///< Average response time
    std::chrono::microseconds max_response_time{0};  ///< Maximum response time
    std::chrono::microseconds min_response_time{0};  ///< Minimum response time
    
    // Throughput metrics
    uint64_t operations_per_second = 0;  ///< Operations per second
    uint64_t total_operations = 0;       ///< Total operations executed
    uint64_t failed_operations = 0;      ///< Failed operations
    
    // System health metrics
    bool system_responsive = true;  ///< System responsiveness
    double error_rate = 0.0;        ///< Error rate percentage
    size_t resource_leaks = 0;      ///< Detected resource leaks
    
    // Platform-specific metrics
    size_t file_handles = 0;        ///< Open file handles
    size_t network_connections = 0; ///< Active network connections
    size_t graphics_resources = 0;  ///< Graphics resources allocated
    size_t audio_buffers = 0;       ///< Audio buffers allocated
};

/**
 * @brief Metrics collection statistics
 */
struct MetricsStatistics {
    size_t sample_count = 0;
    StressMetrics min_values;
    StressMetrics max_values;
    StressMetrics avg_values;
    std::chrono::milliseconds collection_duration{0};
};

/**
 * @brief Performance measurement helper
 */
class PerformanceMeasurement {
public:
    PerformanceMeasurement() : start_time_(std::chrono::high_resolution_clock::now()) {}
    
    std::chrono::microseconds elapsed() const {
        auto end_time = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);
    }
    
    void reset() {
        start_time_ = std::chrono::high_resolution_clock::now();
    }

private:
    std::chrono::high_resolution_clock::time_point start_time_;
};

/**
 * @brief Comprehensive stress test metrics collector
 * 
 * Provides real-time monitoring of system resources, performance metrics,
 * and health indicators during stress testing. Supports cross-platform
 * metrics collection with platform-specific optimizations.
 */
class StressMetricsCollector {
public:
    StressMetricsCollector() : collecting_(false), collection_interval_(std::chrono::milliseconds{100}) {
        initialize_platform_specific();
    }
    
    ~StressMetricsCollector() {
        stop_collection();
    }
    
    /**
     * @brief Start metrics collection
     */
    void start_collection() {
        if (collecting_) {
            return;
        }
        
        collecting_ = true;
        collection_start_time_ = std::chrono::steady_clock::now();
        metrics_history_.clear();
        
        // Start background collection thread
        collection_thread_ = std::thread([this]() {
            collection_loop();
        });
        
        // Stress metrics collection started
    }
    
    /**
     * @brief Stop metrics collection
     */
    void stop_collection() {
        if (!collecting_) {
            return;
        }
        
        collecting_ = false;
        
        if (collection_thread_.joinable()) {
            collection_thread_.join();
        }
        
        // Stress metrics collection stopped
    }
    
    /**
     * @brief Get current metrics snapshot
     */
    StressMetrics get_current_metrics() {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        return collect_metrics();
    }
    
    /**
     * @brief Get metrics history
     */
    std::vector<StressMetrics> get_metrics_history() const {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        return metrics_history_;
    }
    
    /**
     * @brief Get collection statistics
     */
    MetricsStatistics get_statistics() const {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        return calculate_statistics();
    }
    
    /**
     * @brief Set collection interval
     */
    void set_collection_interval(std::chrono::milliseconds interval) {
        collection_interval_ = interval;
    }
    
    /**
     * @brief Record a performance measurement
     */
    void record_performance_measurement(std::chrono::microseconds duration) {
        std::lock_guard<std::mutex> lock(performance_mutex_);
        performance_measurements_.push_back(duration);
        
        // Keep only recent measurements (last 1000)
        if (performance_measurements_.size() > 1000) {
            performance_measurements_.erase(performance_measurements_.begin());
        }
    }
    
    /**
     * @brief Record an operation result
     */
    void record_operation(bool success) {
        total_operations_.fetch_add(1);
        if (!success) {
            failed_operations_.fetch_add(1);
        }
    }
    
    /**
     * @brief Record resource allocation
     */
    void record_allocation(size_t size) {
        total_allocations_.fetch_add(1);
        current_memory_usage_.fetch_add(size);
        
        // Update peak memory usage
        size_t current = current_memory_usage_.load();
        size_t peak = peak_memory_usage_.load();
        while (current > peak && !peak_memory_usage_.compare_exchange_weak(peak, current)) {
            // CAS loop to update peak
        }
    }
    
    /**
     * @brief Record resource deallocation
     */
    void record_deallocation(size_t size) {
        total_deallocations_.fetch_add(1);
        current_memory_usage_.fetch_sub(size);
    }
    
    /**
     * @brief Record resource creation
     */
    void record_resource_creation(const std::string& resource_type) {
        std::lock_guard<std::mutex> lock(resource_mutex_);
        resource_counts_[resource_type]++;
    }
    
    /**
     * @brief Record resource destruction
     */
    void record_resource_destruction(const std::string& resource_type) {
        std::lock_guard<std::mutex> lock(resource_mutex_);
        auto it = resource_counts_.find(resource_type);
        if (it != resource_counts_.end() && it->second > 0) {
            it->second--;
        }
    }
    
    /**
     * @brief Check if system is responsive
     */
    bool is_system_responsive() const {
        // Simple responsiveness check based on recent measurements
        std::lock_guard<std::mutex> lock(performance_mutex_);
        if (performance_measurements_.empty()) {
            return true;
        }
        
        // Check if recent measurements are within acceptable range
        auto recent_count = std::min(static_cast<size_t>(10), performance_measurements_.size());
        auto recent_start = performance_measurements_.end() - recent_count;
        
        auto max_recent = *std::max_element(recent_start, performance_measurements_.end());
        return max_recent < std::chrono::milliseconds{100}; // 100ms threshold
    }

private:
    /**
     * @brief Initialize platform-specific metrics collection
     */
    void initialize_platform_specific() {
#ifdef _WIN32
        // Windows-specific initialization
        GetSystemInfo(&system_info_);
#elif defined(__linux__)
        // Linux-specific initialization
        page_size_ = sysconf(_SC_PAGESIZE);
#elif defined(__APPLE__)
        // macOS-specific initialization
        page_size_ = sysconf(_SC_PAGESIZE);
#endif
    }
    
    /**
     * @brief Main collection loop
     */
    void collection_loop() {
        while (collecting_) {
            auto metrics = collect_metrics();
            
            {
                std::lock_guard<std::mutex> lock(metrics_mutex_);
                metrics_history_.push_back(metrics);
                
                // Limit history size to prevent memory bloat
                if (metrics_history_.size() > 10000) {
                    metrics_history_.erase(metrics_history_.begin());
                }
            }
            
            std::this_thread::sleep_for(collection_interval_);
        }
    }
    
    /**
     * @brief Collect comprehensive metrics snapshot
     */
    StressMetrics collect_metrics() {
        StressMetrics metrics;
        metrics.timestamp = std::chrono::steady_clock::now();
        
        // Memory metrics
        metrics.memory_usage = get_memory_usage();
        metrics.peak_memory_usage = peak_memory_usage_.load();
        metrics.memory_allocations = total_allocations_.load();
        metrics.memory_deallocations = total_deallocations_.load();
        
        // CPU and thread metrics
        metrics.cpu_usage = get_cpu_usage();
        metrics.thread_count = get_thread_count();
        
        // Performance metrics
        auto perf_stats = get_performance_stats();
        metrics.avg_response_time = perf_stats.avg;
        metrics.max_response_time = perf_stats.max;
        metrics.min_response_time = perf_stats.min;
        
        // Throughput metrics
        metrics.total_operations = total_operations_.load();
        metrics.failed_operations = failed_operations_.load();
        metrics.operations_per_second = calculate_operations_per_second();
        
        // System health
        metrics.system_responsive = is_system_responsive();
        metrics.error_rate = calculate_error_rate();
        metrics.resource_leaks = detect_resource_leaks();
        
        // Platform-specific resources
        metrics.file_handles = get_file_handle_count();
        metrics.network_connections = get_network_connection_count();
        
        // HAL-specific resources
        {
            std::lock_guard<std::mutex> lock(resource_mutex_);
            metrics.graphics_resources = resource_counts_["graphics"];
            metrics.audio_buffers = resource_counts_["audio"];
        }
        
        return metrics;
    }
    
    /**
     * @brief Get current memory usage
     */
    size_t get_memory_usage() {
#ifdef _WIN32
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
            return pmc.WorkingSetSize;
        }
#elif defined(__linux__)
        std::ifstream statm("/proc/self/statm");
        if (statm.is_open()) {
            size_t size, resident;
            statm >> size >> resident;
            return resident * page_size_;
        }
#elif defined(__APPLE__)
        struct mach_task_basic_info info;
        mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
        if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                     (task_info_t)&info, &infoCount) == KERN_SUCCESS) {
            return info.resident_size;
        }
#endif
        return current_memory_usage_.load(); // Fallback to tracked usage
    }
    
    /**
     * @brief Get current CPU usage
     */
    double get_cpu_usage() {
        // Simplified CPU usage calculation
        // In a real implementation, this would be more sophisticated
        static auto last_time = std::chrono::steady_clock::now();
        static std::chrono::nanoseconds last_cpu_time{0};
        
        auto current_time = std::chrono::steady_clock::now();
        auto current_cpu_time = get_cpu_time();
        
        auto wall_time = current_time - last_time;
        auto cpu_time = current_cpu_time - last_cpu_time;
        
        double usage = 0.0;
        if (wall_time.count() > 0) {
            usage = (static_cast<double>(cpu_time.count()) / wall_time.count()) * 100.0;
        }
        
        last_time = current_time;
        last_cpu_time = current_cpu_time;
        
        return std::min(usage, 100.0);
    }
    
    /**
     * @brief Get CPU time
     */
    std::chrono::nanoseconds get_cpu_time() {
#ifdef _WIN32
        FILETIME creation, exit, kernel, user;
        if (GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user)) {
            ULARGE_INTEGER k, u;
            k.LowPart = kernel.dwLowDateTime;
            k.HighPart = kernel.dwHighDateTime;
            u.LowPart = user.dwLowDateTime;
            u.HighPart = user.dwHighDateTime;
            return std::chrono::nanoseconds((k.QuadPart + u.QuadPart) * 100);
        }
#elif defined(__linux__) || defined(__APPLE__)
        struct rusage usage;
        if (getrusage(RUSAGE_SELF, &usage) == 0) {
            auto sec = std::chrono::seconds(usage.ru_utime.tv_sec + usage.ru_stime.tv_sec);
            auto usec = std::chrono::microseconds(usage.ru_utime.tv_usec + usage.ru_stime.tv_usec);
            return std::chrono::duration_cast<std::chrono::nanoseconds>(sec + usec);
        }
#endif
        return std::chrono::nanoseconds{0};
    }
    
    /**
     * @brief Get current thread count
     */
    uint32_t get_thread_count() {
#ifdef _WIN32
        // Windows implementation would require more complex code
        return std::thread::hardware_concurrency();
#elif defined(__linux__)
        std::ifstream status("/proc/self/status");
        std::string line;
        while (std::getline(status, line)) {
            if (line.find("Threads:") == 0) {
                return std::stoi(line.substr(8));
            }
        }
#elif defined(__APPLE__)
        // macOS implementation would require thread enumeration
        return std::thread::hardware_concurrency();
#endif
        return 1; // Fallback
    }
    
    /**
     * @brief Performance statistics structure
     */
    struct PerformanceStats {
        std::chrono::microseconds avg{0};
        std::chrono::microseconds max{0};
        std::chrono::microseconds min{0};
    };
    
    /**
     * @brief Get performance statistics
     */
    PerformanceStats get_performance_stats() {
        std::lock_guard<std::mutex> lock(performance_mutex_);
        
        if (performance_measurements_.empty()) {
            return PerformanceStats{};
        }
        
        auto sum = std::accumulate(performance_measurements_.begin(), 
                                 performance_measurements_.end(), 
                                 std::chrono::microseconds{0});
        auto avg = sum / performance_measurements_.size();
        auto max = *std::max_element(performance_measurements_.begin(), 
                                   performance_measurements_.end());
        auto min = *std::min_element(performance_measurements_.begin(), 
                                   performance_measurements_.end());
        
        return PerformanceStats{avg, max, min};
    }
    
    /**
     * @brief Calculate operations per second
     */
    uint64_t calculate_operations_per_second() {
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = current_time - collection_start_time_;
        auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
        
        if (elapsed_seconds > 0) {
            return total_operations_.load() / elapsed_seconds;
        }
        return 0;
    }
    
    /**
     * @brief Calculate error rate
     */
    double calculate_error_rate() {
        auto total = total_operations_.load();
        auto failed = failed_operations_.load();
        
        if (total > 0) {
            return (static_cast<double>(failed) / total) * 100.0;
        }
        return 0.0;
    }
    
    /**
     * @brief Detect resource leaks
     */
    size_t detect_resource_leaks() {
        size_t leaks = 0;
        
        // Check for memory leaks
        auto allocated = total_allocations_.load();
        auto deallocated = total_deallocations_.load();
        if (allocated > deallocated) {
            leaks += (allocated - deallocated);
        }
        
        // Check for resource leaks
        std::lock_guard<std::mutex> lock(resource_mutex_);
        for (const auto& [type, count] : resource_counts_) {
            if (count > 0) {
                leaks += count;
            }
        }
        
        return leaks;
    }
    
    /**
     * @brief Get file handle count
     */
    size_t get_file_handle_count() {
        // Platform-specific implementation would go here
        // For now, return tracked count
        std::lock_guard<std::mutex> lock(resource_mutex_);
        return resource_counts_["file_handles"];
    }
    
    /**
     * @brief Get network connection count
     */
    size_t get_network_connection_count() {
        // Platform-specific implementation would go here
        // For now, return tracked count
        std::lock_guard<std::mutex> lock(resource_mutex_);
        return resource_counts_["network_connections"];
    }
    
    /**
     * @brief Calculate statistics from metrics history
     */
    MetricsStatistics calculate_statistics() const {
        MetricsStatistics stats;
        
        if (metrics_history_.empty()) {
            return stats;
        }
        
        stats.sample_count = metrics_history_.size();
        stats.collection_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            metrics_history_.back().timestamp - metrics_history_.front().timestamp);
        
        // Initialize min/max values
        stats.min_values = metrics_history_.front();
        stats.max_values = metrics_history_.front();
        
        // Calculate statistics
        StressMetrics sum_values = {};
        
        for (const auto& metrics : metrics_history_) {
            // Update min values
            stats.min_values.memory_usage = std::min(stats.min_values.memory_usage, metrics.memory_usage);
            stats.min_values.cpu_usage = std::min(stats.min_values.cpu_usage, metrics.cpu_usage);
            stats.min_values.avg_response_time = std::min(stats.min_values.avg_response_time, metrics.avg_response_time);
            
            // Update max values
            stats.max_values.memory_usage = std::max(stats.max_values.memory_usage, metrics.memory_usage);
            stats.max_values.cpu_usage = std::max(stats.max_values.cpu_usage, metrics.cpu_usage);
            stats.max_values.avg_response_time = std::max(stats.max_values.avg_response_time, metrics.avg_response_time);
            
            // Accumulate for averages
            sum_values.memory_usage += metrics.memory_usage;
            sum_values.cpu_usage += metrics.cpu_usage;
            sum_values.total_operations += metrics.total_operations;
        }
        
        // Calculate averages
        stats.avg_values.memory_usage = sum_values.memory_usage / stats.sample_count;
        stats.avg_values.cpu_usage = sum_values.cpu_usage / stats.sample_count;
        stats.avg_values.total_operations = sum_values.total_operations / stats.sample_count;
        
        return stats;
    }
    
    // Collection control
    std::atomic<bool> collecting_;
    std::thread collection_thread_;
    std::chrono::milliseconds collection_interval_;
    std::chrono::steady_clock::time_point collection_start_time_;
    
    // Metrics storage
    mutable std::mutex metrics_mutex_;
    std::vector<StressMetrics> metrics_history_;
    
    // Performance tracking
    mutable std::mutex performance_mutex_;
    std::vector<std::chrono::microseconds> performance_measurements_;
    
    // Resource tracking
    mutable std::mutex resource_mutex_;
    std::unordered_map<std::string, size_t> resource_counts_;
    
    // Atomic counters
    std::atomic<uint64_t> total_operations_{0};
    std::atomic<uint64_t> failed_operations_{0};
    std::atomic<size_t> total_allocations_{0};
    std::atomic<size_t> total_deallocations_{0};
    std::atomic<size_t> current_memory_usage_{0};
    std::atomic<size_t> peak_memory_usage_{0};
    
    // Platform-specific data
#ifdef _WIN32
    SYSTEM_INFO system_info_;
#elif defined(__linux__) || defined(__APPLE__)
    long page_size_;
#endif
};

/**
 * @brief RAII helper for performance measurement
 */
class ScopedPerformanceMeasurement {
public:
    explicit ScopedPerformanceMeasurement(StressMetricsCollector* collector)
        : collector_(collector), measurement_() {}
    
    ~ScopedPerformanceMeasurement() {
        if (collector_) {
            collector_->record_performance_measurement(measurement_.elapsed());
        }
    }

private:
    StressMetricsCollector* collector_;
    PerformanceMeasurement measurement_;
};

/**
 * @brief Macro for easy performance measurement
 */
#define MEASURE_PERFORMANCE(collector) \
    ScopedPerformanceMeasurement __perf_measure(collector)

} // namespace flight::hal::test::stress
