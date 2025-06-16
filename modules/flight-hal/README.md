# Flight HAL Module

## Purpose and Scope

The `flight-hal` (Hardware Abstraction Layer) module provides platform-independent interfaces for system operations. It abstracts file I/O, threading, timing, and system information queries, allowing Flight to run seamlessly across diverse platforms from embedded systems to cloud servers.

## Public API Overview

### Platform Detection
- `Platform`: Enumeration of supported platforms
- `get_current_platform()`: Runtime platform detection

### File System
- `File`: Abstract file interface for I/O operations
- `open_file()`: Platform-agnostic file opening
- `file_exists()`: Check file existence
- `file_size()`: Get file size

### Threading (Platform-Dependent)
- `Thread`: Abstract thread interface
- Threading support varies by platform

### Time Management
- `TimePoint`: High-resolution time representation
- `Duration`: Time duration utilities
- `now()`: Get current time
- `sleep_for()`: Cross-platform sleep

### System Information
- `SystemInfo`: System capabilities and resources
- `get_system_info()`: Query system properties

## Usage Examples

```cpp
#include <flight/hal/hal.hpp>

using namespace flight::hal;

// Get platform information
Platform platform = get_current_platform();
SystemInfo info = get_system_info();

// File operations
auto file = open_file("data.wasm", "rb");
if (file && file->is_open()) {
    size_t size = file_size("data.wasm");
    std::vector<uint8_t> buffer(size);
    file->read(buffer.data(), size);
}

// Time operations
auto start = now();
sleep_for(std::chrono::milliseconds(100));
auto elapsed = now() - start;
```

## Dependencies

This module has no dependencies on other Flight modules. It provides platform services to all other modules.

## Platform Support

### Fully Supported
- **macOS**: Full feature set including threading
- **Emscripten**: Web-compatible implementations
- **Linux**: Full POSIX compliance

### Embedded Platforms
- **Dreamcast**: Custom file I/O, no threading
- **PSP**: Limited threading, custom allocators
- **PS Vita**: Full feature set with custom optimizations

## Build Instructions

This module is built as part of the Flight project:

```bash
# From project root
cmake -B build
cmake --build build --target flight-hal
```

To run tests:
```bash
cmake --build build --target flight-hal-tests
ctest -R flight-hal
```

## Architecture Notes

- Platform detection at compile-time and runtime
- Conditional compilation for platform-specific code
- Header-only interfaces with platform-specific implementations
- Graceful degradation for unsupported features
- Zero-cost abstractions where possible

## Platform-Specific Implementations

Each platform has its own source directory:
- `src/macos/`: macOS-specific implementations
- `src/emscripten/`: WebAssembly browser implementations
- `src/dreamcast/`: Dreamcast KallistiOS implementations
- `src/psp/`: PSP SDK implementations
