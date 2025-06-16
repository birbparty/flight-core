/**
 * @file enhanced_hal_example.cpp
 * @brief Enhanced HAL Usage Example
 * 
 * Demonstrates the enhanced Flight HAL architecture with capability detection,
 * auto-registration, enhanced error handling, and platform coordination.
 */

#include "flight/hal/core/platform.hpp"
#include "flight/hal/interfaces/memory.hpp"
#include "../drivers/mock_memory_driver.hpp"
#include <iostream>
#include <iomanip>

using namespace flight::hal;

/**
 * @brief Print platform capabilities
 */
void print_platform_capabilities(const Platform& platform) {
    std::cout << "\n=== Platform Capabilities ===" << std::endl;
    
    const auto& capabilities = platform.get_capabilities();
    const auto& platform_info = platform.get_platform_info();
    
    // Platform information
    std::cout << "Platform: " << platform_info.platform_name << std::endl;
    std::cout << "Architecture: " << platform_info.architecture << std::endl;
    std::cout << "Performance Tier: " << capabilities::to_string(platform_info.performance_tier) << std::endl;
    std::cout << "Total Memory: " << (platform_info.total_memory / (1024 * 1024)) << " MB" << std::endl;
    std::cout << "CPU Cores: " << platform_info.cpu_cores << std::endl;
    std::cout << "Has FPU: " << (platform_info.has_fpu ? "Yes" : "No") << std::endl;
    std::cout << "Has SIMD: " << (platform_info.has_simd ? "Yes" : "No") << std::endl;
    
    // Capability list
    std::cout << "\nSupported Capabilities:" << std::endl;
    auto caps = capabilities.get_capabilities();
    for (auto cap : caps) {
        std::cout << "  - " << capabilities::to_string(cap) << std::endl;
    }
}

/**
 * @brief Print memory statistics
 */
void print_memory_stats(const MemoryStats& stats) {
    std::cout << "\n=== Memory Statistics ===" << std::endl;
    std::cout << "Total Memory: " << (stats.total_bytes / 1024) << " KB" << std::endl;
    std::cout << "Used Memory: " << (stats.used_bytes / 1024) << " KB" << std::endl;
    std::cout << "Free Memory: " << (stats.free_bytes / 1024) << " KB" << std::endl;
    std::cout << "Peak Used: " << (stats.peak_used_bytes / 1024) << " KB" << std::endl;
    std::cout << "Allocations: " << stats.allocation_count << std::endl;
    std::cout << "Fragmentation: " << stats.fragmentation_pct << "%" << std::endl;
}

/**
 * @brief Demonstrate enhanced error handling
 */
void demonstrate_error_handling() {
    std::cout << "\n=== Error Handling Demo ===" << std::endl;
    
    // Try to get a memory interface
    auto& platform = Platform::instance();
    auto memory = platform.get_interface<IMemoryInterface>();
    
    if (!memory) {
        std::cout << "❌ Memory interface not available" << std::endl;
        return;
    }
    
    // Test successful allocation
    auto result = memory->allocate(1024, MemoryAlignment::Word, MemoryFlags::Cacheable);
    if (result) {
        std::cout << "✅ Successfully allocated 1024 bytes" << std::endl;
        auto allocation = result.value();
        std::cout << "   Pointer: " << allocation.ptr << std::endl;
        std::cout << "   Actual Size: " << allocation.size << " bytes" << std::endl;
        std::cout << "   Alignment: " << static_cast<uint32_t>(allocation.alignment) << std::endl;
        
        // Free the memory
        auto free_result = memory->deallocate(allocation.ptr);
        if (free_result) {
            std::cout << "✅ Successfully freed memory" << std::endl;
        } else {
            std::cout << "❌ Failed to free memory: " << free_result.error_message() << std::endl;
        }
    } else {
        std::cout << "❌ Failed to allocate memory: " << result.error_message() << std::endl;
    }
    
    // Test invalid allocation (too large)
    auto big_result = memory->allocate(SIZE_MAX, MemoryAlignment::None, MemoryFlags::None);
    if (big_result) {
        std::cout << "⚠️  Unexpectedly succeeded with huge allocation" << std::endl;
    } else {
        std::cout << "✅ Properly rejected oversized allocation: " << big_result.error_message() << std::endl;
    }
}

/**
 * @brief Demonstrate capability-aware programming
 */
void demonstrate_capability_awareness() {
    std::cout << "\n=== Capability-Aware Programming ===" << std::endl;
    
    auto& platform = Platform::instance();
    auto memory = platform.get_interface<IMemoryInterface>();
    
    if (!memory) {
        std::cout << "❌ Memory interface not available" << std::endl;
        return;
    }
    
    // Check for DMA capability
    if (memory->supports_capability(HALCapability::DMA)) {
        std::cout << "✅ Platform supports DMA" << std::endl;
        
        // Try DMA-capable allocation
        if (memory->supports_flags(MemoryFlags::DMACapable)) {
            auto result = memory->allocate(4096, MemoryAlignment::Page, MemoryFlags::DMACapable);
            if (result) {
                std::cout << "✅ Successfully allocated DMA-capable memory" << std::endl;
                memory->deallocate(result.value().ptr);
            } else {
                std::cout << "❌ Failed DMA allocation: " << result.error_message() << std::endl;
            }
        }
    } else {
        std::cout << "ℹ️  Platform does not support DMA, using regular allocation" << std::endl;
        
        // Fallback to regular allocation
        auto result = memory->allocate(4096, MemoryAlignment::Word, MemoryFlags::Cacheable);
        if (result) {
            std::cout << "✅ Successfully allocated regular memory as fallback" << std::endl;
            memory->deallocate(result.value().ptr);
        }
    }
    
    // Check performance tier for optimization decisions
    auto tier = memory->get_performance_tier();
    std::cout << "Platform Performance Tier: " << capabilities::to_string(tier) << std::endl;
    
    switch (tier) {
        case PerformanceTier::Minimal:
            std::cout << "  📱 Using minimal memory allocation strategy for embedded platform" << std::endl;
            break;
        case PerformanceTier::Limited:
            std::cout << "  🎮 Using optimized allocation strategy for gaming console" << std::endl;
            break;
        case PerformanceTier::Standard:
            std::cout << "  💻 Using standard allocation strategy for modern mobile/web" << std::endl;
            break;
        case PerformanceTier::High:
            std::cout << "  🖥️  Using aggressive allocation strategy for desktop PC" << std::endl;
            break;
    }
}

/**
 * @brief Main demonstration function
 */
int main() {
    std::cout << "Flight HAL Enhanced Architecture Demo" << std::endl;
    std::cout << "====================================" << std::endl;
    
    try {
        // Initialize the platform
        auto& platform = Platform::instance();
        
        std::cout << "\n🚀 Initializing HAL Platform..." << std::endl;
        auto init_result = platform.initialize();
        
        if (!init_result) {
            std::cerr << "❌ Failed to initialize platform: " << init_result.error_message() << std::endl;
            return 1;
        }
        
        std::cout << "✅ Platform initialized successfully" << std::endl;
        
        // Print platform information
        print_platform_capabilities(platform);
        
        // Check available interfaces
        auto active_interfaces = platform.get_active_interfaces();
        std::cout << "\n=== Active Interfaces ===" << std::endl;
        for (const auto& interface : active_interfaces) {
            std::cout << "  - " << interface << std::endl;
        }
        
        // Get performance statistics
        auto stats = platform.get_performance_stats();
        std::cout << "\n=== Platform Statistics ===" << std::endl;
        std::cout << "Total Interfaces: " << stats.total_interfaces << std::endl;
        std::cout << "Active Interfaces: " << stats.active_interfaces << std::endl;
        std::cout << "Failed Interfaces: " << stats.failed_interfaces << std::endl;
        std::cout << "Initialization Time: " << std::fixed << std::setprecision(2) 
                  << stats.initialization_time_ms << " ms" << std::endl;
        
        // Demonstrate memory interface usage
        if (platform.has_interface<IMemoryInterface>()) {
            auto memory = platform.get_interface<IMemoryInterface>();
            
            // Get memory statistics
            auto mem_stats_result = memory->get_memory_stats();
            if (mem_stats_result) {
                print_memory_stats(mem_stats_result.value());
            }
            
            // Demonstrate error handling
            demonstrate_error_handling();
            
            // Demonstrate capability-aware programming
            demonstrate_capability_awareness();
        } else {
            std::cout << "\n❌ Memory interface not available" << std::endl;
        }
        
        // Shutdown the platform
        std::cout << "\n🔄 Shutting down platform..." << std::endl;
        auto shutdown_result = platform.shutdown();
        
        if (shutdown_result) {
            std::cout << "✅ Platform shutdown successfully" << std::endl;
        } else {
            std::cout << "⚠️  Platform shutdown with warnings: " << shutdown_result.error_message() << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Exception: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\n🎉 Demo completed successfully!" << std::endl;
    return 0;
}
