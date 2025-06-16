# Flight HAL Resource Management System

## Overview

The Flight HAL Resource Management System provides comprehensive resource management with RAII patterns, budget enforcement, pool management, and cross-driver resource sharing. It's designed to work efficiently across all platforms, from the Dreamcast's constrained 16MB environment to modern systems with abundant resources.

## Architecture

### Core Components

1. **ResourceManager** - Central coordinator for all resource operations
2. **ResourceRef<T>** - RAII-based smart pointer for automatic resource cleanup
3. **ScopedResource** - Stack-based resource management for non-typed resources
4. **ResourceBudgetManager** - Memory budget enforcement and pressure monitoring
5. **PoolManager** - Efficient resource pooling for frequently used objects
6. **ResourceHandle** - Unique resource identification and metadata system

### Key Features

- ✅ **RAII Patterns** - Automatic resource cleanup with zero-overhead abstractions
- ✅ **Budget Enforcement** - Per-type memory budgets with pressure monitoring
- ✅ **Resource Pooling** - Zero-fragmentation allocation for common resource sizes
- ✅ **Cross-Driver Sharing** - Safe resource sharing between different drivers
- ✅ **Emergency Allocation** - Critical resource allocation during pressure situations
- ✅ **Platform Scaling** - Adaptive behavior from embedded to desktop systems

## Resource Types

The system manages several categories of resources:

```cpp
enum class ResourceType : uint32_t {
    Hardware = 0x01,        // Physical hardware resources (timers, DMA channels)
    Memory = 0x02,          // Memory regions, pools, caches
    Performance = 0x04,     // CPU time, bandwidth limits
    Communication = 0x08,   // Message queues, event channels
    Platform = 0x10,        // Platform-specific resources
    Custom = 0x80000000     // Custom resource types
};
```

## RAII Resource Management

### ResourceRef<T> - Type-Safe Resource References

The primary way to manage resources is through `ResourceRef<T>`, which provides automatic cleanup:

```cpp
#include "flight/hal/core/resource_manager.hpp"

auto& resource_manager = ResourceManager::instance();

// Create resource metadata
ResourceMetadata metadata;
metadata.type = ResourceType::Memory;
metadata.size_bytes = 1024 * 1024; // 1MB
metadata.alignment_bytes = 16;
metadata.access_pattern = AccessPattern::ReadWrite;
metadata.priority = ResourcePriority::Normal;
metadata.flags = ResourceFlags::Cacheable | ResourceFlags::GPUAccessible;

// Acquire resource with RAII
auto texture_result = resource_manager.acquire_resource<TextureResource>(
    "MainMenuTexture", metadata);

if (texture_result) {
    auto texture_ref = std::move(texture_result.value());
    
    // Use the resource
    texture_ref->width = 1024;
    texture_ref->height = 768;
    
    // Resource is automatically released when texture_ref goes out of scope
}
```

### ScopedResource - Untyped Resource Management

For resources that don't need typed access:

```cpp
// Acquire scoped resource
auto scoped_result = resource_manager.acquire_scoped_resource(
    "AudioBuffer", metadata);

if (scoped_result) {
    auto scoped_resource = std::move(scoped_result.value());
    
    // Access resource through handle
    auto handle = scoped_resource.handle();
    
    // Resource automatically released on scope exit
}
```

## Budget Management

### Setting Resource Budgets

Configure budgets for different resource types:

```cpp
// Set memory budget: 10MB max, 1MB reserved, 75% warning, 90% critical
ResourceBudget memory_budget(10 * 1024 * 1024, 1024 * 1024, 75, 90);
resource_manager.set_budget(ResourceType::Memory, memory_budget);

// Set hardware budget: 5MB max, 512KB reserved, 80% warning, 95% critical  
ResourceBudget hardware_budget(5 * 1024 * 1024, 512 * 1024, 80, 95);
resource_manager.set_budget(ResourceType::Hardware, hardware_budget);
```

### Pressure Monitoring

Register callbacks to respond to resource pressure:

```cpp
resource_manager.register_pressure_callback([](ResourceType type, 
                                               ResourcePressure pressure, 
                                               const ResourceStats& stats) {
    switch (pressure) {
        case ResourcePressure::High:
            // Free non-essential caches
            break;
        case ResourcePressure::Critical:
            // Emergency cleanup
            break;
    }
});
```

### Emergency Reclamation

Register reclamation callbacks for automatic resource recovery:

```cpp
resource_manager.register_reclamation_callback(ResourceType::Memory, 
    [](ResourceType type, size_t requested_bytes) -> size_t {
        // Free caches, temporary resources, etc.
        size_t freed = free_texture_cache();
        freed += free_audio_buffers();
        return freed;
    });
```

## Resource Pooling

### Creating Resource Pools

Set up pools for frequently allocated resources:

```cpp
auto& pool_manager = resource_manager.pool_manager();

// Create pool for 4KB audio buffers
PoolManager::PoolConfig audio_pool_config{
    .type = ResourceType::Memory,
    .block_size = 4096,           // 4KB blocks
    .initial_count = 10,          // Start with 10 blocks
    .max_count = 50,              // Maximum 50 blocks
    .alignment = 8,               // 8-byte alignment
    .thread_safe = true,          // Thread-safe access
    .name = "AudioBufferPool"
};

auto pool_result = pool_manager.create_pool(audio_pool_config);
```

### Pool Statistics

Monitor pool usage:

```cpp
auto stats_result = pool_manager.get_pool_stats(ResourceType::Memory, 4096);
if (stats_result) {
    const auto& stats = stats_result.value();
    std::cout << "Pool usage: " << stats.used_bytes / 1024 << "KB used, "
              << stats.free_bytes / 1024 << "KB free\n";
}
```

## Cross-Driver Resource Sharing

### Sharing Resources

Share resources safely between drivers:

```cpp
// Create a shareable resource
ResourceMetadata metadata;
metadata.type = ResourceType::Hardware;
metadata.flags = ResourceFlags::Shareable | ResourceFlags::GPUAccessible;

auto vertex_buffer = resource_manager.acquire_resource<VertexBuffer>(
    "SharedVertexBuffer", metadata);

if (vertex_buffer) {
    // Share with audio driver for visualization
    auto shared_result = resource_manager.share_resource(
        vertex_buffer->handle(), "AudioDriver");
    
    if (shared_result) {
        auto shared_handle = shared_result.value();
        // Audio driver can now access this resource
    }
}
```

### Resource Dependencies

The system automatically tracks resource dependencies to prevent deadlocks when sharing resources between drivers.

## Acquisition Modes

### Blocking vs Non-Blocking

Control how resources are acquired:

```cpp
// Non-blocking: Return immediately if resource unavailable
auto result1 = resource_manager.acquire_resource<Texture>(
    "Texture1", metadata, AcquisitionMode::NonBlocking);

// Blocking: Wait until resource becomes available
auto result2 = resource_manager.acquire_resource<Texture>(
    "Texture2", metadata, AcquisitionMode::Blocking);

// Timeout: Block with timeout
auto result3 = resource_manager.acquire_resource<Texture>(
    "Texture3", metadata, AcquisitionMode::Timeout);

// Emergency: Always allocate (may exceed budgets)
auto result4 = resource_manager.acquire_resource<Texture>(
    "CriticalTexture", metadata, AcquisitionMode::Emergency);
```

## Platform Adaptation

### Automatic Budget Configuration

The system automatically configures budgets based on detected platform:

```cpp
// On Dreamcast (16MB total):
// - Memory: 60% (9.6MB) with 10% (1.6MB) reserved
// - Hardware: 20% (3.2MB) with 5% (0.8MB) reserved

// On modern systems (8GB+):
// - Memory: 60% with appropriate scaling
// - Hardware: 20% with appropriate scaling
```

### Pool Sizing

Pool configurations adapt to platform constraints:

```cpp
// Dreamcast: Smaller pools, fewer blocks
// Desktop: Larger pools, more blocks
// Web: Conservative pools for memory constraints
```

## Best Practices

### 1. Use RAII Consistently

Always use `ResourceRef<T>` or `ScopedResource` instead of manual resource management:

```cpp
// ✅ Good - RAII with automatic cleanup
{
    auto texture = resource_manager.acquire_resource<Texture>("tex", metadata);
    // Use texture
    // Automatically cleaned up
}

// ❌ Bad - Manual management (error-prone)
auto handle = resource_manager.register_resource("tex", metadata);
// ... use resource ...
resource_manager.release_resource(handle); // Easy to forget!
```

### 2. Set Appropriate Budgets

Configure budgets based on your application's needs:

```cpp
// ✅ Good - Realistic budgets with reserves
ResourceBudget texture_budget(
    available_memory * 0.6,  // Use 60% for textures
    available_memory * 0.1,  // Reserve 10%
    75, 90                   // Warning at 75%, critical at 90%
);

// ❌ Bad - Too tight or too loose budgets
ResourceBudget bad_budget(
    available_memory * 0.95, // Too tight - no room for spikes
    0,                       // No reserve
    95, 99                   // Warnings too late
);
```

### 3. Use Pools for Frequent Allocations

Create pools for resources allocated frequently:

```cpp
// ✅ Good - Pool for common sizes
create_pool({ResourceType::Memory, 64, 100, 1000});   // Small objects
create_pool({ResourceType::Memory, 4096, 50, 500});   // Medium objects
create_pool({ResourceType::Memory, 64*1024, 10, 100}); // Large objects

// ❌ Bad - No pooling leads to fragmentation
// Every allocation goes through system malloc/free
```

### 4. Handle Resource Pressure

Implement proper pressure handling:

```cpp
// ✅ Good - Responsive pressure handling
resource_manager.register_pressure_callback([](auto type, auto pressure, const auto& stats) {
    switch (pressure) {
        case ResourcePressure::Low:
            // Start cleaning up non-essential caches
            cleanup_old_textures();
            break;
        case ResourcePressure::Medium:
            // More aggressive cleanup
            free_unused_audio_buffers();
            break;
        case ResourcePressure::High:
            // Emergency cleanup
            force_garbage_collection();
            break;
        case ResourcePressure::Critical:
            // Last resort measures
            reduce_quality_settings();
            break;
    }
});

// ❌ Bad - Ignoring pressure leads to allocation failures
// No pressure callback registered
```

### 5. Share Resources Safely

Use the sharing system for cross-driver resources:

```cpp
// ✅ Good - Proper resource sharing
metadata.flags = ResourceFlags::Shareable;
auto resource = acquire_resource<Buffer>("SharedBuffer", metadata);
auto shared_handle = share_resource(resource->handle(), "OtherDriver");

// ❌ Bad - Direct pointer sharing (unsafe)
auto resource = acquire_resource<Buffer>("Buffer", metadata);
other_driver->use_buffer(resource.get()); // Unsafe! No ownership tracking
```

## Error Handling

### Common Error Scenarios

1. **Budget Exceeded**: Resource allocation exceeds configured budget
2. **Pool Full**: All pool blocks are in use
3. **Resource Not Found**: Attempting to access non-existent resource
4. **Sharing Failure**: Resource cannot be shared due to constraints

### Error Recovery

```cpp
auto result = resource_manager.acquire_resource<Texture>("tex", metadata);
if (!result) {
    switch (result.error().category()) {
        case HALErrorCategory::Resource:
            if (result.error().code() == 2) { // Budget exceeded
                // Try with lower priority or emergency mode
                auto emergency_result = resource_manager.acquire_resource<Texture>(
                    "tex", metadata, AcquisitionMode::Emergency);
            }
            break;
    }
}
```

## Performance Considerations

### Memory Overhead

- `ResourceRef<T>`: 24 bytes overhead (pointer + handle + manager pointer)
- `ScopedResource`: 16 bytes overhead (handle + manager pointer)
- Pool allocation: ~2-4 bytes overhead per block
- Resource registry: ~64 bytes per registered resource

### Allocation Performance

- **Pool allocation**: O(1) for pool hits
- **System allocation**: O(log n) for pool misses
- **Budget checking**: O(1) hash table lookup
- **Pressure calculation**: O(1) arithmetic

### Thread Safety

- All public APIs are thread-safe
- Lock-free operations where possible
- Shared reader/writer locks for budget management
- Per-pool synchronization for maximum concurrency

## Integration Examples

### Game Engine Integration

```cpp
class GameEngine {
    ResourceManager& resource_manager_;
    
public:
    void initialize() {
        resource_manager_.initialize();
        
        // Configure for game engine
        setup_texture_pools();
        setup_audio_pools();
        setup_budgets();
    }
    
    std::unique_ptr<Texture> load_texture(const std::string& path) {
        ResourceMetadata metadata = create_texture_metadata(path);
        
        auto result = resource_manager_.acquire_resource<Texture>(path, metadata);
        if (result) {
            return std::make_unique<Texture>(std::move(result.value()));
        }
        return nullptr;
    }
    
private:
    void setup_texture_pools() {
        // Common texture sizes
        create_texture_pool(256, 256);   // UI textures
        create_texture_pool(512, 512);   // Character textures  
        create_texture_pool(1024, 1024); // Environment textures
    }
};
```

### Driver Integration

```cpp
class GraphicsDriver {
    ResourceManager& resource_manager_;
    
public:
    HALResult<BufferHandle> create_vertex_buffer(size_t size) {
        ResourceMetadata metadata;
        metadata.type = ResourceType::Hardware;
        metadata.size_bytes = size;
        metadata.flags = ResourceFlags::GPUAccessible | ResourceFlags::DMACapable;
        
        auto result = resource_manager_.acquire_resource<VertexBuffer>(
            "VertexBuffer_" + std::to_string(next_id_++), metadata);
        
        if (result) {
            return HALResult<BufferHandle>::success(
                BufferHandle{result.value().handle().id()});
        }
        
        return HALResult<BufferHandle>::error(result.error());
    }
    
    HALResult<void> share_buffer_with_audio(BufferHandle handle) {
        auto resource_handle = get_resource_handle(handle);
        return resource_manager_.share_resource(resource_handle, "AudioDriver");
    }
};
```

## Debugging and Monitoring

### Resource Statistics

```cpp
// Get overall statistics
auto stats = resource_manager_.get_resource_stats(ResourceType::Memory);
if (stats) {
    std::cout << "Memory usage: " << stats->current_usage / (1024 * 1024) << "MB\n";
    std::cout << "Peak usage: " << stats->peak_usage / (1024 * 1024) << "MB\n";
    std::cout << "Allocations: " << stats->allocation_count << "\n";
    std::cout << "Pressure: " << static_cast<int>(stats->pressure) << "\n";
}
```

### Pool Monitoring

```cpp
// Monitor pool efficiency
auto pool_stats = pool_manager.get_pool_stats(ResourceType::Memory, 4096);
if (pool_stats) {
    double efficiency = static_cast<double>(pool_stats->used_bytes) / 
                       pool_stats->total_capacity;
    std::cout << "Pool efficiency: " << (efficiency * 100) << "%\n";
}
```

### Leak Detection

The system automatically tracks all resource allocations and can detect leaks:

```cpp
// In debug builds, the system will warn about unreleased resources
// when the ResourceManager is destroyed
```

## Migration Guide

### From Manual Resource Management

```cpp
// Old code - manual management
class OldTexture {
    void* data_;
    size_t size_;
    
public:
    OldTexture(size_t size) : size_(size) {
        data_ = malloc(size);  // Manual allocation
    }
    
    ~OldTexture() {
        free(data_);  // Manual deallocation
    }
};

// New code - RAII resource management
class NewTexture {
    ResourceRef<TextureData> resource_;
    
public:
    NewTexture(size_t size) {
        ResourceMetadata metadata;
        metadata.type = ResourceType::Memory;
        metadata.size_bytes = size;
        
        auto result = ResourceManager::instance().acquire_resource<TextureData>(
            "Texture_" + generate_unique_name(), metadata);
        
        if (result) {
            resource_ = std::move(result.value());
        }
    }
    
    // Automatic cleanup - no destructor needed!
};
```

### Performance Migration

The new system provides better performance characteristics:

- **Fragmentation**: Eliminated through pooling
- **Allocation speed**: Faster through pool reuse  
- **Memory overhead**: Reduced through efficient metadata storage
- **Cross-driver coordination**: Enabled without performance penalties

## Conclusion

The Flight HAL Resource Management System provides a comprehensive, efficient, and safe way to manage resources across all supported platforms. Its RAII-based design ensures automatic cleanup, while budget management and pooling provide optimal performance characteristics from embedded systems to high-end gaming rigs.

Key benefits:
- 🚀 **Zero-overhead** RAII patterns
- 🛡️ **Memory safety** through automatic cleanup
- ⚡ **High performance** through intelligent pooling
- 🔄 **Cross-platform** adaptation
- 🤝 **Driver cooperation** through resource sharing
- 📊 **Monitoring** and debugging capabilities

The system scales from the Dreamcast's 16MB constraints to modern systems with gigabytes of RAM, providing consistent and efficient resource management across the entire Flight HAL ecosystem.
