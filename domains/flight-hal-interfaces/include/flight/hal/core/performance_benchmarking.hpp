/**
 * @file performance_benchmarking.hpp
 * @brief Performance Benchmarking and Tier Classification System
 * 
 * Provides micro-benchmarking capabilities for CPU, GPU, memory, and I/O
 * subsystems to enable dynamic performance tier classification and
 * adaptive performance optimization.
 */

#pragma once

#include "hal_capabilities.hpp"
#include "hal_result.hpp"
#include "hardware_detection.hpp"
#include <chrono>
#include <vector>
#include <string>
#include <memory>
#include <functional>

namespace flight::hal {

/**
 * @brief Benchmark execution result
 */
struct BenchmarkResult {
    std::string name;                    ///< Benchmark name
    std::chrono::nanoseconds duration;   ///< Execution time
    double score;                        ///< Normalized score (higher = better)
    double throughput;                   ///< Operations per second
    bool successful;                     ///< Whether benchmark completed successfully
    std::string error_message;           ///< Error message if failed
};

/**
 * @brief CPU benchmark results
 */
struct CPUBenchmarkResults {
    BenchmarkResult integer_ops;         ///< Integer arithmetic performance
    BenchmarkResult float_ops;           ///< Floating-point arithmetic performance
    BenchmarkResult simd_ops;            ///< SIMD operations performance
    BenchmarkResult memory_access;       ///< Memory access pattern performance
    BenchmarkResult branch_prediction;   ///< Branch prediction performance
    BenchmarkResult cache_performance;   ///< Cache hierarchy performance
    
    double overall_score;                ///< Overall CPU performance score
    PerformanceTier tier;                ///< CPU performance tier
};

/**
 * @brief GPU benchmark results
 */
struct GPUBenchmarkResults {
    BenchmarkResult vertex_throughput;   ///< Vertex processing throughput
    BenchmarkResult fragment_throughput; ///< Fragment processing throughput
    BenchmarkResult texture_bandwidth;   ///< Texture memory bandwidth
    BenchmarkResult shader_complexity;   ///< Complex shader performance
    BenchmarkResult geometry_throughput; ///< Geometry processing throughput
    BenchmarkResult compute_performance; ///< Compute shader performance
    
    double overall_score;                ///< Overall GPU performance score
    PerformanceTier tier;                ///< GPU performance tier
};

/**
 * @brief Memory benchmark results
 */
struct MemoryBenchmarkResults {
    BenchmarkResult sequential_read;     ///< Sequential read bandwidth
    BenchmarkResult sequential_write;    ///< Sequential write bandwidth
    BenchmarkResult random_access;       ///< Random access latency
    BenchmarkResult cache_thrashing;     ///< Cache thrashing performance
    BenchmarkResult memory_bandwidth;    ///< Peak memory bandwidth
    BenchmarkResult allocation_speed;    ///< Memory allocation speed
    
    double overall_score;                ///< Overall memory performance score
    PerformanceTier tier;                ///< Memory performance tier
};

/**
 * @brief I/O benchmark results
 */
struct IOBenchmarkResults {
    BenchmarkResult storage_read;        ///< Storage read performance
    BenchmarkResult storage_write;       ///< Storage write performance
    BenchmarkResult network_throughput;  ///< Network throughput
    BenchmarkResult network_latency;     ///< Network latency
    BenchmarkResult input_latency;       ///< Input device latency
    BenchmarkResult audio_latency;       ///< Audio processing latency
    
    double overall_score;                ///< Overall I/O performance score
    PerformanceTier tier;                ///< I/O performance tier
};

/**
 * @brief Overall system benchmark results
 */
struct SystemBenchmarkResults {
    CPUBenchmarkResults cpu;             ///< CPU benchmark results
    GPUBenchmarkResults gpu;             ///< GPU benchmark results
    MemoryBenchmarkResults memory;       ///< Memory benchmark results
    IOBenchmarkResults io;               ///< I/O benchmark results
    
    double overall_score;                ///< Overall system performance score
    PerformanceTier overall_tier;        ///< Overall system performance tier
    
    std::chrono::steady_clock::time_point timestamp; ///< When benchmarks were run
    std::chrono::milliseconds total_duration;        ///< Total benchmark duration
};

/**
 * @brief Performance tier thresholds
 */
struct PerformanceTierThresholds {
    double minimal_threshold;            ///< Minimum score for Minimal tier
    double limited_threshold;            ///< Minimum score for Limited tier
    double standard_threshold;           ///< Minimum score for Standard tier
    double high_threshold;               ///< Minimum score for High tier
};

/**
 * @brief Benchmark configuration
 */
struct BenchmarkConfig {
    std::chrono::milliseconds max_duration;    ///< Maximum benchmark duration
    uint32_t warmup_iterations;                ///< Number of warmup iterations
    uint32_t measurement_iterations;           ///< Number of measurement iterations
    bool enable_cpu_benchmarks;                ///< Enable CPU benchmarks
    bool enable_gpu_benchmarks;                ///< Enable GPU benchmarks
    bool enable_memory_benchmarks;             ///< Enable memory benchmarks
    bool enable_io_benchmarks;                 ///< Enable I/O benchmarks
    bool quick_benchmark;                      ///< Use quick benchmark mode
    double accuracy_threshold;                 ///< Required accuracy threshold
};

/**
 * @brief Performance benchmarking interface
 */
class IPerformanceBenchmark {
public:
    virtual ~IPerformanceBenchmark() = default;
    
    /**
     * @brief Run CPU performance benchmarks
     * @param config Benchmark configuration
     * @return CPU benchmark results
     */
    virtual HALResult<CPUBenchmarkResults> benchmark_cpu(const BenchmarkConfig& config) = 0;
    
    /**
     * @brief Run GPU performance benchmarks
     * @param config Benchmark configuration
     * @return GPU benchmark results
     */
    virtual HALResult<GPUBenchmarkResults> benchmark_gpu(const BenchmarkConfig& config) = 0;
    
    /**
     * @brief Run memory performance benchmarks
     * @param config Benchmark configuration
     * @return Memory benchmark results
     */
    virtual HALResult<MemoryBenchmarkResults> benchmark_memory(const BenchmarkConfig& config) = 0;
    
    /**
     * @brief Run I/O performance benchmarks
     * @param config Benchmark configuration
     * @return I/O benchmark results
     */
    virtual HALResult<IOBenchmarkResults> benchmark_io(const BenchmarkConfig& config) = 0;
    
    /**
     * @brief Run comprehensive system benchmarks
     * @param config Benchmark configuration
     * @return Complete system benchmark results
     */
    virtual HALResult<SystemBenchmarkResults> benchmark_system(const BenchmarkConfig& config) = 0;
    
    /**
     * @brief Get benchmark progress
     * @return Progress percentage (0.0 to 1.0)
     */
    virtual double get_benchmark_progress() const = 0;
    
    /**
     * @brief Check if benchmarks are currently running
     * @return true if benchmarks are active
     */
    virtual bool is_benchmarking() const = 0;
    
    /**
     * @brief Cancel running benchmarks
     * @return Success result
     */
    virtual HALResult<void> cancel_benchmarks() = 0;
};

/**
 * @brief Performance tier classifier
 */
class IPerformanceTierClassifier {
public:
    virtual ~IPerformanceTierClassifier() = default;
    
    /**
     * @brief Classify performance tier from benchmark results
     * @param results System benchmark results
     * @return Classified performance tier
     */
    virtual PerformanceTier classify_tier(const SystemBenchmarkResults& results) = 0;
    
    /**
     * @brief Classify CPU performance tier
     * @param results CPU benchmark results
     * @return CPU performance tier
     */
    virtual PerformanceTier classify_cpu_tier(const CPUBenchmarkResults& results) = 0;
    
    /**
     * @brief Classify GPU performance tier
     * @param results GPU benchmark results
     * @return GPU performance tier
     */
    virtual PerformanceTier classify_gpu_tier(const GPUBenchmarkResults& results) = 0;
    
    /**
     * @brief Classify memory performance tier
     * @param results Memory benchmark results
     * @return Memory performance tier
     */
    virtual PerformanceTier classify_memory_tier(const MemoryBenchmarkResults& results) = 0;
    
    /**
     * @brief Classify I/O performance tier
     * @param results I/O benchmark results
     * @return I/O performance tier
     */
    virtual PerformanceTier classify_io_tier(const IOBenchmarkResults& results) = 0;
    
    /**
     * @brief Get tier thresholds for a specific subsystem
     * @param subsystem Subsystem name ("cpu", "gpu", "memory", "io", "overall")
     * @return Performance tier thresholds
     */
    virtual PerformanceTierThresholds get_tier_thresholds(const std::string& subsystem) = 0;
    
    /**
     * @brief Update tier thresholds based on new data
     * @param subsystem Subsystem name
     * @param thresholds New threshold values
     * @return Success result
     */
    virtual HALResult<void> update_tier_thresholds(
        const std::string& subsystem, 
        const PerformanceTierThresholds& thresholds
    ) = 0;
};

/**
 * @brief Adaptive performance profiler
 * 
 * Continuously monitors system performance and adapts
 * performance classifications over time.
 */
class IAdaptivePerformanceProfiler {
public:
    virtual ~IAdaptivePerformanceProfiler() = default;
    
    /**
     * @brief Start adaptive performance profiling
     * @param config Benchmark configuration
     * @return Success result
     */
    virtual HALResult<void> start_profiling(const BenchmarkConfig& config) = 0;
    
    /**
     * @brief Stop adaptive performance profiling
     * @return Success result
     */
    virtual HALResult<void> stop_profiling() = 0;
    
    /**
     * @brief Get current performance profile
     * @return Current system performance profile
     */
    virtual SystemBenchmarkResults get_current_profile() const = 0;
    
    /**
     * @brief Get performance trend over time
     * @param duration Time period to analyze
     * @return Performance trend data
     */
    virtual std::vector<SystemBenchmarkResults> get_performance_trend(
        std::chrono::minutes duration
    ) const = 0;
    
    /**
     * @brief Check if profiling is active
     * @return true if profiling is running
     */
    virtual bool is_profiling() const = 0;
    
    /**
     * @brief Register performance change callback
     * @param callback Function to call when performance changes
     * @return Success result
     */
    virtual HALResult<void> register_performance_change_callback(
        std::function<void(const SystemBenchmarkResults&)> callback
    ) = 0;
    
    /**
     * @brief Set profiling interval
     * @param interval How often to run performance checks
     * @return Success result
     */
    virtual HALResult<void> set_profiling_interval(std::chrono::minutes interval) = 0;
};

/**
 * @brief Create platform-specific performance benchmark
 * @return Platform-specific benchmark implementation
 */
std::unique_ptr<IPerformanceBenchmark> create_performance_benchmark();

/**
 * @brief Create performance tier classifier
 * @return Performance tier classifier instance
 */
std::unique_ptr<IPerformanceTierClassifier> create_performance_tier_classifier();

/**
 * @brief Create adaptive performance profiler
 * @return Adaptive performance profiler instance
 */
std::unique_ptr<IAdaptivePerformanceProfiler> create_adaptive_performance_profiler();

/**
 * @brief Create default benchmark configuration
 * @param quick_mode Use quick benchmark mode for faster results
 * @return Default benchmark configuration
 */
BenchmarkConfig create_default_benchmark_config(bool quick_mode = false);

/**
 * @brief Create benchmark configuration for specific use case
 * @param use_case Use case ("gaming", "productivity", "minimal", "comprehensive")
 * @return Optimized benchmark configuration
 */
BenchmarkConfig create_benchmark_config_for_use_case(const std::string& use_case);

/**
 * @brief Convert benchmark results to JSON string
 * @param results Benchmark results to serialize
 * @return JSON string representation
 */
std::string benchmark_results_to_json(const SystemBenchmarkResults& results);

/**
 * @brief Parse benchmark results from JSON string
 * @param json JSON string to parse
 * @return Parsed benchmark results
 */
HALResult<SystemBenchmarkResults> benchmark_results_from_json(const std::string& json);

} // namespace flight::hal
