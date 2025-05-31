# Flight Core

High-performance, cross-platform WebAssembly Component Model runtime designed to work from 16MB embedded systems to modern servers.

## 🚀 Overview

Flight Core is a C++14 runtime that implements the WebAssembly Component Model, optimized for extreme portability and minimal memory footprint. It serves as the foundation for both server applications (Flight) and gaming platforms (UWRC).

## 📁 Project Structure

```
flight-core/
├── cmake/                    # CMake modules and toolchains
│   ├── modules/             # CPM.cmake and utilities
│   └── toolchains/          # Platform-specific toolchains
├── modules/                 # Core library modules
│   ├── flight-core-wasm/    # WASM types and utilities
│   ├── flight-core-runtime/ # WASM interpreter/runtime
│   ├── flight-core-component/ # Component Model implementation
│   ├── flight-core-memory/  # Memory management
│   ├── flight-core-hal/     # Hardware abstraction layer
│   └── flight-core-telemetry/ # OpenTelemetry support
├── examples/                # Example applications
├── docs/                    # Documentation
├── scripts/                 # Build and utility scripts
├── third_party/            # Vendored dependencies
└── tools/                  # Development tools
```

## 🛠️ Build Requirements

- CMake 3.14 or higher
- C++14 compatible compiler
- Platform-specific requirements:
  - **macOS**: Xcode or Command Line Tools
  - **Web**: Emscripten SDK
  - **Dreamcast**: KallistiOS toolchain
  - **PSP**: PSPSDK

## 🔧 Building

### Quick Start

```bash
# Clone the repository
git clone https://github.com/yourusername/flight-core.git
cd flight-core

# Build using the provided script
./scripts/build.sh

# Or build manually
mkdir build && cd build
cmake ..
cmake --build . --parallel
```

### Build Options

```bash
# Release build
./scripts/build.sh --release

# Clean build
./scripts/build.sh --clean

# Using Ninja generator
./scripts/build.sh --generator Ninja
```

### CMake Options

- `FLIGHT_CORE_BUILD_TESTS` - Build unit tests (default: ON)
- `FLIGHT_CORE_BUILD_BENCHMARKS` - Build benchmarks (default: ON)
- `FLIGHT_CORE_BUILD_EXAMPLES` - Build examples (default: ON)
- `FLIGHT_CORE_BUILD_DOCS` - Build documentation (default: OFF)
- `FLIGHT_CORE_USE_OPENTELEMETRY` - Enable OpenTelemetry support (default: OFF)

## 📦 Modules

### flight-core-wasm
WebAssembly types, validation, and core structures.

### flight-core-runtime
WASM interpreter and execution engine optimized for embedded systems.

### flight-core-component
Component Model implementation following the WASI standards.

### flight-core-memory
Memory management with support for linear memory and custom allocators.

### flight-core-hal
Hardware abstraction layer for platform-specific operations:
- macOS (native)
- Emscripten (WebAssembly)
- Dreamcast (KallistiOS)
- PSP (PSPSDK)

### flight-core-telemetry
Lightweight telemetry support with optional OpenTelemetry integration.

## 🧪 Testing

```bash
# Run all tests
cd build
ctest

# Run specific module tests
./modules/flight-core-wasm/flight-core-wasm-tests
```

## 📊 Benchmarks

```bash
# Run benchmarks
./modules/flight-core-wasm/flight-core-wasm-benchmarks
```

## 🤝 Contributing

Please read our contributing guidelines before submitting pull requests.

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.

## 🎯 Design Goals

1. **Extreme Portability**: Run on systems with as little as 16MB RAM
2. **Modern C++**: Use C++14 features while maintaining embedded compatibility
3. **Modular Architecture**: Clear separation of concerns with minimal coupling
4. **Performance**: Optimized for both embedded and server environments
5. **Standards Compliance**: Full WebAssembly Component Model support

## 🚀 Getting Started

Check out the `examples/` directory for sample applications demonstrating Flight Core capabilities.

For detailed API documentation, see the `docs/` directory.
