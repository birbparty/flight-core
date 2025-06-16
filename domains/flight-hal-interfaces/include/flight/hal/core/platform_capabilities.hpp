/**
 * @file platform_capabilities.hpp
 * @brief Platform-Specific Capability Providers
 * 
 * Defines platform-specific capability providers that leverage both
 * compile-time and runtime detection to provide accurate capability
 * information for each target platform.
 */

#pragma once

#include "hal_capabilities.hpp"
#include "platform_detection.hpp"
#include <unordered_map>

namespace flight::hal {

/**
 * @brief Base platform capability provider
 * 
 * Provides common functionality for all platform-specific capability providers.
 * Uses the runtime platform detection system to provide accurate capabilities.
 */
class BasePlatformCapabilityProvider : public ICapabilityProvider {
public:
    BasePlatformCapabilityProvider();
    virtual ~BasePlatformCapabilityProvider() = default;
    
    // ICapabilityProvider implementation
    bool supports_capability(HALCapability capability) const override;
    uint32_t get_capability_mask() const override;
    std::vector<HALCapability> get_capabilities() const override;
    PerformanceTier get_performance_tier() const override;
    const PlatformInfo& get_platform_info() const override;
    bool has_fallback(HALCapability capability) const override;

protected:
    /**
     * @brief Initialize platform-specific capabilities
     * Override this in derived classes to add platform-specific detection
     */
    virtual void initialize_platform_capabilities();
    
    /**
     * @brief Initialize fallback capabilities
     * Override this in derived classes to define platform-specific fallbacks
     */
    virtual void initialize_fallback_capabilities();
    
    uint32_t capability_mask_;
    PlatformInfo platform_info_;
    std::unordered_map<HALCapability, bool> fallback_availability_;
};

#if FLIGHT_PLATFORM_DREAMCAST
/**
 * @brief Dreamcast-specific capability provider
 * 
 * Provides capabilities specific to the Sega Dreamcast platform,
 * including PowerVR2 graphics and SH-4 processor features.
 */
class DreamcastCapabilityProvider : public BasePlatformCapabilityProvider {
public:
    DreamcastCapabilityProvider();

protected:
    void initialize_platform_capabilities() override;
    void initialize_fallback_capabilities() override;

private:
    bool detect_modem_support() const;
    bool detect_vmu_support() const;
    bool detect_maple_devices() const;
};
#endif

#if FLIGHT_PLATFORM_PSP
/**
 * @brief PSP-specific capability provider
 * 
 * Provides capabilities specific to the Sony PSP platform,
 * including detection of PSP model differences and WiFi capabilities.
 */
class PSPCapabilityProvider : public BasePlatformCapabilityProvider {
public:
    PSPCapabilityProvider();

protected:
    void initialize_platform_capabilities() override;
    void initialize_fallback_capabilities() override;

private:
    bool detect_psp_model() const;
    bool detect_memory_stick_support() const;
    bool detect_wifi_capabilities() const;
    uint32_t detect_actual_memory() const;
};
#endif

#if FLIGHT_PLATFORM_WEB
/**
 * @brief Web/Emscripten-specific capability provider
 * 
 * Provides capabilities specific to the web platform,
 * including WebGL feature detection and browser API availability.
 */
class WebCapabilityProvider : public BasePlatformCapabilityProvider {
public:
    WebCapabilityProvider();

protected:
    void initialize_platform_capabilities() override;
    void initialize_fallback_capabilities() override;

private:
    bool detect_webgl_support() const;
    bool detect_webgl2_support() const;
    bool detect_web_audio_support() const;
    bool detect_gamepad_api_support() const;
    bool detect_fullscreen_api_support() const;
    bool detect_pointer_lock_support() const;
};
#endif

#if FLIGHT_PLATFORM_MACOS
/**
 * @brief macOS-specific capability provider
 * 
 * Provides capabilities specific to the macOS platform,
 * including Metal support and system feature detection.
 */
class MacOSCapabilityProvider : public BasePlatformCapabilityProvider {
public:
    MacOSCapabilityProvider();

protected:
    void initialize_platform_capabilities() override;
    void initialize_fallback_capabilities() override;

private:
    bool detect_metal_support() const;
    bool detect_core_audio_support() const;
    bool detect_game_controller_support() const;
    bool detect_avx_support() const;
    std::string detect_macos_version() const;
};
#endif

#if FLIGHT_PLATFORM_WINDOWS
/**
 * @brief Windows-specific capability provider
 * 
 * Provides capabilities specific to the Windows platform,
 * including DirectX support and Windows API feature detection.
 */
class WindowsCapabilityProvider : public BasePlatformCapabilityProvider {
public:
    WindowsCapabilityProvider();

protected:
    void initialize_platform_capabilities() override;
    void initialize_fallback_capabilities() override;

private:
    bool detect_directx_support() const;
    bool detect_wasapi_support() const;
    bool detect_xinput_support() const;
    bool detect_sse_support() const;
    std::string detect_windows_version() const;
};
#endif

#if FLIGHT_PLATFORM_LINUX
/**
 * @brief Linux-specific capability provider
 * 
 * Provides capabilities specific to the Linux platform,
 * including OpenGL/Vulkan support and system feature detection.
 */
class LinuxCapabilityProvider : public BasePlatformCapabilityProvider {
public:
    LinuxCapabilityProvider();

protected:
    void initialize_platform_capabilities() override;
    void initialize_fallback_capabilities() override;

private:
    bool detect_opengl_support() const;
    bool detect_vulkan_support() const;
    bool detect_alsa_support() const;
    bool detect_pulse_audio_support() const;
    bool detect_evdev_support() const;
};
#endif

/**
 * @brief Factory function to create platform-specific capability provider
 * 
 * Creates the appropriate capability provider based on the current platform.
 * This function uses compile-time platform detection to instantiate the
 * correct provider type.
 * 
 * @return Unique pointer to the platform-specific capability provider
 */
std::unique_ptr<ICapabilityProvider> create_platform_capability_provider();

} // namespace flight::hal
