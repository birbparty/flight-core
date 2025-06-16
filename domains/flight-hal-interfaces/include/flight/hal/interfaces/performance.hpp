/**
 * @file performance.hpp
 * @brief Flight HAL Elite Performance Monitoring Interface
 * 
 * Comprehensive performance monitoring system with hardware counters, memory tracking,
 * frame rate metrics, and driver-specific performance data while maintaining minimal
 * overhead across platforms from Dreamcast (16MB) to modern gigabyte systems.
 */

#pragma once

#include "../core/driver_registry.hpp"
#include "../core/hal_result.hpp"
#include "../core/hal_capabilities.hpp"
#include "memory.hpp"
#include "time.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>
#include <string_view>
#include <memory>
#include <unordered_map>

namespace flight::hal {

/**
 * @brief Performance counter types for hardware monitoring
 */
enum class PerformanceCounterType : uint16_t {
    // CPU Counters
    CPUCycles = 0x0001,           ///< CPU cycles executed
    Instructions,                 ///< Instructions retired
    CacheMisses,                  ///< Cache misses (L1/L2/L3)
    CacheHits,                    ///< Cache hits
    BranchMisses,                 ///< Branch mispredictions
    BranchHits,                   ///< Successful branch predictions
    
    // Memory Counters
    MemoryReads = 0x0100,         ///< Memory read operations
    MemoryWrites,                 ///< Memory write operations
    MemoryBandwidth,              ///< Memory bandwidth utilization
    PageFaults,                   ///< Memory page faults
    TLBMisses,                    ///< Translation lookaside buffer misses
    
    // GPU Counters (where available)
    GPUCycles = 0x0200,           ///< GPU cycles
    GPUVerticesProcessed,         ///< Vertices processed by GPU
    GPUPixelsDrawn,               ///< Pixels drawn/rendered
    GPUTextureReads,              ///< GPU texture cache reads
    GPUShaderInvocations,         ///< Shader program invocations
    
    // System Counters
    InterruptCount = 0x0300,      ///< Hardware interrupt count
    ContextSwitches,              ///< Thread/process context switches
    SystemCalls,                  ///< System call count
    IOOperations,                 ///< Input/output operations
    
    // Platform-Specific
    BatteryLevel = 0x1000,        ///< Battery level (PSP, handhelds)
    ThermalState,                 ///< Thermal/temperature state
    PowerConsumption,             ///< Power consumption metrics
    NetworkLatency,               ///< Network round-trip time
    
    // Software Counters
    FrameRate = 0x2000,           ///< Frames per second
    FrameTime,                    ///< Frame processing time
    MemoryAllocations,            ///< Memory allocation count
    MemoryDeallocations,          ///< Memory deallocation count
    DriverOperations,             ///< HAL driver operation count
    
    // Custom counters (driver-specific)
    CustomCounter1 = 0x8000,      ///< Custom counter slot 1
    CustomCounter2,               ///< Custom counter slot 2
    CustomCounter3,               ///< Custom counter slot 3
    CustomCounter4                ///< Custom counter slot 4
};

/**
 * @brief Performance monitoring mode for different use cases
 */
enum class MonitoringMode : uint8_t {
    Disabled = 0,      ///< No performance monitoring
    Minimal,           ///< Essential metrics only (lowest overhead)
    Standard,          ///< Balanced monitoring for production
    Detailed,          ///< Comprehensive monitoring for development
    Profiling,         ///< Maximum detail for performance analysis
    Custom             ///< User-defined counter selection
};

/**
 * @brief Performance data collection strategy
 */
enum class CollectionStrategy : uint8_t {
    Polling = 0,       ///< Regular polling at fixed intervals
    EventDriven,       ///< Collect on specific events
    Continuous,        ///< Continuous real-time collection
    OnDemand,          ///< Collect only when requested
    Streaming,         ///< Stream directly to telemetry
    Batch              ///< Batch collection for efficiency
};

/**
 * @brief Performance alert severity levels
 */
enum class PerformanceAlertLevel : uint8_t {
    Info = 0,          ///< Informational message
    Warning,           ///< Performance warning
    Critical,          ///< Critical performance issue
    Emergency          ///< Emergency performance failure
};

/**
 * @brief Performance counter sample containing value and metadata
 */
struct PerformanceCounterSample {
    PerformanceCounterType type;     ///< Counter type
    uint64_t value;                  ///< Raw counter value
    uint64_t timestamp_ns;           ///< Sample timestamp in nanoseconds
    double normalized_value;         ///< Normalized value (0.0-1.0 for percentages)
    bool is_valid;                  ///< Whether sample is valid
    uint32_t sample_id;             ///< Unique sample identifier
    
    /**
     * @brief Get counter value as percentage (0-100)
     * @return Value as percentage
     */
    double as_percentage() const {
        return normalized_value * 100.0;
    }
    
    /**
     * @brief Get age of sample in nanoseconds
     * @param current_time Current timestamp
     * @return Age in nanoseconds
     */
    uint64_t age_ns(uint64_t current_time) const {
        return current_time > timestamp_ns ? current_time - timestamp_ns : 0;
    }
};

/**
 * @brief Memory performance metrics
 */
struct MemoryPerformanceData {
    size_t allocations_per_second;      ///< Allocations per second
    size_t deallocations_per_second;    ///< Deallocations per second
    size_t bytes_allocated_per_second;  ///< Bytes allocated per second
    size_t peak_allocation_size;        ///< Largest single allocation
    double average_allocation_size;     ///< Average allocation size
    double fragmentation_ratio;         ///< Memory fragmentation (0.0-1.0)
    uint32_t allocation_failures;       ///< Failed allocation attempts
    uint64_t total_allocations;         ///< Total lifetime allocations
    uint64_t total_deallocations;       ///< Total lifetime deallocations
    size_t memory_pressure_events;      ///< Memory pressure event count
    
    /**
     * @brief Calculate allocation efficiency
     * @return Efficiency ratio (0.0-1.0)
     */
    double get_allocation_efficiency() const {
        if (total_allocations == 0) return 1.0;
        return 1.0 - (static_cast<double>(allocation_failures) / total_allocations);
    }
};

/**
 * @brief Frame performance metrics for real-time applications
 */
struct FramePerformanceData {
    double current_fps;              ///< Current frames per second
    double average_fps;              ///< Average FPS over time window
    double minimum_fps;              ///< Minimum FPS in time window
    double maximum_fps;              ///< Maximum FPS in time window
    uint64_t frame_time_us;          ///< Current frame time in microseconds
    uint64_t average_frame_time_us;  ///< Average frame time in microseconds
    uint64_t minimum_frame_time_us;  ///< Minimum frame time in microseconds
    uint64_t maximum_frame_time_us;  ///< Maximum frame time in microseconds
    uint32_t dropped_frames;         ///< Dropped frames in current period
    uint32_t late_frames;            ///< Frames exceeding target time
    uint64_t total_frames;           ///< Total frames processed
    double frame_time_variance;      ///< Frame time variance
    double jitter_ms;                ///< Frame time jitter in milliseconds
    bool vsync_active;              ///< Whether VSync is active
    
    /**
     * @brief Calculate frame time stability
     * @return Stability ratio (0.0-1.0, higher is more stable)
     */
    double get_stability() const {
        if (average_frame_time_us == 0) return 0.0;
        double cv = sqrt(frame_time_variance) / average_frame_time_us; // Coefficient of variation
        return std::max(0.0, 1.0 - cv); // Higher is more stable
    }
    
    /**
     * @brief Check if performance is acceptable
     * @param target_fps Target frame rate
     * @param tolerance_pct Tolerance percentage
     * @return true if performance meets target
     */
    bool meets_target(double target_fps, double tolerance_pct = 10.0) const {
        double min_acceptable = target_fps * (100.0 - tolerance_pct) / 100.0;
        return average_fps >= min_acceptable;
    }
};

/**
 * @brief Driver-specific performance data
 */
struct DriverPerformanceData {
    std::string_view driver_name;        ///< Name of the driver
    std::string_view interface_name;     ///< HAL interface name
    uint64_t operations_per_second;      ///< Operations per second
    uint64_t average_operation_time_ns;  ///< Average operation time
    uint64_t peak_operation_time_ns;     ///< Peak operation time
    uint32_t failed_operations;          ///< Failed operation count
    uint64_t total_operations;           ///< Total lifetime operations
    double cpu_usage_percent;            ///< CPU usage percentage
    size_t memory_usage_bytes;           ///< Driver memory usage
    std::unordered_map<std::string, uint64_t> custom_metrics; ///< Driver-specific metrics
    
    /**
     * @brief Calculate operation success rate
     * @return Success rate (0.0-1.0)
     */
    double get_success_rate() const {
        if (total_operations == 0) return 1.0;
        return 1.0 - (static_cast<double>(failed_operations) / total_operations);
    }
};

/**
 * @brief Comprehensive system performance snapshot
 */
struct SystemPerformanceSnapshot {
    uint64_t timestamp_ns;                   ///< Snapshot timestamp
    std::vector<PerformanceCounterSample> counters; ///< All counter samples
    MemoryPerformanceData memory;            ///< Memory performance
    FramePerformanceData frame;              ///< Frame performance
    std::vector<DriverPerformanceData> drivers; ///< Driver performance data
    double overall_cpu_usage;                ///< Overall CPU usage percentage
    double overall_memory_usage;             ///< Overall memory usage percentage
    double system_load_average;              ///< System load average
    uint32_t active_threads;                 ///< Number of active threads
    
    /**
     * @brief Get counter sample by type
     * @param type Counter type to find
     * @return Pointer to sample or nullptr if not found
     */
    const PerformanceCounterSample* get_counter(PerformanceCounterType type) const {
        for (const auto& sample : counters) {
            if (sample.type == type) return &sample;
        }
        return nullptr;
    }
    
    /**
     * @brief Calculate overall system health score
     * @return Health score (0.0-1.0, higher is better)
     */
    double get_health_score() const;
};

/**
 * @brief Performance monitoring configuration
 */
struct PerformanceMonitoringConfig {
    MonitoringMode mode;                     ///< Monitoring mode
    CollectionStrategy strategy;             ///< Collection strategy
    uint32_t collection_interval_ms;        ///< Collection interval in milliseconds
    uint32_t history_size;                   ///< Number of samples to keep
    std::vector<PerformanceCounterType> enabled_counters; ///< Enabled counters
    bool enable_telemetry_streaming;         ///< Whether to stream to telemetry
    bool enable_alerts;                      ///< Whether to generate alerts
    double cpu_alert_threshold;              ///< CPU usage alert threshold (0-100)
    double memory_alert_threshold;           ///< Memory usage alert threshold (0-100)
    double frame_rate_alert_threshold;       ///< Frame rate alert threshold
    
    /**
     * @brief Create configuration for specific monitoring mode
     * @param mode Monitoring mode
     * @return Pre-configured monitoring config
     */
    static PerformanceMonitoringConfig for_mode(MonitoringMode mode);
    
    /**
     * @brief Create minimal overhead configuration
     * @return Minimal monitoring configuration
     */
    static PerformanceMonitoringConfig minimal();
    
    /**
     * @brief Create comprehensive monitoring configuration
     * @return Detailed monitoring configuration
     */
    static PerformanceMonitoringConfig comprehensive();
};

/**
 * @brief Performance alert information
 */
struct PerformanceAlert {
    uint64_t timestamp_ns;               ///< Alert timestamp
    PerformanceAlertLevel level;         ///< Alert severity level
    PerformanceCounterType counter_type; ///< Counter that triggered alert
    std::string_view message;            ///< Human-readable alert message
    double threshold_value;              ///< Threshold that was exceeded
    double actual_value;                 ///< Actual measured value
    uint32_t alert_id;                  ///< Unique alert identifier
    bool is_resolved;                   ///< Whether alert has been resolved
    
    /**
     * @brief Check if alert is critical
     * @return true if alert level is critical or emergency
     */
    bool is_critical() const {
        return level == PerformanceAlertLevel::Critical || level == PerformanceAlertLevel::Emergency;
    }
};

/**
 * @brief Platform performance capabilities
 */
struct PlatformPerformanceCapabilities {
    std::vector<PerformanceCounterType> available_counters; ///< Available hardware counters
    MonitoringMode max_supported_mode;       ///< Maximum supported monitoring mode
    uint32_t max_collection_frequency_hz;   ///< Maximum collection frequency
    uint32_t min_collection_interval_ms;    ///< Minimum collection interval
    bool supports_hardware_counters;        ///< Hardware counter support
    bool supports_memory_tracking;          ///< Memory tracking support
    bool supports_frame_timing;             ///< Frame timing support
    bool supports_thermal_monitoring;       ///< Thermal monitoring support
    bool supports_power_monitoring;         ///< Power monitoring support
    uint32_t counter_read_overhead_ns;      ///< Overhead per counter read
    size_t max_history_size;                ///< Maximum history buffer size
    
    /**
     * @brief Check if specific counter type is supported
     * @param type Counter type to check
     * @return true if supported
     */
    bool supports_counter(PerformanceCounterType type) const {
        for (const auto& counter : available_counters) {
            if (counter == type) return true;
        }
        return false;
    }
};

/**
 * @brief Telemetry packet for performance data transmission
 */
struct PerformanceTelemetryPacket {
    uint64_t timestamp_ns;               ///< Packet timestamp
    uint32_t sequence_number;            ///< Packet sequence number
    uint16_t platform_id;                ///< Platform identifier
    uint16_t version;                    ///< Packet format version
    std::vector<PerformanceCounterSample> counter_samples; ///< Counter samples
    MemoryPerformanceData memory_data;   ///< Memory performance data
    FramePerformanceData frame_data;     ///< Frame performance data
    std::vector<PerformanceAlert> alerts; ///< Active alerts
    uint32_t checksum;                   ///< Data integrity checksum
    
    /**
     * @brief Calculate packet size in bytes
     * @return Packet size
     */
    size_t get_packet_size() const;
    
    /**
     * @brief Validate packet integrity
     * @return true if packet is valid
     */
    bool is_valid() const;
};

// Forward declarations
class IPerformanceCollector;
class IPerformanceAnalyzer;
class IPerformanceTelemetry;

/**
 * @brief Performance alert callback function type
 */
using PerformanceAlertCallback = std::function<void(const PerformanceAlert& alert)>;

/**
 * @brief Performance data callback function type
 */
using PerformanceDataCallback = std::function<void(const SystemPerformanceSnapshot& snapshot)>;

/**
 * @brief Telemetry transmission callback function type
 */
using TelemetryCallback = std::function<void(const PerformanceTelemetryPacket& packet)>;

/**
 * @brief Performance data collector interface
 */
class IPerformanceCollector {
public:
    virtual ~IPerformanceCollector() = default;
    
    /**
     * @brief Start performance data collection
     * @param config Collection configuration
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> start_collection(const PerformanceMonitoringConfig& config) = 0;
    
    /**
     * @brief Stop performance data collection
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> stop_collection() = 0;
    
    /**
     * @brief Collect single performance snapshot
     * @return HALResult containing performance snapshot
     */
    virtual HALResult<SystemPerformanceSnapshot> collect_snapshot() = 0;
    
    /**
     * @brief Get current collection configuration
     * @return Current configuration
     */
    virtual const PerformanceMonitoringConfig& get_config() const = 0;
    
    /**
     * @brief Check if collection is active
     * @return true if actively collecting
     */
    virtual bool is_collecting() const = 0;
    
    /**
     * @brief Get collection statistics
     * @return Collection statistics
     */
    virtual HALResult<std::unordered_map<std::string, uint64_t>> get_collection_stats() const = 0;
};

/**
 * @brief Performance data analyzer interface
 */
class IPerformanceAnalyzer {
public:
    virtual ~IPerformanceAnalyzer() = default;
    
    /**
     * @brief Analyze performance snapshot for issues
     * @param snapshot Performance snapshot to analyze
     * @return HALResult containing alerts
     */
    virtual HALResult<std::vector<PerformanceAlert>> analyze_snapshot(const SystemPerformanceSnapshot& snapshot) = 0;
    
    /**
     * @brief Set alert thresholds
     * @param counter_type Counter type
     * @param warning_threshold Warning threshold value
     * @param critical_threshold Critical threshold value
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_alert_threshold(PerformanceCounterType counter_type,
                                               double warning_threshold,
                                               double critical_threshold) = 0;
    
    /**
     * @brief Get performance trend analysis
     * @param counter_type Counter type to analyze
     * @param time_window_ms Time window for analysis
     * @return HALResult containing trend data
     */
    virtual HALResult<std::vector<double>> get_trend_analysis(PerformanceCounterType counter_type,
                                                             uint32_t time_window_ms) = 0;
    
    /**
     * @brief Generate performance report
     * @param time_period_ms Time period for report
     * @return HALResult containing report data
     */
    virtual HALResult<std::string> generate_report(uint32_t time_period_ms) = 0;
};

/**
 * @brief Performance telemetry interface
 */
class IPerformanceTelemetry {
public:
    virtual ~IPerformanceTelemetry() = default;
    
    /**
     * @brief Start telemetry streaming
     * @param callback Callback for telemetry packets
     * @param transmission_interval_ms Transmission interval
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> start_streaming(TelemetryCallback callback, uint32_t transmission_interval_ms) = 0;
    
    /**
     * @brief Stop telemetry streaming
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> stop_streaming() = 0;
    
    /**
     * @brief Send single telemetry packet
     * @param packet Telemetry packet to send
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> send_packet(const PerformanceTelemetryPacket& packet) = 0;
    
    /**
     * @brief Get telemetry statistics
     * @return HALResult containing transmission statistics
     */
    virtual HALResult<std::unordered_map<std::string, uint64_t>> get_telemetry_stats() = 0;
};

/**
 * @brief Enhanced performance monitoring interface
 * 
 * Comprehensive performance monitoring system providing hardware counters,
 * memory tracking, frame metrics, and driver-specific performance data
 * while maintaining minimal overhead across diverse platform capabilities.
 */
class IPerformanceInterface : public IHALInterface {
public:
    virtual ~IPerformanceInterface() = default;
    
    // === Configuration and Capabilities ===
    
    /**
     * @brief Get platform performance capabilities
     * @return Platform performance capabilities
     */
    virtual const PlatformPerformanceCapabilities& get_performance_capabilities() const = 0;
    
    /**
     * @brief Configure performance monitoring
     * @param config Monitoring configuration
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> configure_monitoring(const PerformanceMonitoringConfig& config) = 0;
    
    /**
     * @brief Get current monitoring configuration
     * @return Current configuration
     */
    virtual const PerformanceMonitoringConfig& get_monitoring_config() const = 0;
    
    // === Performance Data Collection ===
    
    /**
     * @brief Start performance monitoring
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> start_monitoring() = 0;
    
    /**
     * @brief Stop performance monitoring
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> stop_monitoring() = 0;
    
    /**
     * @brief Check if monitoring is active
     * @return true if monitoring is active
     */
    virtual bool is_monitoring_active() const = 0;
    
    /**
     * @brief Get current performance snapshot
     * @return HALResult containing current performance data
     */
    virtual HALResult<SystemPerformanceSnapshot> get_current_snapshot() = 0;
    
    /**
     * @brief Get performance history
     * @param time_window_ms Time window for history
     * @return HALResult containing historical snapshots
     */
    virtual HALResult<std::vector<SystemPerformanceSnapshot>> get_performance_history(uint32_t time_window_ms) = 0;
    
    // === Specific Counter Operations ===
    
    /**
     * @brief Read specific performance counter
     * @param counter_type Counter type to read
     * @return HALResult containing counter sample
     */
    virtual HALResult<PerformanceCounterSample> read_counter(PerformanceCounterType counter_type) = 0;
    
    /**
     * @brief Read multiple performance counters
     * @param counter_types Vector of counter types to read
     * @return HALResult containing counter samples
     */
    virtual HALResult<std::vector<PerformanceCounterSample>> read_counters(const std::vector<PerformanceCounterType>& counter_types) = 0;
    
    /**
     * @brief Check if specific counter is supported
     * @param counter_type Counter type to check
     * @return true if counter is supported
     */
    virtual bool is_counter_supported(PerformanceCounterType counter_type) const = 0;
    
    /**
     * @brief Get available counter types
     * @return Vector of available counter types
     */
    virtual std::vector<PerformanceCounterType> get_available_counters() const = 0;
    
    // === Memory Performance ===
    
    /**
     * @brief Get current memory performance data
     * @return HALResult containing memory performance
     */
    virtual HALResult<MemoryPerformanceData> get_memory_performance() = 0;
    
    /**
     * @brief Reset memory performance statistics
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> reset_memory_statistics() = 0;
    
    // === Frame Performance ===
    
    /**
     * @brief Get current frame performance data
     * @return HALResult containing frame performance
     */
    virtual HALResult<FramePerformanceData> get_frame_performance() = 0;
    
    /**
     * @brief Begin frame timing measurement
     * @return HALResult with frame start timestamp
     */
    virtual HALResult<uint64_t> begin_frame_timing() = 0;
    
    /**
     * @brief End frame timing measurement
     * @param frame_start_timestamp Timestamp from begin_frame_timing
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> end_frame_timing(uint64_t frame_start_timestamp) = 0;
    
    /**
     * @brief Set target frame rate for performance monitoring
     * @param target_fps Target frames per second
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_target_frame_rate(double target_fps) = 0;
    
    // === Driver Performance ===
    
    /**
     * @brief Register driver for performance monitoring
     * @param driver_name Name of the driver
     * @param interface_name HAL interface name
     * @return HALResult with driver ID for performance tracking
     */
    virtual HALResult<uint32_t> register_driver_monitoring(std::string_view driver_name,
                                                          std::string_view interface_name) = 0;
    
    /**
     * @brief Unregister driver from performance monitoring
     * @param driver_id Driver ID from register_driver_monitoring
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> unregister_driver_monitoring(uint32_t driver_id) = 0;
    
    /**
     * @brief Record driver operation
     * @param driver_id Driver ID
     * @param operation_name Operation name
     * @param duration_ns Operation duration in nanoseconds
     * @param success Whether operation succeeded
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> record_driver_operation(uint32_t driver_id,
                                                   std::string_view operation_name,
                                                   uint64_t duration_ns,
                                                   bool success = true) = 0;
    
    /**
     * @brief Get driver performance data
     * @param driver_id Driver ID
     * @return HALResult containing driver performance data
     */
    virtual HALResult<DriverPerformanceData> get_driver_performance(uint32_t driver_id) = 0;
    
    /**
     * @brief Get all driver performance data
     * @return HALResult containing all driver performance data
     */
    virtual HALResult<std::vector<DriverPerformanceData>> get_all_driver_performance() = 0;
    
    // === Alert Management ===
    
    /**
     * @brief Register performance alert callback
     * @param callback Callback function for alerts
     * @return HALResult with callback ID
     */
    virtual HALResult<uint32_t> register_alert_callback(PerformanceAlertCallback callback) = 0;
    
    /**
     * @brief Unregister alert callback
     * @param callback_id Callback ID to remove
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> unregister_alert_callback(uint32_t callback_id) = 0;
    
    /**
     * @brief Get active performance alerts
     * @return HALResult containing active alerts
     */
    virtual HALResult<std::vector<PerformanceAlert>> get_active_alerts() = 0;
    
    /**
     * @brief Clear resolved alerts
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> clear_resolved_alerts() = 0;
    
    // === Telemetry Integration ===
    
    /**
     * @brief Enable telemetry streaming
     * @param callback Telemetry callback function
     * @param interval_ms Streaming interval in milliseconds
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> enable_telemetry_streaming(TelemetryCallback callback, uint32_t interval_ms) = 0;
    
    /**
     * @brief Disable telemetry streaming
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> disable_telemetry_streaming() = 0;
    
    /**
     * @brief Generate telemetry packet
     * @return HALResult containing telemetry packet
     */
    virtual HALResult<PerformanceTelemetryPacket> generate_telemetry_packet() = 0;
    
    // === Component Interfaces ===
    
    /**
     * @brief Get performance collector interface
     * @return Pointer to collector interface
     */
    virtual IPerformanceCollector* get_collector() = 0;
    
    /**
     * @brief Get performance analyzer interface
     * @return Pointer to analyzer interface
     */
    virtual IPerformanceAnalyzer* get_analyzer() = 0;
    
    /**
     * @brief Get telemetry interface
     * @return Pointer to telemetry interface
     */
    virtual IPerformanceTelemetry* get_telemetry() = 0;
    
    // === Utility Methods ===
    
    /**
     * @brief Calibrate monitoring overhead
     * @return HALResult containing overhead measurements
     */
    virtual HALResult<std::unordered_map<std::string, uint64_t>> calibrate_overhead() = 0;
    
    /**
     * @brief Get monitoring overhead estimate
     * @param config Configuration to estimate
     * @return HALResult containing overhead estimate in nanoseconds
     */
    virtual HALResult<uint64_t> estimate_monitoring_overhead(const PerformanceMonitoringConfig& config) = 0;
    
    /**
     * @brief Reset all performance statistics
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> reset_all_statistics() = 0;
    
    /**
     * @brief Export performance data
     * @param format Export format ("json", "csv", "binary")
     * @param time_range_ms Time range to export
     * @return HALResult containing exported data
     */
    virtual HALResult<std::string> export_performance_data(std::string_view format, uint32_t time_range_ms) = 0;
    
    // === IHALInterface implementation ===
    std::string_view get_interface_name() const override {
        return "performance";
    }
    
    std::string_view get_driver_name() const override {
        return "performance_driver";
    }
    
    int get_priority() const override {
        return static_cast<int>(DriverPriority::High);
    }
    
    bool is_available() const override {
        return true;
    }
    
    std::string_view get_version() const override {
        return "1.0.0";
    }
    
    // === ICapabilityProvider implementation ===
    
    /**
     * @brief Check if a specific capability is supported
     * @param capability The capability to check
     * @return true if the capability is supported
     */
    virtual bool supports_capability(HALCapability capability) const override = 0;
    
    /**
     * @brief Get all supported capabilities as a bitmask
     * @return Bitmask of all supported capabilities
     */
    virtual uint32_t get_capability_mask() const override = 0;
    
    /**
     * @brief Get list of all supported capabilities
     * @return Vector of supported capability enums
     */
    virtual std::vector<HALCapability> get_capabilities() const override = 0;
    
    /**
     * @brief Get platform performance tier
     * @return Performance tier classification
     */
    virtual PerformanceTier get_performance_tier() const override = 0;
    
    /**
     * @brief Get detailed platform information
     * @return Structure containing platform details
     */
    virtual const PlatformInfo& get_platform_info() const override = 0;
    
    /**
     * @brief Check if graceful degradation is available for a capability
     * @param capability The capability to check
     * @return true if software fallback is available
     */
    virtual bool has_fallback(HALCapability capability) const override = 0;
};

// === Utility Functions ===

/**
 * @brief Performance monitoring utility functions
 */
namespace performance {

/**
 * @brief Convert performance counter type to string
 * @param type Counter type to convert
 * @return String representation
 */
const char* to_string(PerformanceCounterType type);

/**
 * @brief Convert monitoring mode to string
 * @param mode Monitoring mode to convert
 * @return String representation
 */
const char* to_string(MonitoringMode mode);

/**
 * @brief Convert collection strategy to string
 * @param strategy Collection strategy to convert
 * @return String representation
 */
const char* to_string(CollectionStrategy strategy);

/**
 * @brief Convert alert level to string
 * @param level Alert level to convert
 * @return String representation
 */
const char* to_string(PerformanceAlertLevel level);

/**
 * @brief Calculate performance overhead estimate
 * @param config Monitoring configuration
 * @param platform_capabilities Platform capabilities
 * @return Estimated overhead in nanoseconds per collection
 */
uint64_t estimate_overhead(const PerformanceMonitoringConfig& config,
                          const PlatformPerformanceCapabilities& platform_capabilities);

/**
 * @brief Create optimal configuration for platform
 * @param platform_capabilities Platform capabilities
 * @param target_mode Desired monitoring mode
 * @return Optimized configuration
 */
PerformanceMonitoringConfig create_optimal_config(const PlatformPerformanceCapabilities& platform_capabilities,
                                                  MonitoringMode target_mode);

/**
 * @brief Validate monitoring configuration
 * @param config Configuration to validate
 * @param platform_capabilities Platform capabilities
 * @return HALResult indicating validation result
 */
HALResult<void> validate_config(const PerformanceMonitoringConfig& config,
                               const PlatformPerformanceCapabilities& platform_capabilities);

/**
 * @brief Calculate health score from performance data
 * @param snapshot Performance snapshot
 * @param thresholds Performance thresholds for scoring
 * @return Health score (0.0-1.0)
 */
double calculate_health_score(const SystemPerformanceSnapshot& snapshot,
                             const PerformanceMonitoringConfig& thresholds);

/**
 * @brief Detect performance anomalies
 * @param current_snapshot Current performance data
 * @param historical_data Historical performance data
 * @param sensitivity Anomaly detection sensitivity (0.0-1.0)
 * @return Vector of detected anomalies
 */
std::vector<PerformanceAlert> detect_anomalies(const SystemPerformanceSnapshot& current_snapshot,
                                              const std::vector<SystemPerformanceSnapshot>& historical_data,
                                              double sensitivity = 0.8);

/**
 * @brief Calculate counter normalization value
 * @param counter_type Counter type
 * @param raw_value Raw counter value
 * @param platform_capabilities Platform capabilities
 * @return Normalized value (0.0-1.0)
 */
double normalize_counter_value(PerformanceCounterType counter_type,
                              uint64_t raw_value,
                              const PlatformPerformanceCapabilities& platform_capabilities);

/**
 * @brief Generate counter mask for minimal monitoring
 * @param platform_capabilities Platform capabilities
 * @return Vector of essential counters for minimal overhead
 */
std::vector<PerformanceCounterType> get_minimal_counter_set(const PlatformPerformanceCapabilities& platform_capabilities);

/**
 * @brief Generate counter mask for comprehensive monitoring
 * @param platform_capabilities Platform capabilities
 * @return Vector of all available counters
 */
std::vector<PerformanceCounterType> get_comprehensive_counter_set(const PlatformPerformanceCapabilities& platform_capabilities);

/**
 * @brief Calculate telemetry packet overhead
 * @param packet Telemetry packet
 * @return Overhead in bytes for transmission
 */
size_t calculate_telemetry_overhead(const PerformanceTelemetryPacket& packet);

/**
 * @brief Compress performance data for storage/transmission
 * @param snapshot Performance snapshot to compress
 * @param compression_level Compression level (0-9)
 * @return Compressed data
 */
std::vector<uint8_t> compress_performance_data(const SystemPerformanceSnapshot& snapshot,
                                              int compression_level = 6);

/**
 * @brief Decompress performance data
 * @param compressed_data Compressed performance data
 * @return Decompressed performance snapshot
 */
HALResult<SystemPerformanceSnapshot> decompress_performance_data(const std::vector<uint8_t>& compressed_data);

} // namespace performance

} // namespace flight::hal
