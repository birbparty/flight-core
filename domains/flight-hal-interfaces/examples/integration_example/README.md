# Flight HAL Integration Example

This directory contains a practical example demonstrating how to implement a Flight HAL driver following the comprehensive Integration Guidelines.

## Files

- `simple_platform_memory_driver.hpp` - Header file showing the complete driver interface implementation
- `simple_platform_memory_driver.cpp` - Implementation demonstrating all integration patterns
- `README.md` - This documentation file

## Purpose

This example serves as a practical reference for platform teams implementing HAL drivers. It demonstrates:

### Universal Implementation Patterns (Integration Guidelines Section 1)
- **Base Driver Architecture**: Proper inheritance from HAL interfaces
- **Auto-Registration System**: Using `REGISTER_HAL_DRIVER` macros  
- **HALResult Error Handling**: Comprehensive error reporting patterns
- **Capability Detection**: Platform-specific capability discovery
- **Resource Management**: Thread-safe allocation tracking and cleanup

### Platform-Specific Integration (Integration Guidelines Section 2)
- **macOS**: System memory detection using sysctl, aligned allocation with posix_memalign
- **Linux**: Memory detection via sysinfo, POSIX memory management
- **Windows**: GlobalMemoryStatusEx for system info, _aligned_malloc for allocation
- **Web/Emscripten**: Browser memory limits, garbage collection hints

### Testing Integration (Integration Guidelines Section 3)
- Performance monitoring with platform-appropriate SLAs
- Memory pressure detection and management
- Resource leak prevention and detection
- Thread-safe state management

## Usage

This example is intended for study and as a template. Platform teams should:

1. **Copy the structure** for their platform-specific driver
2. **Replace platform detection** with their target platform's APIs  
3. **Implement platform allocation** using native memory management APIs
4. **Add capability detection** for platform-specific features
5. **Test thoroughly** using the HAL compliance testing framework

## Integration with Build System

To enable this example in your build:

```cmake
option(FLIGHT_ENABLE_INTEGRATION_EXAMPLES "Enable integration examples" OFF)

if(FLIGHT_ENABLE_INTEGRATION_EXAMPLES)
    add_subdirectory(examples/integration_example)
endif()
```

## Key Learning Points

### Error Handling Pattern
```cpp
// Always use HALResult for operations that can fail
HALResult<MemoryAllocation> allocate(size_t size) override {
    if (size == 0) {
        return flight::hal::errors::invalid_parameter(1, "Size cannot be zero");
    }
    
    void* ptr = platform_allocate(size);
    if (!ptr) {
        return flight::hal::errors::out_of_memory(1, "Platform allocation failed");
    }
    
    MemoryAllocation result;
    result.ptr = ptr;
    result.size = size;
    return result;
}
```

### Capability Detection Pattern
```cpp
void init_platform_capabilities() {
    capability_mask_ = 0;
    
    // Always include basic capabilities
    capability_mask_ |= static_cast<uint32_t>(HALCapability::BasicMemory);
    
    // Platform-specific detection
    if (platform_supports_threading()) {
        capability_mask_ |= static_cast<uint32_t>(HALCapability::Threading);
    }
    
    // Set performance tier based on available resources
    if (total_memory_ < 32 * 1024 * 1024) {
        performance_tier_ = PerformanceTier::Minimal;
    } else {
        performance_tier_ = PerformanceTier::High;
    }
}
```

### Resource Management Pattern
```cpp
~SimplePlatformMemoryDriver() {
    // Always verify cleanup in destructor
    if (initialized_) {
        shutdown();
    }
    
    // Check for resource leaks
    std::lock_guard<std::mutex> lock(allocations_mutex_);
    if (!allocations_.empty()) {
        // Log warning and clean up
        for (const auto& [ptr, entry] : allocations_) {
            platform_deallocate(ptr);
        }
    }
}
```

## Next Steps

1. Study the complete Integration Guidelines: `docs/integration_guidelines.md`
2. Choose your platform-specific guide (macOS, Dreamcast, PSP, or Web)
3. Implement your driver following these patterns
4. Test using the HAL compliance testing framework
5. Optimize using the performance guidelines

## Support

Refer to the comprehensive Integration Guidelines for detailed platform-specific instructions, troubleshooting guides, and optimization strategies.
