/**
 * @file basic_usage_example.cpp
 * @brief Basic Usage Example for Software-Only HAL Memory Driver
 * 
 * This example demonstrates fundamental usage patterns of the software-only
 * memory driver, showcasing the key features and best practices for
 * cross-platform HAL implementation.
 */

#include "../drivers/software_memory_driver.hpp"
#include "../../../../include/flight/hal/core/driver_registry.hpp"
#include "../../../../include/flight/hal/interfaces/memory.hpp"

#include <iostream>
#include <vector>
#include <chrono>

using namespace flight::hal;
using namespace flight::hal::software;

/**
 * @brief Demonstrate basic memory allocation and deallocation
 */
void demonstrate_basic_allocation() {
    std::cout << "\n=== Basic Memory Allocation Demo ===" << std::endl;
    
    // Create software memory driver with custom configuration
    SoftwareMemoryConfig config;
    config.max_allocation_size = 10 * 1024 * 1024;      // 10MB max
    config.total_memory_limit = 100 * 1024 * 1024;      // 100MB total
    config.enable_debug_tracking = true;
    config.enable_leak_detection = true;
    
    auto driver = std::make_unique<SoftwareMemoryDriver>(config);
    
    // Initialize the driver
    auto init_result = driver->initialize();
    if (!init_result) {
        std::cerr << "Failed to initialize driver: " << init_result.error().message() << std::endl;
        return;
    }
    
    std::cout << "Driver initialized: " << driver->get_driver_name() << std::endl;
    std::cout << "Version: " << driver->get_version() << std::endl;
    std::cout << "Performance Tier: " << static_cast<int>(driver->get_performance_tier()) << std::endl;
    
    // Basic allocation
    std::cout << "\n--- Basic Allocation ---" << std::endl;
    auto allocation_result = driver->allocate(1024);  // 1KB
    if (allocation_result) {
        auto allocation = allocation_result.value();
        std::cout << "Allocated 1KB at: " << allocation.ptr << std::endl;
        std::cout << "Actual size: " << allocation.size << " bytes" << std::endl;
        
        // Use the memory
        std::memset(allocation.ptr, 0xAA, allocation.size);
        
        // Deallocate
        auto dealloc_result = driver->deallocate(allocation.ptr);
        if (dealloc_result) {
            std::cout << "Successfully deallocated memory" << std::endl;
        } else {
            std::cerr << "Deallocation failed: " << dealloc_result.error().message() << std::endl;
        }
    } else {
        std::cerr << "Allocation failed: " << allocation_result.error().message() << std::endl;
    }
    
    // Shutdown driver
    auto shutdown_result = driver->shutdown();
    if (shutdown_result) {
        std::cout << "Driver shutdown successfully" << std::endl;
    }
}

/**
 * @brief Demonstrate different allocation types and alignment
 */
void demonstrate_advanced_allocation() {
    std::cout << "\n=== Advanced Allocation Demo ===" << std::endl;
    
    auto driver = std::make_unique<SoftwareMemoryDriver>();
    driver->initialize();
    
    // Aligned allocation
    std::cout << "\n--- Aligned Allocation ---" << std::endl;
    AllocationRequest aligned_request;
    aligned_request.size = 2048;
    aligned_request.alignment = MemoryAlignment::Cache;  // 32-byte alignment
    aligned_request.flags = MemoryFlags::Zero;           // Zero-initialize
    aligned_request.preferred_type = MemoryType::System;
    
    auto aligned_result = driver->allocate(aligned_request);
    if (aligned_result) {
        auto allocation = aligned_result.value();
        std::cout << "Allocated " << allocation.size << " bytes" << std::endl;
        std::cout << "Alignment: " << static_cast<uint32_t>(allocation.alignment) << " bytes" << std::endl;
        std::cout << "Address: " << allocation.ptr << std::endl;
        
        // Verify alignment
        auto addr = reinterpret_cast<uintptr_t>(allocation.ptr);
        bool is_aligned = (addr % static_cast<uint32_t>(MemoryAlignment::Cache)) == 0;
        std::cout << "Properly aligned: " << (is_aligned ? "Yes" : "No") << std::endl;
        
        // Verify zero initialization
        auto* bytes = static_cast<uint8_t*>(allocation.ptr);
        bool is_zeroed = true;
        for (size_t i = 0; i < std::min(allocation.size, size_t(64)); ++i) {
            if (bytes[i] != 0) {
                is_zeroed = false;
                break;
            }
        }
        std::cout << "Zero initialized: " << (is_zeroed ? "Yes" : "No") << std::endl;
        
        driver->deallocate(allocation.ptr);
    }
    
    driver->shutdown();
}

/**
 * @brief Demonstrate memory statistics and monitoring
 */
void demonstrate_memory_statistics() {
    std::cout << "\n=== Memory Statistics Demo ===" << std::endl;
    
    auto driver = std::make_unique<SoftwareMemoryDriver>();
    driver->initialize();
    
    // Get initial statistics
    auto stats_result = driver->get_memory_stats();
    if (stats_result) {
        auto stats = stats_result.value();
        std::cout << "Initial Memory Stats:" << std::endl;
        std::cout << "  Total: " << stats.total_bytes << " bytes" << std::endl;
        std::cout << "  Used: " << stats.used_bytes << " bytes" << std::endl;
        std::cout << "  Free: " << stats.free_bytes << " bytes" << std::endl;
        std::cout << "  Allocations: " << stats.allocation_count << std::endl;
    }
    
    // Allocate several blocks
    std::vector<void*> allocations;
    const size_t block_size = 4096;  // 4KB blocks
    const size_t num_blocks = 10;
    
    std::cout << "\n--- Allocating " << num_blocks << " blocks of " << block_size << " bytes ---" << std::endl;
    
    for (size_t i = 0; i < num_blocks; ++i) {
        auto result = driver->allocate(block_size);
        if (result) {
            allocations.push_back(result.value().ptr);
            std::cout << "Block " << (i + 1) << " allocated at: " << result.value().ptr << std::endl;
        } else {
            std::cerr << "Failed to allocate block " << (i + 1) << ": " << result.error().message() << std::endl;
        }
    }
    
    // Get updated statistics
    stats_result = driver->get_memory_stats();
    if (stats_result) {
        auto stats = stats_result.value();
        std::cout << "\nAfter Allocation Memory Stats:" << std::endl;
        std::cout << "  Total: " << stats.total_bytes << " bytes" << std::endl;
        std::cout << "  Used: " << stats.used_bytes << " bytes" << std::endl;
        std::cout << "  Free: " << stats.free_bytes << " bytes" << std::endl;
        std::cout << "  Allocations: " << stats.allocation_count << std::endl;
        std::cout << "  Peak Usage: " << stats.peak_used_bytes << " bytes" << std::endl;
        std::cout << "  Efficiency: " << (stats.allocation_efficiency * 100.0) << "%" << std::endl;
    }
    
    // Clean up allocations
    std::cout << "\n--- Deallocating blocks ---" << std::endl;
    for (void* ptr : allocations) {
        auto result = driver->deallocate(ptr);
        if (!result) {
            std::cerr << "Failed to deallocate: " << result.error().message() << std::endl;
        }
    }
    
    // Final statistics
    stats_result = driver->get_memory_stats();
    if (stats_result) {
        auto stats = stats_result.value();
        std::cout << "\nFinal Memory Stats:" << std::endl;
        std::cout << "  Used: " << stats.used_bytes << " bytes" << std::endl;
        std::cout << "  Allocations: " << stats.allocation_count << std::endl;
    }
    
    driver->shutdown();
}

/**
 * @brief Demonstrate memory pressure monitoring
 */
void demonstrate_pressure_monitoring() {
    std::cout << "\n=== Memory Pressure Monitoring Demo ===" << std::endl;
    
    // Configure with low memory limit to trigger pressure
    SoftwareMemoryConfig config;
    config.total_memory_limit = 1024 * 1024;      // 1MB limit
    config.memory_pressure_threshold = 0.7;       // 70% triggers pressure
    
    auto driver = std::make_unique<SoftwareMemoryDriver>(config);
    driver->initialize();
    
    // Register pressure callback
    bool pressure_triggered = false;
    auto callback = [&pressure_triggered](MemoryPressureLevel level, const MemoryPressureInfo& info) {
        std::cout << "Memory pressure detected!" << std::endl;
        std::cout << "  Level: " << static_cast<int>(level) << std::endl;
        std::cout << "  Available: " << info.available_bytes << " bytes" << std::endl;
        std::cout << "  Pressure Ratio: " << (info.pressure_ratio * 100.0) << "%" << std::endl;
        pressure_triggered = true;
    };
    
    auto callback_result = driver->register_pressure_callback(MemoryPressureLevel::Medium, callback);
    if (callback_result) {
        std::cout << "Pressure callback registered with ID: " << callback_result.value() << std::endl;
    }
    
    // Allocate memory to trigger pressure
    std::vector<void*> allocations;
    const size_t block_size = 64 * 1024;  // 64KB blocks
    
    std::cout << "\n--- Allocating memory to trigger pressure ---" << std::endl;
    for (int i = 0; i < 20; ++i) {  // Try to allocate 1.25MB (will exceed 1MB limit)
        auto result = driver->allocate(block_size);
        if (result) {
            allocations.push_back(result.value().ptr);
            std::cout << "Allocated block " << (i + 1) << std::endl;
        } else {
            std::cout << "Allocation failed: " << result.error().message() << std::endl;
            break;
        }
        
        if (pressure_triggered) {
            std::cout << "Pressure triggered after " << (i + 1) << " allocations" << std::endl;
            break;
        }
    }
    
    // Clean up
    for (void* ptr : allocations) {
        driver->deallocate(ptr);
    }
    
    if (callback_result) {
        driver->unregister_pressure_callback(callback_result.value());
    }
    
    driver->shutdown();
}

/**
 * @brief Demonstrate performance characteristics
 */
void demonstrate_performance() {
    std::cout << "\n=== Performance Demo ===" << std::endl;
    
    auto driver = std::make_unique<SoftwareMemoryDriver>();
    driver->initialize();
    
    // Benchmark allocation performance
    const size_t num_allocations = 1000;
    const size_t allocation_size = 1024;  // 1KB
    
    std::cout << "\n--- Allocation Performance Test ---" << std::endl;
    std::cout << "Allocating " << num_allocations << " blocks of " << allocation_size << " bytes" << std::endl;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<void*> allocations;
    allocations.reserve(num_allocations);
    
    for (size_t i = 0; i < num_allocations; ++i) {
        auto result = driver->allocate(allocation_size);
        if (result) {
            allocations.push_back(result.value().ptr);
        }
    }
    
    auto alloc_time = std::chrono::high_resolution_clock::now();
    
    // Benchmark deallocation performance
    for (void* ptr : allocations) {
        driver->deallocate(ptr);
    }
    
    auto dealloc_time = std::chrono::high_resolution_clock::now();
    
    // Calculate timings
    auto alloc_duration = std::chrono::duration_cast<std::chrono::microseconds>(alloc_time - start_time).count();
    auto dealloc_duration = std::chrono::duration_cast<std::chrono::microseconds>(dealloc_time - alloc_time).count();
    
    std::cout << "Results:" << std::endl;
    std::cout << "  Allocation time: " << alloc_duration << " μs" << std::endl;
    std::cout << "  Deallocation time: " << dealloc_duration << " μs" << std::endl;
    std::cout << "  Avg allocation time: " << (alloc_duration / num_allocations) << " μs per allocation" << std::endl;
    std::cout << "  Avg deallocation time: " << (dealloc_duration / num_allocations) << " μs per deallocation" << std::endl;
    
    driver->shutdown();
}

/**
 * @brief Main function demonstrating software-only HAL usage
 */
int main() {
    std::cout << "Flight HAL Software-Only Reference Implementation Demo" << std::endl;
    std::cout << "=====================================================" << std::endl;
    
    try {
        // Run all demonstrations
        demonstrate_basic_allocation();
        demonstrate_advanced_allocation();
        demonstrate_memory_statistics();
        demonstrate_pressure_monitoring();
        demonstrate_performance();
        
        std::cout << "\n=== Demo Complete ===" << std::endl;
        std::cout << "All software HAL features demonstrated successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Demo failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
