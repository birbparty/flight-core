/**
 * @file platform_detection.cpp
 * @brief Flight HAL Platform Detection Implementation
 * 
 * Implements runtime platform detection and capability probing for
 * dynamic feature detection and platform-specific optimizations.
 */

#include "flight/hal/core/platform_detection.hpp"
#include <cstring>
#include <algorithm>

// Platform-specific includes
#if FLIGHT_PLATFORM_DREAMCAST
    // KallistiOS headers would go here
    // #include <kos.h>
#elif FLIGHT_PLATFORM_PSP
    // PSPSDK headers would go here
    // #include <pspkernel.h>
    // #include <pspsysmem.h>
#elif FLIGHT_PLATFORM_WEB
    #include <emscripten.h>
    #include <emscripten/html5.h>
#elif FLIGHT_PLATFORM_MACOS
    #include <sys/types.h>
    #include <sys/sysctl.h>
    #include <mach/mach.h>
    #include <mach/mach_host.h>
#elif FLIGHT_PLATFORM_WINDOWS
    #include <windows.h>
    #include <sysinfoapi.h>
#elif FLIGHT_PLATFORM_LINUX
    #include <sys/sysinfo.h>
    #include <unistd.h>
#endif

// Threading detection
#if FLIGHT_HAS_THREADING
    #include <thread>
#endif

namespace flight::hal {

// ============================================================================
// RUNTIME PLATFORM DETECTOR IMPLEMENTATION
// ============================================================================

PlatformInfo RuntimePlatformDetector::detect_platform_info() {
    PlatformInfo info;
    
    // Set compile-time known values
    info.platform_name = FLIGHT_PLATFORM_NAME;
    info.architecture = FLIGHT_ARCH_NAME;
    info.performance_tier = get_compile_time_performance_tier();
    
    // Detect runtime values
    info.total_memory = probe_total_memory();
    info.cpu_cores = detect_cpu_cores();
    info.has_fpu = has_fpu();
    info.has_simd = has_simd();
    
    return info;
}

uint32_t RuntimePlatformDetector::detect_capabilities() {
    // Start with compile-time capabilities
    uint32_t capabilities = get_compile_time_capabilities();
    
    // Add runtime-detected capabilities
    // For most embedded platforms, compile-time detection is sufficient
    // For desktop/web platforms, we might want to validate certain capabilities
    
#if FLIGHT_PLATFORM_WEB
    // Web platform might need runtime validation of WebGL features
    if (test_capability_support(HALCapability::Hardware3D)) {
        capabilities |= static_cast<uint32_t>(HALCapability::Hardware3D);
    }
    if (test_capability_support(HALCapability::VertexShaders)) {
        capabilities |= static_cast<uint32_t>(HALCapability::VertexShaders);
    }
    if (test_capability_support(HALCapability::FragmentShaders)) {
        capabilities |= static_cast<uint32_t>(HALCapability::FragmentShaders);
    }
#endif

#if FLIGHT_PLATFORM_PSP
    // PSP might have different RAM amounts (32MB vs 64MB)
    uint64_t actual_memory = probe_total_memory();
    if (actual_memory > (48 * 1024 * 1024)) { // More than 48MB suggests 64MB model
        // Could set additional capability flags for higher memory models
    }
#endif
    
    return capabilities;
}

PerformanceTier RuntimePlatformDetector::classify_performance() {
    // Start with compile-time classification
    PerformanceTier tier = get_compile_time_performance_tier();
    
    // Refine based on runtime detection
    uint64_t total_memory = probe_total_memory();
    uint32_t cpu_cores = detect_cpu_cores();
    
    // Adjust performance tier based on actual detected capabilities
    if (total_memory == 0) {
        // Couldn't detect memory, stick with compile-time classification
        return tier;
    }
    
#if FLIGHT_PLATFORM_PSP
    // PSP: Adjust tier based on actual memory detected
    if (total_memory >= (60 * 1024 * 1024)) {
        // 64MB PSP model might warrant slight performance boost
        // but still limited overall
        return PerformanceTier::Limited;
    }
#elif FLIGHT_PLATFORM_WEB
    // Web: Memory constraints can vary significantly
    if (total_memory < (512 * 1024 * 1024)) {
        return PerformanceTier::Limited;
    } else if (total_memory >= (2ULL * 1024 * 1024 * 1024)) {
        return PerformanceTier::High;
    }
#elif !FLIGHT_PLATFORM_DREAMCAST
    // Desktop platforms: Adjust based on actual resources
    if (total_memory < (2ULL * 1024 * 1024 * 1024)) {
        return PerformanceTier::Standard;
    } else if (total_memory >= (16ULL * 1024 * 1024 * 1024) && cpu_cores >= 8) {
        return PerformanceTier::High;
    }
#endif
    
    return tier;
}

uint64_t RuntimePlatformDetector::probe_total_memory() {
#if FLIGHT_PLATFORM_DREAMCAST
    // Dreamcast: Known fixed memory configuration
    return (16 + 8 + 2) * 1024 * 1024; // 26MB total
    
#elif FLIGHT_PLATFORM_PSP
    // PSP: Try to detect if we're on 32MB or 64MB model
    // This would require PSP-specific system calls
    // For now, return conservative estimate
    return 32 * 1024 * 1024; // 32MB baseline
    
#elif FLIGHT_PLATFORM_WEB
    // Web: Limited memory detection via JavaScript
    // This is a rough approximation
    return 1024 * 1024 * 1024; // 1GB estimate
    
#elif FLIGHT_PLATFORM_MACOS
    // macOS: Use sysctl to get memory info
    uint64_t memory = 0;
    size_t size = sizeof(memory);
    if (sysctlbyname("hw.memsize", &memory, &size, nullptr, 0) == 0) {
        return memory;
    }
    return 0;
    
#elif FLIGHT_PLATFORM_WINDOWS
    // Windows: Use GlobalMemoryStatusEx
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    if (GlobalMemoryStatusEx(&status)) {
        return status.ullTotalPhys;
    }
    return 0;
    
#elif FLIGHT_PLATFORM_LINUX
    // Linux: Use sysinfo
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        return static_cast<uint64_t>(info.totalram) * info.mem_unit;
    }
    return 0;
    
#else
    // Unknown platform
    return 0;
#endif
}

uint32_t RuntimePlatformDetector::detect_cpu_cores() {
#if FLIGHT_PLATFORM_DREAMCAST
    // Dreamcast: Single SH-4 core
    return 1;
    
#elif FLIGHT_PLATFORM_PSP
    // PSP: Dual MIPS cores (main + media engine)
    return 2;
    
#elif FLIGHT_PLATFORM_WEB
    // Web: Use Emscripten API or conservative estimate
    return 1; // Conservative for web
    
#elif FLIGHT_HAS_THREADING
    // Platforms with threading support: Use std::thread
    uint32_t cores = std::thread::hardware_concurrency();
    return cores > 0 ? cores : 1;
    
#else
    // No threading support: Single core
    return 1;
#endif
}

bool RuntimePlatformDetector::test_capability_support(HALCapability capability) {
    // This would perform actual runtime testing of capabilities
    // For now, we'll do basic checks based on platform
    
    switch (capability) {
        case HALCapability::Threading:
#if FLIGHT_HAS_THREADING
            return true;
#else
            return false;
#endif
            
        case HALCapability::Hardware3D:
#if FLIGHT_HAS_HARDWARE_3D
            return true;
#else
            return false;
#endif
            
        case HALCapability::VertexShaders:
#if FLIGHT_HAS_VERTEX_SHADERS
            return true;
#else
            return false;
#endif
            
        case HALCapability::FragmentShaders:
#if FLIGHT_HAS_FRAGMENT_SHADERS
            return true;
#else
            return false;
#endif
            
        case HALCapability::WiFi:
#if FLIGHT_HAS_WIFI
            return true;
#else
            return false;
#endif
            
        default:
            // For other capabilities, fall back to compile-time detection
            uint32_t compile_time_mask = get_compile_time_capabilities();
            return (compile_time_mask & static_cast<uint32_t>(capability)) != 0;
    }
}

bool RuntimePlatformDetector::has_fpu() {
#if FLIGHT_PLATFORM_DREAMCAST
    // SH-4 has FPU
    return true;
    
#elif FLIGHT_PLATFORM_PSP
    // MIPS R4000 has FPU
    return true;
    
#elif FLIGHT_PLATFORM_WEB
    // WebAssembly supports floating point
    return true;
    
#else
    // Desktop platforms: Assume FPU is available
    return true;
#endif
}

bool RuntimePlatformDetector::has_simd() {
#if FLIGHT_PLATFORM_DREAMCAST
    // SH-4 has vector unit (128-bit SIMD)
    return true;
    
#elif FLIGHT_PLATFORM_PSP
    // MIPS has VFPU (Vector Floating Point Unit)
    return true;
    
#elif FLIGHT_PLATFORM_WEB
    // WebAssembly SIMD support varies by browser
    return false; // Conservative
    
#elif FLIGHT_PLATFORM_MACOS || FLIGHT_PLATFORM_WINDOWS || FLIGHT_PLATFORM_LINUX
    // Desktop platforms: Likely have SSE/AVX or NEON
    return true;
    
#else
    return false;
#endif
}

} // namespace flight::hal
