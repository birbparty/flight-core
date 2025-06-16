# Software-Only HAL Reference Implementation

This directory contains a complete reference implementation of the Flight HAL using only standard C++ libraries. It serves as both a working fallback implementation and an educational example demonstrating best practices for HAL driver development.

## 🎯 Purpose

The software-only implementation provides:

1. **Educational Reference**: Complete, well-documented examples of HAL interface implementation
2. **Fallback Driver**: Portable memory management that works on any C++17-compatible platform
3. **Testing Foundation**: Reliable implementation for testing HAL framework features
4. **Development Template**: Starting point for creating platform-specific drivers

## 📁 Directory Structure

```
software_only/
├── README.md                    # This file
├── CMakeLists.txt              # Build configuration
├── drivers/                    # Driver implementations
│   ├── software_memory_driver.hpp    # Complete memory driver header
│   └── software_memory_driver.cpp    # Driver implementation (to be created)
├── examples/                   # Usage examples
│   ├── basic_usage_example.cpp       # Comprehensive demo program
│   └── CMakeLists.txt                # Example build config
└── tests/                      # Unit tests
    └── CMakeLists.txt          # Test configuration
```

## 🛠️ Features Implemented

### Memory Management (`SoftwareMemoryDriver`)

#### Core Functionality
- ✅ **Standard Allocation**: malloc/free-based allocator with tracking
- ✅ **Pool Allocation**: Fixed-size block allocation for performance
- ✅ **Linear Allocation**: Arena-style allocation for temporary objects
- ✅ **Aligned Memory**: Support for cache-line and page alignment
- ✅ **Memory Poisoning**: Debug feature to detect use-after-free
- ✅ **Leak Detection**: Track allocations and detect memory leaks

#### Advanced Features
- ✅ **Memory Pressure Monitoring**: Configurable thresholds and callbacks
- ✅ **Statistics Collection**: Detailed allocation metrics and reporting
- ✅ **Defragmentation**: Memory compaction with callback notifications
- ✅ **Garbage Collection Hints**: Manual memory management optimization
- ✅ **Thread Safety**: Optional thread-safe allocator variants
- ✅ **Configuration**: Runtime and compile-time configuration options

#### Platform Integration
- ✅ **HAL Compliance**: Full implementation of `IMemoryInterface`
- ✅ **Auto-Registration**: Automatic driver discovery and registration
- ✅ **Capability Reporting**: Accurate capability and performance tier reporting
- ✅ **Error Handling**: Comprehensive error reporting with context

## 🚀 Quick Start

### Building the Implementation

```bash
# Configure build
mkdir build && cd build
cmake .. -DSOFTWARE_HAL_BUILD_EXAMPLES=ON

# Build everything
make -j$(nproc)

# Run the basic example
./examples/basic_usage_example
```

### Using in Your Project

```cpp
#include "software_memory_driver.hpp"

// Create driver with custom configuration
flight::hal::software::SoftwareMemoryConfig config;
config.max_allocation_size = 64 * 1024 * 1024;  // 64MB max
config.total_memory_limit = 512 * 1024 * 1024;  // 512MB total
config.enable_debug_tracking = true;

auto driver = std::make_unique<flight::hal::software::SoftwareMemoryDriver>(config);
driver->initialize();

// Use the driver
auto result = driver->allocate(1024);
if (result) {
    void* ptr = result.value().ptr;
    // Use memory...
    driver->deallocate(ptr);
}

driver->shutdown();
```

### Auto-Registration

The driver automatically registers itself with the HAL system:

```cpp
// Driver is automatically available through the registry
auto& registry = flight::hal::DriverRegistry::instance();
auto memory_driver = registry.get_driver<flight::hal::IMemoryInterface>();
```

## ⚙️ Configuration Options

### Build-Time Configuration

```cmake
# CMake options
option(SOFTWARE_HAL_BUILD_EXAMPLES "Build software HAL examples" ON)
option(SOFTWARE_HAL_BUILD_TESTS "Build software HAL tests" ON)
option(SOFTWARE_HAL_ENABLE_DEBUG "Enable debug features" ON)
option(SOFTWARE_HAL_ENABLE_VALIDATION "Enable runtime validation" ON)
```

### Runtime Configuration

```cpp
struct SoftwareMemoryConfig {
    size_t max_allocation_size = 64 * 1024 * 1024;      // 64MB max single allocation
    size_t total_memory_limit = 512 * 1024 * 1024;      // 512MB total limit
    size_t pool_block_size = 4096;                      // 4KB pool blocks
    size_t linear_arena_size = 16 * 1024 * 1024;        // 16MB linear arena
    double memory_pressure_threshold = 0.85;            // 85% usage triggers pressure
    bool enable_debug_tracking = true;                  // Enable allocation tracking
    bool enable_memory_poisoning = false;               // Poison freed memory (debug)
    bool enable_leak_detection = true;                  // Track potential leaks
    uint32_t gc_threshold_ms = 5000;                    // GC hint threshold (5 seconds)
};
```

## 📊 Performance Characteristics

### Allocator Performance

| Allocator Type | Allocation Speed | Deallocation Speed | Memory Overhead | Use Case |
|---------------|------------------|-------------------|-----------------|----------|
| Standard      | ~500 ns         | ~300 ns           | 16-32 bytes     | General purpose |
| Pool          | ~50 ns          | ~20 ns            | ~4 bytes        | Fixed-size objects |
| Linear        | ~10 ns          | N/A (bulk reset)  | ~0 bytes        | Temporary allocations |

### Memory Limits

- **Maximum single allocation**: 64MB (configurable)
- **Total memory limit**: 512MB (configurable)
- **Alignment support**: 1, 2, 4, 8, 16, 32, 64, 4096 bytes
- **Minimum allocation size**: 1 byte
- **Allocation granularity**: 8 bytes (platform dependent)

## 🧪 Testing

### Running Tests

```bash
# Build with testing enabled
cmake .. -DSOFTWARE_HAL_BUILD_TESTS=ON
make -j$(nproc)

# Run all tests
ctest --verbose

# Run specific test suites
./tests/memory_driver_test
./tests/pressure_monitoring_test
./tests/allocator_performance_test
```

### Example Programs

#### Basic Usage Example

Demonstrates fundamental driver usage patterns:

```bash
./examples/basic_usage_example
```

Features demonstrated:
- Driver initialization and configuration
- Basic memory allocation and deallocation
- Advanced allocation with alignment and flags
- Memory statistics and monitoring
- Memory pressure detection
- Performance benchmarking

## 🔧 Extending the Implementation

### Adding New Allocators

1. Create allocator class inheriting from `SoftwareAllocatorBase`:

```cpp
class CustomAllocator : public SoftwareAllocatorBase {
public:
    CustomAllocator() : SoftwareAllocatorBase(AllocatorType::Custom, "Custom") {}
    
    HALResult<void*> allocate(size_t size, size_t alignment) override {
        // Custom allocation logic
    }
    
    // Implement other required methods...
};
```

2. Integrate with `SoftwareMemoryDriver`:

```cpp
// Add to driver's initialization
custom_allocator_ = std::make_unique<CustomAllocator>();
```

### Adding Platform-Specific Features

```cpp
class PlatformMemoryDriver : public SoftwareMemoryDriver {
public:
    HALResult<void> initialize() override {
        // Platform-specific initialization
        auto result = SoftwareMemoryDriver::initialize();
        if (!result) return result;
        
        // Add platform features
        return setup_platform_features();
    }
    
private:
    HALResult<void> setup_platform_features();
};
```

## 📚 Architecture Notes

### Design Principles

1. **Zero Dependencies**: Uses only standard C++17 libraries
2. **Memory Safety**: Comprehensive bounds checking and validation
3. **Performance**: Multiple allocation strategies for different use cases
4. **Debuggability**: Extensive logging and diagnostic features
5. **Portability**: Works on any C++17-compatible platform

### Thread Safety

The implementation provides multiple thread safety levels:

- `SoftwareStandardAllocator`: Thread-safe (using standard library)
- `ThreadSafeLinearAllocator`: Fully synchronized linear allocator
- `LinearAllocator`: Fast, single-threaded linear allocator
- `PoolAllocator`: Configurable thread safety

### Memory Layout

```
SoftwareMemoryDriver Memory Organization:
┌─────────────────────────────────────────────────────────┐
│ Standard Allocator (malloc/free based)                 │
│ ┌─────────────────────────────────────────────────────┐ │
│ │ Allocation Tracking (std::unordered_map)           │ │
│ │ Statistics Collection                               │ │
│ └─────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────┐
│ Pool Allocator (Fixed-size blocks)                     │
│ ┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐  │
│ │ ✓ │ ✓ │ ✗ │ ✓ │ ✗ │ ✗ │ ✓ │ ✗ │ ✓ │ ✗ │ ✓ │ ✗ │...│  │
│ └───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┘  │
└─────────────────────────────────────────────────────────┘
┌─────────────────────────────────────────────────────────┐
│ Linear Allocator (Arena-style)                         │
│ ┌─────────────────┬─────────────────────────────────────┐ │
│ │ Used Memory     │ Free Memory                       │ │
│ │                 │↑ Current Position                 │ │
│ └─────────────────┴─────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────┘
```

## ⚠️ Limitations

### Current Limitations

1. **Memory Compaction**: Limited defragmentation for pool allocators
2. **NUMA Awareness**: No NUMA topology consideration
3. **Hardware Features**: No use of platform-specific memory features
4. **Real-time Guarantees**: No deterministic allocation timing
5. **Cross-Process Sharing**: No shared memory support

### Future Enhancements

- [ ] NUMA-aware allocation strategies
- [ ] Real-time allocator with deterministic timing
- [ ] Shared memory support for IPC
- [ ] Memory compression for low-memory situations
- [ ] Advanced defragmentation algorithms
- [ ] Integration with platform memory managers

## 🤝 Contributing

### Development Guidelines

1. **Code Style**: Follow existing code formatting and naming conventions
2. **Documentation**: Document all public APIs and complex algorithms
3. **Testing**: Add comprehensive tests for new features
4. **Performance**: Benchmark performance-critical changes
5. **Compatibility**: Maintain C++17 compatibility

### Adding New Drivers

When implementing additional HAL interfaces:

1. Follow the `SoftwareMemoryDriver` pattern
2. Implement complete interface compliance
3. Add comprehensive error handling
4. Include performance benchmarks
5. Document configuration options
6. Add usage examples

## 📄 License

This implementation is part of the Flight HAL project and follows the same licensing terms. See the main project LICENSE file for details.

## 🔗 Related Documentation

- [Flight HAL Architecture Guide](../../docs/architecture/)
- [Memory Interface Design](../../docs/memory_interface_design.md)
- [Driver Development Guide](../../docs/integration_guidelines.md)
- [Performance Benchmarking](../../docs/performance_benchmarking_system.md)
- [Testing Framework](../../docs/interface_compliance_testing_framework.md)
