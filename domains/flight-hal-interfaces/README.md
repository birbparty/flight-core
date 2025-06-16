# Flight HAL Interfaces

## Overview

Flight HAL Interfaces provides the foundational interface definitions for hardware abstraction across the Flight ecosystem. This project enables cross-platform development spanning from vintage gaming hardware like the Sega Dreamcast (16MB RAM) to modern high-performance systems.

## Key Features

- **Cross-Platform HAL Interfaces**: Unified hardware abstraction for 9+ platforms
- **C++17 Compatibility**: Supports legacy platforms while maintaining modern C++ practices  
- **Interface Compliance Testing**: Comprehensive validation framework for HAL implementations
- **Performance Benchmarking**: Built-in performance testing and validation tools
- **Resource Management**: Sophisticated resource coordination and lifecycle management
- **Event System**: Unified event handling across all HAL interfaces

## Supported Platforms

- **Retro Gaming**: Sega Dreamcast, Sony PSP
- **Desktop**: Linux, macOS, Windows
- **Mobile**: iOS, Android  
- **Web**: WebAssembly/Emscripten
- **Specialized**: Steam Deck, Terminal/Headless

## Project Structure

```
flight-hal-interfaces/
├── include/flight/hal/         # Public HAL interface headers
│   ├── core/                   # Core HAL infrastructure
│   ├── interfaces/             # Hardware interface definitions
│   └── platform/               # Platform abstraction utilities
├── src/                        # Interface implementation utilities
├── tests/                      # Comprehensive test infrastructure
├── benchmarks/                 # Performance benchmarking
├── docs/                       # Interface documentation
├── examples/                   # Usage examples
├── tools/                      # Development tools
└── scripts/                    # Build and utility scripts
```

## Quick Start

### Building with CMake

```bash
# Configure the project
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# Build the library
cmake --build build

# Run tests
ctest --test-dir build
```

### Integration

```cpp
#include <flight/hal/interfaces/memory.hpp>
#include <flight/hal/core/driver_registry.hpp>

// Register platform-specific memory driver
auto& registry = flight::hal::DriverRegistry::instance();
registry.register_driver<flight::hal::IMemoryInterface>(
    std::make_unique<PlatformMemoryDriver>()
);

// Use the interface
auto memory = registry.get_interface<flight::hal::IMemoryInterface>();
auto buffer = memory->allocate(1024, flight::hal::MemoryAlignment::Cache);
```

## Documentation

- **[API Reference](docs/api/)** - Complete interface documentation
- **[Integration Guide](docs/guides/integration_guide.md)** - How to use HAL interfaces
- **[Driver Development](docs/guides/driver_development.md)** - Implementing HAL drivers
- **[Architecture Guide](docs/architecture/)** - Design principles and patterns

## Contributing

This project follows Flight ecosystem development standards:

1. **Code Quality**: All code must pass static analysis and formatting checks
2. **Testing**: 95% test coverage requirement with comprehensive compliance tests
3. **Documentation**: All public interfaces must be fully documented
4. **Performance**: Benchmark validation required for performance-critical paths

## Requirements

- **Compiler**: C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- **Build System**: CMake 3.20 or later
- **Testing**: Google Test (optional, for running tests)
- **Benchmarking**: Google Benchmark (optional, for performance tests)

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Flight Ecosystem

Flight HAL Interfaces is part of the larger Flight project, enabling modern WebAssembly gaming experiences across vintage and modern hardware. For more information about the Flight ecosystem, visit [flight-project.org](https://flight-project.org).

---

**Status**: Foundation Phase - Interface definitions and core infrastructure  
**Next Phase**: Platform-specific driver implementations
