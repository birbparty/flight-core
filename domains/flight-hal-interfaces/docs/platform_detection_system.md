# Flight HAL Platform Detection System

## Overview

The Flight HAL Platform Detection System provides comprehensive compile-time and runtime platform identification and capability detection across all target platforms, from resource-constrained embedded systems like the Dreamcast to modern desktop environments.

## System Architecture

### Core Components

1. **Platform Detection Header** (`platform_detection.hpp`)
   - Compile-time platform identification macros
   - Memory constraint constants
   - Capability flags
   - Runtime detection interface

2. **Platform-Specific Providers** (`platform_capabilities.hpp`)
   - Platform-specific capability providers
   - Factory pattern for provider creation
   - Fallback capability mapping

3. **Runtime Detection** (`platform_detection.cpp`)
   - Dynamic platform information gathering
   - Memory probing
   - CPU core detection
   - Feature validation

4. **Capability Providers** (`platform_capabilities.cpp`)
   - Platform-specific implementations
   - Runtime capability validation
   - Graceful degradation support

## Supported Platforms

### Target Platforms

| Platform | Status | Memory Baseline | Performance Tier | Key Features |
|----------|--------|----------------|------------------|--------------|
| **Sega Dreamcast** | ✅ Implemented | 16+8+2 MB | Minimal | PowerVR2, SH-4, Fixed-function 3D |
| **Sony PSP** | ✅ Implemented | 32-64 MB | Limited | MIPS dual-core, WiFi, 3D GPU |
| **Web/Emscripten** | ✅ Implemented | 512+ MB | Standard | WebGL, Web Audio, Sandbox restrictions |
| **macOS** | ✅ Implemented | 8+ GB | High | Metal, Core Audio, Full capabilities |
| **Windows** | 🔄 Planned | 8+ GB | High | DirectX, WASAPI, Full capabilities |
| **Linux** | 🔄 Planned | 8+ GB | High | OpenGL/Vulkan, ALSA/PulseAudio |

### Platform Detection Features

#### Compile-Time Detection
```cpp
// Platform identification
#if FLIGHT_PLATFORM_DREAMCAST
    // Dreamcast-specific code
#elif FLIGHT_PLATFORM_PSP
    // PSP-specific code
#elif FLIGHT_PLATFORM_WEB
    // Web-specific code
#endif

// Capability checks
#if FLIGHT_HAS_THREADING
    // Multi-threaded code path
#else
    // Single-threaded fallback
#endif
```

#### Runtime Detection
```cpp
// Platform information
PlatformInfo info = RuntimePlatformDetector::detect_platform_info();
std::cout << "Platform: " << info.platform_name << "\n";
std::cout << "Memory: " << info.total_memory << " bytes\n";
std::cout << "CPU Cores: " << info.cpu_cores << "\n";

// Capability detection
uint32_t capabilities = RuntimePlatformDetector::detect_capabilities();
bool has_shaders = capabilities::has_capability(capabilities, HALCapability::VertexShaders);
```

## Capability System

### Capability Categories

1. **Threading Capabilities**
   - Multi-threading support
   - Atomic operations
   - Thread synchronization

2. **Memory Management**
   - Virtual memory support
   - Memory protection
   - DMA access

3. **Graphics Capabilities**
   - Hardware 2D/3D acceleration
   - Programmable shaders (vertex/fragment/compute)
   - Modern graphics APIs

4. **Audio Capabilities**
   - Hardware audio acceleration
   - Multi-channel output
   - Hardware effects/DSP

5. **Input Capabilities**
   - Multi-touch support
   - Accelerometer/sensors
   - Gamepad support

6. **Networking Capabilities**
   - Basic networking
   - WiFi support
   - Bluetooth connectivity

7. **System Capabilities**
   - High-precision timing
   - Persistent storage
   - Asynchronous I/O
   - Power management

### Performance Tiers

| Tier | Description | Memory Range | Examples |
|------|-------------|--------------|----------|
| **Minimal** | Basic functionality | < 64 MB | Dreamcast |
| **Limited** | Moderate capabilities | 64-512 MB | PSP, low-end mobile |
| **Standard** | Good performance | 512MB-2GB | Web, mid-range systems |
| **High** | Full capabilities | 2GB+ | Desktop, high-end systems |

## Usage Examples

### Basic Platform Detection
```cpp
#include "flight/hal/core/platform_detection.hpp"

// Compile-time checks
if constexpr (platform_detection::is_dreamcast()) {
    // Dreamcast-specific optimizations
    use_fixed_function_pipeline();
} else if constexpr (platform_detection::has_modern_graphics()) {
    // Modern shader-based rendering
    use_programmable_shaders();
}

// Runtime detection
auto info = RuntimePlatformDetector::detect_platform_info();
if (info.total_memory < 64 * 1024 * 1024) {
    enable_aggressive_memory_pooling();
}
```

### Capability-Based Feature Selection
```cpp
#include "flight/hal/core/platform_capabilities.hpp"

auto provider = create_platform_capability_provider();

// Graphics strategy selection
if (provider->supports_capability(HALCapability::ComputeShaders)) {
    use_compute_based_renderer();
} else if (provider->supports_capability(HALCapability::VertexShaders)) {
    use_shader_based_renderer();
} else if (provider->supports_capability(HALCapability::Hardware3D)) {
    use_fixed_function_renderer();
} else {
    use_software_renderer();
}

// Threading strategy
if (provider->supports_capability(HALCapability::Threading)) {
    uint32_t cores = provider->get_platform_info().cpu_cores;
    create_thread_pool(cores);
} else {
    use_single_threaded_execution();
}
```

### Graceful Degradation
```cpp
// Audio processing with fallbacks
bool setup_audio() {
    auto provider = create_platform_capability_provider();
    
    if (provider->supports_capability(HALCapability::AudioEffects)) {
        return setup_hardware_dsp();
    } else if (provider->supports_capability(HALCapability::HardwareAudio)) {
        return setup_hardware_mixing();
    } else if (provider->has_fallback(HALCapability::HardwareAudio)) {
        return setup_software_mixing();
    }
    
    return false; // No audio support
}
```

## Memory Management Strategy

### Platform-Specific Constraints

#### Dreamcast (16+8+2 MB)
- **Main RAM**: 16 MB for application data
- **VRAM**: 8 MB for textures and framebuffers  
- **ARAM**: 2 MB for audio samples
- **Strategy**: Aggressive pooling, minimal allocations

#### PSP (32-64 MB)
- **Model Detection**: 32MB (PSP-1000) vs 64MB (PSP-2000+)
- **Strategy**: Conservative allocation, moderate pooling

#### Web Platform (Variable)
- **Detection**: Browser-dependent memory limits
- **Strategy**: Adaptive allocation based on available memory

#### Desktop (8+ GB)
- **Strategy**: Liberal allocation, minimal constraints

## Integration with HAL Architecture

### Platform Coordination
The platform detection system integrates seamlessly with the HAL architecture:

```cpp
class Platform {
    // Uses platform detection for capability provider creation
    void detect_platform_capabilities() {
        capability_provider_ = create_platform_capability_provider();
        platform_info_ = std::make_unique<PlatformInfo>(
            capability_provider_->get_platform_info()
        );
    }
    
    // Provides capability information to all HAL interfaces
    const ICapabilityProvider& get_capabilities() const {
        return *capability_provider_;
    }
};
```

### Driver Registration
Platform capabilities influence driver selection and registration:

```cpp
// Only register drivers for supported capabilities
if (platform.get_capabilities().supports_capability(HALCapability::WiFi)) {
    registry.register_driver<WiFiInterface>(std::make_unique<WiFiDriver>());
}
```

## Testing and Validation

### Test Coverage
- ✅ Compile-time platform detection accuracy
- ✅ Runtime memory detection 
- ✅ CPU core detection
- ✅ Capability enumeration correctness
- ✅ Performance tier classification
- ✅ Graceful degradation chains
- ✅ Platform-specific provider creation

### Example Output (macOS ARM64)
```
Platform Name: macOS
Architecture: ARM64
Performance Tier: High
Total Memory: 64.00 GB
CPU Cores: 16
Capabilities: 21 detected
- Threading, Hardware3D, VertexShaders, WiFi, etc.
```

## Future Enhancements

### Planned Features
1. **Dynamic Capability Changes**: Runtime capability updates
2. **Performance Benchmarking**: Actual performance measurement
3. **Resource Monitoring**: Real-time resource usage tracking
4. **Platform Variants**: Sub-platform detection (iOS, Android variants)

### Additional Platforms
- **Windows**: DirectX detection, Windows API capabilities
- **Linux**: OpenGL/Vulkan support, audio system detection
- **iOS**: Metal support, mobile-specific capabilities
- **Android**: OpenGL ES, Vulkan, device-specific features

## API Reference

### Key Classes
- `RuntimePlatformDetector`: Runtime detection utilities
- `BasePlatformCapabilityProvider`: Base capability provider
- `DreamcastCapabilityProvider`: Dreamcast-specific implementation
- `PSPCapabilityProvider`: PSP-specific implementation  
- `WebCapabilityProvider`: Web platform implementation
- `MacOSCapabilityProvider`: macOS-specific implementation

### Key Functions
- `create_platform_capability_provider()`: Factory function
- `platform_detection::is_*()`: Compile-time platform checks
- `platform_detection::has_*()`: Compile-time capability checks
- `capabilities::has_capability()`: Runtime capability testing

## Conclusion

The Flight HAL Platform Detection System provides a robust, accurate, and flexible foundation for cross-platform development. It enables developers to write platform-aware code that automatically adapts to the capabilities and constraints of each target platform, from resource-constrained retro systems to modern high-performance environments.
