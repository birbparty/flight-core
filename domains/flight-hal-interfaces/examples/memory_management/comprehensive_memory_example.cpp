/**
 * @file comprehensive_memory_example.cpp
 * @brief Comprehensive Memory Management Example
 * 
 * Demonstrates the full capabilities of the Flight HAL memory interface
 * including multiple allocators, memory types, pressure management,
 * and platform-specific optimizations.
 */

#include "../../include/flight/hal/interfaces/memory.hpp"
#include "../../include/flight/hal/allocators/linear_allocator.hpp"
#include "../../include/flight/hal/allocators/pool_allocator.hpp"
#include "../../include/flight/hal/core/hal_error_macros.hpp"
#include "../../include/flight/hal/core/hal_logging.hpp"
#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <thread>
#include <string>
#include <exception>

using namespace flight::hal;

/**
 * @brief Example game object for pool allocation demonstration
 */
struct GameObject {
    float x, y, z;
    float vx, vy, vz;
    uint32_t id;
    bool active;
    
    GameObject(uint32_t obj_id) : x(0), y(0), z(0), vx(0), vy(0), vz(0), id(obj_id), active(true) {}
    
    void update(float dt) {
        if (active) {
            x += vx * dt;
            y += vy * dt;
            z += vz * dt;
        }
    }
};

/**
 * @brief Memory pressure callback for demonstration
 */
void on_memory_pressure(MemoryPressureLevel level, const MemoryPressureInfo& info) {
    std::cout << "🚨 Memory Pressure Alert: " << memory::to_string(level) 
              << " (Available: " << info.available_bytes << " bytes)\n";
    
    switch (level) {
        case MemoryPressureLevel::Low:
            std::cout << "   💡 Consider freeing non-essential caches\n";
            break;
        case MemoryPressureLevel::Medium:
            std::cout << "   ⚠️  Free temporary buffers and reduce quality\n";
            break;
        case MemoryPressureLevel::High:
            std::cout << "   🔥 Critical: Free all non-essential memory\n";
            break;
        case MemoryPressureLevel::Critical:
            std::cout << "   💀 EMERGENCY: System may crash soon!\n";
            break;
        default:
            break;
    }
}

/**
 * @brief Demonstrate linear allocator usage
 */
void demonstrate_linear_allocator() {
    std::cout << "\n🔄 === Linear Allocator Demo ===\n";
    
    // Create 1MB linear allocator for frame-based allocations
    allocators::LinearAllocator linear_alloc(1024 * 1024, "FrameAllocator");
    
    std::cout << "📊 Initial Stats:\n";
    auto stats = linear_alloc.get_stats();
    std::cout << "   Capacity: " << stats.total_capacity << " bytes\n";
    std::cout << "   Used: " << stats.used_bytes << " bytes\n";
    std::cout << "   Free: " << stats.free_bytes << " bytes\n";
    
    // Allocate some temporary objects
    std::vector<void*> allocations;
    
    // Simulate frame allocations
    for (int frame = 0; frame < 3; ++frame) {
        std::cout << "\n🎬 Frame " << frame << ":\n";
        
        // Create checkpoint for this frame
        auto checkpoint = linear_alloc.create_checkpoint();
        std::cout << "   📍 Checkpoint created at position: " << checkpoint << "\n";
        
        // Allocate various temporary objects
        for (int i = 0; i < 10; ++i) {
            auto result = linear_alloc.allocate(128, alignof(std::max_align_t));
            if (result) {
                allocations.push_back(result.value());
                std::cout << "   ✅ Allocated 128 bytes at " << result.value() << "\n";
            }
        }
        
        std::cout << "   📊 Frame Stats: Used=" << linear_alloc.get_stats().used_bytes 
                  << ", Remaining=" << linear_alloc.get_remaining_space() << "\n";
        
        // Simulate frame processing
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Reset to checkpoint (frame cleanup)
        linear_alloc.restore_checkpoint(checkpoint);
        std::cout << "   🔄 Reset to checkpoint - memory freed\n";
        allocations.clear();
    }
    
    std::cout << "📊 Final Stats:\n";
    stats = linear_alloc.get_stats();
    std::cout << "   Used: " << stats.used_bytes << " bytes\n";
    std::cout << "   Allocations: " << stats.allocation_count << "\n";
}

/**
 * @brief Demonstrate pool allocator usage
 */
void demonstrate_pool_allocator() {
    std::cout << "\n🎯 === Pool Allocator Demo ===\n";
    
    // Create object pool for GameObjects
    allocators::ObjectPool<GameObject> object_pool(100);
    
    std::cout << "🎮 Created GameObject pool with 100 objects\n";
    std::cout << "📊 Initial Stats:\n";
    auto stats = object_pool.get_stats();
    std::cout << "   Capacity: " << stats.total_capacity << " bytes\n";
    std::cout << "   Block Size: " << sizeof(GameObject) << " bytes\n";
    std::cout << "   Free Blocks: " << stats.free_bytes / sizeof(GameObject) << "\n";
    
    // Allocate game objects
    std::vector<GameObject*> objects;
    
    std::cout << "\n🚀 Spawning game objects:\n";
    for (int i = 0; i < 50; ++i) {
        auto* obj = object_pool.acquire(i);
        if (obj) {
            obj->x = i * 10.0f;
            obj->vx = (i % 2 == 0) ? 1.0f : -1.0f;
            objects.push_back(obj);
            
            if (i % 10 == 0) {
                std::cout << "   🎯 Spawned object " << i << " at (" << obj->x << ", 0, 0)\n";
            }
        }
    }
    
    std::cout << "📊 After allocation:\n";
    stats = object_pool.get_stats();
    std::cout << "   Used: " << stats.used_bytes << " bytes\n";
    std::cout << "   Free: " << stats.free_bytes << " bytes\n";
    std::cout << "   Allocations: " << stats.allocation_count << "\n";
    
    // Simulate game loop
    std::cout << "\n🎮 Running game simulation:\n";
    for (int frame = 0; frame < 5; ++frame) {
        for (auto* obj : objects) {
            obj->update(0.016f); // 60 FPS
        }
        
        // Randomly despawn some objects
        if (frame == 2 && objects.size() > 20) {
            std::cout << "   💥 Despawning some objects...\n";
            for (int i = 0; i < 20; ++i) {
                if (!objects.empty()) {
                    object_pool.release(objects.back());
                    objects.pop_back();
                }
            }
            
            stats = object_pool.get_stats();
            std::cout << "   📊 After despawn - Free: " << stats.free_bytes / sizeof(GameObject) << " objects\n";
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    // Cleanup
    for (auto* obj : objects) {
        object_pool.release(obj);
    }
    
    std::cout << "🧹 Cleanup complete\n";
    std::cout << "📊 Final pool state: " << (object_pool.is_empty() ? "Empty" : "Not Empty") << "\n";
}

/**
 * @brief Demonstrate memory types and allocation strategies
 */
void demonstrate_memory_types() {
    std::cout << "\n🏷️ === Memory Types Demo ===\n";
    
    // Demonstrate different memory type allocations
    std::vector<MemoryType> types = {
        MemoryType::System,
        MemoryType::Video,
        MemoryType::Audio,
        MemoryType::Network,
        MemoryType::Temporary
    };
    
    for (auto type : types) {
        std::cout << "📝 " << memory::to_string(type) << " Memory:\n";
        
        // Create allocation request for this type
        auto request = memory::make_allocation_request(1024, type, MemoryAlignment::Cache);
        
        std::cout << "   Request: " << request.size << " bytes, "
                  << memory::to_string(request.preferred_type) << " type\n";
        std::cout << "   Alignment: " << static_cast<uint32_t>(request.alignment) << " bytes\n";
        std::cout << "   Fallback allowed: " << (request.allow_fallback ? "Yes" : "No") << "\n";
        
        // Demonstrate memory alignment utilities
        size_t aligned_size = memory::align_size(1000, 64);
        std::cout << "   Aligned 1000 bytes to 64-byte boundary: " << aligned_size << " bytes\n";
    }
}

/**
 * @brief Demonstrate memory pressure monitoring
 */
void demonstrate_memory_pressure() {
    std::cout << "\n⚡ === Memory Pressure Demo ===\n";
    
    std::cout << "🔍 Simulating memory pressure scenarios:\n";
    
    // Simulate different pressure levels
    std::vector<MemoryPressureLevel> levels = {
        MemoryPressureLevel::None,
        MemoryPressureLevel::Low,
        MemoryPressureLevel::Medium,
        MemoryPressureLevel::High,
        MemoryPressureLevel::Critical
    };
    
    for (auto level : levels) {
        MemoryPressureInfo info{};
        info.level = level;
        info.available_bytes = (level == MemoryPressureLevel::None) ? 16*1024*1024 : 
                              (level == MemoryPressureLevel::Low) ? 4*1024*1024 :
                              (level == MemoryPressureLevel::Medium) ? 1*1024*1024 :
                              (level == MemoryPressureLevel::High) ? 256*1024 :
                              64*1024; // Critical
        info.pressure_ratio = static_cast<double>(level) / 4.0;
        info.gc_recommended = (level >= MemoryPressureLevel::Medium);
        info.emergency_reserve = 128*1024;
        
        on_memory_pressure(level, info);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

/**
 * @brief Demonstrate platform-specific optimizations
 */
void demonstrate_platform_optimizations() {
    std::cout << "\n🖥️ === Platform Optimizations Demo ===\n";
    
    // Simulate different platform constraints
    struct PlatformConfig {
        std::string name;
        size_t total_memory;
        size_t max_allocation;
        bool has_virtual_memory;
        bool supports_dma;
    };
    
    std::vector<PlatformConfig> platforms = {
        {"Dreamcast", 16*1024*1024, 1*1024*1024, false, true},
        {"PSP", 32*1024*1024, 4*1024*1024, false, true},
        {"Web Browser", 128*1024*1024, 16*1024*1024, true, false},
        {"Modern Desktop", 8ULL*1024*1024*1024, 1ULL*1024*1024*1024, true, true}
    };
    
    for (const auto& platform : platforms) {
        std::cout << "🖥️ " << platform.name << ":\n";
        std::cout << "   Total Memory: " << platform.total_memory / (1024*1024) << " MB\n";
        std::cout << "   Max Allocation: " << platform.max_allocation / (1024*1024) << " MB\n";
        std::cout << "   Virtual Memory: " << (platform.has_virtual_memory ? "Yes" : "No") << "\n";
        std::cout << "   DMA Support: " << (platform.supports_dma ? "Yes" : "No") << "\n";
        
        // Suggest optimal allocator strategy
        if (platform.total_memory <= 32*1024*1024) {
            std::cout << "   💡 Recommended: Pool + Linear allocators for zero fragmentation\n";
        } else if (platform.total_memory <= 128*1024*1024) {
            std::cout << "   💡 Recommended: Mixed strategy with buddy allocator\n";
        } else {
            std::cout << "   💡 Recommended: System allocator with specialized pools\n";
        }
        std::cout << "\n";
    }
}

/**
 * @brief Performance benchmarking
 */
void benchmark_allocators() {
    std::cout << "\n⚡ === Allocator Performance Benchmark ===\n";
    
    const size_t num_allocations = 10000;
    const size_t allocation_size = 64;
    
    // Benchmark linear allocator
    {
        allocators::LinearAllocator linear_alloc(num_allocations * allocation_size * 2, "Benchmark");
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<void*> ptrs;
        for (size_t i = 0; i < num_allocations; ++i) {
            auto result = linear_alloc.allocate(allocation_size);
            if (result) ptrs.push_back(result.value());
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "📊 Linear Allocator:\n";
        std::cout << "   " << num_allocations << " allocations in " << duration.count() << "μs\n";
        std::cout << "   " << (duration.count() / (double)num_allocations) << "μs per allocation\n";
        std::cout << "   " << (num_allocations * 1000000.0 / duration.count()) << " allocs/sec\n";
    }
    
    // Benchmark pool allocator
    {
        allocators::PoolAllocator pool_alloc(num_allocations, allocation_size, alignof(std::max_align_t), "Benchmark");
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<void*> ptrs;
        for (size_t i = 0; i < num_allocations; ++i) {
            auto result = pool_alloc.allocate(allocation_size);
            if (result) ptrs.push_back(result.value());
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "📊 Pool Allocator:\n";
        std::cout << "   " << num_allocations << " allocations in " << duration.count() << "μs\n";
        std::cout << "   " << (duration.count() / (double)num_allocations) << "μs per allocation\n";
        std::cout << "   " << (num_allocations * 1000000.0 / duration.count()) << " allocs/sec\n";
        
        // Benchmark deallocation
        start = std::chrono::high_resolution_clock::now();
        for (auto ptr : ptrs) {
            pool_alloc.deallocate(ptr);
        }
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "   Deallocation: " << duration.count() << "μs total\n";
    }
}

int main() {
    std::cout << "🚀 Flight HAL Elite Memory Management Demo\n";
    std::cout << "==========================================\n";
    
    try {
        // Demonstrate core allocator types
        demonstrate_linear_allocator();
        demonstrate_pool_allocator();
        
        // Demonstrate memory management features
        demonstrate_memory_types();
        demonstrate_memory_pressure();
        demonstrate_platform_optimizations();
        
        // Performance benchmarking
        benchmark_allocators();
        
        std::cout << "\n✅ === Demo Complete ===\n";
        std::cout << "🎯 Key Features Demonstrated:\n";
        std::cout << "   • Linear/Arena allocators for frame-based memory\n";
        std::cout << "   • Pool allocators for zero-fragmentation object management\n";
        std::cout << "   • Memory type classification and specialized allocation\n";
        std::cout << "   • Memory pressure monitoring and callbacks\n";
        std::cout << "   • Platform-specific optimization strategies\n";
        std::cout << "   • High-performance allocation benchmarks\n";
        std::cout << "\n💪 Ready for production on platforms from Dreamcast to modern systems!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
