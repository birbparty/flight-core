/**
 * @file comprehensive_performance_example.cpp
 * @brief Comprehensive Performance Monitoring Example
 * 
 * Demonstrates the complete Flight HAL performance monitoring system including
 * hardware counters, memory tracking, frame timing, alerts, and telemetry
 * integration across different platforms and monitoring modes.
 */

#include "flight/hal/interfaces/performance.hpp"
#include "flight/hal/core/driver_registry.hpp"
#include "flight/hal/core/hal_logging.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <cstdlib>

using namespace flight::hal;
using namespace flight::hal::performance;

/**
 * @brief Mock performance interface implementation for demonstration
 */
class MockPerformanceInterface : public IPerformanceInterface {
private:
    PlatformPerformanceCapabilities capabilities_;
    PerformanceMonitoringConfig config_;
    bool monitoring_active_ = false;
    std::vector<SystemPerformanceSnapshot> history_;
    std::vector<PerformanceAlert> active_alerts_;
    uint64_t frame_counter_ = 0;
    
public:
    MockPerformanceInterface() {
        // Initialize platform capabilities (simulating a modern desktop system)
        capabilities_.available_counters = {
            PerformanceCounterType::CPUCycles,
            PerformanceCounterType::Instructions,
            PerformanceCounterType::CacheMisses,
            PerformanceCounterType::CacheHits,
            PerformanceCounterType::MemoryReads,
            PerformanceCounterType::MemoryWrites,
            PerformanceCounterType::FrameRate,
            PerformanceCounterType::FrameTime,
            PerformanceCounterType::MemoryAllocations,
            PerformanceCounterType::MemoryDeallocations,
            PerformanceCounterType::DriverOperations
        };
        capabilities_.max_supported_mode = MonitoringMode::Profiling;
        capabilities_.max_collection_frequency_hz = 1000;
        capabilities_.min_collection_interval_ms = 1;
        capabilities_.supports_hardware_counters = true;
        capabilities_.supports_memory_tracking = true;
        capabilities_.supports_frame_timing = true;
        capabilities_.supports_thermal_monitoring = false;
        capabilities_.supports_power_monitoring = false;
        capabilities_.counter_read_overhead_ns = 100;
        capabilities_.max_history_size = 10000;
        
        // Set default configuration
        config_ = PerformanceMonitoringConfig::for_mode(MonitoringMode::Standard);
    }
    
    // IPerformanceInterface implementation
    const PlatformPerformanceCapabilities& get_performance_capabilities() const override {
        return capabilities_;
    }
    
    HALResult<void> configure_monitoring(const PerformanceMonitoringConfig& config) override {
        auto validation_result = validate_config(config, capabilities_);
        if (!validation_result.is_ok()) {
            return validation_result;
        }
        
        config_ = config;
        std::cout << "Performance monitoring configured for " 
                  << to_string(config.mode) << " mode" << std::endl;
        return HALResult<void>::success();
    }
    
    const PerformanceMonitoringConfig& get_monitoring_config() const override {
        return config_;
    }
    
    HALResult<void> start_monitoring() override {
        if (monitoring_active_) {
            return HALResult<void>::error(errors::invalid_state(1, "Monitoring already active"));
        }
        
        monitoring_active_ = true;
        std::cout << "Performance monitoring started" << std::endl;
        return HALResult<void>::success();
    }
    
    HALResult<void> stop_monitoring() override {
        monitoring_active_ = false;
        std::cout << "Performance monitoring stopped" << std::endl;
        return HALResult<void>::success();
    }
    
    bool is_monitoring_active() const override {
        return monitoring_active_;
    }
    
    HALResult<SystemPerformanceSnapshot> get_current_snapshot() override {
        if (!monitoring_active_) {
            return HALResult<SystemPerformanceSnapshot>::error(
                errors::invalid_state(2, "Monitoring not active"));
        }
        
        // Generate mock performance data
        SystemPerformanceSnapshot snapshot;
        snapshot.timestamp_ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        
        // Simulate some performance counters
        for (const auto& counter_type : config_.enabled_counters) {
            PerformanceCounterSample sample;
            sample.type = counter_type;
            sample.timestamp_ns = snapshot.timestamp_ns;
            sample.is_valid = true;
            sample.sample_id = static_cast<uint32_t>(snapshot.counters.size());
            
            // Generate realistic mock data
            switch (counter_type) {
                case PerformanceCounterType::CPUCycles:
                    sample.value = 1000000 + (rand() % 500000);
                    sample.normalized_value = static_cast<double>(sample.value) / 2000000.0;
                    break;
                case PerformanceCounterType::FrameRate:
                    sample.value = 58 + (rand() % 5); // 58-62 FPS
                    sample.normalized_value = static_cast<double>(sample.value) / 60.0;
                    break;
                case PerformanceCounterType::MemoryAllocations:
                    sample.value = 100 + (rand() % 50);
                    sample.normalized_value = static_cast<double>(sample.value) / 200.0;
                    break;
                default:
                    sample.value = rand() % 1000;
                    sample.normalized_value = static_cast<double>(sample.value) / 1000.0;
                    break;
            }
            
            snapshot.counters.push_back(sample);
        }
        
        // Mock memory performance data
        snapshot.memory.allocations_per_second = 150 + (rand() % 100);
        snapshot.memory.deallocations_per_second = 140 + (rand() % 100);
        snapshot.memory.bytes_allocated_per_second = 1024 * (100 + (rand() % 200));
        snapshot.memory.peak_allocation_size = 1024 * 1024;
        snapshot.memory.average_allocation_size = 1024.0;
        snapshot.memory.fragmentation_ratio = 0.1 + (static_cast<double>(rand()) / RAND_MAX) * 0.2;
        snapshot.memory.allocation_failures = 0;
        snapshot.memory.total_allocations = 10000 + (rand() % 5000);
        snapshot.memory.total_deallocations = 9900 + (rand() % 5000);
        snapshot.memory.memory_pressure_events = 0;
        
        // Mock frame performance data
        snapshot.frame.current_fps = 58.0 + (static_cast<double>(rand()) / RAND_MAX) * 4.0;
        snapshot.frame.average_fps = 59.5 + (static_cast<double>(rand()) / RAND_MAX) * 1.0;
        snapshot.frame.minimum_fps = 55.0;
        snapshot.frame.maximum_fps = 62.0;
        snapshot.frame.frame_time_us = static_cast<uint64_t>(1000000.0 / snapshot.frame.current_fps);
        snapshot.frame.average_frame_time_us = static_cast<uint64_t>(1000000.0 / snapshot.frame.average_fps);
        snapshot.frame.minimum_frame_time_us = static_cast<uint64_t>(1000000.0 / snapshot.frame.maximum_fps);
        snapshot.frame.maximum_frame_time_us = static_cast<uint64_t>(1000000.0 / snapshot.frame.minimum_fps);
        snapshot.frame.dropped_frames = 0;
        snapshot.frame.late_frames = 0;
        snapshot.frame.total_frames = ++frame_counter_;
        snapshot.frame.frame_time_variance = 100.0;
        snapshot.frame.jitter_ms = 0.5;
        snapshot.frame.vsync_active = true;
        
        // Mock system-wide data
        snapshot.overall_cpu_usage = 25.0 + (static_cast<double>(rand()) / RAND_MAX) * 20.0;
        snapshot.overall_memory_usage = 60.0 + (static_cast<double>(rand()) / RAND_MAX) * 15.0;
        snapshot.system_load_average = 1.5 + (static_cast<double>(rand()) / RAND_MAX) * 0.5;
        snapshot.active_threads = 8 + (rand() % 4);
        
        // Add to history
        history_.push_back(snapshot);
        if (history_.size() > config_.history_size) {
            history_.erase(history_.begin());
        }
        
        // Check for alerts if enabled
        if (config_.enable_alerts) {
            auto anomalies = detect_anomalies(snapshot, history_, 0.8);
            for (const auto& anomaly : anomalies) {
                active_alerts_.push_back(anomaly);
            }
        }
        
        return HALResult<SystemPerformanceSnapshot>::success(std::move(snapshot));
    }
    
    HALResult<std::vector<SystemPerformanceSnapshot>> get_performance_history(uint32_t time_window_ms) override {
        // Return recent history (simplified - not actually filtering by time)
        size_t max_samples = std::min(static_cast<size_t>(time_window_ms / config_.collection_interval_ms), history_.size());
        
        std::vector<SystemPerformanceSnapshot> recent_history;
        if (history_.size() >= max_samples) {
            recent_history.assign(history_.end() - max_samples, history_.end());
        } else {
            recent_history = history_;
        }
        
        return HALResult<std::vector<SystemPerformanceSnapshot>>::success(std::move(recent_history));
    }
    
    HALResult<PerformanceCounterSample> read_counter(PerformanceCounterType counter_type) override {
        auto snapshot_result = get_current_snapshot();
        if (!snapshot_result.is_ok()) {
            return HALResult<PerformanceCounterSample>::error(snapshot_result.error());
        }
        
        const auto& snapshot = snapshot_result.value();
        for (const auto& counter : snapshot.counters) {
            if (counter.type == counter_type) {
                return HALResult<PerformanceCounterSample>::success(counter);
            }
        }
        
        return HALResult<PerformanceCounterSample>::error(
            errors::feature_not_supported(1, "Counter type not enabled"));
    }
    
    HALResult<std::vector<PerformanceAlert>> get_active_alerts() override {
        return HALResult<std::vector<PerformanceAlert>>::success(active_alerts_);
    }
    
    HALResult<void> clear_resolved_alerts() override {
        active_alerts_.erase(
            std::remove_if(active_alerts_.begin(), active_alerts_.end(),
                [](const PerformanceAlert& alert) { return alert.is_resolved; }),
            active_alerts_.end());
        return HALResult<void>::success();
    }
    
    // Simplified implementations for other required methods
    HALResult<std::vector<PerformanceCounterSample>> read_counters(const std::vector<PerformanceCounterType>&) override {
        return HALResult<std::vector<PerformanceCounterSample>>::error(
            errors::not_implemented(1, "Method not implemented in mock"));
    }
    
    bool is_counter_supported(PerformanceCounterType counter_type) const override {
        return capabilities_.supports_counter(counter_type);
    }
    
    std::vector<PerformanceCounterType> get_available_counters() const override {
        return capabilities_.available_counters;
    }
    
    HALResult<MemoryPerformanceData> get_memory_performance() override {
        auto snapshot_result = get_current_snapshot();
        if (snapshot_result.is_ok()) {
            return HALResult<MemoryPerformanceData>::success(snapshot_result.value().memory);
        }
        return HALResult<MemoryPerformanceData>::error(snapshot_result.error());
    }
    
    HALResult<FramePerformanceData> get_frame_performance() override {
        auto snapshot_result = get_current_snapshot();
        if (snapshot_result.is_ok()) {
            return HALResult<FramePerformanceData>::success(snapshot_result.value().frame);
        }
        return HALResult<FramePerformanceData>::error(snapshot_result.error());
    }
    
    // IHALInterface implementation
    HALResult<void> initialize() override {
        std::cout << "Mock Performance Interface initialized" << std::endl;
        return HALResult<void>::success();
    }
    
    HALResult<void> shutdown() override {
        stop_monitoring();
        std::cout << "Mock Performance Interface shutdown" << std::endl;
        return HALResult<void>::success();
    }
    
    bool is_active() const override { return true; }
    
    // ICapabilityProvider implementation
    bool supports_capability(HALCapability capability) const override {
        switch (capability) {
            case HALCapability::HighPrecisionTimer:
            case HALCapability::Threading:
                return true;
            default:
                return false;
        }
    }
    
    uint32_t get_capability_mask() const override {
        return static_cast<uint32_t>(HALCapability::HighPrecisionTimer) |
               static_cast<uint32_t>(HALCapability::Threading);
    }
    
    std::vector<HALCapability> get_capabilities() const override {
        return {HALCapability::HighPrecisionTimer, HALCapability::Threading};
    }
    
    PerformanceTier get_performance_tier() const override {
        return PerformanceTier::High;
    }
    
    const PlatformInfo& get_platform_info() const override {
        static PlatformInfo info{
            "Mock Desktop Platform",
            "x86_64",
            PerformanceTier::High,
            16ULL * 1024 * 1024 * 1024, // 16GB
            8, // 8 cores
            true, // FPU
            true  // SIMD
        };
        return info;
    }
    
    bool has_fallback(HALCapability) const override { return false; }
    
    // Stub implementations for remaining methods
    HALResult<void> reset_memory_statistics() override { return HALResult<void>::success(); }
    HALResult<uint64_t> begin_frame_timing() override { return HALResult<uint64_t>::success(123456789); }
    HALResult<void> end_frame_timing(uint64_t) override { return HALResult<void>::success(); }
    HALResult<void> set_target_frame_rate(double) override { return HALResult<void>::success(); }
    HALResult<uint32_t> register_driver_monitoring(std::string_view, std::string_view) override { return HALResult<uint32_t>::success(1); }
    HALResult<void> unregister_driver_monitoring(uint32_t) override { return HALResult<void>::success(); }
    HALResult<void> record_driver_operation(uint32_t, std::string_view, uint64_t, bool) override { return HALResult<void>::success(); }
    HALResult<DriverPerformanceData> get_driver_performance(uint32_t) override { return HALResult<DriverPerformanceData>::error(errors::not_implemented(1, "")); }
    HALResult<std::vector<DriverPerformanceData>> get_all_driver_performance() override { return HALResult<std::vector<DriverPerformanceData>>::success({}); }
    HALResult<uint32_t> register_alert_callback(PerformanceAlertCallback) override { return HALResult<uint32_t>::success(1); }
    HALResult<void> unregister_alert_callback(uint32_t) override { return HALResult<void>::success(); }
    HALResult<void> enable_telemetry_streaming(TelemetryCallback, uint32_t) override { return HALResult<void>::success(); }
    HALResult<void> disable_telemetry_streaming() override { return HALResult<void>::success(); }
    HALResult<PerformanceTelemetryPacket> generate_telemetry_packet() override { return HALResult<PerformanceTelemetryPacket>::error(errors::not_implemented(1, "")); }
    IPerformanceCollector* get_collector() override { return nullptr; }
    IPerformanceAnalyzer* get_analyzer() override { return nullptr; }
    IPerformanceTelemetry* get_telemetry() override { return nullptr; }
    HALResult<std::unordered_map<std::string, uint64_t>> calibrate_overhead() override { return HALResult<std::unordered_map<std::string, uint64_t>>::success({}); }
    HALResult<uint64_t> estimate_monitoring_overhead(const PerformanceMonitoringConfig&) override { return HALResult<uint64_t>::success(1000); }
    HALResult<void> reset_all_statistics() override { return HALResult<void>::success(); }
    HALResult<std::string> export_performance_data(std::string_view, uint32_t) override { return HALResult<std::string>::success("{}"); }
};

/**
 * @brief Demonstrate basic performance monitoring functionality
 */
void demonstrate_basic_monitoring() {
    std::cout << "\n=== Basic Performance Monitoring Demo ===" << std::endl;
    
    // Create and initialize performance interface
    auto perf_interface = std::make_unique<MockPerformanceInterface>();
    auto init_result = perf_interface->initialize();
    if (!init_result.is_ok()) {
        std::cout << "Failed to initialize performance interface" << std::endl;
        return;
    }
    
    // Get platform capabilities
    const auto& capabilities = perf_interface->get_performance_capabilities();
    std::cout << "Platform supports " << capabilities.available_counters.size() 
              << " performance counters" << std::endl;
    std::cout << "Maximum monitoring mode: " 
              << to_string(capabilities.max_supported_mode) << std::endl;
    std::cout << "Hardware counters supported: " 
              << (capabilities.supports_hardware_counters ? "Yes" : "No") << std::endl;
    
    // Configure for minimal monitoring
    auto config = PerformanceMonitoringConfig::for_mode(MonitoringMode::Minimal);
    auto config_result = perf_interface->configure_monitoring(config);
    if (!config_result.is_ok()) {
        std::cout << "Failed to configure monitoring: " << config_result.error().message() << std::endl;
        return;
    }
    
    // Start monitoring
    auto start_result = perf_interface->start_monitoring();
    if (!start_result.is_ok()) {
        std::cout << "Failed to start monitoring" << std::endl;
        return;
    }
    
    // Collect some performance snapshots
    std::cout << "\nCollecting performance data..." << std::endl;
    for (int i = 0; i < 5; ++i) {
        auto snapshot_result = perf_interface->get_current_snapshot();
        if (snapshot_result.is_ok()) {
            const auto& snapshot = snapshot_result.value();
            std::cout << "Frame " << i + 1 << ": "
                      << std::fixed << std::setprecision(1)
                      << "CPU: " << snapshot.overall_cpu_usage << "%, "
                      << "Memory: " << snapshot.overall_memory_usage << "%, "
                      << "FPS: " << snapshot.frame.current_fps << std::endl;
        }
        
        // Simulate some work
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Stop monitoring
    perf_interface->stop_monitoring();
    perf_interface->shutdown();
}

/**
 * @brief Demonstrate advanced performance monitoring with alerts
 */
void demonstrate_advanced_monitoring() {
    std::cout << "\n=== Advanced Performance Monitoring Demo ===" << std::endl;
    
    auto perf_interface = std::make_unique<MockPerformanceInterface>();
    perf_interface->initialize();
    
    // Configure for detailed monitoring with alerts
    auto config = PerformanceMonitoringConfig::for_mode(MonitoringMode::Detailed);
    config.enable_alerts = true;
    config.cpu_alert_threshold = 40.0; // Lower threshold to trigger alerts
    config.memory_alert_threshold = 70.0;
    config.frame_rate_alert_threshold = 58.0;
    
    perf_interface->configure_monitoring(config);
    perf_interface->start_monitoring();
    
    std::cout << "Monitoring with alerts enabled..." << std::endl;
    std::cout << "CPU alert threshold: " << config.cpu_alert_threshold << "%" << std::endl;
    std::cout << "Memory alert threshold: " << config.memory_alert_threshold << "%" << std::endl;
    std::cout << "Frame rate alert threshold: " << config.frame_rate_alert_threshold << " FPS" << std::endl;
    
    // Collect performance data and check for alerts
    for (int i = 0; i < 10; ++i) {
        auto snapshot_result = perf_interface->get_current_snapshot();
        if (snapshot_result.is_ok()) {
            const auto& snapshot = snapshot_result.value();
            
            // Check for alerts
            auto alerts_result = perf_interface->get_active_alerts();
            if (alerts_result.is_ok() && !alerts_result.value().empty()) {
                for (const auto& alert : alerts_result.value()) {
                    std::cout << "ALERT [" << to_string(alert.level) << "]: "
                              << alert.message << " (Threshold: " 
                              << alert.threshold_value << ", Actual: " 
                              << alert.actual_value << ")" << std::endl;
                }
            }
            
            // Calculate health score
            double health_score = calculate_health_score(snapshot, config);
            std::cout << "System health score: " << std::fixed << std::setprecision(2) 
                      << health_score * 100.0 << "%" << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    perf_interface->stop_monitoring();
    perf_interface->shutdown();
}

/**
 * @brief Demonstrate configuration optimization for different platforms
 */
void demonstrate_platform_optimization() {
    std::cout << "\n=== Platform Optimization Demo ===" << std::endl;
    
    auto perf_interface = std::make_unique<MockPerformanceInterface>();
    perf_interface->initialize();
    
    const auto& capabilities = perf_interface->get_performance_capabilities();
    
    // Test different monitoring modes
    std::vector<MonitoringMode> modes = {
        MonitoringMode::Minimal,
        MonitoringMode::Standard,
        MonitoringMode::Detailed,
        MonitoringMode::Profiling
    };
    
    for (auto mode : modes) {
        std::cout << "\nTesting " << to_string(mode) << " mode:" << std::endl;
        
        // Create optimal configuration for this platform
        auto config = create_optimal_config(capabilities, mode);
        
        // Validate configuration
        auto validation_result = validate_config(config, capabilities);
        if (validation_result.is_ok()) {
            std::cout << "  ✓ Configuration valid" << std::endl;
            std::cout << "  Enabled counters: " << config.enabled_counters.size() << std::endl;
            std::cout << "  Collection interval: " << config.collection_interval_ms << "ms" << std::endl;
            std::cout << "  History size: " << config.history_size << " samples" << std::endl;
            
            // Estimate overhead
            uint64_t overhead = estimate_overhead(config, capabilities);
            std::cout << "  Estimated overhead: " << overhead << "ns per collection" << std::endl;
        } else {
            std::cout << "  ✗ Configuration invalid: " << validation_result.error().message() << std::endl;
        }
    }
    
    perf_interface->shutdown();
}

/**
 * @brief Demonstrate utility functions
 */
void demonstrate_utilities() {
    std::cout << "\n=== Utility Functions Demo ===" << std::endl;
    
    // Test string conversion functions
    std::cout << "String conversions:" << std::endl;
    std::cout << "  PerformanceCounterType::CPUCycles -> " 
              << to_string(PerformanceCounterType::CPUCycles) << std::endl;
    std::cout << "  MonitoringMode::Standard -> " 
              << to_string(MonitoringMode::Standard) << std::endl;
    std::cout << "  CollectionStrategy::Continuous -> " 
              << to_string(CollectionStrategy::Continuous) << std::endl;
    std::cout << "  PerformanceAlertLevel::Warning -> " 
              << to_string(PerformanceAlertLevel::Warning) << std::endl;
    
    // Test counter normalization
    std::cout << "\nCounter normalization:" << std::endl;
    PlatformPerformanceCapabilities mock_capabilities;
    
    uint64_t cpu_cycles = 500000000; // 500M cycles
    double normalized_cpu = normalize_counter_value(
        PerformanceCounterType::CPUCycles, cpu_cycles, mock_capabilities);
    std::cout << "  CPU cycles " << cpu_cycles << " -> " 
              << std::fixed << std::setprecision(3) << normalized_cpu << std::endl;
    
    uint64_t frame_rate = 45; // 45 FPS
    double normalized_fps = normalize_counter_value(
        PerformanceCounterType::FrameRate, frame_rate, mock_capabilities);
    std::cout << "  Frame rate " << frame_rate << " -> " 
              << std::fixed << std::setprecision(3) << normalized_fps << std::endl;
    
    // Test counter set generation
    auto minimal_counters = get_minimal_counter_set(mock_capabilities);
    std::cout << "\nMinimal counter set: " << minimal_counters.size() << " counters" << std::endl;
    for (const auto& counter : minimal_counters) {
        std::cout << "  - " << to_string(counter) << std::endl;
    }
}

int main() {
    std::cout << "Flight HAL Performance Monitoring System Demo" << std::endl;
    std::cout << "=============================================" << std::endl;
    
    try {
        // Seed random number generator for consistent mock data
        srand(static_cast<unsigned int>(std::time(nullptr)));
        
        // Run demonstrations
        demonstrate_basic_monitoring();
        demonstrate_advanced_monitoring();
        demonstrate_platform_optimization();
        demonstrate_utilities();
        
        std::cout << "\n=== Demo Complete ===" << std::endl;
        std::cout << "The Flight HAL Performance Monitoring system provides:" << std::endl;
        std::cout << "• Cross-platform performance counter access" << std::endl;
        std::cout << "• Adaptive monitoring modes for different use cases" << std::endl;
        std::cout << "• Real-time alerting and anomaly detection" << std::endl;
        std::cout << "• Memory and frame performance tracking" << std::endl;
        std::cout << "• Telemetry integration for remote monitoring" << std::endl;
        std::cout << "• Low-overhead design suitable for production use" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Demo failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
