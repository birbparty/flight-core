#pragma once

#include <memory>
#include <optional>
#include <variant>
#include <string>
#include <vector>

namespace flight::hal::macos {

/// System information for Apple Silicon M4 Max
struct SystemInfo {
    std::string cpu_model;
    uint32_t performance_cores;
    uint32_t efficiency_cores;
    uint32_t gpu_cores;
    uint64_t unified_memory_size;
    uint64_t memory_bandwidth_gbps;
    bool metal_support;
    std::string macos_version;
};

/// Resource coordination result
enum class CoordinationResult {
    Success,
    PartialSuccess,
    Failed,
    NotSupported
};

/// Platform coordinator for Flight ecosystem integration
class PlatformCoordinator {
public:
    /// Create platform coordinator instance
    static std::unique_ptr<PlatformCoordinator> create();
    
    /// Destructor
    virtual ~PlatformCoordinator() = default;
    
    // System Information
    /// Get Apple Silicon M4 Max system information
    virtual std::optional<SystemInfo> getSystemInfo() const = 0;
    
    /// Check if running on Apple Silicon
    virtual bool isAppleSilicon() const = 0;
    
    /// Check if running on M4 Max specifically
    virtual bool isM4Max() const = 0;
    
    // Resource Coordination
    /// Initialize platform optimizations for Apple Silicon
    virtual CoordinationResult initializeAppleSiliconOptimizations() = 0;
    
    /// Coordinate CPU resources (16-core: 12 performance + 4 efficiency)
    virtual CoordinationResult coordinateCPUResources() = 0;
    
    /// Coordinate GPU resources (40-core GPU)
    virtual CoordinationResult coordinateGPUResources() = 0;
    
    /// Optimize unified memory bandwidth utilization (546 GB/s)
    virtual CoordinationResult optimizeMemoryBandwidth() = 0;
    
    // Flight Ecosystem Integration
    /// Initialize coordination with Flight CLI
    virtual CoordinationResult coordinateWithCLI() = 0;
    
    /// Initialize coordination with Flight Runtime
    virtual CoordinationResult coordinateWithRuntime() = 0;
    
    /// Initialize coordination with Component Flattening system
    virtual CoordinationResult coordinateWithComponentFlattening() = 0;
    
    // Performance Monitoring
    /// Get current CPU utilization across all cores
    virtual std::vector<float> getCPUUtilization() const = 0;
    
    /// Get current GPU utilization
    virtual float getGPUUtilization() const = 0;
    
    /// Get current memory pressure
    virtual float getMemoryPressure() const = 0;
    
    /// Get thermal state
    virtual std::string getThermalState() const = 0;
    
protected:
    PlatformCoordinator() = default;
    
private:
    // Non-copyable, non-movable
    PlatformCoordinator(const PlatformCoordinator&) = delete;
    PlatformCoordinator& operator=(const PlatformCoordinator&) = delete;
    PlatformCoordinator(PlatformCoordinator&&) = delete;
    PlatformCoordinator& operator=(PlatformCoordinator&&) = delete;
};

} // namespace flight::hal::macos
