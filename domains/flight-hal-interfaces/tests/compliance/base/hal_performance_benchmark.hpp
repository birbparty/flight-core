/**
 * @file hal_performance_benchmark.hpp
 * @brief Base Template Class for HAL Interface Performance Benchmarking
 * 
 * Provides comprehensive performance benchmarking framework using Google Benchmark
 * for measuring HAL interface implementation performance across all target platforms.
 */

#pragma once

#include <benchmark/benchmark.h>
#include <memory>
#include <chrono>
#include <vector>
#include <string>
#include <type_traits>
#include <functional>
#include <map>

#include "../../../include/flight/hal/core/hal_result.hpp"
#include "../../../include/flight/hal/core/hal_error.hpp"
#include "../../../include/flight/hal/core/platform_detection.hpp"
#include "../../../include/flight/hal/core/platform_capabilities.hpp"

namespace flight::hal::benchmarking {

/**
 * @brief Benchmark configuration for platform-specific optimizations
 */
struct BenchmarkConfig {
    size_t min_iterations{100};                        ///< Minimum benchmark iterations
    size_t max_iterations{1000000};                   ///< Maximum benchmark iterations
    std::chrono::milliseconds min_time{1000};         ///< Minimum benchmark time
    std::chrono::milliseconds max_time{10000};        ///< Maximum benchmark time
    bool enable_memory_tracking{true};                ///< Track memory usage
    bool enable_cpu_profiling{true};                  ///< Enable CPU profiling
    bool enable_regression_testing{true};             ///< Enable regression detection
    std::string platform_name{"unknown"};             ///< Platform identifier
    
    // Platform-specific performance targets
    struct PerformanceTargets {
        std::chrono::nanoseconds target_latency{1000}; ///< Target operation latency
        double target_throughput{10000.0};             ///< Target throughput (ops/sec)
        size_t max_memory_per_op{1024};               ///< Max memory per operation
        double cpu_efficiency_target{0.9};            ///< CPU efficiency target (0-1)
    } targets;
};

/**
 * @brief Detailed benchmark results with statistical analysis
 */
struct BenchmarkResults {
    std::string benchmark_name;
    size_t iterations_run{0};
    std::chrono::nanoseconds total_time{};
    std::chrono::nanoseconds min_time{};
    std::chrono::nanoseconds max_time{};
    std::chrono::nanoseconds mean_time{};
    std::chrono::nanoseconds median_time{};
    std::chrono::nanoseconds stddev_time{};
    
    // Throughput metrics
    double ops_per_second{0.0};
    double items_per_second{0.0};
    double bytes_per_second{0.0};
    
    // Memory metrics
    size_t peak_memory_usage{0};
    size_t avg_memory_per_op{0};
    size_t memory_allocations{0};
    size_t memory_deallocations{0};
    
    // CPU metrics
    double cpu_utilization{0.0};
    size_t cache_misses{0};
    size_t branch_mispredictions{0};
    
    // Regression analysis
    bool performance_regression{false};
    double regression_percentage{0.0};
    std::string baseline_version;
    
    // Platform-specific metrics
    std::map<std::string, double> custom_metrics;
};

/**
 * @brief Memory tracking utilities for benchmarks
 */
class MemoryTracker {
public:
    static MemoryTracker& instance() {
        static MemoryTracker instance;
        return instance;
    }
    
    void start_tracking() {
        tracking_active_ = true;
        peak_usage_ = 0;
        current_usage_ = 0;
        allocation_count_ = 0;
        deallocation_count_ = 0;
    }
    
    void stop_tracking() {
        tracking_active_ = false;
    }
    
    void record_allocation(size_t size) {
        if (tracking_active_) {
            current_usage_ += size;
            peak_usage_ = std::max(peak_usage_, current_usage_);
            ++allocation_count_;
        }
    }
    
    void record_deallocation(size_t size) {
        if (tracking_active_) {
            current_usage_ -= size;
            ++deallocation_count_;
        }
    }
    
    size_t get_peak_usage() const { return peak_usage_; }
    size_t get_current_usage() const { return current_usage_; }
    size_t get_allocation_count() const { return allocation_count_; }
    size_t get_deallocation_count() const { return deallocation_count_; }

private:
    bool tracking_active_{false};
    size_t peak_usage_{0};
    size_t current_usage_{0};
    size_t allocation_count_{0};
    size_t deallocation_count_{0};
};

/**
 * @brief Base class for HAL interface performance benchmarks
 * 
 * Template class that provides comprehensive benchmarking infrastructure
 * with platform-specific optimizations and regression detection.
 * 
 * @tparam InterfaceType The HAL interface type being benchmarked
 */
template<typename InterfaceType>
class HALPerformanceBenchmark {
    static_assert(std::is_base_of_v<IHALInterface, InterfaceType>, 
                  "InterfaceType must inherit from IHALInterface");

public:
    using Interface = InterfaceType;
    using InterfacePtr = std::unique_ptr<InterfaceType>;
    
    /**
     * @brief Constructor with platform detection
     */
    HALPerformanceBenchmark() {
        // Initialize platform detection
        auto platform_result = PlatformDetection::detect_current_platform();
        if (platform_result.is_ok()) {
            current_platform_ = platform_result.value();
        } else {
            current_platform_ = PlatformType::Unknown;
        }
        
        // Get platform capabilities
        auto caps_result = PlatformCapabilities::get_capabilities(current_platform_);
        if (caps_result.is_ok()) {
            platform_capabilities_ = caps_result.value();
        }
        
        // Configure benchmark for platform
        config_ = get_benchmark_config();
        config_.platform_name = to_string(current_platform_);
        apply_platform_targets();
    }
    
    /**
     * @brief Virtual destructor
     */
    virtual ~HALPerformanceBenchmark() = default;
    
    // === Virtual methods for derived classes to implement ===
    
    /**
     * @brief Create interface instance for benchmarking
     * Must be implemented by derived benchmark classes
     * @return Unique pointer to interface instance
     */
    virtual InterfacePtr create_interface() = 0;
    
    /**
     * @brief Get benchmark configuration for this interface
     * Can be overridden by derived classes for custom configuration
     * @return Benchmark configuration structure
     */
    virtual BenchmarkConfig get_benchmark_config() {
        return BenchmarkConfig{};  // Default configuration
    }
    
    /**
     * @brief Setup benchmark-specific resources
     * Can be overridden by derived classes
     */
    virtual void setup_benchmark() {
        // Create interface instance
        interface_ = create_interface();
        if (!interface_) {
            throw std::runtime_error("Failed to create interface instance for benchmarking");
        }
    }
    
    /**
     * @brief Cleanup benchmark-specific resources
     * Can be overridden by derived classes
     */
    virtual void cleanup_benchmark() {
        interface_.reset();
    }
    
    // === Benchmark execution utilities ===
    
    /**
     * @brief Run benchmark with Google Benchmark integration
     * @param state Google Benchmark state object
     * @param operation Function to benchmark
     */
    template<typename Func>
    void run_benchmark(benchmark::State& state, Func&& operation) {
        setup_benchmark();
        
        // Start memory tracking
        MemoryTracker::instance().start_tracking();
        
        // Run benchmark iterations
        for (auto _ : state) {
            // Pause timing for setup if needed
            // state.PauseTiming();
            // ... setup code ...
            // state.ResumeTiming();
            
            try {
                operation();
            } catch (const std::exception& e) {
                state.SkipWithError(e.what());
                break;
            }
        }
        
        // Stop memory tracking
        MemoryTracker::instance().stop_tracking();
        
        // Add custom counters
        add_performance_counters(state);
        
        cleanup_benchmark();
    }
    
    /**
     * @brief Run throughput benchmark measuring operations per second
     * @param state Google Benchmark state object
     * @param operation Function to benchmark
     * @param items_per_operation Number of items processed per operation
     */
    template<typename Func>
    void run_throughput_benchmark(benchmark::State& state, Func&& operation, 
                                 size_t items_per_operation = 1) {
        setup_benchmark();
        
        MemoryTracker::instance().start_tracking();
        
        size_t total_items = 0;
        for (auto _ : state) {
            try {
                operation();
                total_items += items_per_operation;
            } catch (const std::exception& e) {
                state.SkipWithError(e.what());
                break;
            }
        }
        
        MemoryTracker::instance().stop_tracking();
        
        // Set throughput metrics
        state.SetItemsProcessed(total_items);
        state.counters["ops/sec"] = benchmark::Counter(state.iterations(), 
                                                      benchmark::Counter::kIsRate);
        
        add_performance_counters(state);
        cleanup_benchmark();
    }
    
    /**
     * @brief Run memory benchmark measuring bytes processed
     * @param state Google Benchmark state object
     * @param operation Function to benchmark
     * @param bytes_per_operation Number of bytes processed per operation
     */
    template<typename Func>
    void run_memory_benchmark(benchmark::State& state, Func&& operation,
                             size_t bytes_per_operation) {
        setup_benchmark();
        
        MemoryTracker::instance().start_tracking();
        
        size_t total_bytes = 0;
        for (auto _ : state) {
            try {
                operation();
                total_bytes += bytes_per_operation;
            } catch (const std::exception& e) {
                state.SkipWithError(e.what());
                break;
            }
        }
        
        MemoryTracker::instance().stop_tracking();
        
        // Set memory metrics
        state.SetBytesProcessed(total_bytes);
        state.counters["MB/s"] = benchmark::Counter(total_bytes, 
                                                   benchmark::Counter::kIsRate,
                                                   benchmark::Counter::kIs1024);
        
        add_performance_counters(state);
        cleanup_benchmark();
    }
    
    /**
     * @brief Run latency benchmark with detailed timing analysis
     * @param state Google Benchmark state object
     * @param operation Function to benchmark
     */
    template<typename Func>
    void run_latency_benchmark(benchmark::State& state, Func&& operation) {
        setup_benchmark();
        
        std::vector<std::chrono::nanoseconds> latencies;
        latencies.reserve(1000);  // Pre-allocate for efficiency
        
        for (auto _ : state) {
            auto start = std::chrono::high_resolution_clock::now();
            
            try {
                operation();
            } catch (const std::exception& e) {
                state.SkipWithError(e.what());
                break;
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
            latencies.push_back(latency);
        }
        
        // Calculate latency statistics
        if (!latencies.empty()) {
            std::sort(latencies.begin(), latencies.end());
            
            auto min_lat = latencies.front();
            auto max_lat = latencies.back();
            auto p50_lat = latencies[latencies.size() / 2];
            auto p95_lat = latencies[static_cast<size_t>(latencies.size() * 0.95)];
            auto p99_lat = latencies[static_cast<size_t>(latencies.size() * 0.99)];
            
            state.counters["min_ns"] = min_lat.count();
            state.counters["max_ns"] = max_lat.count();
            state.counters["p50_ns"] = p50_lat.count();
            state.counters["p95_ns"] = p95_lat.count();
            state.counters["p99_ns"] = p99_lat.count();
        }
        
        add_performance_counters(state);
        cleanup_benchmark();
    }
    
    /**
     * @brief Validate performance against platform targets
     * @param results Benchmark results to validate
     * @return true if performance meets targets
     */
    bool validate_performance(const BenchmarkResults& results) const {
        // Check latency target
        if (results.mean_time > config_.targets.target_latency) {
            return false;
        }
        
        // Check throughput target
        if (results.ops_per_second < config_.targets.target_throughput) {
            return false;
        }
        
        // Check memory usage target
        if (results.avg_memory_per_op > config_.targets.max_memory_per_op) {
            return false;
        }
        
        return true;
    }

protected:
    /**
     * @brief Get interface instance
     * @return Reference to interface instance
     */
    InterfaceType& get_interface() {
        return *interface_;
    }
    
    /**
     * @brief Get benchmark configuration
     * @return Reference to benchmark configuration
     */
    const BenchmarkConfig& get_config() const {
        return config_;
    }
    
    /**
     * @brief Get current platform
     * @return Current platform type
     */
    PlatformType get_platform() const {
        return current_platform_;
    }
    
    /**
     * @brief Get platform capabilities
     * @return Platform capabilities
     */
    const PlatformCapabilities& get_platform_capabilities() const {
        return platform_capabilities_;
    }

private:
    InterfacePtr interface_;
    BenchmarkConfig config_;
    PlatformType current_platform_;
    PlatformCapabilities platform_capabilities_;
    
    /**
     * @brief Add performance counters to benchmark state
     * @param state Google Benchmark state object
     */
    void add_performance_counters(benchmark::State& state) {
        auto& tracker = MemoryTracker::instance();
        
        if (config_.enable_memory_tracking) {
            state.counters["peak_memory"] = tracker.get_peak_usage();
            state.counters["allocations"] = tracker.get_allocation_count();
            state.counters["deallocations"] = tracker.get_deallocation_count();
            
            if (state.iterations() > 0) {
                state.counters["memory_per_op"] = static_cast<double>(tracker.get_peak_usage()) / 
                                                  state.iterations();
            }
        }
        
        // Add platform-specific counters
        state.counters["platform"] = static_cast<double>(current_platform_);
    }
    
    /**
     * @brief Apply platform-specific performance targets
     */
    void apply_platform_targets() {
        switch (current_platform_) {
            case PlatformType::Dreamcast:
                // Dreamcast: Relaxed performance targets
                config_.targets.target_latency = std::chrono::microseconds{5000};
                config_.targets.target_throughput = 100.0;
                config_.targets.max_memory_per_op = 4096;
                config_.min_iterations = 10;
                config_.max_iterations = 1000;
                break;
                
            case PlatformType::PSP:
                // PSP: Moderate performance targets
                config_.targets.target_latency = std::chrono::microseconds{2000};
                config_.targets.target_throughput = 500.0;
                config_.targets.max_memory_per_op = 2048;
                config_.min_iterations = 50;
                config_.max_iterations = 5000;
                break;
                
            case PlatformType::Web:
                // Web: Variable performance, focus on consistency
                config_.targets.target_latency = std::chrono::microseconds{10000};
                config_.targets.target_throughput = 200.0;
                config_.targets.max_memory_per_op = 8192;
                config_.min_iterations = 20;
                config_.max_iterations = 2000;
                break;
                
            case PlatformType::macOS:
            case PlatformType::Linux:
            case PlatformType::Windows:
                // Desktop: High performance targets
                config_.targets.target_latency = std::chrono::microseconds{1000};
                config_.targets.target_throughput = 10000.0;
                config_.targets.max_memory_per_op = 1024;
                config_.min_iterations = 100;
                config_.max_iterations = 100000;
                break;
                
            default:
                // Unknown platform: Conservative targets
                config_.targets.target_latency = std::chrono::microseconds{5000};
                config_.targets.target_throughput = 1000.0;
                config_.targets.max_memory_per_op = 2048;
                config_.min_iterations = 50;
                config_.max_iterations = 10000;
                break;
        }
    }
};

// === Benchmark Registration Macros ===

/**
 * @brief Macro for registering latency benchmarks
 */
#define HAL_LATENCY_BENCHMARK(interface_name, operation_name) \
    static void BM_##interface_name##_##operation_name##_Latency(benchmark::State& state) { \
        interface_name##Benchmark benchmark; \
        benchmark.run_latency_benchmark(state, [&benchmark]() { \
            benchmark.perform_##operation_name##_operation(); \
        }); \
    } \
    BENCHMARK(BM_##interface_name##_##operation_name##_Latency)

/**
 * @brief Macro for registering throughput benchmarks
 */
#define HAL_THROUGHPUT_BENCHMARK(interface_name, operation_name, items_per_op) \
    static void BM_##interface_name##_##operation_name##_Throughput(benchmark::State& state) { \
        interface_name##Benchmark benchmark; \
        benchmark.run_throughput_benchmark(state, [&benchmark]() { \
            benchmark.perform_##operation_name##_operation(); \
        }, items_per_op); \
    } \
    BENCHMARK(BM_##interface_name##_##operation_name##_Throughput)

/**
 * @brief Macro for registering memory benchmarks
 */
#define HAL_MEMORY_BENCHMARK(interface_name, operation_name, bytes_per_op) \
    static void BM_##interface_name##_##operation_name##_Memory(benchmark::State& state) { \
        interface_name##Benchmark benchmark; \
        benchmark.run_memory_benchmark(state, [&benchmark]() { \
            benchmark.perform_##operation_name##_operation(); \
        }, bytes_per_op); \
    } \
    BENCHMARK(BM_##interface_name##_##operation_name##_Memory)

/**
 * @brief Macro for registering parameterized benchmarks
 */
#define HAL_PARAMETERIZED_BENCHMARK(interface_name, operation_name, param_name) \
    static void BM_##interface_name##_##operation_name##_##param_name(benchmark::State& state) { \
        interface_name##Benchmark benchmark; \
        auto param_value = state.range(0); \
        benchmark.run_benchmark(state, [&benchmark, param_value]() { \
            benchmark.perform_##operation_name##_operation(param_value); \
        }); \
    } \
    BENCHMARK(BM_##interface_name##_##operation_name##_##param_name)->Arg(1)->Arg(10)->Arg(100)->Arg(1000)

} // namespace flight::hal::benchmarking
