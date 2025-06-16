/**
 * @file basic_platform_example.cpp
 * @brief Comprehensive example demonstrating Flight HAL Platform Interface usage
 */

#include "flight/hal/core/platform.hpp"
#include "flight/hal/core/platform_config.hpp"
#include "flight/hal/interfaces/graphics.hpp"
#include "flight/hal/interfaces/audio.hpp"
#include "flight/hal/interfaces/memory.hpp"
#include <iostream>
#include <chrono>
#include <thread>

using namespace flight::hal;

void demonstrate_basic_platform_usage() {
    std::cout << "=== Flight HAL Platform Interface Demo ===" << std::endl;
    
    // Get the platform singleton
    auto& platform = Platform::instance();
    
    // Check if platform is already initialized
    if (platform.is_initialized()) {
        std::cout << "Platform is already initialized!" << std::endl;
        return;
    }
    
    std::cout << "Initializing Flight HAL Platform..." << std::endl;
    
    // Initialize the platform
    auto init_result = platform.initialize();
    if (!init_result.is_success()) {
        std::cout << "Failed to initialize platform: " << init_result.error().message() << std::endl;
        return;
    }
    
    std::cout << "Platform initialized successfully!" << std::endl;
    
    // Get platform information
    const auto& platform_info = platform.get_platform_info();
    std::cout << "\nPlatform Information:" << std::endl;
    std::cout << "  Name: " << platform_info.platform_name << std::endl;
    std::cout << "  Architecture: " << platform_info.architecture << std::endl;
    std::cout << "  Performance Tier: " << static_cast<int>(platform_info.performance_tier) << std::endl;
    std::cout << "  Total Memory: " << (platform_info.total_memory / (1024 * 1024)) << " MB" << std::endl;
    std::cout << "  CPU Cores: " << platform_info.cpu_cores << std::endl;
    std::cout << "  Has FPU: " << (platform_info.has_fpu ? "Yes" : "No") << std::endl;
    std::cout << "  Has SIMD: " << (platform_info.has_simd ? "Yes" : "No") << std::endl;
    
    // Get platform capabilities
    const auto& capabilities = platform.get_capabilities();
    std::cout << "\nPlatform Capabilities:" << std::endl;
    std::cout << "  Threading: " << (capabilities.supports_capability(HALCapability::Threading) ? "Yes" : "No") << std::endl;
    std::cout << "  Hardware 3D: " << (capabilities.supports_capability(HALCapability::Hardware3D) ? "Yes" : "No") << std::endl;
    std::cout << "  Virtual Memory: " << (capabilities.supports_capability(HALCapability::VirtualMemory) ? "Yes" : "No") << std::endl;
    std::cout << "  Networking: " << (capabilities.supports_capability(HALCapability::Networking) ? "Yes" : "No") << std::endl;
    
    // Get active interfaces
    auto active_interfaces = platform.get_active_interfaces();
    std::cout << "\nActive Interfaces (" << active_interfaces.size() << "):" << std::endl;
    for (const auto& interface_name : active_interfaces) {
        std::cout << "  - " << interface_name << std::endl;
    }
    
    // Get performance statistics
    auto stats = platform.get_performance_stats();
    std::cout << "\nPerformance Statistics:" << std::endl;
    std::cout << "  Total Interfaces: " << stats.total_interfaces << std::endl;
    std::cout << "  Active Interfaces: " << stats.active_interfaces << std::endl;
    std::cout << "  Failed Interfaces: " << stats.failed_interfaces << std::endl;
    std::cout << "  Initialization Time: " << stats.initialization_time_ms << " ms" << std::endl;
    
    // Demonstrate resource coordination
    std::cout << "\n=== Resource Coordination Demo ===" << std::endl;
    
    // Request exclusive access to a GPU resource
    auto gpu_request = platform.request_exclusive_resource("gpu_memory_pool", "graphics_driver");
    if (gpu_request.is_success()) {
        std::cout << "Successfully acquired exclusive access to GPU memory pool" << std::endl;
        
        // Try to request the same resource from another driver (should fail)
        auto audio_request = platform.request_exclusive_resource("gpu_memory_pool", "audio_driver");
        if (!audio_request.is_success()) {
            std::cout << "Audio driver correctly denied access to locked GPU resource" << std::endl;
        }
        
        // Release the resource
        auto release_result = platform.release_resource("gpu_memory_pool", "graphics_driver");
        if (release_result.is_success()) {
            std::cout << "Successfully released GPU memory pool" << std::endl;
        }
    }
    
    // Demonstrate shared resource access
    auto shared_timer_1 = platform.request_shared_resource("system_timer", "driver_1");
    auto shared_timer_2 = platform.request_shared_resource("system_timer", "driver_2");
    
    if (shared_timer_1.is_success() && shared_timer_2.is_success()) {
        std::cout << "Multiple drivers successfully sharing system timer resource" << std::endl;
        
        // Release shared resources
        platform.release_resource("system_timer", "driver_1");
        platform.release_resource("system_timer", "driver_2");
    }
    
    std::cout << "\n=== Shutdown Demo ===" << std::endl;
    
    // Shutdown the platform
    auto shutdown_result = platform.shutdown();
    if (shutdown_result.is_success()) {
        std::cout << "Platform shutdown successfully!" << std::endl;
    } else {
        std::cout << "Platform shutdown failed: " << shutdown_result.error().message() << std::endl;
    }
}

void demonstrate_configuration_management() {
    std::cout << "\n=== Configuration Management Demo ===" << std::endl;
    
    // Create different configuration profiles
    auto default_config = PlatformConfig::create_default();
    auto minimal_config = PlatformConfig::create_minimal();
    auto high_perf_config = PlatformConfig::create_high_performance();
    
    std::cout << "Configuration Profiles:" << std::endl;
    std::cout << "  Default: " << default_config.memory_budget.total_budget_mb << " MB total memory" << std::endl;
    std::cout << "  Minimal: " << minimal_config.memory_budget.total_budget_mb << " MB total memory" << std::endl;
    std::cout << "  High Performance: " << high_perf_config.memory_budget.total_budget_mb << " MB total memory" << std::endl;
    
    // Validate configurations
    auto default_validation = default_config.validate();
    auto minimal_validation = minimal_config.validate();
    auto high_perf_validation = high_perf_config.validate();
    
    std::cout << "\nValidation Results:" << std::endl;
    std::cout << "  Default: " << (default_validation.is_success() ? "Valid" : "Invalid") << std::endl;
    std::cout << "  Minimal: " << (minimal_validation.is_success() ? "Valid" : "Invalid") << std::endl;
    std::cout << "  High Performance: " << (high_perf_validation.is_success() ? "Valid" : "Invalid") << std::endl;
    
    // Demonstrate configuration merging
    auto merged_config = PlatformConfigManager::merge_configs(default_config, minimal_config);
    std::cout << "\nMerged Configuration:" << std::endl;
    std::cout << "  Total Memory: " << merged_config.memory_budget.total_budget_mb << " MB" << std::endl;
    std::cout << "  Graphics Memory: " << merged_config.memory_budget.graphics_budget_mb << " MB" << std::endl;
    std::cout << "  Audio Memory: " << merged_config.memory_budget.audio_budget_mb << " MB" << std::endl;
    
    // Demonstrate environment variable loading
    auto env_config = PlatformConfigManager::load_from_environment(default_config);
    if (env_config.is_success()) {
        std::cout << "\nEnvironment configuration loaded successfully" << std::endl;
    }
    
    // Demonstrate platform-specific validation
    auto dreamcast_validation = PlatformConfigManager::validate_for_platform(high_perf_config, "dreamcast");
    auto windows_validation = PlatformConfigManager::validate_for_platform(high_perf_config, "windows");
    
    std::cout << "\nPlatform-Specific Validation:" << std::endl;
    std::cout << "  High-perf config on Dreamcast: " << (dreamcast_validation.is_success() ? "Valid" : "Invalid") << std::endl;
    std::cout << "  High-perf config on Windows: " << (windows_validation.is_success() ? "Valid" : "Invalid") << std::endl;
}

void demonstrate_cross_driver_coordination() {
    std::cout << "\n=== Cross-Driver Coordination Demo ===" << std::endl;
    
    auto& platform = Platform::instance();
    
    // Simulate a complex resource coordination scenario
    std::cout << "Simulating GPU-Audio coordination scenario..." << std::endl;
    
    // Graphics driver requests GPU memory
    auto gpu_memory = platform.request_exclusive_resource("gpu_memory_bank_1", "graphics_driver");
    if (gpu_memory.is_success()) {
        std::cout << "Graphics driver acquired GPU memory bank 1" << std::endl;
        
        // Audio driver requests shared DMA channel
        auto dma_channel = platform.request_shared_resource("dma_channel_2", "audio_driver");
        if (dma_channel.is_success()) {
            std::cout << "Audio driver acquired shared DMA channel 2" << std::endl;
            
            // Graphics driver also needs the DMA channel (should succeed - shared)
            auto graphics_dma = platform.request_shared_resource("dma_channel_2", "graphics_driver");
            if (graphics_dma.is_success()) {
                std::cout << "Graphics driver also acquired shared DMA channel 2" << std::endl;
                
                // Simulate some work
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                
                // Release resources
                platform.release_resource("dma_channel_2", "graphics_driver");
                platform.release_resource("dma_channel_2", "audio_driver");
                std::cout << "Both drivers released shared DMA channel" << std::endl;
            }
        }
        
        platform.release_resource("gpu_memory_bank_1", "graphics_driver");
        std::cout << "Graphics driver released GPU memory bank 1" << std::endl;
    }
}

int main() {
    try {
        // Demonstrate basic platform usage
        demonstrate_basic_platform_usage();
        
        // Demonstrate configuration management
        demonstrate_configuration_management();
        
        // Demonstrate cross-driver coordination
        demonstrate_cross_driver_coordination();
        
        std::cout << "\n=== Demo Complete ===" << std::endl;
        std::cout << "All platform interface features demonstrated successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Demo failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
