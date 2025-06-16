#pragma once

#include <cstdint>
#include <string>
#include <optional>
#include <vector>

namespace flight::hal::macos {

/// Workload types for optimization recommendations
enum class WorkloadType {
    ComponentFlattening,
    AssetProcessing,
    NetworkIO,
    FileSystemIO,
    CPUIntensive,
    GPUCompute,
    Background
};

/// Task types for core assignment
enum class TaskType {
    RealTimeProcessing,
    BackgroundProcessing,
    BatchProcessing,
    UserInterface,
    SystemMaintenance
};

/// Power profile recommendations
enum class PowerProfile {
    MaxPerformance,
    Balanced,
    PowerEfficient,
    Thermal
};

/// Comprehensive platform capabilities structure
struct PlatformCapabilities {
    struct CPU {
        uint32_t performance_cores;    // M4 Max: 12 cores
        uint32_t efficiency_cores;     // M4 Max: 4 cores
        uint32_t total_cores;          // Total logical cores
        bool supports_arm64e;          // Enhanced ARM64 features
        std::string architecture;      // "arm64", "x86_64"
        float base_frequency_ghz;      // Base clock frequency
        float boost_frequency_ghz;     // Maximum boost frequency
        std::string brand_string;      // CPU brand identifier
        uint32_t cache_line_size;      // Cache line size in bytes
        uint64_t l1_cache_size;        // L1 cache size in bytes
        uint64_t l2_cache_size;        // L2 cache size in bytes
        uint64_t l3_cache_size;        // L3 cache size in bytes
    } cpu;
    
    struct Memory {
        uint64_t total_bytes;          // Total unified memory
        uint64_t bandwidth_gbps;       // M4 Max: 546 GB/s
        bool unified_architecture;     // Always true for Apple Silicon
        uint32_t memory_type;          // LPDDR5, etc.
        uint32_t page_size;            // Memory page size
        uint64_t available_bytes;      // Currently available memory
    } memory;
    
    struct GPU {
        uint32_t core_count;           // M4 Max: 40 cores
        bool metal_support;            // Metal API availability
        std::string family;            // "Apple8", "Apple9", etc.
        uint64_t memory_shared_mb;     // Shared with system memory
        bool supports_raytracing;     // Hardware ray tracing support
        bool supports_mesh_shading;   // Mesh shading support
        bool supports_function_pointers; // Function pointer support
        std::string metal_version;     // Metal version string
    } gpu;
    
    struct System {
        std::string macos_version;     // "14.2.1", etc.
        std::string chip_identifier;  // "M4 Max", "M3 Pro", etc.
        std::string model_identifier;  // "Mac15,13", etc.
        bool xcode_available;          // Development tools present
        bool instruments_available;    // Profiling tools present
        bool sandboxed;               // App sandbox restrictions
        bool sip_enabled;             // System Integrity Protection
        std::string xcode_version;     // Xcode version if available
        bool command_line_tools;       // Command line tools installed
    } system;
};

/// Optimization profile for specific workloads
struct OptimizationProfile {
    uint32_t recommended_worker_threads;
    bool use_efficiency_cores_for_background;
    size_t optimal_memory_pool_size_mb;
    bool enable_metal_acceleration;
    bool enable_advanced_simd;
    PowerProfile recommended_power_profile;
    float cpu_usage_target;           // Target CPU usage (0.0-1.0)
    bool enable_thermal_management;
};

/// Resource constraints for the current system
struct ResourceConstraints {
    uint32_t max_concurrent_threads;
    uint64_t max_memory_allocation_mb;
    bool can_use_metal_compute;
    bool can_use_performance_cores;
    bool can_use_efficiency_cores;
    float thermal_headroom;          // 0.0-1.0, 1.0 = no thermal constraints
};

/// Development environment information
struct DevelopmentEnvironment {
    bool has_xcode;
    bool has_command_line_tools;
    bool has_instruments;
    bool has_simulator;
    std::string xcode_path;
    std::string xcode_version;
    std::vector<std::string> available_sdks;
    bool code_signing_available;
};

/// Platform detector for comprehensive Apple Silicon analysis
class PlatformDetector {
public:
    // Primary detection interface
    static PlatformCapabilities detect_capabilities();
    
    // Convenience methods for specific checks
    static bool is_development_environment();
    static std::optional<std::string> xcode_path();
    static bool supports_metal_performance_shaders();
    
    // Performance optimization queries
    static uint32_t optimal_thread_count_for_workload(WorkloadType type);
    static bool should_use_efficiency_cores_for(TaskType task);
    static size_t recommended_memory_pool_size();
    
    // Capability-based feature detection
    static bool supports_advanced_simd();
    static bool supports_amx_instructions();
    static bool supports_neural_engine();
    
    // Apple Silicon variant detection
    static bool is_apple_silicon();
    static bool is_m1_family();
    static bool is_m2_family(); 
    static bool is_m3_family();
    static bool is_m4_family();
    static bool is_m4_max();
    
    // Optimization helpers
    static OptimizationProfile get_optimization_profile(WorkloadType workload);
    static ResourceConstraints get_resource_constraints();
    static DevelopmentEnvironment get_development_environment();
    
    // Cache management
    static void invalidate_cache();
    static bool is_cache_valid();
    
private:
    // Internal detection methods
    static PlatformCapabilities detect_cpu_capabilities();
    static PlatformCapabilities::Memory detect_memory_capabilities();
    static PlatformCapabilities::GPU detect_gpu_capabilities();
    static PlatformCapabilities::System detect_system_capabilities();
    
    // Chip identification helpers
    static std::string identify_chip_from_model(const std::string& model);
    static uint64_t estimate_memory_bandwidth(const std::string& chip_id);
    static uint32_t estimate_gpu_cores(const std::string& chip_id);
    
    // Cache for expensive operations
    static std::optional<PlatformCapabilities> cached_capabilities_;
    static bool cache_valid_;
};

/// Singleton detection manager for coordinated access
class PlatformDetectionManager {
public:
    static PlatformDetectionManager& getInstance();
    
    // Core detection interface
    const PlatformCapabilities& getCapabilities();
    void invalidateCache(); // Force re-detection
    
    // Optimization helpers
    OptimizationProfile getOptimizationProfile(WorkloadType workload);
    ResourceConstraints getResourceConstraints();
    
    // Development environment utilities
    DevelopmentEnvironment getDevEnvironment();
    
    // Performance monitoring
    bool isPerformanceThrottled() const;
    float getCurrentThermalState() const;
    
private:
    PlatformDetectionManager() = default;
    PlatformCapabilities cached_capabilities_;
    bool cache_valid_ = false;
    
    // Singleton management
    static std::unique_ptr<PlatformDetectionManager> instance_;
};

} // namespace flight::hal::macos
