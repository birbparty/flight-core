# Memory Interface

**Comprehensive Memory Management for Cross-Platform Game Development**

The Memory interface provides sophisticated memory management capabilities spanning from resource-constrained systems (Dreamcast: 16MB) to modern platforms with gigabytes available. It supports multiple allocation strategies, memory pressure monitoring, and platform-specific optimizations.

## Overview

The Memory interface is built around three core concepts:

1. **Multiple Allocators** - Different allocation strategies for different use cases
2. **Memory Types** - Classification system for specialized memory regions
3. **Pressure Management** - Proactive monitoring and response to memory constraints

```cpp
#include <flight/hal/interfaces/memory.hpp>
using namespace flight::hal;

// Get memory interface from platform
auto memory = platform->get_memory();

// Basic allocation
auto result = memory->allocate(1024, MemoryAlignment::Cache);
if (result) {
    void* ptr = result.value().ptr;
    // Use memory...
    memory->deallocate(ptr);
}
```

## Quick Reference

| Operation | Method | Purpose |
|-----------|--------|---------|
| **Basic Allocation** | `allocate(size, alignment)` | General purpose memory allocation |
| **Typed Allocation** | `allocate_typed(size, type)` | Allocate from specific memory type |
| **Advanced Allocation** | `allocate(AllocationRequest)` | Full control over allocation parameters |
| **Deallocation** | `deallocate(ptr)` | Free previously allocated memory |
| **Statistics** | `get_memory_stats()` | Get comprehensive memory usage statistics |
| **Pressure Monitoring** | `register_pressure_callback()` | Monitor memory pressure events |

## Core Concepts

### Memory Types

The interface supports classification of memory into specialized types:

```cpp
enum class MemoryType : uint8_t {
    System = 0,      // General purpose system memory
    Video,           // GPU-accessible video memory  
    Audio,           // Audio buffer memory (DMA capable)
    Network,         // Network packet buffers (aligned)
    Shared,          // Inter-process shared memory
    Temporary,       // Temporary/scratch memory
    Stack,           // Stack-based allocations
    Pool             // Fixed-size pool allocations
};
```

### Allocator Types

Different allocation strategies for different performance characteristics:

```cpp
enum class AllocatorType : uint8_t {
    Linear = 0,      // Linear/Arena allocator (O(1) alloc, bulk free)
    Stack,          // Stack allocator (LIFO, O(1) operations)
    Pool,           // Pool allocator (fixed-size, zero fragmentation)
    Buddy,          // Buddy allocator (power-of-2, low fragmentation)
    FreeList,       // Free list allocator (general purpose)
    System          // System default allocator
};
```

### Memory Alignment

Support for various alignment requirements:

```cpp
enum class MemoryAlignment : uint32_t {
    None = 1,        // No special alignment
    Word = 4,        // 32-bit word alignment
    Cache = 32,      // Cache line alignment
    Page = 4096,     // Page alignment
    SIMD = 16,       // 128-bit SIMD alignment
    GPU = 256        // GPU memory alignment
};
```

## Basic Usage

### Simple Allocation

```cpp
#include <flight/hal/interfaces/memory.hpp>
using namespace flight::hal;

void basic_allocation_example(IMemoryInterface* memory) {
    // Allocate 1KB with cache alignment
    auto result = memory->allocate(1024, MemoryAlignment::Cache);
    
    if (!result) {
        // Handle allocation failure
        std::cerr << "Allocation failed: " << result.error() << std::endl;
        return;
    }
    
    MemoryAllocation allocation = result.value();
    void* ptr = allocation.ptr;
    size_t actual_size = allocation.size;
    
    std::cout << "Allocated " << actual_size << " bytes at " << ptr << std::endl;
    std::cout << "Alignment: " << static_cast<uint32_t>(allocation.alignment) << std::endl;
    std::cout << "Allocator used: " << memory::to_string(allocation.allocator) << std::endl;
    
    // Use the memory...
    std::memset(ptr, 0, actual_size);
    
    // Free the memory
    auto free_result = memory->deallocate(ptr);
    if (!free_result) {
        std::cerr << "Deallocation failed: " << free_result.error() << std::endl;
    }
}
```

### Memory Type Allocation

```cpp
void typed_allocation_example(IMemoryInterface* memory) {
    // Allocate video memory for textures
    auto video_result = memory->allocate_typed(
        4 * 1024 * 1024,  // 4MB
        MemoryType::Video,
        MemoryAlignment::GPU
    );
    
    if (video_result) {
        std::cout << "Video memory allocated: " << video_result.value().ptr << std::endl;
        // Use for GPU resources...
        memory->deallocate(video_result.value().ptr);
    }
    
    // Allocate audio buffer memory
    auto audio_result = memory->allocate_typed(
        64 * 1024,        // 64KB audio buffer
        MemoryType::Audio,
        MemoryAlignment::Cache,
        MemoryFlags::DMACapable
    );
    
    if (audio_result) {
        std::cout << "Audio buffer allocated: " << audio_result.value().ptr << std::endl;
        // Use for audio streaming...
        memory->deallocate(audio_result.value().ptr);
    }
}
```

### Advanced Allocation with Full Control

```cpp
void advanced_allocation_example(IMemoryInterface* memory) {
    // Create detailed allocation request
    AllocationRequest request = memory::make_allocation_request(
        2 * 1024 * 1024,                    // 2MB
        MemoryType::System,                 // Preferred type
        MemoryAlignment::Page,              // Page aligned
        MemoryFlags::Cacheable | MemoryFlags::Zero  // Cacheable and zero-init
    );
    
    request.preferred_allocator = AllocatorType::Buddy;
    request.timeout_ms = 5000;              // 5 second timeout
    request.allow_fallback = true;          // Allow fallback to different type/allocator
    
    auto result = memory->allocate(request);
    
    if (result) {
        MemoryAllocation allocation = result.value();
        
        std::cout << "Advanced allocation successful:\n";
        std::cout << "  Size: " << allocation.size << " bytes\n";
        std::cout << "  Type: " << memory::to_string(allocation.type) << "\n";
        std::cout << "  Allocator: " << memory::to_string(allocation.allocator) << "\n";
        std::cout << "  Flags: " << static_cast<uint32_t>(allocation.flags) << "\n";
        
        // Memory is guaranteed to be zero-initialized
        // Use the memory...
        
        memory->deallocate(allocation.ptr);
    }
}
```

## Memory Statistics and Monitoring

### Getting Memory Statistics

```cpp
void memory_statistics_example(IMemoryInterface* memory) {
    // Get overall memory statistics
    auto stats_result = memory->get_memory_stats();
    if (stats_result) {
        MemoryStats stats = stats_result.value();
        
        std::cout << "Memory Statistics:\n";
        std::cout << "  Total: " << stats.total_bytes / (1024*1024) << " MB\n";
        std::cout << "  Used: " << stats.used_bytes / (1024*1024) << " MB\n";
        std::cout << "  Free: " << stats.free_bytes / (1024*1024) << " MB\n";
        std::cout << "  Peak: " << stats.peak_used_bytes / (1024*1024) << " MB\n";
        std::cout << "  Allocations: " << stats.allocation_count << "\n";
        std::cout << "  Fragmentation: " << stats.fragmentation_pct << "%\n";
        std::cout << "  Largest free block: " << stats.largest_free_block / 1024 << " KB\n";
        std::cout << "  Efficiency: " << (stats.allocation_efficiency * 100) << "%\n";
    }
    
    // Get statistics for specific memory type
    auto video_stats = memory->get_memory_stats(MemoryType::Video);
    if (video_stats) {
        std::cout << "\nVideo Memory:\n";
        std::cout << "  Used: " << video_stats.value().used_bytes / (1024*1024) << " MB\n";
        std::cout << "  Free: " << video_stats.value().free_bytes / (1024*1024) << " MB\n";
    }
}
```

### Memory Pressure Monitoring

```cpp
class MemoryManager {
private:
    IMemoryInterface* memory_;
    uint32_t pressure_callback_id_;
    
public:
    MemoryManager(IMemoryInterface* memory) : memory_(memory) {
        // Register for memory pressure notifications
        auto callback_result = memory_->register_pressure_callback(
            MemoryPressureLevel::Medium,
            [this](MemoryPressureLevel level, const MemoryPressureInfo& info) {
                handle_memory_pressure(level, info);
            }
        );
        
        if (callback_result) {
            pressure_callback_id_ = callback_result.value();
        }
    }
    
    ~MemoryManager() {
        if (pressure_callback_id_ != 0) {
            memory_->unregister_pressure_callback(pressure_callback_id_);
        }
    }
    
private:
    void handle_memory_pressure(MemoryPressureLevel level, const MemoryPressureInfo& info) {
        std::cout << "Memory pressure: " << memory::to_string(level) << std::endl;
        std::cout << "Available: " << info.available_bytes / (1024*1024) << " MB\n";
        std::cout << "Pressure ratio: " << (info.pressure_ratio * 100) << "%\n";
        
        switch (level) {
            case MemoryPressureLevel::Medium:
                // Free non-essential caches
                free_shader_cache();
                free_texture_cache();
                break;
                
            case MemoryPressureLevel::High:
                // Aggressive cleanup
                free_all_caches();
                reduce_texture_quality();
                break;
                
            case MemoryPressureLevel::Critical:
                // Emergency measures
                free_everything_possible();
                request_gc();
                break;
        }
    }
    
    void free_shader_cache() { /* Implementation */ }
    void free_texture_cache() { /* Implementation */ }
    void free_all_caches() { /* Implementation */ }
    void reduce_texture_quality() { /* Implementation */ }
    void free_everything_possible() { /* Implementation */ }
    void request_gc() { 
        // Hint to system for garbage collection
        memory_->gc_hint();
    }
};
```

## Allocator Management

### Working with Specific Allocators

```cpp
void allocator_management_example(IMemoryInterface* memory) {
    // Get pool allocator for fixed-size objects
    auto pool_result = memory->get_allocator(AllocatorType::Pool);
    if (pool_result) {
        IMemoryAllocator* pool = pool_result.value();
        
        // Check allocator capabilities
        std::cout << "Pool allocator: " << pool->get_name() << std::endl;
        
        // Get allocator stats
        AllocatorStats stats = pool->get_stats();
        std::cout << "  Capacity: " << stats.total_capacity << " bytes\n";
        std::cout << "  Block size overhead: " << stats.allocation_overhead << " bytes\n";
        std::cout << "  Fragmentation: " << (stats.fragmentation_ratio * 100) << "%\n";
        std::cout << "  Supports defrag: " << (stats.supports_defrag ? "Yes" : "No") << "\n";
        
        // Use allocator directly
        auto alloc_result = pool->allocate(256, 16);
        if (alloc_result) {
            // Use allocation...
            pool->deallocate(alloc_result.value());
        }
    }
    
    // Get list of available allocators
    auto available = memory->get_available_allocators();
    std::cout << "\nAvailable allocators:\n";
    for (auto type : available) {
        std::cout << "  " << memory::to_string(type) << std::endl;
    }
}
```

### Memory Defragmentation

```cpp
void defragmentation_example(IMemoryInterface* memory) {
    // Global defragmentation with progress callback
    auto defrag_result = memory->defragment([](size_t moved, size_t total, double progress) {
        std::cout << "Defrag progress: " << (progress * 100) << "% "
                  << "(" << moved << "/" << total << " bytes moved)\n";
    });
    
    if (defrag_result) {
        std::cout << "Global defragmentation completed\n";
    }
    
    // Defragment specific allocator
    auto buddy_defrag = memory->defragment_allocator(AllocatorType::Buddy);
    if (buddy_defrag) {
        std::cout << "Buddy allocator defragmentation completed\n";
    }
}
```

## Platform-Specific Optimizations

### Dreamcast Optimization (16MB RAM)

```cpp
void dreamcast_optimization_example(IMemoryInterface* memory) {
    // Use pool allocators for zero fragmentation
    auto sprite_pool_request = memory::make_allocation_request(
        1024 * sizeof(Sprite),  // 1024 sprites
        MemoryType::System,
        MemoryAlignment::Cache
    );
    sprite_pool_request.preferred_allocator = AllocatorType::Pool;
    
    auto sprite_pool = memory->allocate(sprite_pool_request);
    
    // Use linear allocator for frame-based allocations
    auto frame_buffer_request = memory::make_allocation_request(
        256 * 1024,  // 256KB frame buffer
        MemoryType::System,
        MemoryAlignment::Cache
    );
    frame_buffer_request.preferred_allocator = AllocatorType::Linear;
    
    auto frame_buffer = memory->allocate(frame_buffer_request);
    
    std::cout << "Dreamcast-optimized allocation strategy applied\n";
    
    // Cleanup
    if (sprite_pool) memory->deallocate(sprite_pool.value().ptr);
    if (frame_buffer) memory->deallocate(frame_buffer.value().ptr);
}
```

### Web Platform Constraints

```cpp
void web_optimization_example(IMemoryInterface* memory) {
    // Check if we're on a memory-constrained platform
    auto stats = memory->get_memory_stats();
    if (stats && stats.value().total_bytes < 128 * 1024 * 1024) {  // < 128MB
        std::cout << "Memory constrained platform detected\n";
        
        // Use aggressive memory management
        auto request = memory::make_allocation_request(
            1024 * 1024,  // 1MB
            MemoryType::System,
            MemoryAlignment::None,
            MemoryFlags::Temporary  // Hint that this is temporary
        );
        request.timeout_ms = 1000;  // Quick timeout for web
        
        auto result = memory->allocate(request);
        if (result) {
            // Use immediately and free quickly
            memory->deallocate(result.value().ptr);
        }
    }
}
```

## Error Handling

### Comprehensive Error Handling

```cpp
void robust_memory_usage(IMemoryInterface* memory) {
    // Allocation with full error handling
    auto result = memory->allocate(1024 * 1024);  // 1MB
    
    if (!result) {
        HALError error = result.error();
        
        switch (error.category) {
            case HALErrorCategory::OutOfMemory:
                std::cerr << "Out of memory - trying smaller allocation\n";
                // Try smaller allocation
                result = memory->allocate(512 * 1024);
                break;
                
            case HALErrorCategory::InvalidParameters:
                std::cerr << "Invalid allocation parameters\n";
                break;
                
            case HALErrorCategory::PlatformError:
                std::cerr << "Platform-specific error: " << error.message << std::endl;
                break;
                
            default:
                std::cerr << "Unknown memory error: " << error.message << std::endl;
                break;
        }
    }
    
    if (result) {
        // Validate allocation
        void* ptr = result.value().ptr;
        if (!memory->is_valid_pointer(ptr)) {
            std::cerr << "Invalid pointer returned from allocation!\n";
            return;
        }
        
        // Get allocation info for validation
        auto info_result = memory->get_allocation_info(ptr);
        if (info_result) {
            MemoryAllocation info = info_result.value();
            std::cout << "Allocation validated: " << info.size << " bytes\n";
        }
        
        // Use memory safely...
        
        // Always clean up
        memory->deallocate(ptr);
    }
}
```

## Performance Characteristics

### Memory Interface Performance

| Operation | Typical Latency | Memory Overhead | Notes |
|-----------|----------------|-----------------|-------|
| **allocate()** | < 10μs | 8-32 bytes | Depends on allocator type |
| **deallocate()** | < 5μs | 0 bytes | Linear allocator: O(1) |
| **get_stats()** | < 1μs | 0 bytes | Cached statistics |
| **pressure_check()** | < 1μs | 0 bytes | Lightweight monitoring |

### Platform Performance Characteristics

#### Dreamcast (16MB RAM)
- **Initialization**: < 100ms
- **Pool allocation**: < 1μs per allocation
- **Linear allocation**: < 0.5μs per allocation
- **Memory overhead**: < 1% of total memory

#### PSP (32MB RAM)
- **Initialization**: < 50ms
- **Cache-aligned allocation**: < 2μs
- **DMA-capable allocation**: < 5μs
- **Memory overhead**: < 2% of total memory

#### Modern Desktop (GB+ RAM)
- **Initialization**: < 10ms
- **Large allocation (>1MB)**: < 100μs
- **Small allocation (<1KB)**: < 1μs
- **Memory overhead**: < 0.1% of total memory

## Memory Pressure Guidelines

### Pressure Level Responses

| Level | Available Memory | Recommended Actions |
|-------|-----------------|-------------------|
| **None** | > 50% | Normal operation |
| **Low** | 25-50% | Free non-essential caches |
| **Medium** | 10-25% | Reduce quality settings, aggressive cache cleanup |
| **High** | 5-10% | Emergency cleanup, defer non-critical allocations |
| **Critical** | < 5% | Minimal operation, free everything possible |

### Platform-Specific Thresholds

```cpp
void setup_platform_pressure_thresholds(IMemoryInterface* memory) {
    auto stats = memory->get_memory_stats();
    if (!stats) return;
    
    size_t total = stats.value().total_bytes;
    
    if (total <= 32 * 1024 * 1024) {  // <= 32MB (Dreamcast, PSP)
        // Aggressive thresholds for constrained systems
        // Medium pressure at 8MB free, High at 2MB free
        std::cout << "Constrained memory thresholds applied\n";
    } else if (total <= 128 * 1024 * 1024) {  // <= 128MB (Web, mobile)
        // Moderate thresholds
        std::cout << "Moderate memory thresholds applied\n";
    } else {  // > 128MB (Desktop)
        // Relaxed thresholds
        std::cout << "Relaxed memory thresholds applied\n";
    }
}
```

## Best Practices

### 1. Choose the Right Allocator

```cpp
// For objects with known lifetime - use Pool allocator
auto sprites = allocate_from_pool<Sprite>(1000);

// For frame-based allocations - use Linear allocator  
auto frame_memory = allocate_linear(256 * 1024);

// For general purpose - use System allocator
auto general_buffer = memory->allocate(size);
```

### 2. Monitor Memory Pressure

```cpp
// Always register for memory pressure on constrained platforms
if (platform->get_total_memory() < 64 * 1024 * 1024) {
    memory->register_pressure_callback(MemoryPressureLevel::Medium, callback);
}
```

### 3. Use Appropriate Memory Types

```cpp
// GPU resources - use Video memory type
auto texture_buffer = memory->allocate_typed(size, MemoryType::Video);

// Audio buffers - use Audio memory type for DMA capability
auto audio_buffer = memory->allocate_typed(size, MemoryType::Audio, 
                                          MemoryAlignment::Cache, 
                                          MemoryFlags::DMACapable);
```

### 4. Handle Allocation Failures Gracefully

```cpp
auto result = memory->allocate(requested_size);
if (!result) {
    // Try smaller size
    result = memory->allocate(requested_size / 2);
    if (!result) {
        // Use streaming/paging approach
        return use_streaming_approach();
    }
}
```

---

## API Reference

For complete method signatures and detailed parameter information, see [Memory API Reference](api-reference.md).

## Related Documentation

- [Memory Interface Design](../../memory_interface_design.md) - Design rationale and architecture
- [Linear Allocator](allocators/linear-allocator.md) - Frame-based allocation strategies
- [Pool Allocator](allocators/pool-allocator.md) - Zero-fragmentation object pools
- [Platform Memory Guides](../../platform-guides/) - Platform-specific memory optimization
- [Performance Benchmarking](../../performance_benchmarking_system.md) - Memory performance testing

---

*The Memory interface enables efficient cross-platform memory management from Dreamcast's 16MB to modern gigabyte systems.*
