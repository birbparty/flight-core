/**
 * @file hardware_detection.hpp
 * @brief Runtime Hardware Feature Detection System
 * 
 * Provides comprehensive runtime detection of hardware capabilities
 * across CPU, GPU, memory, and I/O subsystems for accurate capability
 * reporting and performance optimization.
 */

#pragma once

#include "hal_capabilities.hpp"
#include "hal_result.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>

namespace flight::hal {

/**
 * @brief CPU feature detection results
 */
struct CPUFeatures {
    // Architecture information
    std::string vendor;           ///< CPU vendor (Intel, AMD, ARM, etc.)
    std::string model;            ///< CPU model name
    std::string architecture;     ///< Architecture (x86_64, ARM64, SH-4, etc.)
    uint32_t cores;              ///< Number of physical cores
    uint32_t threads;            ///< Number of logical threads
    uint32_t cache_line_size;    ///< Cache line size in bytes
    
    // Feature flags
    bool has_fpu;                ///< Floating-point unit
    bool has_mmx;                ///< MMX instructions
    bool has_sse;                ///< SSE instructions
    bool has_sse2;               ///< SSE2 instructions
    bool has_sse3;               ///< SSE3 instructions
    bool has_ssse3;              ///< SSSE3 instructions
    bool has_sse4_1;             ///< SSE4.1 instructions
    bool has_sse4_2;             ///< SSE4.2 instructions
    bool has_avx;                ///< AVX instructions
    bool has_avx2;               ///< AVX2 instructions
    bool has_avx512;             ///< AVX-512 instructions
    bool has_neon;               ///< ARM NEON instructions
    bool has_aes;                ///< AES acceleration
    bool has_sha;                ///< SHA acceleration
    
    // Cache information
    uint32_t l1_cache_size;      ///< L1 cache size in KB
    uint32_t l2_cache_size;      ///< L2 cache size in KB
    uint32_t l3_cache_size;      ///< L3 cache size in KB
    
    // Performance characteristics
    uint32_t base_frequency_mhz; ///< Base frequency in MHz
    uint32_t max_frequency_mhz;  ///< Maximum frequency in MHz
};

/**
 * @brief GPU feature detection results
 */
struct GPUFeatures {
    // Device information
    std::string vendor;          ///< GPU vendor (NVIDIA, AMD, Intel, etc.)
    std::string device_name;     ///< GPU device name
    std::string driver_version;  ///< Driver version
    
    // API support
    bool supports_opengl;        ///< OpenGL support
    bool supports_vulkan;        ///< Vulkan support
    bool supports_directx;       ///< DirectX support
    bool supports_metal;         ///< Metal support
    bool supports_webgl;         ///< WebGL support
    bool supports_webgl2;        ///< WebGL2 support
    
    // OpenGL capabilities
    std::string opengl_version;  ///< OpenGL version string
    std::string glsl_version;    ///< GLSL version string
    uint32_t max_texture_size;   ///< Maximum texture size
    uint32_t max_texture_units;  ///< Maximum texture units
    uint32_t max_vertex_attribs; ///< Maximum vertex attributes
    uint32_t max_varying_vectors; ///< Maximum varying vectors
    
    // Shader support
    bool supports_vertex_shaders;   ///< Vertex shader support
    bool supports_fragment_shaders; ///< Fragment shader support
    bool supports_geometry_shaders; ///< Geometry shader support
    bool supports_compute_shaders;  ///< Compute shader support
    bool supports_tessellation;     ///< Tessellation support
    
    // Memory information
    uint64_t total_memory;       ///< Total GPU memory in bytes
    uint64_t available_memory;   ///< Available GPU memory in bytes
    
    // Performance characteristics
    uint32_t shader_units;       ///< Number of shader units
    uint32_t texture_units;      ///< Number of texture units
    uint32_t render_targets;     ///< Maximum render targets
};

/**
 * @brief Memory subsystem features
 */
struct MemoryFeatures {
    // Physical memory
    uint64_t total_physical;     ///< Total physical memory in bytes
    uint64_t available_physical; ///< Available physical memory in bytes
    uint64_t total_virtual;      ///< Total virtual memory in bytes
    uint64_t available_virtual;  ///< Available virtual memory in bytes
    
    // Memory characteristics
    uint32_t page_size;          ///< Memory page size in bytes
    uint32_t allocation_granularity; ///< Allocation granularity
    bool supports_large_pages;   ///< Large page support
    bool supports_numa;          ///< NUMA support
    bool supports_dma;           ///< DMA support
    bool supports_memory_protection; ///< Memory protection support
    
    // Cache information
    uint64_t cache_line_size;    ///< Cache line size
    uint64_t l1_cache_size;      ///< L1 cache size per core
    uint64_t l2_cache_size;      ///< L2 cache size per core
    uint64_t l3_cache_size;      ///< L3 cache size total
    
    // Performance characteristics
    uint32_t memory_bandwidth_mbps; ///< Memory bandwidth in MB/s
    uint32_t memory_latency_ns;     ///< Memory latency in nanoseconds
};

/**
 * @brief I/O subsystem features
 */
struct IOFeatures {
    // Storage capabilities
    bool supports_persistent_storage; ///< Persistent storage available
    bool supports_async_io;           ///< Asynchronous I/O support
    bool supports_memory_mapped_io;   ///< Memory-mapped I/O support
    bool supports_direct_io;          ///< Direct I/O support
    
    // Network capabilities
    bool supports_networking;         ///< Network connectivity
    bool supports_wifi;               ///< WiFi support
    bool supports_bluetooth;          ///< Bluetooth support
    bool supports_ethernet;           ///< Ethernet support
    
    // Input capabilities
    bool supports_keyboard;           ///< Keyboard input
    bool supports_mouse;              ///< Mouse input
    bool supports_touch;              ///< Touch input
    bool supports_multitouch;         ///< Multi-touch support
    bool supports_gamepad;            ///< Gamepad support
    bool supports_accelerometer;      ///< Accelerometer support
    bool supports_gyroscope;          ///< Gyroscope support
    
    // Audio capabilities
    bool supports_audio_output;       ///< Audio output
    bool supports_audio_input;        ///< Audio input (microphone)
    bool supports_multichannel_audio; ///< Multi-channel audio
    bool supports_hardware_effects;   ///< Hardware audio effects
    uint32_t max_audio_channels;      ///< Maximum audio channels
    uint32_t max_sample_rate;         ///< Maximum sample rate
};

/**
 * @brief Hardware detection interface
 * 
 * Provides runtime detection of hardware capabilities across
 * all major subsystems (CPU, GPU, Memory, I/O).
 */
class IHardwareDetector {
public:
    virtual ~IHardwareDetector() = default;
    
    /**
     * @brief Detect CPU features and capabilities
     * @return CPU feature detection results
     */
    virtual HALResult<CPUFeatures> detect_cpu_features() = 0;
    
    /**
     * @brief Detect GPU features and capabilities
     * @return GPU feature detection results
     */
    virtual HALResult<GPUFeatures> detect_gpu_features() = 0;
    
    /**
     * @brief Detect memory subsystem features
     * @return Memory feature detection results
     */
    virtual HALResult<MemoryFeatures> detect_memory_features() = 0;
    
    /**
     * @brief Detect I/O subsystem features
     * @return I/O feature detection results
     */
    virtual HALResult<IOFeatures> detect_io_features() = 0;
    
    /**
     * @brief Perform comprehensive hardware detection
     * @return Complete hardware detection results
     */
    virtual HALResult<void> detect_all_features() = 0;
    
    /**
     * @brief Check if hardware detection is complete
     * @return true if all hardware has been detected
     */
    virtual bool is_detection_complete() const = 0;
    
    /**
     * @brief Get detection timestamp
     * @return When hardware detection was last performed
     */
    virtual std::chrono::steady_clock::time_point get_detection_timestamp() const = 0;
};

/**
 * @brief Hardware feature validator
 * 
 * Validates detected hardware features through runtime testing
 * to ensure capabilities are actually functional.
 */
class IHardwareValidator {
public:
    virtual ~IHardwareValidator() = default;
    
    /**
     * @brief Validate CPU features through runtime tests
     * @param features CPU features to validate
     * @return Validation results
     */
    virtual HALResult<CPUFeatures> validate_cpu_features(const CPUFeatures& features) = 0;
    
    /**
     * @brief Validate GPU features through runtime tests
     * @param features GPU features to validate
     * @return Validation results
     */
    virtual HALResult<GPUFeatures> validate_gpu_features(const GPUFeatures& features) = 0;
    
    /**
     * @brief Validate memory features through runtime tests
     * @param features Memory features to validate
     * @return Validation results
     */
    virtual HALResult<MemoryFeatures> validate_memory_features(const MemoryFeatures& features) = 0;
    
    /**
     * @brief Validate I/O features through runtime tests
     * @param features I/O features to validate
     * @return Validation results
     */
    virtual HALResult<IOFeatures> validate_io_features(const IOFeatures& features) = 0;
};

/**
 * @brief Create platform-specific hardware detector
 * @return Platform-specific hardware detector instance
 */
std::unique_ptr<IHardwareDetector> create_hardware_detector();

/**
 * @brief Create platform-specific hardware validator
 * @return Platform-specific hardware validator instance
 */
std::unique_ptr<IHardwareValidator> create_hardware_validator();

/**
 * @brief Convert hardware features to capability flags
 * @param cpu_features CPU detection results
 * @param gpu_features GPU detection results
 * @param memory_features Memory detection results
 * @param io_features I/O detection results
 * @return Capability bitmask based on hardware features
 */
uint32_t hardware_features_to_capabilities(
    const CPUFeatures& cpu_features,
    const GPUFeatures& gpu_features,
    const MemoryFeatures& memory_features,
    const IOFeatures& io_features
);

/**
 * @brief Determine performance tier from hardware features
 * @param cpu_features CPU detection results
 * @param gpu_features GPU detection results
 * @param memory_features Memory detection results
 * @return Performance tier classification
 */
PerformanceTier determine_performance_tier(
    const CPUFeatures& cpu_features,
    const GPUFeatures& gpu_features,
    const MemoryFeatures& memory_features
);

} // namespace flight::hal
