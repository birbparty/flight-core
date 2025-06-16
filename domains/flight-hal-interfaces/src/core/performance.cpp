/**
 * @file performance.cpp
 * @brief Flight HAL Performance Monitoring Implementation
 * 
 * Implementation of performance monitoring utility functions and
 * configuration helpers for cross-platform performance analysis.
 */

#include "flight/hal/interfaces/performance.hpp"
#include "flight/hal/core/hal_logging.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <numeric>

namespace flight::hal {

// SystemPerformanceSnapshot implementation
double SystemPerformanceSnapshot::get_health_score() const {
    double cpu_score = 1.0 - (overall_cpu_usage / 100.0);
    double memory_score = 1.0 - (overall_memory_usage / 100.0);
    double frame_score = frame.get_stability();
    
    // Weight different factors
    double weighted_score = (cpu_score * 0.3) + (memory_score * 0.3) + (frame_score * 0.4);
    
    // Penalize for active alerts
    double alert_penalty = 0.0;
    for (const auto& driver : drivers) {
        if (driver.get_success_rate() < 0.95) {
            alert_penalty += 0.1;
        }
    }
    
    return std::max(0.0, std::min(1.0, weighted_score - alert_penalty));
}

// PerformanceMonitoringConfig static methods
PerformanceMonitoringConfig PerformanceMonitoringConfig::for_mode(MonitoringMode mode) {
    PerformanceMonitoringConfig config;
    config.mode = mode;
    
    switch (mode) {
        case MonitoringMode::Disabled:
            config.strategy = CollectionStrategy::OnDemand;
            config.collection_interval_ms = 0;
            config.history_size = 0;
            config.enable_telemetry_streaming = false;
            config.enable_alerts = false;
            break;
            
        case MonitoringMode::Minimal:
            config.strategy = CollectionStrategy::Polling;
            config.collection_interval_ms = 1000; // 1 second
            config.history_size = 60; // 1 minute
            config.enabled_counters = {
                PerformanceCounterType::FrameRate,
                PerformanceCounterType::MemoryAllocations
            };
            config.enable_telemetry_streaming = false;
            config.enable_alerts = true;
            config.cpu_alert_threshold = 90.0;
            config.memory_alert_threshold = 85.0;
            config.frame_rate_alert_threshold = 30.0;
            break;
            
        case MonitoringMode::Standard:
            config.strategy = CollectionStrategy::Polling;
            config.collection_interval_ms = 500; // 500ms
            config.history_size = 300; // 2.5 minutes
            config.enabled_counters = {
                PerformanceCounterType::CPUCycles,
                PerformanceCounterType::FrameRate,
                PerformanceCounterType::FrameTime,
                PerformanceCounterType::MemoryAllocations,
                PerformanceCounterType::MemoryDeallocations
            };
            config.enable_telemetry_streaming = true;
            config.enable_alerts = true;
            config.cpu_alert_threshold = 80.0;
            config.memory_alert_threshold = 75.0;
            config.frame_rate_alert_threshold = 45.0;
            break;
            
        case MonitoringMode::Detailed:
            config.strategy = CollectionStrategy::Continuous;
            config.collection_interval_ms = 100; // 100ms
            config.history_size = 1200; // 2 minutes at 100ms
            config.enabled_counters = {
                PerformanceCounterType::CPUCycles,
                PerformanceCounterType::Instructions,
                PerformanceCounterType::CacheMisses,
                PerformanceCounterType::MemoryReads,
                PerformanceCounterType::MemoryWrites,
                PerformanceCounterType::FrameRate,
                PerformanceCounterType::FrameTime,
                PerformanceCounterType::MemoryAllocations,
                PerformanceCounterType::MemoryDeallocations,
                PerformanceCounterType::DriverOperations
            };
            config.enable_telemetry_streaming = true;
            config.enable_alerts = true;
            config.cpu_alert_threshold = 70.0;
            config.memory_alert_threshold = 65.0;
            config.frame_rate_alert_threshold = 55.0;
            break;
            
        case MonitoringMode::Profiling:
            config.strategy = CollectionStrategy::Continuous;
            config.collection_interval_ms = 10; // 10ms for high precision
            config.history_size = 6000; // 1 minute at 10ms
            config.enabled_counters = {
                PerformanceCounterType::CPUCycles,
                PerformanceCounterType::Instructions,
                PerformanceCounterType::CacheMisses,
                PerformanceCounterType::CacheHits,
                PerformanceCounterType::BranchMisses,
                PerformanceCounterType::BranchHits,
                PerformanceCounterType::MemoryReads,
                PerformanceCounterType::MemoryWrites,
                PerformanceCounterType::MemoryBandwidth,
                PerformanceCounterType::PageFaults,
                PerformanceCounterType::TLBMisses,
                PerformanceCounterType::FrameRate,
                PerformanceCounterType::FrameTime,
                PerformanceCounterType::MemoryAllocations,
                PerformanceCounterType::MemoryDeallocations,
                PerformanceCounterType::DriverOperations
            };
            config.enable_telemetry_streaming = true;
            config.enable_alerts = true;
            config.cpu_alert_threshold = 60.0;
            config.memory_alert_threshold = 55.0;
            config.frame_rate_alert_threshold = 58.0;
            break;
            
        case MonitoringMode::Custom:
            // Custom mode starts with minimal settings
            config = minimal();
            config.mode = MonitoringMode::Custom;
            break;
    }
    
    return config;
}

PerformanceMonitoringConfig PerformanceMonitoringConfig::minimal() {
    return for_mode(MonitoringMode::Minimal);
}

PerformanceMonitoringConfig PerformanceMonitoringConfig::comprehensive() {
    return for_mode(MonitoringMode::Profiling);
}

// PerformanceTelemetryPacket implementation
size_t PerformanceTelemetryPacket::get_packet_size() const {
    size_t size = sizeof(timestamp_ns) + sizeof(sequence_number) + 
                  sizeof(platform_id) + sizeof(version) + sizeof(checksum);
    
    // Counter samples
    size += sizeof(size_t); // Count
    size += counter_samples.size() * sizeof(PerformanceCounterSample);
    
    // Memory data
    size += sizeof(MemoryPerformanceData);
    
    // Frame data
    size += sizeof(FramePerformanceData);
    
    // Alerts
    size += sizeof(size_t); // Count
    for (const auto& alert : alerts) {
        size += sizeof(PerformanceAlert);
        size += alert.message.size();
    }
    
    return size;
}

bool PerformanceTelemetryPacket::is_valid() const {
    // Simple checksum validation (would be more sophisticated in practice)
    uint32_t calculated_checksum = 0;
    calculated_checksum ^= static_cast<uint32_t>(timestamp_ns);
    calculated_checksum ^= sequence_number;
    calculated_checksum ^= platform_id;
    calculated_checksum ^= version;
    
    return calculated_checksum == checksum;
}

namespace performance {

const char* to_string(PerformanceCounterType type) {
    switch (type) {
        // CPU Counters
        case PerformanceCounterType::CPUCycles: return "CPUCycles";
        case PerformanceCounterType::Instructions: return "Instructions";
        case PerformanceCounterType::CacheMisses: return "CacheMisses";
        case PerformanceCounterType::CacheHits: return "CacheHits";
        case PerformanceCounterType::BranchMisses: return "BranchMisses";
        case PerformanceCounterType::BranchHits: return "BranchHits";
        
        // Memory Counters
        case PerformanceCounterType::MemoryReads: return "MemoryReads";
        case PerformanceCounterType::MemoryWrites: return "MemoryWrites";
        case PerformanceCounterType::MemoryBandwidth: return "MemoryBandwidth";
        case PerformanceCounterType::PageFaults: return "PageFaults";
        case PerformanceCounterType::TLBMisses: return "TLBMisses";
        
        // GPU Counters
        case PerformanceCounterType::GPUCycles: return "GPUCycles";
        case PerformanceCounterType::GPUVerticesProcessed: return "GPUVerticesProcessed";
        case PerformanceCounterType::GPUPixelsDrawn: return "GPUPixelsDrawn";
        case PerformanceCounterType::GPUTextureReads: return "GPUTextureReads";
        case PerformanceCounterType::GPUShaderInvocations: return "GPUShaderInvocations";
        
        // System Counters
        case PerformanceCounterType::InterruptCount: return "InterruptCount";
        case PerformanceCounterType::ContextSwitches: return "ContextSwitches";
        case PerformanceCounterType::SystemCalls: return "SystemCalls";
        case PerformanceCounterType::IOOperations: return "IOOperations";
        
        // Platform-Specific
        case PerformanceCounterType::BatteryLevel: return "BatteryLevel";
        case PerformanceCounterType::ThermalState: return "ThermalState";
        case PerformanceCounterType::PowerConsumption: return "PowerConsumption";
        case PerformanceCounterType::NetworkLatency: return "NetworkLatency";
        
        // Software Counters
        case PerformanceCounterType::FrameRate: return "FrameRate";
        case PerformanceCounterType::FrameTime: return "FrameTime";
        case PerformanceCounterType::MemoryAllocations: return "MemoryAllocations";
        case PerformanceCounterType::MemoryDeallocations: return "MemoryDeallocations";
        case PerformanceCounterType::DriverOperations: return "DriverOperations";
        
        // Custom counters
        case PerformanceCounterType::CustomCounter1: return "CustomCounter1";
        case PerformanceCounterType::CustomCounter2: return "CustomCounter2";
        case PerformanceCounterType::CustomCounter3: return "CustomCounter3";
        case PerformanceCounterType::CustomCounter4: return "CustomCounter4";
        
        default: return "Unknown";
    }
}

const char* to_string(MonitoringMode mode) {
    switch (mode) {
        case MonitoringMode::Disabled: return "Disabled";
        case MonitoringMode::Minimal: return "Minimal";
        case MonitoringMode::Standard: return "Standard";
        case MonitoringMode::Detailed: return "Detailed";
        case MonitoringMode::Profiling: return "Profiling";
        case MonitoringMode::Custom: return "Custom";
        default: return "Unknown";
    }
}

const char* to_string(CollectionStrategy strategy) {
    switch (strategy) {
        case CollectionStrategy::Polling: return "Polling";
        case CollectionStrategy::EventDriven: return "EventDriven";
        case CollectionStrategy::Continuous: return "Continuous";
        case CollectionStrategy::OnDemand: return "OnDemand";
        case CollectionStrategy::Streaming: return "Streaming";
        case CollectionStrategy::Batch: return "Batch";
        default: return "Unknown";
    }
}

const char* to_string(PerformanceAlertLevel level) {
    switch (level) {
        case PerformanceAlertLevel::Info: return "Info";
        case PerformanceAlertLevel::Warning: return "Warning";
        case PerformanceAlertLevel::Critical: return "Critical";
        case PerformanceAlertLevel::Emergency: return "Emergency";
        default: return "Unknown";
    }
}

uint64_t estimate_overhead(const PerformanceMonitoringConfig& config,
                          const PlatformPerformanceCapabilities& platform_capabilities) {
    uint64_t base_overhead = platform_capabilities.counter_read_overhead_ns;
    uint64_t total_overhead = 0;
    
    // Calculate overhead per counter
    size_t counter_count = config.enabled_counters.size();
    total_overhead += base_overhead * counter_count;
    
    // Add strategy-specific overhead
    switch (config.strategy) {
        case CollectionStrategy::Polling:
            total_overhead += 1000; // Polling loop overhead
            break;
        case CollectionStrategy::EventDriven:
            total_overhead += 500; // Event handling overhead
            break;
        case CollectionStrategy::Continuous:
            total_overhead += 2000; // Continuous sampling overhead
            break;
        case CollectionStrategy::OnDemand:
            total_overhead += 100; // Minimal overhead
            break;
        case CollectionStrategy::Streaming:
            total_overhead += 5000; // Network/telemetry overhead
            break;
        case CollectionStrategy::Batch:
            total_overhead += 800; // Batching overhead
            break;
    }
    
    // Add alert processing overhead if enabled
    if (config.enable_alerts) {
        total_overhead += 300;
    }
    
    // Add telemetry streaming overhead if enabled
    if (config.enable_telemetry_streaming) {
        total_overhead += 2000;
    }
    
    return total_overhead;
}

PerformanceMonitoringConfig create_optimal_config(const PlatformPerformanceCapabilities& platform_capabilities,
                                                  MonitoringMode target_mode) {
    PerformanceMonitoringConfig config = PerformanceMonitoringConfig::for_mode(target_mode);
    
    // Adjust based on platform capabilities
    if (platform_capabilities.max_supported_mode < target_mode) {
        config = PerformanceMonitoringConfig::for_mode(platform_capabilities.max_supported_mode);
    }
    
    // Adjust collection interval based on platform limits
    if (config.collection_interval_ms < platform_capabilities.min_collection_interval_ms) {
        config.collection_interval_ms = platform_capabilities.min_collection_interval_ms;
    }
    
    // Adjust history size based on platform limits
    if (config.history_size > platform_capabilities.max_history_size) {
        config.history_size = platform_capabilities.max_history_size;
    }
    
    // Filter counters to only include supported ones
    std::vector<PerformanceCounterType> supported_counters;
    for (const auto& counter : config.enabled_counters) {
        if (platform_capabilities.supports_counter(counter)) {
            supported_counters.push_back(counter);
        }
    }
    config.enabled_counters = supported_counters;
    
    // Disable features not supported by platform
    if (!platform_capabilities.supports_memory_tracking) {
        // Remove memory-related counters
        config.enabled_counters.erase(
            std::remove_if(config.enabled_counters.begin(), config.enabled_counters.end(),
                [](PerformanceCounterType type) {
                    return type == PerformanceCounterType::MemoryAllocations ||
                           type == PerformanceCounterType::MemoryDeallocations ||
                           type == PerformanceCounterType::MemoryReads ||
                           type == PerformanceCounterType::MemoryWrites ||
                           type == PerformanceCounterType::MemoryBandwidth;
                }),
            config.enabled_counters.end());
    }
    
    if (!platform_capabilities.supports_frame_timing) {
        // Remove frame-related counters
        config.enabled_counters.erase(
            std::remove_if(config.enabled_counters.begin(), config.enabled_counters.end(),
                [](PerformanceCounterType type) {
                    return type == PerformanceCounterType::FrameRate ||
                           type == PerformanceCounterType::FrameTime;
                }),
            config.enabled_counters.end());
    }
    
    return config;
}

HALResult<void> validate_config(const PerformanceMonitoringConfig& config,
                               const PlatformPerformanceCapabilities& platform_capabilities) {
    // Check if monitoring mode is supported
    if (config.mode > platform_capabilities.max_supported_mode) {
        return HALResult<void>::error(errors::invalid_parameter(1, 
            "Monitoring mode not supported by platform"));
    }
    
    // Check collection interval
    if (config.collection_interval_ms > 0 && 
        config.collection_interval_ms < platform_capabilities.min_collection_interval_ms) {
        return HALResult<void>::error(errors::invalid_parameter(2, 
            "Collection interval too small for platform"));
    }
    
    // Check history size
    if (config.history_size > platform_capabilities.max_history_size) {
        return HALResult<void>::error(errors::invalid_parameter(3, 
            "History size exceeds platform limit"));
    }
    
    // Check counter support
    for (const auto& counter : config.enabled_counters) {
        if (!platform_capabilities.supports_counter(counter)) {
            return HALResult<void>::error(errors::feature_not_supported(1, 
                "Counter not supported by platform"));
        }
    }
    
    // Check feature support
    if (config.enable_telemetry_streaming && !platform_capabilities.supports_hardware_counters) {
        return HALResult<void>::error(errors::feature_not_supported(2, 
            "Telemetry streaming requires hardware counter support"));
    }
    
    return HALResult<void>::success();
}

double calculate_health_score(const SystemPerformanceSnapshot& snapshot,
                             const PerformanceMonitoringConfig& thresholds) {
    double score = 1.0;
    
    // CPU usage scoring
    if (snapshot.overall_cpu_usage > thresholds.cpu_alert_threshold) {
        score -= 0.3 * (snapshot.overall_cpu_usage - thresholds.cpu_alert_threshold) / 
                      (100.0 - thresholds.cpu_alert_threshold);
    }
    
    // Memory usage scoring
    if (snapshot.overall_memory_usage > thresholds.memory_alert_threshold) {
        score -= 0.3 * (snapshot.overall_memory_usage - thresholds.memory_alert_threshold) / 
                      (100.0 - thresholds.memory_alert_threshold);
    }
    
    // Frame rate scoring
    if (snapshot.frame.average_fps < thresholds.frame_rate_alert_threshold) {
        score -= 0.4 * (thresholds.frame_rate_alert_threshold - snapshot.frame.average_fps) / 
                      thresholds.frame_rate_alert_threshold;
    }
    
    return std::max(0.0, std::min(1.0, score));
}

std::vector<PerformanceAlert> detect_anomalies(const SystemPerformanceSnapshot& current_snapshot,
                                              const std::vector<SystemPerformanceSnapshot>& historical_data,
                                              double sensitivity) {
    std::vector<PerformanceAlert> anomalies;
    
    if (historical_data.size() < 10) {
        // Not enough historical data for anomaly detection
        return anomalies;
    }
    
    // Calculate statistical baselines
    double avg_cpu = 0.0, avg_memory = 0.0, avg_fps = 0.0;
    for (const auto& snapshot : historical_data) {
        avg_cpu += snapshot.overall_cpu_usage;
        avg_memory += snapshot.overall_memory_usage;
        avg_fps += snapshot.frame.average_fps;
    }
    avg_cpu /= historical_data.size();
    avg_memory /= historical_data.size();
    avg_fps /= historical_data.size();
    
    // Calculate standard deviations
    double cpu_variance = 0.0, memory_variance = 0.0, fps_variance = 0.0;
    for (const auto& snapshot : historical_data) {
        cpu_variance += std::pow(snapshot.overall_cpu_usage - avg_cpu, 2);
        memory_variance += std::pow(snapshot.overall_memory_usage - avg_memory, 2);
        fps_variance += std::pow(snapshot.frame.average_fps - avg_fps, 2);
    }
    cpu_variance /= historical_data.size();
    memory_variance /= historical_data.size();
    fps_variance /= historical_data.size();
    
    double cpu_stddev = std::sqrt(cpu_variance);
    double memory_stddev = std::sqrt(memory_variance);
    double fps_stddev = std::sqrt(fps_variance);
    
    // Detect anomalies based on z-score
    double z_threshold = 2.0 + (1.0 - sensitivity) * 2.0; // Higher sensitivity = lower threshold
    
    // CPU anomaly
    double cpu_z_score = std::abs(current_snapshot.overall_cpu_usage - avg_cpu) / cpu_stddev;
    if (cpu_z_score > z_threshold) {
        PerformanceAlert alert;
        alert.timestamp_ns = current_snapshot.timestamp_ns;
        alert.level = cpu_z_score > 3.0 ? PerformanceAlertLevel::Critical : PerformanceAlertLevel::Warning;
        alert.counter_type = PerformanceCounterType::CPUCycles;
        alert.message = "CPU usage anomaly detected";
        alert.threshold_value = avg_cpu + z_threshold * cpu_stddev;
        alert.actual_value = current_snapshot.overall_cpu_usage;
        alert.alert_id = static_cast<uint32_t>(std::hash<uint64_t>{}(current_snapshot.timestamp_ns));
        alert.is_resolved = false;
        anomalies.push_back(alert);
    }
    
    // Memory anomaly
    double memory_z_score = std::abs(current_snapshot.overall_memory_usage - avg_memory) / memory_stddev;
    if (memory_z_score > z_threshold) {
        PerformanceAlert alert;
        alert.timestamp_ns = current_snapshot.timestamp_ns;
        alert.level = memory_z_score > 3.0 ? PerformanceAlertLevel::Critical : PerformanceAlertLevel::Warning;
        alert.counter_type = PerformanceCounterType::MemoryAllocations;
        alert.message = "Memory usage anomaly detected";
        alert.threshold_value = avg_memory + z_threshold * memory_stddev;
        alert.actual_value = current_snapshot.overall_memory_usage;
        alert.alert_id = static_cast<uint32_t>(std::hash<uint64_t>{}(current_snapshot.timestamp_ns + 1));
        alert.is_resolved = false;
        anomalies.push_back(alert);
    }
    
    // Frame rate anomaly (lower is worse for FPS)
    double fps_z_score = (avg_fps - current_snapshot.frame.average_fps) / fps_stddev;
    if (fps_z_score > z_threshold) {
        PerformanceAlert alert;
        alert.timestamp_ns = current_snapshot.timestamp_ns;
        alert.level = fps_z_score > 3.0 ? PerformanceAlertLevel::Critical : PerformanceAlertLevel::Warning;
        alert.counter_type = PerformanceCounterType::FrameRate;
        alert.message = "Frame rate performance degradation detected";
        alert.threshold_value = avg_fps - z_threshold * fps_stddev;
        alert.actual_value = current_snapshot.frame.average_fps;
        alert.alert_id = static_cast<uint32_t>(std::hash<uint64_t>{}(current_snapshot.timestamp_ns + 2));
        alert.is_resolved = false;
        anomalies.push_back(alert);
    }
    
    return anomalies;
}

double normalize_counter_value(PerformanceCounterType counter_type,
                              uint64_t raw_value,
                              const PlatformPerformanceCapabilities& platform_capabilities) {
    // Platform-specific normalization (simplified)
    switch (counter_type) {
        case PerformanceCounterType::CPUCycles:
            // Normalize based on typical CPU frequency
            return std::min(1.0, static_cast<double>(raw_value) / 1000000000.0); // 1GHz reference
            
        case PerformanceCounterType::MemoryBandwidth:
            // Normalize based on typical memory bandwidth
            return std::min(1.0, static_cast<double>(raw_value) / (1024 * 1024 * 1024)); // 1GB/s reference
            
        case PerformanceCounterType::FrameRate:
            // Normalize to 60 FPS as reference
            return std::min(1.0, static_cast<double>(raw_value) / 60.0);
            
        case PerformanceCounterType::BatteryLevel:
            // Already a percentage
            return static_cast<double>(raw_value) / 100.0;
            
        default:
            // Default: assume already normalized or use simple scaling
            return std::min(1.0, static_cast<double>(raw_value) / 1000.0);
    }
}

std::vector<PerformanceCounterType> get_minimal_counter_set(const PlatformPerformanceCapabilities& platform_capabilities) {
    std::vector<PerformanceCounterType> minimal_set = {
        PerformanceCounterType::FrameRate,
        PerformanceCounterType::MemoryAllocations
    };
    
    // Filter based on platform support
    std::vector<PerformanceCounterType> supported_set;
    for (const auto& counter : minimal_set) {
        if (platform_capabilities.supports_counter(counter)) {
            supported_set.push_back(counter);
        }
    }
    
    return supported_set;
}

std::vector<PerformanceCounterType> get_comprehensive_counter_set(const PlatformPerformanceCapabilities& platform_capabilities) {
    return platform_capabilities.available_counters;
}

size_t calculate_telemetry_overhead(const PerformanceTelemetryPacket& packet) {
    return packet.get_packet_size() + 64; // 64 bytes for network/transport overhead
}

std::vector<uint8_t> compress_performance_data(const SystemPerformanceSnapshot& snapshot,
                                              int compression_level) {
    // Simplified compression (in practice would use zlib, lz4, etc.)
    std::vector<uint8_t> compressed_data;
    
    // For now, just serialize the data (placeholder for real compression)
    const uint8_t* data_ptr = reinterpret_cast<const uint8_t*>(&snapshot);
    compressed_data.assign(data_ptr, data_ptr + sizeof(SystemPerformanceSnapshot));
    
    return compressed_data;
}

HALResult<SystemPerformanceSnapshot> decompress_performance_data(const std::vector<uint8_t>& compressed_data) {
    // Simplified decompression (placeholder for real decompression)
    if (compressed_data.size() < sizeof(SystemPerformanceSnapshot)) {
        return HALResult<SystemPerformanceSnapshot>::error(errors::validation_failed(1, "Compressed data too small"));
    }
    
    SystemPerformanceSnapshot snapshot;
    std::memcpy(&snapshot, compressed_data.data(), sizeof(SystemPerformanceSnapshot));
    
    return HALResult<SystemPerformanceSnapshot>::success(std::move(snapshot));
}

} // namespace performance

} // namespace flight::hal
