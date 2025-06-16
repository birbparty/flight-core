/**
 * @file hal_capabilities.hpp
 * @brief Flight HAL Capability Detection System
 * 
 * Defines capability detection interfaces for graceful degradation
 * across platforms with varying hardware and software support.
 */

#pragma once

#include <vector>
#include <string>
#include <cstdint>

namespace flight::hal {

/**
 * @brief Platform capabilities enumeration
 * 
 * Defines the various capabilities that a platform may or may not support.
 * Used for runtime feature detection and graceful degradation.
 */
enum class HALCapability : uint32_t {
    // Threading capabilities
    Threading = 1 << 0,              ///< Multi-threading support
    AtomicOperations = 1 << 1,       ///< Hardware atomic operations
    
    // Memory capabilities
    DMA = 1 << 2,                    ///< Direct Memory Access
    VirtualMemory = 1 << 3,          ///< Virtual memory management
    MemoryProtection = 1 << 4,       ///< Memory protection/segmentation
    
    // Graphics capabilities
    Hardware3D = 1 << 5,             ///< Hardware-accelerated 3D graphics
    Hardware2D = 1 << 6,             ///< Hardware-accelerated 2D graphics
    VertexShaders = 1 << 7,          ///< Programmable vertex shaders
    FragmentShaders = 1 << 8,        ///< Programmable fragment shaders
    ComputeShaders = 1 << 9,         ///< Compute shader support
    
    // Audio capabilities
    HardwareAudio = 1 << 10,         ///< Hardware audio acceleration
    MultiChannelAudio = 1 << 11,     ///< Multi-channel audio output
    AudioEffects = 1 << 12,          ///< Hardware audio effects/DSP
    
    // Input capabilities
    MultiTouch = 1 << 13,            ///< Multi-touch input support
    Accelerometer = 1 << 14,         ///< Accelerometer/motion sensors
    Gamepad = 1 << 15,               ///< Gamepad/controller support
    
    // Networking capabilities
    Networking = 1 << 16,            ///< Network connectivity
    WiFi = 1 << 17,                  ///< WiFi networking
    Bluetooth = 1 << 18,             ///< Bluetooth connectivity
    
    // Timing capabilities
    HighPrecisionTimer = 1 << 19,    ///< High-precision timing
    RealTimeClock = 1 << 20,         ///< Real-time clock
    
    // Storage capabilities
    PersistentStorage = 1 << 21,     ///< Persistent file storage
    AsyncIO = 1 << 22,               ///< Asynchronous I/O operations
    
    // Power management
    PowerManagement = 1 << 23,       ///< Power state management
    
    // Platform-specific
    SandboxRestrictions = 1 << 24,   ///< Sandbox/security restrictions (Web)
};

/**
 * @brief Performance tier classification
 * 
 * Classifies platforms into performance tiers for resource allocation
 * and optimization decisions.
 */
enum class PerformanceTier : uint8_t {
    Minimal = 0,        ///< Dreamcast-level: 16MB RAM, limited CPU
    Limited = 1,        ///< PSP-level: 32-64MB RAM, modest CPU
    Standard = 2,       ///< Modern mobile/web: Decent resources
    High = 3           ///< Desktop PC: Abundant resources
};

/**
 * @brief Platform information structure
 * 
 * Contains detailed information about the current platform's
 * capabilities and characteristics.
 */
struct PlatformInfo {
    std::string platform_name;      ///< Human-readable platform name
    std::string architecture;       ///< CPU architecture (e.g., "SH-4", "MIPS", "x86_64")
    PerformanceTier performance_tier; ///< Performance classification
    uint64_t total_memory;          ///< Total system memory in bytes
    uint32_t cpu_cores;             ///< Number of CPU cores/threads
    bool has_fpu;                   ///< Floating-point unit available
    bool has_simd;                  ///< SIMD instructions available
};

/**
 * @brief Capability provider interface
 * 
 * Interface for querying platform capabilities and characteristics.
 * All HAL interfaces should implement this to enable graceful degradation.
 */
class ICapabilityProvider {
public:
    virtual ~ICapabilityProvider() = default;
    
    /**
     * @brief Check if a specific capability is supported
     * @param capability The capability to check
     * @return true if the capability is supported
     */
    virtual bool supports_capability(HALCapability capability) const = 0;
    
    /**
     * @brief Get all supported capabilities as a bitmask
     * @return Bitmask of all supported capabilities
     */
    virtual uint32_t get_capability_mask() const = 0;
    
    /**
     * @brief Get list of all supported capabilities
     * @return Vector of supported capability enums
     */
    virtual std::vector<HALCapability> get_capabilities() const = 0;
    
    /**
     * @brief Get platform performance tier
     * @return Performance tier classification
     */
    virtual PerformanceTier get_performance_tier() const = 0;
    
    /**
     * @brief Get detailed platform information
     * @return Structure containing platform details
     */
    virtual const PlatformInfo& get_platform_info() const = 0;
    
    /**
     * @brief Check if graceful degradation is available for a capability
     * @param capability The capability to check
     * @return true if software fallback is available
     */
    virtual bool has_fallback(HALCapability capability) const = 0;
};

/**
 * @brief Capability helper functions
 */
namespace capabilities {

/**
 * @brief Convert capability enum to string name
 * @param capability The capability to convert
 * @return String name of the capability
 */
const char* to_string(HALCapability capability);

/**
 * @brief Convert performance tier to string name
 * @param tier The performance tier to convert
 * @return String name of the performance tier
 */
const char* to_string(PerformanceTier tier);

/**
 * @brief Check if capabilities mask contains a specific capability
 * @param mask Capability bitmask to check
 * @param capability Specific capability to look for
 * @return true if the capability is present in the mask
 */
inline bool has_capability(uint32_t mask, HALCapability capability) {
    return (mask & static_cast<uint32_t>(capability)) != 0;
}

/**
 * @brief Combine multiple capabilities into a bitmask
 * @param caps List of capabilities to combine
 * @return Combined capability bitmask
 */
uint32_t make_capability_mask(const std::vector<HALCapability>& caps);

/**
 * @brief Extract individual capabilities from a bitmask
 * @param mask Capability bitmask to extract from
 * @return Vector of individual capabilities
 */
std::vector<HALCapability> extract_capabilities(uint32_t mask);

} // namespace capabilities

} // namespace flight::hal
