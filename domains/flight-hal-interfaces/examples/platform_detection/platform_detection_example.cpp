/**
 * @file platform_detection_example.cpp
 * @brief Flight HAL Platform Detection System Example
 * 
 * Demonstrates how to use the platform detection system to:
 * - Identify the current platform at compile-time and runtime
 * - Query platform capabilities and constraints
 * - Make platform-specific optimizations
 * - Handle graceful degradation
 */

#include "flight/hal/core/platform_detection.hpp"
#include "flight/hal/core/platform_capabilities.hpp"
#include "flight/hal/core/hal_capabilities.hpp"

#include <iostream>
#include <iomanip>
#include <sstream>

using namespace flight::hal;

/**
 * @brief Format memory size in human-readable format
 */
std::string format_memory_size(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit_index < 4) {
        size /= 1024.0;
        unit_index++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unit_index];
    return oss.str();
}

/**
 * @brief Print platform information
 */
void print_platform_info() {
    std::cout << "=== PLATFORM DETECTION EXAMPLE ===\n\n";
    
    // Compile-time platform detection
    std::cout << "--- Compile-Time Detection ---\n";
    std::cout << "Platform Name: " << platform_detection::get_platform_name() << "\n";
    std::cout << "Architecture: " << platform_detection::get_architecture_name() << "\n";
    std::cout << "Is Dreamcast: " << (platform_detection::is_dreamcast() ? "Yes" : "No") << "\n";
    std::cout << "Is PSP: " << (platform_detection::is_psp() ? "Yes" : "No") << "\n";
    std::cout << "Is Web: " << (platform_detection::is_web() ? "Yes" : "No") << "\n";
    std::cout << "Is macOS: " << (platform_detection::is_macos() ? "Yes" : "No") << "\n";
    std::cout << "Is Desktop: " << (platform_detection::is_desktop() ? "Yes" : "No") << "\n";
    std::cout << "Is Embedded: " << (platform_detection::is_embedded() ? "Yes" : "No") << "\n";
    std::cout << "Is Resource Constrained: " << (platform_detection::is_resource_constrained() ? "Yes" : "No") << "\n";
    
    // Memory constraints
    std::cout << "\n--- Memory Constraints ---\n";
    std::cout << "Main Memory: " << platform_detection::get_main_memory_mb() << " MB\n";
    std::cout << "Video Memory: " << platform_detection::get_video_memory_mb() << " MB\n";
    std::cout << "Audio Memory: " << platform_detection::get_audio_memory_mb() << " MB\n";
    std::cout << "Total Memory: " << platform_detection::get_total_memory_mb() << " MB\n";
    
    // Compile-time capabilities
    std::cout << "\n--- Compile-Time Capabilities ---\n";
    std::cout << "Threading: " << (platform_detection::has_threading() ? "Yes" : "No") << "\n";
    std::cout << "Modern Graphics: " << (platform_detection::has_modern_graphics() ? "Yes" : "No") << "\n";
    std::cout << "Legacy Graphics: " << (platform_detection::has_legacy_graphics() ? "Yes" : "No") << "\n";
    std::cout << "Networking: " << (platform_detection::has_networking() ? "Yes" : "No") << "\n";
}

/**
 * @brief Print runtime detection results
 */
void print_runtime_info() {
    std::cout << "\n--- Runtime Detection ---\n";
    
    // Runtime platform info
    PlatformInfo info = RuntimePlatformDetector::detect_platform_info();
    std::cout << "Platform: " << info.platform_name << "\n";
    std::cout << "Architecture: " << info.architecture << "\n";
    std::cout << "Performance Tier: " << capabilities::to_string(info.performance_tier) << "\n";
    std::cout << "Total Memory: " << format_memory_size(info.total_memory) << "\n";
    std::cout << "CPU Cores: " << info.cpu_cores << "\n";
    std::cout << "Has FPU: " << (info.has_fpu ? "Yes" : "No") << "\n";
    std::cout << "Has SIMD: " << (info.has_simd ? "Yes" : "No") << "\n";
    
    // Runtime capabilities
    uint32_t capabilities_mask = RuntimePlatformDetector::detect_capabilities();
    auto capabilities_list = capabilities::extract_capabilities(capabilities_mask);
    
    std::cout << "\n--- Runtime Capabilities ---\n";
    std::cout << "Total Capabilities: " << capabilities_list.size() << "\n";
    
    for (const auto& capability : capabilities_list) {
        std::cout << "  - " << capabilities::to_string(capability) << "\n";
    }
}

/**
 * @brief Demonstrate platform-specific capability provider
 */
void print_capability_provider_info() {
    std::cout << "\n--- Platform Capability Provider ---\n";
    
    auto provider = create_platform_capability_provider();
    if (!provider) {
        std::cout << "Failed to create capability provider\n";
        return;
    }
    
    const auto& platform_info = provider->get_platform_info();
    uint32_t capability_mask = provider->get_capability_mask();
    auto capabilities_list = provider->get_capabilities();
    PerformanceTier tier = provider->get_performance_tier();
    
    std::cout << "Platform: " << platform_info.platform_name << "\n";
    std::cout << "Performance Tier: " << capabilities::to_string(tier) << "\n";
    std::cout << "Capability Count: " << capabilities_list.size() << "\n";
    
    // Test specific capabilities
    std::cout << "\n--- Capability Tests ---\n";
    std::vector<HALCapability> test_capabilities = {
        HALCapability::Threading,
        HALCapability::Hardware3D,
        HALCapability::VertexShaders,
        HALCapability::FragmentShaders,
        HALCapability::WiFi,
        HALCapability::MultiTouch,
        HALCapability::Gamepad
    };
    
    for (const auto& capability : test_capabilities) {
        bool supported = provider->supports_capability(capability);
        bool has_fallback = provider->has_fallback(capability);
        
        std::cout << std::setw(20) << capabilities::to_string(capability) << ": ";
        std::cout << (supported ? "✓" : "✗");
        if (!supported && has_fallback) {
            std::cout << " (fallback available)";
        }
        std::cout << "\n";
    }
}

/**
 * @brief Demonstrate platform-specific optimization decisions
 */
void demonstrate_optimization_decisions() {
    std::cout << "\n--- Optimization Decisions ---\n";
    
    auto provider = create_platform_capability_provider();
    if (!provider) {
        return;
    }
    
    // Memory allocation strategy
    uint64_t total_memory = provider->get_platform_info().total_memory;
    std::cout << "Memory Strategy: ";
    if (total_memory < 64 * 1024 * 1024) {
        std::cout << "Minimal allocation, aggressive pooling\n";
    } else if (total_memory < 512 * 1024 * 1024) {
        std::cout << "Conservative allocation, moderate pooling\n";
    } else {
        std::cout << "Liberal allocation, minimal pooling\n";
    }
    
    // Graphics rendering strategy
    std::cout << "Graphics Strategy: ";
    if (provider->supports_capability(HALCapability::VertexShaders)) {
        std::cout << "Modern shader-based rendering\n";
    } else if (provider->supports_capability(HALCapability::Hardware3D)) {
        std::cout << "Fixed-function hardware rendering\n";
    } else {
        std::cout << "Software rendering fallback\n";
    }
    
    // Threading strategy
    std::cout << "Threading Strategy: ";
    if (provider->supports_capability(HALCapability::Threading)) {
        uint32_t cores = provider->get_platform_info().cpu_cores;
        std::cout << "Multi-threaded (" << cores << " cores)\n";
    } else {
        std::cout << "Single-threaded\n";
    }
    
    // Audio strategy
    std::cout << "Audio Strategy: ";
    if (provider->supports_capability(HALCapability::HardwareAudio)) {
        std::cout << "Hardware-accelerated audio\n";
    } else {
        std::cout << "Software audio mixing\n";
    }
    
    // Network strategy
    std::cout << "Network Strategy: ";
    if (provider->supports_capability(HALCapability::WiFi)) {
        std::cout << "WiFi networking enabled\n";
    } else if (provider->supports_capability(HALCapability::Networking)) {
        std::cout << "Wired networking only\n";
    } else {
        std::cout << "No networking support\n";
    }
}

/**
 * @brief Demonstrate graceful degradation
 */
void demonstrate_graceful_degradation() {
    std::cout << "\n--- Graceful Degradation Examples ---\n";
    
    auto provider = create_platform_capability_provider();
    if (!provider) {
        return;
    }
    
    // Graphics degradation chain
    std::cout << "Graphics Rendering Chain:\n";
    if (provider->supports_capability(HALCapability::ComputeShaders)) {
        std::cout << "  1. Compute shader-based rendering (best)\n";
    }
    if (provider->supports_capability(HALCapability::FragmentShaders)) {
        std::cout << "  2. Programmable shader rendering\n";
    }
    if (provider->supports_capability(HALCapability::Hardware3D)) {
        std::cout << "  3. Fixed-function hardware rendering\n";
    }
    if (provider->has_fallback(HALCapability::Hardware3D)) {
        std::cout << "  4. Software 3D rendering (fallback)\n";
    }
    
    // Audio degradation chain
    std::cout << "\nAudio Processing Chain:\n";
    if (provider->supports_capability(HALCapability::AudioEffects)) {
        std::cout << "  1. Hardware DSP effects (best)\n";
    }
    if (provider->supports_capability(HALCapability::HardwareAudio)) {
        std::cout << "  2. Hardware audio mixing\n";
    }
    if (provider->has_fallback(HALCapability::HardwareAudio)) {
        std::cout << "  3. Software audio mixing (fallback)\n";
    }
    
    // Input degradation chain
    std::cout << "\nInput Handling Chain:\n";
    if (provider->supports_capability(HALCapability::MultiTouch)) {
        std::cout << "  1. Multi-touch input (best)\n";
    }
    if (provider->supports_capability(HALCapability::Gamepad)) {
        std::cout << "  2. Gamepad input\n";
    }
    std::cout << "  3. Basic input handling (always available)\n";
}

int main() {
    try {
        print_platform_info();
        print_runtime_info();
        print_capability_provider_info();
        demonstrate_optimization_decisions();
        demonstrate_graceful_degradation();
        
        std::cout << "\n=== PLATFORM DETECTION COMPLETE ===\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
