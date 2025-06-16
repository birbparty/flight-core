/**
 * @file platform_capabilities.cpp
 * @brief Platform-Specific Capability Provider Implementations
 * 
 * Implements platform-specific capability providers that provide accurate
 * capability detection for each target platform using both compile-time
 * and runtime detection methods.
 */

#include "flight/hal/core/platform_capabilities.hpp"
#include "flight/hal/core/hal_capabilities.hpp"

// Platform-specific includes for capability detection
#if FLIGHT_PLATFORM_WEB
    #include <emscripten.h>
    #include <emscripten/html5.h>
#elif FLIGHT_PLATFORM_MACOS
    #include <sys/types.h>
    #include <sys/sysctl.h>
    #include <Availability.h>
#elif FLIGHT_PLATFORM_WINDOWS
    #include <windows.h>
    #include <intrin.h>
#elif FLIGHT_PLATFORM_LINUX
    #include <sys/utsname.h>
    #include <cpuid.h>
#endif

namespace flight::hal {

// ============================================================================
// BASE PLATFORM CAPABILITY PROVIDER
// ============================================================================

BasePlatformCapabilityProvider::BasePlatformCapabilityProvider() {
    // Initialize with runtime detection
    platform_info_ = RuntimePlatformDetector::detect_platform_info();
    capability_mask_ = RuntimePlatformDetector::detect_capabilities();
    
    // Allow derived classes to override/enhance
    initialize_platform_capabilities();
    initialize_fallback_capabilities();
}

bool BasePlatformCapabilityProvider::supports_capability(HALCapability capability) const {
    return capabilities::has_capability(capability_mask_, capability);
}

uint32_t BasePlatformCapabilityProvider::get_capability_mask() const {
    return capability_mask_;
}

std::vector<HALCapability> BasePlatformCapabilityProvider::get_capabilities() const {
    return capabilities::extract_capabilities(capability_mask_);
}

PerformanceTier BasePlatformCapabilityProvider::get_performance_tier() const {
    return platform_info_.performance_tier;
}

const PlatformInfo& BasePlatformCapabilityProvider::get_platform_info() const {
    return platform_info_;
}

bool BasePlatformCapabilityProvider::has_fallback(HALCapability capability) const {
    auto it = fallback_availability_.find(capability);
    return it != fallback_availability_.end() && it->second;
}

void BasePlatformCapabilityProvider::initialize_platform_capabilities() {
    // Base implementation - no additional capabilities
}

void BasePlatformCapabilityProvider::initialize_fallback_capabilities() {
    // Define common fallback capabilities
    fallback_availability_[HALCapability::Hardware3D] = true;  // Software 3D rendering
    fallback_availability_[HALCapability::Hardware2D] = true;  // Software 2D rendering
    fallback_availability_[HALCapability::HardwareAudio] = true; // Software audio mixing
    fallback_availability_[HALCapability::Threading] = false; // No software fallback for threading
    fallback_availability_[HALCapability::Networking] = false; // No software fallback for networking
}

// ============================================================================
// PLATFORM-SPECIFIC IMPLEMENTATIONS
// ============================================================================

#if FLIGHT_PLATFORM_DREAMCAST
DreamcastCapabilityProvider::DreamcastCapabilityProvider() : BasePlatformCapabilityProvider() {
    // Dreamcast initialization is handled by base class
}

void DreamcastCapabilityProvider::initialize_platform_capabilities() {
    // Dreamcast-specific capability refinements
    // Most capabilities are already correctly set by compile-time detection
    
    // Check for optional Dreamcast peripherals
    if (detect_modem_support()) {
        capability_mask_ |= static_cast<uint32_t>(HALCapability::Networking);
    }
    
    if (detect_vmu_support()) {
        capability_mask_ |= static_cast<uint32_t>(HALCapability::PersistentStorage);
    }
}

void DreamcastCapabilityProvider::initialize_fallback_capabilities() {
    BasePlatformCapabilityProvider::initialize_fallback_capabilities();
    
    // Dreamcast-specific fallbacks
    fallback_availability_[HALCapability::VertexShaders] = false; // PowerVR2 is fixed-function
    fallback_availability_[HALCapability::FragmentShaders] = false;
    fallback_availability_[HALCapability::Threading] = false; // Single-threaded only
    fallback_availability_[HALCapability::WiFi] = false; // No WiFi hardware
}

bool DreamcastCapabilityProvider::detect_modem_support() const {
    // Would check for modem adapter presence
    // For now, assume modem might be present
    return true;
}

bool DreamcastCapabilityProvider::detect_vmu_support() const {
    // Would check for VMU presence in controller ports
    // For now, assume VMU is available
    return true;
}

bool DreamcastCapabilityProvider::detect_maple_devices() const {
    // Would enumerate Maple bus devices
    return true;
}
#endif

#if FLIGHT_PLATFORM_PSP
PSPCapabilityProvider::PSPCapabilityProvider() : BasePlatformCapabilityProvider() {
    // PSP initialization is handled by base class
}

void PSPCapabilityProvider::initialize_platform_capabilities() {
    // PSP-specific capability refinements
    
    // Detect PSP model and adjust memory capability
    uint32_t actual_memory = detect_actual_memory();
    if (actual_memory >= 64) {
        platform_info_.total_memory = 64 * 1024 * 1024;
        // Could add a capability flag for "high memory" models
    }
    
    // Check WiFi functionality
    if (detect_wifi_capabilities()) {
        capability_mask_ |= static_cast<uint32_t>(HALCapability::WiFi);
        capability_mask_ |= static_cast<uint32_t>(HALCapability::Networking);
    }
    
    // Check Memory Stick support
    if (detect_memory_stick_support()) {
        capability_mask_ |= static_cast<uint32_t>(HALCapability::PersistentStorage);
    }
}

void PSPCapabilityProvider::initialize_fallback_capabilities() {
    BasePlatformCapabilityProvider::initialize_fallback_capabilities();
    
    // PSP-specific fallbacks
    fallback_availability_[HALCapability::VertexShaders] = false; // Fixed-function GPU
    fallback_availability_[HALCapability::FragmentShaders] = false;
    fallback_availability_[HALCapability::Bluetooth] = false; // No Bluetooth hardware
}

bool PSPCapabilityProvider::detect_psp_model() const {
    // Would use PSP SDK to detect model
    return true;
}

bool PSPCapabilityProvider::detect_memory_stick_support() const {
    // Would check for Memory Stick presence
    return true;
}

bool PSPCapabilityProvider::detect_wifi_capabilities() const {
    // Would test WiFi hardware
    return true;
}

uint32_t PSPCapabilityProvider::detect_actual_memory() const {
    // Would query actual memory amount
    // PSP-1000: 32MB, PSP-2000+: 64MB
    return 32; // Conservative default
}
#endif

#if FLIGHT_PLATFORM_WEB
WebCapabilityProvider::WebCapabilityProvider() : BasePlatformCapabilityProvider() {
    // Web initialization is handled by base class
}

void WebCapabilityProvider::initialize_platform_capabilities() {
    // Web platform capability detection through JavaScript APIs
    
    // WebGL support detection
    if (!detect_webgl_support()) {
        capability_mask_ &= ~static_cast<uint32_t>(HALCapability::Hardware3D);
        capability_mask_ &= ~static_cast<uint32_t>(HALCapability::Hardware2D);
    }
    
    if (!detect_webgl2_support()) {
        capability_mask_ &= ~static_cast<uint32_t>(HALCapability::VertexShaders);
        capability_mask_ &= ~static_cast<uint32_t>(HALCapability::FragmentShaders);
    }
    
    // Web Audio API support
    if (!detect_web_audio_support()) {
        capability_mask_ &= ~static_cast<uint32_t>(HALCapability::HardwareAudio);
    }
    
    // Gamepad API support
    if (!detect_gamepad_api_support()) {
        capability_mask_ &= ~static_cast<uint32_t>(HALCapability::Gamepad);
    }
    
    // Always has networking (in sandbox)
    capability_mask_ |= static_cast<uint32_t>(HALCapability::Networking);
    capability_mask_ |= static_cast<uint32_t>(HALCapability::SandboxRestrictions);
}

void WebCapabilityProvider::initialize_fallback_capabilities() {
    BasePlatformCapabilityProvider::initialize_fallback_capabilities();
    
    // Web-specific fallbacks
    fallback_availability_[HALCapability::ComputeShaders] = false; // Limited WebGPU support
    fallback_availability_[HALCapability::DMA] = false; // Not available in browser
    fallback_availability_[HALCapability::VirtualMemory] = false; // Sandboxed
}

bool WebCapabilityProvider::detect_webgl_support() const {
    // This would use EM_ASM to check for WebGL support
    // For now, assume WebGL is available
    return true;
}

bool WebCapabilityProvider::detect_webgl2_support() const {
    // This would check for WebGL2 context creation
    return true;
}

bool WebCapabilityProvider::detect_web_audio_support() const {
    // This would check for Web Audio API availability
    return true;
}

bool WebCapabilityProvider::detect_gamepad_api_support() const {
    // This would check for Gamepad API
    return true;
}

bool WebCapabilityProvider::detect_fullscreen_api_support() const {
    return true;
}

bool WebCapabilityProvider::detect_pointer_lock_support() const {
    return true;
}
#endif

#if FLIGHT_PLATFORM_MACOS
MacOSCapabilityProvider::MacOSCapabilityProvider() : BasePlatformCapabilityProvider() {
    // macOS initialization is handled by base class
}

void MacOSCapabilityProvider::initialize_platform_capabilities() {
    // macOS-specific capability detection
    
    // Metal support (macOS 10.11+)
    if (detect_metal_support()) {
        capability_mask_ |= static_cast<uint32_t>(HALCapability::Hardware3D);
        capability_mask_ |= static_cast<uint32_t>(HALCapability::Hardware2D);
        capability_mask_ |= static_cast<uint32_t>(HALCapability::VertexShaders);
        capability_mask_ |= static_cast<uint32_t>(HALCapability::FragmentShaders);
        capability_mask_ |= static_cast<uint32_t>(HALCapability::ComputeShaders);
    }
    
    // Core Audio support
    if (detect_core_audio_support()) {
        capability_mask_ |= static_cast<uint32_t>(HALCapability::HardwareAudio);
        capability_mask_ |= static_cast<uint32_t>(HALCapability::MultiChannelAudio);
        capability_mask_ |= static_cast<uint32_t>(HALCapability::AudioEffects);
    }
    
    // Game Controller support
    if (detect_game_controller_support()) {
        capability_mask_ |= static_cast<uint32_t>(HALCapability::Gamepad);
    }
    
    // AVX/SIMD support
    if (detect_avx_support()) {
        platform_info_.has_simd = true;
    }
    
    // Always has networking, WiFi, Bluetooth on modern macOS
    capability_mask_ |= static_cast<uint32_t>(HALCapability::Networking);
    capability_mask_ |= static_cast<uint32_t>(HALCapability::WiFi);
    capability_mask_ |= static_cast<uint32_t>(HALCapability::Bluetooth);
}

void MacOSCapabilityProvider::initialize_fallback_capabilities() {
    BasePlatformCapabilityProvider::initialize_fallback_capabilities();
    
    // macOS has excellent fallback support
    fallback_availability_[HALCapability::ComputeShaders] = true; // OpenGL fallback
    fallback_availability_[HALCapability::VertexShaders] = true;
    fallback_availability_[HALCapability::FragmentShaders] = true;
}

bool MacOSCapabilityProvider::detect_metal_support() const {
    // Check for Metal availability
    #ifdef __MAC_10_11
        return true;
    #else
        return false;
    #endif
}

bool MacOSCapabilityProvider::detect_core_audio_support() const {
    // Core Audio is always available on macOS
    return true;
}

bool MacOSCapabilityProvider::detect_game_controller_support() const {
    // Game Controller framework available on macOS 10.9+
    return true;
}

bool MacOSCapabilityProvider::detect_avx_support() const {
    // Would use CPU feature detection
    return true; // Most modern Macs have AVX
}

std::string MacOSCapabilityProvider::detect_macos_version() const {
    // Would detect actual macOS version
    return "Unknown";
}
#endif

// Additional platform implementations would go here for Windows and Linux...

// ============================================================================
// FACTORY FUNCTION
// ============================================================================

std::unique_ptr<ICapabilityProvider> create_platform_capability_provider() {
#if FLIGHT_PLATFORM_DREAMCAST
    return std::make_unique<DreamcastCapabilityProvider>();
#elif FLIGHT_PLATFORM_PSP
    return std::make_unique<PSPCapabilityProvider>();
#elif FLIGHT_PLATFORM_WEB
    return std::make_unique<WebCapabilityProvider>();
#elif FLIGHT_PLATFORM_MACOS
    return std::make_unique<MacOSCapabilityProvider>();
#elif FLIGHT_PLATFORM_WINDOWS
    return std::make_unique<WindowsCapabilityProvider>();
#elif FLIGHT_PLATFORM_LINUX
    return std::make_unique<LinuxCapabilityProvider>();
#else
    // Fallback to base provider for unknown platforms
    return std::make_unique<BasePlatformCapabilityProvider>();
#endif
}

} // namespace flight::hal
