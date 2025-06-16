# Universal WASM Runtime Core (UWRC) - Architecture Overview

## Project Vision

The Universal WASM Runtime Core (UWRC) is a hardware-agnostic WebAssembly Component Model runtime designed for embedded gaming systems. It provides a clean abstraction layer that allows the same game components to run on vastly different hardware platforms - from 1994 Sony PlayStation 1 (2MB RAM, 33MHz MIPS) to 2012 PS Vita (512MB RAM, quad-core ARM).

## Core Design Principles

### 1. Hardware Abstraction Through Composition
Rather than building platform-specific code into the core, UWRC uses a Hardware Abstraction Layer (HAL) where platform teams implement driver interfaces. The core runtime remains completely hardware-agnostic.

### 2. Component Model First
UWRC is built around the WebAssembly Component Model, not traditional WASM modules. This enables:
- **Type-safe composition** between components written in different languages
- **Resource ownership semantics** preventing leaks and use-after-free
- **Interface-driven development** with rich type systems
- **Language-agnostic ecosystem** where Rust physics engines work with C++ renderers

### 3. Performance at Scale
Designed to run on severely constrained hardware while maintaining high performance:
- **Zero-allocation hot paths** for real-time gaming
- **Adaptive JIT compilation** that falls back to interpretation on platforms without JIT support
- **Memory pooling and region-based allocation** to minimize fragmentation
- **Lock-free data structures** for multi-core platforms

### 4. Developer Experience Priority
Making retro game development feel modern:
- **Hot reload** of components during development
- **Rich debugging tools** with component-level profiling
- **Package management** with semantic versioning
- **Cross-platform compatibility** testing

## System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     Game Components                         │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐   │
│  │   Player    │ │   Physics   │ │     Rendering       │   │
│  │ Controller  │ │   Engine    │ │     System          │   │
│  │   (Rust)    │ │    (C++)    │ │   (JavaScript)      │   │
│  └─────────────┘ └─────────────┘ └─────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│                    UWRC Runtime Core                        │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────────┐    │
│  │ Component    │ │    Memory    │ │      Asset       │    │
│  │ Model Engine │ │   Manager    │ │    Streaming     │    │
│  └──────────────┘ └──────────────┘ └──────────────────┘    │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────────┐    │
│  │     JIT      │ │  Component   │ │    Registry      │    │
│  │  Compiler    │ │  Scheduler   │ │    System        │    │
│  └──────────────┘ └──────────────┘ └──────────────────┘    │
├─────────────────────────────────────────────────────────────┤
│              Hardware Abstraction Layer (HAL)               │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐   │
│  │   Display   │ │    Audio    │ │      Network        │   │
│  │   Driver    │ │   Driver    │ │      Driver         │   │
│  └─────────────┘ └─────────────┘ └─────────────────────┘   │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐   │
│  │    Input    │ │   Storage   │ │      Memory         │   │
│  │   Driver    │ │   Driver    │ │      Driver         │   │
│  └─────────────┘ └─────────────┘ └─────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│                Platform Implementation                       │
│        (PlayStation 1, Dreamcast, PSP, Vita, etc.)        │
└─────────────────────────────────────────────────────────────┘
```

## Core Modules

### 1. Component Model Engine (`uwrc-component-model/`)
The heart of UWRC - implements the WebAssembly Component Model specification.

**Key Features:**
- **Canonical ABI Implementation**: Type-safe conversion between component and core WASM types
- **Resource Handle Management**: Ownership semantics with automatic cleanup
- **Interface Linking**: Type-safe connection of component imports/exports
- **Component Composition**: Building applications from multiple components

**Performance Targets:**
- Component instantiation: < 1ms
- Inter-component calls: < 1μs overhead
- Type conversion: < 100ns for primitive types

### 2. Memory Management System (`uwrc-memory/`)
Sophisticated memory management designed for resource-constrained environments.

**Features:**
- **Component-Isolated Memory**: Each component gets its own memory space
- **Memory Pools**: Pre-allocated pools for common allocation sizes
- **Canonical Memory**: Special memory region for Component Model type conversions
- **Platform Adaptation**: Automatically adapts to available memory (2MB to 512MB+)

**Design Goals:**
- Zero fragmentation on long-running games
- Predictable allocation performance
- Memory safety between components

### 3. JIT Compilation Framework (`uwrc-jit/`)
Adaptive compilation system that provides native performance where possible.

**Architecture:**
```cpp
class JITCompiler {
public:
    virtual CompiledFunction* compile(WasmFunction* func) = 0;
    virtual void optimize(CompiledFunction* func, ProfileData* profile) = 0;
};

// Platform-specific implementations
class ARMJITCompiler : public JITCompiler;   // Vita, modern systems
class MIPSJITCompiler : public JITCompiler;  // PSP
class SH4Interpreter : public JITCompiler;   // Dreamcast (interpretation only)
class MIPSInterpreter : public JITCompiler;  // PlayStation 1 (interpretation only)
```

**Features:**
- **Profile-guided optimization**: Hot functions get aggressive optimization
- **Fallback interpretation**: Works on any platform
- **Incremental compilation**: Compile functions as needed
- **Platform-specific backends**: ARM NEON, MIPS DSP, etc.

### 4. Component Scheduler (`uwrc-scheduler/`)
Manages component execution across potentially multiple CPU cores.

**Scheduling Strategies:**
- **Single-threaded**: For platforms like PlayStation 1 and Dreamcast
- **Multi-core affinity**: Pin components to specific cores on Vita
- **Priority-based**: Critical components (input, network) get priority
- **Adaptive load balancing**: Move components between cores based on load

### 5. Asset Streaming System (`uwrc-assets/`)
Intelligent asset loading and caching optimized for each platform's storage characteristics.

**Features:**
- **Texture streaming**: Load high-res textures based on proximity
- **Audio compression**: Platform-specific audio formats
- **Model optimization**: LOD generation and mesh simplification
- **Caching strategies**: Memory vs storage trade-offs

### 6. Registry and Package Management (`uwrc-registry/`)
Component ecosystem management with semantic versioning.

**Capabilities:**
- **Dependency resolution**: Automatically resolve component dependencies
- **Version compatibility**: Semantic versioning with compatibility checking
- **Hot reload**: Update components without restarting the game
- **Cross-platform builds**: Same component source, multiple platform targets

## Implementation Language Choice: C++20

UWRC is implemented in modern C++20 for several key reasons:

### Performance Requirements
- **Zero-cost abstractions**: Templates and constexpr enable compile-time optimization
- **Manual memory management**: Critical for embedded systems with limited RAM
- **Direct hardware access**: Can interface with platform-specific APIs efficiently

### Platform Compatibility
- **Universal toolchain support**: Every target platform has a C++ compiler
- **Existing ecosystems**: Integrates well with platform SDKs (KOS, PSPSDK, VitaSDK)
- **Stable ABI**: Can interface with C libraries and platform APIs

### Modern C++20 Features Used
```cpp
// Concepts for type safety
template<ComponentInterface T>
class ComponentInstance;

// Coroutines for async operations
co_await load_asset("texture.png");

// Modules for better compilation
import uwrc.component_model;

// consteval for compile-time evaluation
consteval auto get_platform_constraints() -> PlatformTraits;
```

## Build System and Dependencies

### CMake + CPM Architecture
```cmake
# Root CMakeLists.txt
project(uwrc VERSION 2.0.0 LANGUAGES CXX)

# Core modules (always built)
add_subdirectory(uwrc-hal)              # Hardware abstraction interfaces
add_subdirectory(uwrc-component-model)  # Component Model implementation
add_subdirectory(uwrc-memory)           # Memory management
add_subdirectory(uwrc-scheduler)        # Component scheduling
add_subdirectory(uwrc-assets)           # Asset streaming
add_subdirectory(uwrc-registry)         # Package management

# Optional modules based on platform capabilities
if(UWRC_ENABLE_JIT)
    add_subdirectory(uwrc-jit)
endif()

if(UWRC_ENABLE_NETWORKING) 
    add_subdirectory(uwrc-network)
endif()

# Platform implementations (separate repositories)
# These are added by the platform teams
```

### Dependency Management
UWRC uses CPM (CMake Package Manager) for all dependencies:

```cmake
# External dependencies
CPMAddPackage("gh:microsoft/GSL@3.1.0")           # Guidelines Support Library
CPMAddPackage("gh:fmtlib/fmt@9.1.0")              # String formatting
CPMAddPackage("gh:gabime/spdlog@1.11.0")          # Logging
CPMAddPackage("gh:nlohmann/json@3.11.2")          # JSON parsing

# Platform-specific packages added by platform teams
if(DREAMCAST)
    CPMAddPackage("gh:sega-team/uwrc-dreamcast-platform@1.0.0")
endif()
```

## Configuration System

### Compile-Time Configuration
```cpp
// uwrc-core/include/uwrc/config.h

namespace uwrc::config {

// Platform capabilities (set by platform implementation)
struct PlatformTraits {
    static constexpr size_t TOTAL_RAM = /* set by platform */;
    static constexpr size_t CACHE_LINE_SIZE = /* set by platform */;
    static constexpr bool HAS_JIT = /* set by platform */;
    static constexpr bool HAS_MULTICORE = /* set by platform */;
    static constexpr bool HAS_SIMD = /* set by platform */;
    static constexpr bool HAS_NETWORKING = /* set by platform */;
    
    using CPUArchitecture = /* ARM/MIPS/SH4/x86 */;
    using GraphicsAPI = /* OpenGL ES/PowerVR/Custom */;
};

// Derived configuration
struct RuntimeConfig {
    static constexpr size_t COMPONENT_POOL_SIZE = PlatformTraits::TOTAL_RAM / 4;
    static constexpr size_t JIT_CACHE_SIZE = PlatformTraits::HAS_JIT ? 
        PlatformTraits::TOTAL_RAM / 8 : 0;
    static constexpr size_t MAX_COMPONENTS = PlatformTraits::TOTAL_RAM > 64*1024*1024 ? 
        32 : 8;
};

} // namespace uwrc::config
```

### Runtime Configuration
```cpp
// Runtime configuration for dynamic adaptation
struct RuntimeOptions {
    // Memory configuration
    float memory_component_ratio = 0.6f;    // 60% for components
    float memory_assets_ratio = 0.3f;       // 30% for assets
    float memory_system_ratio = 0.1f;       // 10% for system
    
    // Performance tuning
    uint32_t jit_compilation_threshold = 100;  // Calls before JIT
    uint32_t hot_reload_check_interval = 1000; // ms between checks
    
    // Quality settings
    enum class Quality { Low, Medium, High, Auto };
    Quality rendering_quality = Quality::Auto;
    Quality audio_quality = Quality::Auto;
    
    // Debug options
    bool enable_profiling = false;
    bool enable_validation = false;
    bool enable_hot_reload = false;
};
```

## Error Handling Strategy

### Exception Policy
UWRC uses a **no-exceptions policy** suitable for embedded systems:

```cpp
// All operations return Results instead of throwing
template<typename T, typename E = Error>
class Result {
public:
    bool is_ok() const noexcept;
    bool is_err() const noexcept;
    
    T& value() & noexcept;      // Only call if is_ok()
    E& error() & noexcept;      // Only call if is_err()
    
    // Monadic operations
    template<typename F>
    auto map(F&& func) -> Result<decltype(func(std::declval<T>())), E>;
    
    template<typename F>
    auto and_then(F&& func) -> decltype(func(std::declval<T>()));
};

// Usage example
auto load_component(const std::string& path) -> Result<ComponentInstance> {
    auto component_data = read_file(path);
    if (!component_data.is_ok()) {
        return component_data.error();
    }
    
    return parse_component(component_data.value());
}
```

### Error Categories
```cpp
enum class ErrorCategory {
    Memory,      // Out of memory, allocation failure
    IO,          // File not found, read/write errors
    Parse,       // Invalid WASM, malformed component
    Runtime,     // Component execution errors
    Platform,    // Hardware-specific errors
    Network,     // Connection failures, timeouts
};

struct Error {
    ErrorCategory category;
    std::string message;
    std::source_location location;  // C++20 feature for debugging
    
    // Optional error chaining
    std::unique_ptr<Error> cause;
};
```

## Testing Strategy

### Multi-Level Testing
```cpp
// Unit tests for individual components
TEST(ComponentModel, CanonicalABI_StringLifting) {
    auto abi = create_test_abi();
    auto result = abi.lift_string(test_ptr, test_len);
    EXPECT_EQ(result.value(), "test string");
}

// Integration tests across modules
TEST(Integration, ComponentComposition) {
    auto physics = load_component("test_physics.wasm");
    auto renderer = load_component("test_renderer.wasm");
    
    auto composite = compose_components({physics, renderer});
    EXPECT_TRUE(composite.is_ok());
}

// Platform compatibility tests
TEST(Platform, DreamcastMemoryConstraints) {
    RuntimeConfig config;
    config.total_memory = 16 * 1024 * 1024;  // 16MB
    
    auto runtime = create_runtime(config);
    EXPECT_TRUE(runtime.can_load_component("minimal_game.wasm"));
}

// Performance benchmarks
BENCHMARK(ComponentCall_Overhead) {
    auto component = load_test_component();
    for (auto _ : state) {
        component.call("update", delta_time);
    }
}
```

### Fuzzing and Property Testing
```cpp
// Fuzz testing for component parsing
void fuzz_component_parser(const uint8_t* data, size_t size) {
    auto result = parse_component(std::span{data, size});
    // Should never crash, even on invalid input
}

// Property-based testing
PROPERTY_TEST(Memory, AllocationsAreDeterministic) {
    auto allocator = create_test_allocator();
    
    std::vector<size_t> sizes = generate_random_sizes(100);
    std::vector<void*> ptrs;
    
    // Allocate
    for (size_t size : sizes) {
        ptrs.push_back(allocator.allocate(size));
    }
    
    // Deallocate in random order
    std::shuffle(ptrs.begin(), ptrs.end());
    for (void* ptr : ptrs) {
        allocator.deallocate(ptr);
    }
    
    // Allocator should be back to initial state
    EXPECT_EQ(allocator.allocated_bytes(), 0);
}
```

## Documentation Requirements

Each module must include:

1. **Architecture Decision Records (ADRs)**: Why specific designs were chosen
2. **API Documentation**: Comprehensive documentation of all public interfaces
3. **Performance Characteristics**: Big-O analysis and benchmark results
4. **Platform Compatibility Matrix**: What works on which platforms
5. **Migration Guides**: How to upgrade between versions
6. **Example Code**: Working examples for common use cases

## Next Steps for Implementation

An AI agent implementing UWRC should start with:

1. **Read all the companion specification documents** (component-model.md, memory-system.md, etc.)
2. **Set up the basic project structure** with CMake and CPM
3. **Implement the HAL interfaces** (see hal-specification-guide.md)
4. **Create a minimal Component Model parser** that can load simple components
5. **Build the memory management foundation** with pool allocators
6. **Add component instantiation and basic execution**
7. **Implement the type system and Canonical ABI**
8. **Add JIT compilation for supported platforms**
9. **Build the registry and package management system**
10. **Create comprehensive tests and benchmarks**

The goal is to create a production-ready WebAssembly Component Model runtime that can bring modern game development practices to classic gaming hardware while maintaining the performance characteristics those platforms require.
