# Flight-Core C++17 Bindings

Modern C++17 bindings for Flight Shared Types, providing zero-cost abstractions for multi-platform development spanning from retro gaming platforms (Dreamcast, PSP, Vita) to modern cloud infrastructure (V6R).

## Features

### 🚀 Modern C++17 Design
- **Zero-cost abstractions** - No runtime overhead for Flight-Core integration
- **Exception-free error handling** using `std::variant` result types
- **RAII resource management** with automatic cleanup
- **Platform-specific optimizations** for constrained environments
- **Template metaprogramming** for compile-time platform adaptation

### 🎯 Multi-Platform Support
- **Retro Gaming**: Dreamcast (16MB), PSP (32MB), Vita (512MB)
- **Modern Desktop**: Windows, macOS, Linux
- **Cloud Infrastructure**: V6R containers and VMs
- **Embedded Systems**: Memory-constrained environments

### 🛠 Flight-Core Integration
- **Memory subsystem** integration with platform-aware allocators
- **Component Model** support for Flight-Core components
- **Session management** across development and production environments
- **Error handling** with context preservation and recovery suggestions
- **Platform detection** with capability-based feature selection

## Quick Start

### Prerequisites
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.17+
- Flight-Core development environment

### Building

```bash
# Configure for your target platform
mkdir build && cd build
cmake .. -DFLIGHT_TARGET_PLATFORM=v6r -DCMAKE_BUILD_TYPE=Release

# Build the library and examples
make -j$(nproc)

# Run the integration example
./examples/flight_core_integration_example
```

### Platform-Specific Builds

```bash
# Dreamcast (memory-constrained)
cmake .. -DFLIGHT_TARGET_PLATFORM=dreamcast -DCMAKE_BUILD_TYPE=MinSizeRel

# V6R Cloud
cmake .. -DFLIGHT_TARGET_PLATFORM=v6r -DCMAKE_BUILD_TYPE=Release

# Desktop development
cmake .. -DFLIGHT_TARGET_PLATFORM=desktop -DCMAKE_BUILD_TYPE=Debug
```

## Usage Examples

### Memory Management

```cpp
#include "flight_shared_types.hpp"
using namespace flight::shared_types;

// Platform-aware memory allocation
auto memory_size = memory::MemorySize::from_mb(256);
auto allocation_result = memory::MemoryOperations::create_allocation(
    "session-123", memory_size, memory::MemoryPurpose::VmHeap
);

if (memory::is_ok(allocation_result)) {
    const auto& allocation = memory::unwrap(allocation_result);
    std::cout << "Allocated: " << allocation.size.to_string() << std::endl;
    
    // Automatic cleanup via RAII
    memory::MemoryOperations::free_allocation(allocation.id);
}
```

### Error Handling

```cpp
// Exception-free error handling with std::variant
auto platform_result = platform::PlatformDetector::detect_current_platform();

if (error::is_ok(platform_result)) {
    const auto& platform = error::unwrap(platform_result);
    std::cout << "Platform: " << platform.name << std::endl;
    
    if (platform.is_constrained()) {
        // Optimize for memory-constrained environments
        std::cout << "Using constrained mode" << std::endl;
    }
} else {
    const auto& error = error::unwrap_err(platform_result);
    
    // Get recovery suggestions
    auto suggestions = error::ErrorOperations::get_recovery_suggestions(error);
    for (const auto& suggestion : suggestions) {
        std::cout << "Suggestion: " << suggestion.description << std::endl;
    }
}
```

### Platform Detection

```cpp
// Adaptive behavior based on platform capabilities
auto platforms = platform::PlatformDetector::get_supported_platforms();

for (const auto& platform : platforms) {
    std::cout << platform.name << ": " 
              << platform.memory.total_memory.to_string();
    
    if (platform.is_cloud_platform()) {
        std::cout << " (Cloud)";
    } else if (platform.is_retro_platform()) {
        std::cout << " (Retro)";
    }
    
    std::cout << std::endl;
}
```

### Session Management

```cpp
// Universal session handling for Flight-Core and V6R
auto session_result = session::SessionOperations::create_session(
    session::SessionType::Development, 
    "v6r-medium", 
    std::string("user123")
);

if (error::is_ok(session_result)) {
    const auto& session = error::unwrap(session_result);
    
    // Monitor session health
    auto health_result = session::SessionOperations::get_session_health(session.id);
    if (error::is_ok(health_result)) {
        auto health = error::unwrap(health_result);
        std::cout << "Session health: " << 
            (health == session::SessionHealth::Healthy ? "Healthy" : "Degraded") 
            << std::endl;
    }
}
```

### Component Integration

```cpp
// Flight-Core component lifecycle management
auto component_result = integration::FlightCoreIntegration::create_hal_component("dreamcast");

if (error::is_ok(component_result)) {
    const auto& component = error::unwrap(component_result);
    
    std::cout << "Component: " << component.name << std::endl;
    std::cout << "Memory usage: " << component.memory_usage.used.to_string() << std::endl;
    std::cout << "Running: " << (component.is_running() ? "Yes" : "No") << std::endl;
}
```

## API Reference

### Core Namespaces

- `flight::shared_types::memory` - Memory management and allocation
- `flight::shared_types::error` - Exception-free error handling
- `flight::shared_types::platform` - Platform detection and capabilities
- `flight::shared_types::session` - Session lifecycle management  
- `flight::shared_types::component` - Component Model integration
- `flight::shared_types::integration` - High-level Flight-Core utilities

### Key Types

#### Result Types
```cpp
template<typename T>
using MemoryResult = std::variant<T, MemoryError>;

template<typename T>
using FlightResult = std::variant<T, FlightError>;
```

#### Platform Information
```cpp
struct PlatformInfo {
    std::string id, name;
    PlatformType type;
    PlatformCapability capability;
    PlatformMemoryInfo memory;
    // ...
};
```

#### Memory Management
```cpp
struct MemorySize {
    std::uint64_t bytes;
    std::string human_readable;
    
    static MemorySize from_mb(std::uint64_t mb);
    // Platform-specific helpers
    static MemorySize dreamcast_total();
    static MemorySize v6r_large_total();
};
```

## Platform Configurations

### Dreamcast (16MB RAM)
```cpp
#define FLIGHT_PLATFORM_DREAMCAST 1
#define FLIGHT_MEMORY_CONSTRAINED 1
#define FLIGHT_MAX_MEMORY_MB 16
```

### PlayStation Portable (32MB RAM)
```cpp
#define FLIGHT_PLATFORM_PSP 1
#define FLIGHT_MEMORY_CONSTRAINED 1
#define FLIGHT_MAX_MEMORY_MB 32
```

### V6R Cloud (Up to 2GB+)
```cpp
#define FLIGHT_PLATFORM_V6R 1
#define FLIGHT_CLOUD_PLATFORM 1
```

## Performance Characteristics

### Zero-Cost Abstractions
- **Compile-time platform detection** - No runtime platform checks
- **Template-based memory sizing** - Compile-time memory layout optimization
- **Inlined error handling** - Exception-free with minimal overhead
- **RAII resource management** - Automatic cleanup with no GC overhead

### Memory Footprint
- **Dreamcast**: ~4KB library overhead
- **PSP**: ~8KB library overhead  
- **Vita**: ~16KB library overhead
- **V6R**: ~32KB library overhead (includes full feature set)

## Integration with Flight-Core

### Memory Subsystem
The C++17 bindings integrate directly with Flight-Core's memory subsystem:

```cpp
// Direct integration with Flight-Core allocators
auto snapshot = memory::MemoryOperations::get_memory_snapshot("session-id");
auto v6r_config = memory::v6r::V6RMemoryUtils::create_snapshot(
    "session", "v6r-medium", memory::MemorySize::from_mb(512)
);
```

### Component Model
Full support for Flight-Core's Advanced Component Model:

```cpp
// Create components that integrate with Flight-Core runtime
auto hal = integration::FlightCoreIntegration::create_hal_component("platform");
auto runtime = integration::FlightCoreIntegration::create_runtime_component("platform");
```

### Error Propagation
Seamless error propagation between C++17 and Flight-Core:

```cpp
// Errors maintain full context and recovery information
auto error = error::ErrorOperations::create_platform_error(
    error::PlatformErrorCode::InsufficientPlatformMemory,
    "Dreamcast memory constraint violated",
    "dreamcast", "allocate_texture"
);
```

## Development Workflow

### Local Development
```bash
# Debug build with full error checking
cmake .. -DCMAKE_BUILD_TYPE=Debug -DFLIGHT_BUILD_CPP17_TESTS=ON
make && make test
```

### Cross-Platform Builds
```bash
# Dreamcast cross-compilation (requires KallistiOS)
cmake .. -DFLIGHT_TARGET_PLATFORM=dreamcast \
         -DCMAKE_TOOLCHAIN_FILE=dreamcast.cmake

# V6R container build
docker build -t flight-cpp17 .
docker run flight-cpp17 ./examples/flight_core_integration_example
```

### Integration Testing
```bash
# Run comprehensive integration tests
make test

# Platform-specific validation
ctest -L dreamcast
ctest -L v6r
ctest -L memory_constrained
```

## Contributing

### Code Style
- **Modern C++17** features encouraged (structured bindings, if constexpr, etc.)
- **Exception-free design** - Use result types instead of exceptions
- **Platform-aware implementations** - Consider memory constraints
- **Zero-cost abstractions** - No runtime overhead for compile-time features

### Testing
- Unit tests for all public APIs
- Platform-specific validation tests
- Memory constraint compliance tests
- Integration tests with actual Flight-Core components

### Documentation
- Comprehensive API documentation with examples
- Platform-specific usage guides
- Performance benchmarks for constrained platforms
- Migration guides from C bindings

## License

Licensed under the same terms as Flight-Core. See LICENSE file for details.

## Related Documentation

- [Flight-Core Integration Guide](../../docs/v6r-integration-guide.md)
- [Memory Types Reference](../../docs/README.md)
- [Cross-Language Compatibility](../../tests/README.md)
- [Platform Migration Guides](../../proompts/docs/v6r-migration/)
