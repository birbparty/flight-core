#include "flight/hal/macos/platform_detector.hpp"

#ifdef __APPLE__
#include <sys/sysctl.h>
#include <sys/types.h>
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <mach/host_info.h>
#include <unistd.h>
#include <string.h>
#include <cmath>
#endif

#include <algorithm>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <future>

namespace flight::hal::macos {

// Static member initialization
std::optional<PlatformCapabilities> PlatformDetector::cached_capabilities_;
bool PlatformDetector::cache_valid_ = false;
std::unique_ptr<PlatformDetectionManager> PlatformDetectionManager::instance_;

namespace {

// Chip specifications database
struct ChipSpecs {
    uint32_t perf_cores;
    uint32_t eff_cores;
    uint32_t gpu_cores;
    uint64_t memory_bandwidth_gbps;
    std::string gpu_family;
};

static const std::unordered_map<std::string, ChipSpecs> CHIP_DATABASE = {
    {"M1", {4, 4, 8, 68, "Apple7"}},
    {"M1 Pro", {8, 2, 16, 200, "Apple7"}},
    {"M1 Max", {8, 2, 32, 400, "Apple7"}},
    {"M1 Ultra", {16, 4, 64, 800, "Apple7"}},
    {"M2", {4, 4, 10, 100, "Apple8"}},
    {"M2 Pro", {8, 4, 19, 200, "Apple8"}},
    {"M2 Max", {8, 4, 38, 400, "Apple8"}},
    {"M2 Ultra", {16, 8, 76, 800, "Apple8"}},
    {"M3", {4, 4, 10, 100, "Apple9"}},
    {"M3 Pro", {6, 6, 18, 150, "Apple9"}},
    {"M3 Max", {8, 4, 40, 400, "Apple9"}},
    {"M4", {4, 6, 10, 120, "Apple10"}},
    {"M4 Pro", {10, 4, 20, 273, "Apple10"}},
    {"M4 Max", {12, 4, 40, 546, "Apple10"}}
};

#ifdef __APPLE__
// Helper function to get sysctl string value
std::string get_sysctl_string(const char* name) {
    size_t size = 0;
    if (sysctlbyname(name, nullptr, &size, nullptr, 0) != 0) {
        return "";
    }
    
    std::string result(size, '\0');
    if (sysctlbyname(name, result.data(), &size, nullptr, 0) != 0) {
        return "";
    }
    
    // Remove null terminator if present
    if (!result.empty() && result.back() == '\0') {
        result.pop_back();
    }
    
    return result;
}

// Helper function to get sysctl integer value
template<typename T>
T get_sysctl_value(const char* name, T default_value = T{}) {
    T value = default_value;
    size_t size = sizeof(value);
    if (sysctlbyname(name, &value, &size, nullptr, 0) != 0) {
        return default_value;
    }
    return value;
}

// Helper function to get memory info
uint64_t get_memory_info(int selector) {
    vm_statistics64_data_t vm_stat;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    
    if (host_statistics64(mach_host_self(), HOST_VM_INFO64, 
                         (host_info64_t)&vm_stat, &count) != KERN_SUCCESS) {
        return 0;
    }
    
    // Use getpagesize() instead of vm_page_size to avoid naming conflicts
    uint64_t page_sz = static_cast<uint64_t>(getpagesize());
    
    switch (selector) {
        case 0: // Available memory
            return static_cast<uint64_t>(vm_stat.free_count + vm_stat.inactive_count) * page_sz;
        case 1: // Page size
            return page_sz;
        default:
            return 0;
    }
}
#endif

} // anonymous namespace

// Main detection function
PlatformCapabilities PlatformDetector::detect_capabilities() {
    static std::mutex detection_mutex;
    std::lock_guard<std::mutex> lock(detection_mutex);
    
    if (cache_valid_ && cached_capabilities_.has_value()) {
        return cached_capabilities_.value();
    }
    
    PlatformCapabilities caps = {};
    
#ifdef __APPLE__
    // Detect CPU capabilities
    PlatformCapabilities cpu_caps = detect_cpu_capabilities();
    caps.cpu = cpu_caps.cpu;
    
    // Detect memory capabilities
    caps.memory = detect_memory_capabilities();
    
    // Detect GPU capabilities (will be completed in the Objective-C++ file)
    caps.gpu = detect_gpu_capabilities();
    
    // Detect system capabilities
    caps.system = detect_system_capabilities();
    
    // Cache the results
    cached_capabilities_ = caps;
    cache_valid_ = true;
#endif
    
    return caps;
}

PlatformCapabilities PlatformDetector::detect_cpu_capabilities() {
    PlatformCapabilities caps = {};
    
#ifdef __APPLE__
    // Get CPU brand string
    caps.cpu.brand_string = get_sysctl_string("machdep.cpu.brand_string");
    
    // Get architecture
    caps.cpu.architecture = get_sysctl_string("hw.targettype");
    if (caps.cpu.architecture.empty()) {
        caps.cpu.architecture = "unknown";
    }
    
    // Detect performance and efficiency cores
    caps.cpu.performance_cores = get_sysctl_value<uint32_t>("hw.perflevel0.physicalcpu", 0);
    caps.cpu.efficiency_cores = get_sysctl_value<uint32_t>("hw.perflevel1.physicalcpu", 0);
    
    // If the perflevel sysctls aren't available, fall back to detecting cores differently
    if (caps.cpu.performance_cores == 0 && caps.cpu.efficiency_cores == 0) {
        uint32_t total_physical = get_sysctl_value<uint32_t>("hw.physicalcpu", 0);
        
        // For older detection methods, make reasonable assumptions
        if (total_physical > 0) {
            // Estimate based on common Apple Silicon configurations
            if (total_physical <= 8) {
                caps.cpu.performance_cores = total_physical / 2;
                caps.cpu.efficiency_cores = total_physical / 2;
            } else {
                caps.cpu.performance_cores = (total_physical * 2) / 3;
                caps.cpu.efficiency_cores = total_physical / 3;
            }
        }
    }
    
    caps.cpu.total_cores = caps.cpu.performance_cores + caps.cpu.efficiency_cores;
    
    // Get frequencies (approximate values)
    caps.cpu.base_frequency_ghz = get_sysctl_value<uint64_t>("hw.cpufrequency_max", 0) / 1000000000.0f;
    caps.cpu.boost_frequency_ghz = caps.cpu.base_frequency_ghz * 1.2f; // Estimate boost
    
    // Cache information
    caps.cpu.cache_line_size = get_sysctl_value<uint32_t>("hw.cachelinesize", 64);
    caps.cpu.l1_cache_size = get_sysctl_value<uint64_t>("hw.l1dcachesize", 0);
    caps.cpu.l2_cache_size = get_sysctl_value<uint64_t>("hw.l2cachesize", 0);
    caps.cpu.l3_cache_size = get_sysctl_value<uint64_t>("hw.l3cachesize", 0);
    
    // ARM64 feature detection
    caps.cpu.supports_arm64e = caps.cpu.architecture.find("arm64") != std::string::npos;
#endif
    
    return caps;
}

PlatformCapabilities::Memory PlatformDetector::detect_memory_capabilities() {
    PlatformCapabilities::Memory memory = {};
    
#ifdef __APPLE__
    // Get total memory
    memory.total_bytes = get_sysctl_value<uint64_t>("hw.memsize", 0);
    
    // Get available memory
    memory.available_bytes = get_memory_info(0);
    
    // Get page size
    memory.page_size = static_cast<uint32_t>(get_memory_info(1));
    
    // Apple Silicon always has unified memory
    memory.unified_architecture = is_apple_silicon();
    
    // Memory type (LPDDR5 for recent Apple Silicon)
    memory.memory_type = 5; // LPDDR5
    
    // Estimate memory bandwidth based on chip identification
    std::string model = get_sysctl_string("hw.model");
    std::string chip_id = identify_chip_from_model(model);
    memory.bandwidth_gbps = estimate_memory_bandwidth(chip_id);
#endif
    
    return memory;
}

PlatformCapabilities::GPU PlatformDetector::detect_gpu_capabilities() {
    PlatformCapabilities::GPU gpu = {};
    
#ifdef __APPLE__
    // Basic GPU detection (Metal detection will be implemented in .mm file)
    gpu.metal_support = true; // All Apple Silicon supports Metal
    
    // Get chip identification for GPU core estimation
    std::string model = get_sysctl_string("hw.model");
    std::string chip_id = identify_chip_from_model(model);
    gpu.core_count = estimate_gpu_cores(chip_id);
    
    // Shared memory (same as system memory for Apple Silicon)
    gpu.memory_shared_mb = get_sysctl_value<uint64_t>("hw.memsize", 0) / (1024 * 1024);
    
    // Modern Apple Silicon features
    gpu.supports_raytracing = is_m3_family() || is_m4_family();
    gpu.supports_mesh_shading = is_m3_family() || is_m4_family();
    gpu.supports_function_pointers = is_m3_family() || is_m4_family();
    
    // Family detection
    auto it = CHIP_DATABASE.find(chip_id);
    if (it != CHIP_DATABASE.end()) {
        gpu.family = it->second.gpu_family;
    } else {
        gpu.family = "Apple";
    }
    
    gpu.metal_version = "3.0"; // Default for modern Apple Silicon
#endif
    
    return gpu;
}

PlatformCapabilities::System PlatformDetector::detect_system_capabilities() {
    PlatformCapabilities::System system = {};
    
#ifdef __APPLE__
    // Get macOS version
    system.macos_version = get_sysctl_string("kern.version");
    
    // Get model identifier
    system.model_identifier = get_sysctl_string("hw.model");
    
    // Get chip identifier
    system.chip_identifier = identify_chip_from_model(system.model_identifier);
    
    // Development environment detection (basic implementation)
    system.xcode_available = false; // Will be implemented in development environment detection
    system.instruments_available = false;
    system.command_line_tools = false;
    
    // Security features
    system.sandboxed = false; // Simplified for now
    system.sip_enabled = true; // Assume SIP is enabled by default
#endif
    
    return system;
}

// Chip identification helper
std::string PlatformDetector::identify_chip_from_model(const std::string& model) {
    if (model.empty()) return "Unknown";
    
    // Simple pattern matching for common models
    if (model.find("Mac14,15") != std::string::npos || 
        model.find("Mac15,13") != std::string::npos) {
        return "M4 Max";
    } else if (model.find("Mac14,14") != std::string::npos ||
               model.find("Mac15,12") != std::string::npos) {
        return "M4 Pro";
    } else if (model.find("Mac15,") != std::string::npos) {
        return "M4";
    } else if (model.find("Mac14,") != std::string::npos) {
        return "M3 Max";
    } else if (model.find("Mac13,") != std::string::npos) {
        return "M2";
    } else if (model.find("Mac12,") != std::string::npos) {
        return "M1";
    }
    
    return "Unknown Apple Silicon";
}

// Memory bandwidth estimation
uint64_t PlatformDetector::estimate_memory_bandwidth(const std::string& chip_id) {
    auto it = CHIP_DATABASE.find(chip_id);
    if (it != CHIP_DATABASE.end()) {
        return it->second.memory_bandwidth_gbps;
    }
    return 100; // Conservative default
}

// GPU core estimation
uint32_t PlatformDetector::estimate_gpu_cores(const std::string& chip_id) {
    auto it = CHIP_DATABASE.find(chip_id);
    if (it != CHIP_DATABASE.end()) {
        return it->second.gpu_cores;
    }
    return 8; // Conservative default
}

// Apple Silicon detection
bool PlatformDetector::is_apple_silicon() {
#ifdef __APPLE__
    // Check if running under Rosetta
    int is_translated = 0;
    size_t size = sizeof(is_translated);
    if (sysctlbyname("sysctl.proc_translated", &is_translated, &size, nullptr, 0) == 0) {
        return is_translated == 0;
    }
    
    // Fallback: check architecture
    std::string arch = get_sysctl_string("hw.targettype");
    return arch.find("arm") != std::string::npos;
#else
    return false;
#endif
}

// M-series family detection
bool PlatformDetector::is_m1_family() {
    auto caps = detect_capabilities();
    return caps.system.chip_identifier.find("M1") != std::string::npos;
}

bool PlatformDetector::is_m2_family() {
    auto caps = detect_capabilities();
    return caps.system.chip_identifier.find("M2") != std::string::npos;
}

bool PlatformDetector::is_m3_family() {
    auto caps = detect_capabilities();
    return caps.system.chip_identifier.find("M3") != std::string::npos;
}

bool PlatformDetector::is_m4_family() {
    auto caps = detect_capabilities();
    return caps.system.chip_identifier.find("M4") != std::string::npos;
}

bool PlatformDetector::is_m4_max() {
    auto caps = detect_capabilities();
    return caps.system.chip_identifier == "M4 Max";
}

// Optimization recommendations
uint32_t PlatformDetector::optimal_thread_count_for_workload(WorkloadType type) {
    auto caps = detect_capabilities();
    
    switch (type) {
        case WorkloadType::ComponentFlattening:
            // Use mostly performance cores for component flattening
            return caps.cpu.performance_cores + (caps.cpu.efficiency_cores / 2);
        
        case WorkloadType::CPUIntensive:
            // Use all performance cores
            return caps.cpu.performance_cores;
            
        case WorkloadType::Background:
            // Prefer efficiency cores for background tasks
            return caps.cpu.efficiency_cores;
            
        case WorkloadType::AssetProcessing:
            // Use all cores for asset processing
            return caps.cpu.total_cores;
            
        case WorkloadType::NetworkIO:
        case WorkloadType::FileSystemIO:
            // IO-bound tasks don't need many threads
            return std::min(caps.cpu.total_cores, 4u);
            
        case WorkloadType::GPUCompute:
            // Minimal CPU threads for GPU compute coordination
            return 2;
            
        default:
            return caps.cpu.performance_cores;
    }
}

bool PlatformDetector::should_use_efficiency_cores_for(TaskType task) {
    switch (task) {
        case TaskType::BackgroundProcessing:
        case TaskType::SystemMaintenance:
            return true;
            
        case TaskType::RealTimeProcessing:
        case TaskType::UserInterface:
            return false;
            
        case TaskType::BatchProcessing:
            return false; // Use performance cores for batch processing
            
        default:
            return false;
    }
}

size_t PlatformDetector::recommended_memory_pool_size() {
    auto caps = detect_capabilities();
    
    // Recommend 1/8 of available memory for memory pools
    return (caps.memory.available_bytes / 8) / (1024 * 1024); // Return in MB
}

// Feature support detection
bool PlatformDetector::supports_advanced_simd() {
    return is_apple_silicon();
}

bool PlatformDetector::supports_amx_instructions() {
    // AMX (Apple Matrix) instructions available on M1 and later
    return is_apple_silicon();
}

bool PlatformDetector::supports_neural_engine() {
    // Neural Engine available on all Apple Silicon
    return is_apple_silicon();
}

bool PlatformDetector::supports_metal_performance_shaders() {
    auto caps = detect_capabilities();
    return caps.gpu.metal_support;
}

// Cache management
void PlatformDetector::invalidate_cache() {
    cache_valid_ = false;
    cached_capabilities_.reset();
}

bool PlatformDetector::is_cache_valid() {
    return cache_valid_;
}

// Development environment detection (basic implementation)
bool PlatformDetector::is_development_environment() {
    // Simple check for development environment
    return false; // Will be implemented in the .mm file
}

std::optional<std::string> PlatformDetector::xcode_path() {
    // Will be implemented in the .mm file
    return std::nullopt;
}

// Optimization profile generation
OptimizationProfile PlatformDetector::get_optimization_profile(WorkloadType workload) {
    auto caps = detect_capabilities();
    
    OptimizationProfile profile = {};
    profile.recommended_worker_threads = optimal_thread_count_for_workload(workload);
    profile.use_efficiency_cores_for_background = true;
    profile.enable_metal_acceleration = caps.gpu.metal_support;
    profile.enable_advanced_simd = supports_advanced_simd();
    profile.enable_thermal_management = true;
    
    switch (workload) {
        case WorkloadType::ComponentFlattening:
            profile.optimal_memory_pool_size_mb = 512;
            profile.recommended_power_profile = PowerProfile::MaxPerformance;
            profile.cpu_usage_target = 0.8f;
            break;
            
        case WorkloadType::Background:
            profile.optimal_memory_pool_size_mb = 64;
            profile.recommended_power_profile = PowerProfile::PowerEfficient;
            profile.cpu_usage_target = 0.3f;
            break;
            
        default:
            profile.optimal_memory_pool_size_mb = 256;
            profile.recommended_power_profile = PowerProfile::Balanced;
            profile.cpu_usage_target = 0.6f;
            break;
    }
    
    return profile;
}

ResourceConstraints PlatformDetector::get_resource_constraints() {
    auto caps = detect_capabilities();
    
    ResourceConstraints constraints = {};
    constraints.max_concurrent_threads = caps.cpu.total_cores * 2; // Allow hyperthreading
    constraints.max_memory_allocation_mb = caps.memory.available_bytes / (1024 * 1024 * 2); // Half of available
    constraints.can_use_metal_compute = caps.gpu.metal_support;
    constraints.can_use_performance_cores = caps.cpu.performance_cores > 0;
    constraints.can_use_efficiency_cores = caps.cpu.efficiency_cores > 0;
    constraints.thermal_headroom = 1.0f; // Assume full thermal headroom initially
    
    return constraints;
}

DevelopmentEnvironment PlatformDetector::get_development_environment() {
    // Basic implementation - will be enhanced in .mm file
    DevelopmentEnvironment env = {};
    env.has_xcode = false;
    env.has_command_line_tools = false;
    env.has_instruments = false;
    env.has_simulator = false;
    env.code_signing_available = false;
    
    return env;
}

// Singleton implementation
PlatformDetectionManager& PlatformDetectionManager::getInstance() {
    static std::unique_ptr<PlatformDetectionManager> instance(new PlatformDetectionManager());
    return *instance;
}

const PlatformCapabilities& PlatformDetectionManager::getCapabilities() {
    if (!cache_valid_) {
        cached_capabilities_ = PlatformDetector::detect_capabilities();
        cache_valid_ = true;
    }
    return cached_capabilities_;
}

void PlatformDetectionManager::invalidateCache() {
    cache_valid_ = false;
    PlatformDetector::invalidate_cache();
}

OptimizationProfile PlatformDetectionManager::getOptimizationProfile(WorkloadType workload) {
    return PlatformDetector::get_optimization_profile(workload);
}

ResourceConstraints PlatformDetectionManager::getResourceConstraints() {
    return PlatformDetector::get_resource_constraints();
}

DevelopmentEnvironment PlatformDetectionManager::getDevEnvironment() {
    return PlatformDetector::get_development_environment();
}

bool PlatformDetectionManager::isPerformanceThrottled() const {
    // Simplified implementation
    return false;
}

float PlatformDetectionManager::getCurrentThermalState() const {
    // Simplified implementation - returns normal state
    return 0.0f; // 0.0 = normal, 1.0 = critical
}

} // namespace flight::hal::macos
