/**
 * @file platform_detection.hpp
 * @brief Flight HAL Platform Detection System
 * 
 * Provides compile-time and runtime platform detection for enabling
 * platform-specific optimizations and graceful capability degradation
 * across all target platforms from Dreamcast to modern systems.
 */

#pragma once

#include "hal_capabilities.hpp"
#include <cstdint>

namespace flight::hal {

// ============================================================================
// COMPILE-TIME PLATFORM DETECTION
// ============================================================================

/**
 * @brief Platform identification macros
 * 
 * These macros are set based on compiler-defined platform detection.
 * Used for conditional compilation and platform-specific code paths.
 */

// Sega Dreamcast Detection
#if defined(__DREAMCAST__) || defined(_arch_dreamcast) || defined(__sh__)
    #define FLIGHT_PLATFORM_DREAMCAST 1
    #define FLIGHT_PLATFORM_NAME "Dreamcast"
    #define FLIGHT_ARCH_SH4 1
    #define FLIGHT_ARCH_NAME "SH-4"
    
// Sony PSP Detection  
#elif defined(__PSP__) || defined(PSP) || defined(__psp__)
    #define FLIGHT_PLATFORM_PSP 1
    #define FLIGHT_PLATFORM_NAME "PSP"
    #define FLIGHT_ARCH_MIPS 1
    #define FLIGHT_ARCH_NAME "MIPS"
    
// Web/Emscripten Detection
#elif defined(__EMSCRIPTEN__)
    #define FLIGHT_PLATFORM_WEB 1
    #define FLIGHT_PLATFORM_NAME "Web"
    #define FLIGHT_ARCH_WASM 1
    #define FLIGHT_ARCH_NAME "WebAssembly"
    
// macOS Detection
#elif defined(__APPLE__) && defined(__MACH__)
    #include <TargetConditionals.h>
    #if TARGET_OS_MAC && !TARGET_OS_IPHONE
        #define FLIGHT_PLATFORM_MACOS 1
        #define FLIGHT_PLATFORM_NAME "macOS"
        #if defined(__x86_64__)
            #define FLIGHT_ARCH_X64 1
            #define FLIGHT_ARCH_NAME "x86_64"
        #elif defined(__aarch64__)
            #define FLIGHT_ARCH_ARM64 1
            #define FLIGHT_ARCH_NAME "ARM64"
        #endif
    #endif
    
// Windows Detection (future)
#elif defined(_WIN32) || defined(_WIN64)
    #define FLIGHT_PLATFORM_WINDOWS 1
    #define FLIGHT_PLATFORM_NAME "Windows"
    #if defined(_WIN64)
        #define FLIGHT_ARCH_X64 1
        #define FLIGHT_ARCH_NAME "x86_64"
    #else
        #define FLIGHT_ARCH_X86 1
        #define FLIGHT_ARCH_NAME "x86"
    #endif
    
// Linux Detection (future)
#elif defined(__linux__)
    #define FLIGHT_PLATFORM_LINUX 1
    #define FLIGHT_PLATFORM_NAME "Linux"
    #if defined(__x86_64__)
        #define FLIGHT_ARCH_X64 1
        #define FLIGHT_ARCH_NAME "x86_64"
    #elif defined(__aarch64__)
        #define FLIGHT_ARCH_ARM64 1
        #define FLIGHT_ARCH_NAME "ARM64"
    #endif
    
// Unknown platform
#else
    #define FLIGHT_PLATFORM_UNKNOWN 1
    #define FLIGHT_PLATFORM_NAME "Unknown"
    #define FLIGHT_ARCH_UNKNOWN 1
    #define FLIGHT_ARCH_NAME "Unknown"
#endif

// Set defaults for undefined platforms
#ifndef FLIGHT_PLATFORM_DREAMCAST
    #define FLIGHT_PLATFORM_DREAMCAST 0
#endif
#ifndef FLIGHT_PLATFORM_PSP
    #define FLIGHT_PLATFORM_PSP 0
#endif
#ifndef FLIGHT_PLATFORM_WEB
    #define FLIGHT_PLATFORM_WEB 0
#endif
#ifndef FLIGHT_PLATFORM_MACOS
    #define FLIGHT_PLATFORM_MACOS 0
#endif
#ifndef FLIGHT_PLATFORM_WINDOWS
    #define FLIGHT_PLATFORM_WINDOWS 0
#endif
#ifndef FLIGHT_PLATFORM_LINUX
    #define FLIGHT_PLATFORM_LINUX 0
#endif
#ifndef FLIGHT_PLATFORM_UNKNOWN
    #define FLIGHT_PLATFORM_UNKNOWN 0
#endif

// ============================================================================
// PLATFORM MEMORY CONSTRAINTS
// ============================================================================

/**
 * @brief Platform-specific memory constraints
 * 
 * These constants define the baseline memory availability for each platform.
 * Used for allocation strategies and resource management decisions.
 */

#if FLIGHT_PLATFORM_DREAMCAST
    #define FLIGHT_MEMORY_MAIN_MB 16        ///< 16MB main system RAM
    #define FLIGHT_MEMORY_VIDEO_MB 8        ///< 8MB video RAM (VRAM)
    #define FLIGHT_MEMORY_AUDIO_MB 2        ///< 2MB audio RAM (ARAM)
    #define FLIGHT_MEMORY_TOTAL_MB 26       ///< Total memory available
    #define FLIGHT_PERFORMANCE_TIER_VALUE static_cast<uint8_t>(PerformanceTier::Minimal)
    
#elif FLIGHT_PLATFORM_PSP
    #define FLIGHT_MEMORY_MAIN_MB 32        ///< 32MB main RAM (64MB on later models)
    #define FLIGHT_MEMORY_VIDEO_MB 4        ///< 4MB embedded DRAM for GPU
    #define FLIGHT_MEMORY_AUDIO_MB 2        ///< Audio processing memory
    #define FLIGHT_MEMORY_TOTAL_MB 38       ///< Conservative total estimate
    #define FLIGHT_PERFORMANCE_TIER_VALUE static_cast<uint8_t>(PerformanceTier::Limited)
    
#elif FLIGHT_PLATFORM_WEB
    #define FLIGHT_MEMORY_MAIN_MB 512       ///< Conservative web estimate
    #define FLIGHT_MEMORY_VIDEO_MB 256      ///< WebGL memory estimate
    #define FLIGHT_MEMORY_AUDIO_MB 64       ///< Web Audio memory estimate
    #define FLIGHT_MEMORY_TOTAL_MB 832      ///< Total conservative estimate
    #define FLIGHT_PERFORMANCE_TIER_VALUE static_cast<uint8_t>(PerformanceTier::Standard)
    
#else
    // Desktop platforms (macOS, Windows, Linux)
    #define FLIGHT_MEMORY_MAIN_MB 8192      ///< 8GB baseline for desktop
    #define FLIGHT_MEMORY_VIDEO_MB 4096     ///< 4GB video memory estimate
    #define FLIGHT_MEMORY_AUDIO_MB 256      ///< Audio subsystem memory
    #define FLIGHT_MEMORY_TOTAL_MB 12544    ///< Total desktop estimate
    #define FLIGHT_PERFORMANCE_TIER_VALUE static_cast<uint8_t>(PerformanceTier::High)
#endif

// ============================================================================
// PLATFORM CAPABILITY FLAGS
// ============================================================================

/**
 * @brief Compile-time capability detection
 * 
 * These macros indicate which capabilities are available at compile-time
 * based on the target platform. Used for conditional compilation.
 */

// Threading capabilities
#if FLIGHT_PLATFORM_DREAMCAST
    #define FLIGHT_HAS_THREADING 0
    #define FLIGHT_HAS_ATOMIC_OPERATIONS 0
#else
    #define FLIGHT_HAS_THREADING 1
    #define FLIGHT_HAS_ATOMIC_OPERATIONS 1
#endif

// Memory management capabilities
#if FLIGHT_PLATFORM_DREAMCAST || FLIGHT_PLATFORM_PSP
    #define FLIGHT_HAS_VIRTUAL_MEMORY 0
    #define FLIGHT_HAS_MEMORY_PROTECTION 0
    #define FLIGHT_HAS_DMA 1
#else
    #define FLIGHT_HAS_VIRTUAL_MEMORY 1
    #define FLIGHT_HAS_MEMORY_PROTECTION 1
    #define FLIGHT_HAS_DMA 0  // Not typically exposed on desktop
#endif

// Graphics capabilities
#if FLIGHT_PLATFORM_DREAMCAST
    #define FLIGHT_HAS_HARDWARE_3D 1
    #define FLIGHT_HAS_HARDWARE_2D 1
    #define FLIGHT_HAS_VERTEX_SHADERS 0
    #define FLIGHT_HAS_FRAGMENT_SHADERS 0
    #define FLIGHT_HAS_COMPUTE_SHADERS 0
#elif FLIGHT_PLATFORM_PSP
    #define FLIGHT_HAS_HARDWARE_3D 1
    #define FLIGHT_HAS_HARDWARE_2D 1
    #define FLIGHT_HAS_VERTEX_SHADERS 0
    #define FLIGHT_HAS_FRAGMENT_SHADERS 0
    #define FLIGHT_HAS_COMPUTE_SHADERS 0
#elif FLIGHT_PLATFORM_WEB
    #define FLIGHT_HAS_HARDWARE_3D 1
    #define FLIGHT_HAS_HARDWARE_2D 1
    #define FLIGHT_HAS_VERTEX_SHADERS 1
    #define FLIGHT_HAS_FRAGMENT_SHADERS 1
    #define FLIGHT_HAS_COMPUTE_SHADERS 0  // WebGPU compute shaders not universal yet
#else
    // Desktop platforms
    #define FLIGHT_HAS_HARDWARE_3D 1
    #define FLIGHT_HAS_HARDWARE_2D 1
    #define FLIGHT_HAS_VERTEX_SHADERS 1
    #define FLIGHT_HAS_FRAGMENT_SHADERS 1
    #define FLIGHT_HAS_COMPUTE_SHADERS 1
#endif

// Audio capabilities
#if FLIGHT_PLATFORM_DREAMCAST || FLIGHT_PLATFORM_PSP
    #define FLIGHT_HAS_HARDWARE_AUDIO 1
    #define FLIGHT_HAS_MULTICHANNEL_AUDIO 1
    #define FLIGHT_HAS_AUDIO_EFFECTS 1
#else
    #define FLIGHT_HAS_HARDWARE_AUDIO 1
    #define FLIGHT_HAS_MULTICHANNEL_AUDIO 1
    #define FLIGHT_HAS_AUDIO_EFFECTS 1
#endif

// Input capabilities
#if FLIGHT_PLATFORM_DREAMCAST
    #define FLIGHT_HAS_MULTITOUCH 0
    #define FLIGHT_HAS_ACCELEROMETER 0
    #define FLIGHT_HAS_GAMEPAD 1
#elif FLIGHT_PLATFORM_PSP
    #define FLIGHT_HAS_MULTITOUCH 0
    #define FLIGHT_HAS_ACCELEROMETER 0
    #define FLIGHT_HAS_GAMEPAD 1
#elif FLIGHT_PLATFORM_WEB
    #define FLIGHT_HAS_MULTITOUCH 1
    #define FLIGHT_HAS_ACCELEROMETER 1
    #define FLIGHT_HAS_GAMEPAD 1
#else
    #define FLIGHT_HAS_MULTITOUCH 1
    #define FLIGHT_HAS_ACCELEROMETER 0  // Not typical on desktop
    #define FLIGHT_HAS_GAMEPAD 1
#endif

// Networking capabilities
#if FLIGHT_PLATFORM_DREAMCAST
    #define FLIGHT_HAS_NETWORKING 1
    #define FLIGHT_HAS_WIFI 0
    #define FLIGHT_HAS_BLUETOOTH 0
#elif FLIGHT_PLATFORM_PSP
    #define FLIGHT_HAS_NETWORKING 1
    #define FLIGHT_HAS_WIFI 1
    #define FLIGHT_HAS_BLUETOOTH 0
#else
    #define FLIGHT_HAS_NETWORKING 1
    #define FLIGHT_HAS_WIFI 1
    #define FLIGHT_HAS_BLUETOOTH 1
#endif

// Timing capabilities
#if FLIGHT_PLATFORM_DREAMCAST || FLIGHT_PLATFORM_PSP
    #define FLIGHT_HAS_HIGH_PRECISION_TIMER 1
    #define FLIGHT_HAS_REAL_TIME_CLOCK 1
#else
    #define FLIGHT_HAS_HIGH_PRECISION_TIMER 1
    #define FLIGHT_HAS_REAL_TIME_CLOCK 1
#endif

// Storage capabilities
#if FLIGHT_PLATFORM_DREAMCAST
    #define FLIGHT_HAS_PERSISTENT_STORAGE 1  // VMU
    #define FLIGHT_HAS_ASYNC_IO 0
#elif FLIGHT_PLATFORM_PSP
    #define FLIGHT_HAS_PERSISTENT_STORAGE 1  // Memory Stick
    #define FLIGHT_HAS_ASYNC_IO 0
#else
    #define FLIGHT_HAS_PERSISTENT_STORAGE 1
    #define FLIGHT_HAS_ASYNC_IO 1
#endif

// Power management
#if FLIGHT_PLATFORM_PSP || FLIGHT_PLATFORM_WEB
    #define FLIGHT_HAS_POWER_MANAGEMENT 1
#else
    #define FLIGHT_HAS_POWER_MANAGEMENT 0
#endif

// Platform restrictions
#if FLIGHT_PLATFORM_WEB
    #define FLIGHT_HAS_SANDBOX_RESTRICTIONS 1
#else
    #define FLIGHT_HAS_SANDBOX_RESTRICTIONS 0
#endif

// ============================================================================
// RUNTIME PLATFORM DETECTION
// ============================================================================

/**
 * @brief Runtime platform detection and capability probing
 * 
 * Provides runtime detection of platform characteristics that cannot
 * be determined at compile-time, such as exact memory amounts or
 * dynamic feature availability.
 */
class RuntimePlatformDetector {
public:
    /**
     * @brief Detect complete platform information at runtime
     * @return Detailed platform information structure
     */
    static PlatformInfo detect_platform_info();
    
    /**
     * @brief Detect all available capabilities at runtime
     * @return Bitmask of all detected capabilities
     */
    static uint32_t detect_capabilities();
    
    /**
     * @brief Classify platform performance tier
     * @return Performance tier classification
     */
    static PerformanceTier classify_performance();
    
    /**
     * @brief Probe total system memory
     * @return Total memory in bytes, or 0 if cannot be determined
     */
    static uint64_t probe_total_memory();
    
    /**
     * @brief Detect number of CPU cores/threads
     * @return Number of available CPU cores
     */
    static uint32_t detect_cpu_cores();
    
    /**
     * @brief Test if a specific capability works at runtime
     * @param capability The capability to test
     * @return true if the capability is functional
     */
    static bool test_capability_support(HALCapability capability);
    
    /**
     * @brief Check if floating-point unit is available
     * @return true if FPU is present and functional
     */
    static bool has_fpu();
    
    /**
     * @brief Check if SIMD instructions are available
     * @return true if SIMD is supported
     */
    static bool has_simd();
    
    /**
     * @brief Get compile-time capability mask
     * @return Bitmask of compile-time detected capabilities
     */
    static constexpr uint32_t get_compile_time_capabilities() {
        uint32_t mask = 0;
        
        if (FLIGHT_HAS_THREADING) mask |= static_cast<uint32_t>(HALCapability::Threading);
        if (FLIGHT_HAS_ATOMIC_OPERATIONS) mask |= static_cast<uint32_t>(HALCapability::AtomicOperations);
        if (FLIGHT_HAS_DMA) mask |= static_cast<uint32_t>(HALCapability::DMA);
        if (FLIGHT_HAS_VIRTUAL_MEMORY) mask |= static_cast<uint32_t>(HALCapability::VirtualMemory);
        if (FLIGHT_HAS_MEMORY_PROTECTION) mask |= static_cast<uint32_t>(HALCapability::MemoryProtection);
        if (FLIGHT_HAS_HARDWARE_3D) mask |= static_cast<uint32_t>(HALCapability::Hardware3D);
        if (FLIGHT_HAS_HARDWARE_2D) mask |= static_cast<uint32_t>(HALCapability::Hardware2D);
        if (FLIGHT_HAS_VERTEX_SHADERS) mask |= static_cast<uint32_t>(HALCapability::VertexShaders);
        if (FLIGHT_HAS_FRAGMENT_SHADERS) mask |= static_cast<uint32_t>(HALCapability::FragmentShaders);
        if (FLIGHT_HAS_COMPUTE_SHADERS) mask |= static_cast<uint32_t>(HALCapability::ComputeShaders);
        if (FLIGHT_HAS_HARDWARE_AUDIO) mask |= static_cast<uint32_t>(HALCapability::HardwareAudio);
        if (FLIGHT_HAS_MULTICHANNEL_AUDIO) mask |= static_cast<uint32_t>(HALCapability::MultiChannelAudio);
        if (FLIGHT_HAS_AUDIO_EFFECTS) mask |= static_cast<uint32_t>(HALCapability::AudioEffects);
        if (FLIGHT_HAS_MULTITOUCH) mask |= static_cast<uint32_t>(HALCapability::MultiTouch);
        if (FLIGHT_HAS_ACCELEROMETER) mask |= static_cast<uint32_t>(HALCapability::Accelerometer);
        if (FLIGHT_HAS_GAMEPAD) mask |= static_cast<uint32_t>(HALCapability::Gamepad);
        if (FLIGHT_HAS_NETWORKING) mask |= static_cast<uint32_t>(HALCapability::Networking);
        if (FLIGHT_HAS_WIFI) mask |= static_cast<uint32_t>(HALCapability::WiFi);
        if (FLIGHT_HAS_BLUETOOTH) mask |= static_cast<uint32_t>(HALCapability::Bluetooth);
        if (FLIGHT_HAS_HIGH_PRECISION_TIMER) mask |= static_cast<uint32_t>(HALCapability::HighPrecisionTimer);
        if (FLIGHT_HAS_REAL_TIME_CLOCK) mask |= static_cast<uint32_t>(HALCapability::RealTimeClock);
        if (FLIGHT_HAS_PERSISTENT_STORAGE) mask |= static_cast<uint32_t>(HALCapability::PersistentStorage);
        if (FLIGHT_HAS_ASYNC_IO) mask |= static_cast<uint32_t>(HALCapability::AsyncIO);
        if (FLIGHT_HAS_POWER_MANAGEMENT) mask |= static_cast<uint32_t>(HALCapability::PowerManagement);
        if (FLIGHT_HAS_SANDBOX_RESTRICTIONS) mask |= static_cast<uint32_t>(HALCapability::SandboxRestrictions);
        
        return mask;
    }
    
    /**
     * @brief Get compile-time performance tier
     * @return Performance tier based on compile-time platform detection
     */
    static constexpr PerformanceTier get_compile_time_performance_tier() {
        return static_cast<PerformanceTier>(FLIGHT_PERFORMANCE_TIER_VALUE);
    }

private:
    RuntimePlatformDetector() = default;
};

// ============================================================================
// CONVENIENCE FUNCTIONS
// ============================================================================

/**
 * @brief Platform detection utility functions
 */
namespace platform_detection {

/**
 * @brief Check if running on a specific platform at compile-time
 */
constexpr bool is_dreamcast() { return FLIGHT_PLATFORM_DREAMCAST; }
constexpr bool is_psp() { return FLIGHT_PLATFORM_PSP; }
constexpr bool is_web() { return FLIGHT_PLATFORM_WEB; }
constexpr bool is_macos() { return FLIGHT_PLATFORM_MACOS; }
constexpr bool is_windows() { return FLIGHT_PLATFORM_WINDOWS; }
constexpr bool is_linux() { return FLIGHT_PLATFORM_LINUX; }
constexpr bool is_desktop() { return FLIGHT_PLATFORM_MACOS || FLIGHT_PLATFORM_WINDOWS || FLIGHT_PLATFORM_LINUX; }
constexpr bool is_embedded() { return FLIGHT_PLATFORM_DREAMCAST || FLIGHT_PLATFORM_PSP; }

/**
 * @brief Get platform name at compile-time
 */
constexpr const char* get_platform_name() { return FLIGHT_PLATFORM_NAME; }
constexpr const char* get_architecture_name() { return FLIGHT_ARCH_NAME; }

/**
 * @brief Memory constraint helpers
 */
constexpr uint32_t get_main_memory_mb() { return FLIGHT_MEMORY_MAIN_MB; }
constexpr uint32_t get_video_memory_mb() { return FLIGHT_MEMORY_VIDEO_MB; }
constexpr uint32_t get_audio_memory_mb() { return FLIGHT_MEMORY_AUDIO_MB; }
constexpr uint32_t get_total_memory_mb() { return FLIGHT_MEMORY_TOTAL_MB; }

/**
 * @brief Capability check helpers
 */
constexpr bool has_threading() { return FLIGHT_HAS_THREADING; }
constexpr bool has_modern_graphics() { return FLIGHT_HAS_VERTEX_SHADERS && FLIGHT_HAS_FRAGMENT_SHADERS; }
constexpr bool has_legacy_graphics() { return FLIGHT_HAS_HARDWARE_3D && !has_modern_graphics(); }
constexpr bool has_networking() { return FLIGHT_HAS_NETWORKING; }
constexpr bool is_resource_constrained() { return get_total_memory_mb() < 128; }

} // namespace platform_detection

} // namespace flight::hal
