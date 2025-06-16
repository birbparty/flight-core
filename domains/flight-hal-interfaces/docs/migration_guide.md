# 🚀 Flight HAL Migration and Upgrade Guide

## Overview

This comprehensive guide provides step-by-step procedures for migrating between Flight HAL versions, maintaining compatibility across extreme platform diversity, and managing the evolution of this sophisticated cross-platform Hardware Abstraction Layer.

The Flight HAL system supports platforms ranging from memory-constrained embedded devices (Sega Dreamcast - 16MB) to unlimited modern systems, requiring careful consideration during migration procedures.

---

## 📋 Table of Contents

1. [Migration Fundamentals](#migration-fundamentals)
2. [Semantic Versioning Strategy](#semantic-versioning-strategy)
3. [Universal API Evolution](#universal-api-evolution)
4. [Hardware-Capability Migration](#hardware-capability-migration)
5. [Interface Migration Patterns](#interface-migration-patterns)
6. [Driver Migration Workflows](#driver-migration-workflows)
7. [Build System Migration](#build-system-migration)
8. [Platform-Specific Considerations](#platform-specific-considerations)
9. [Validation and Testing](#validation-and-testing)
10. [Deprecation Management](#deprecation-management)
11. [Troubleshooting Common Issues](#troubleshooting-common-issues)
12. [Migration Examples](#migration-examples)

---

## Migration Fundamentals

### Core Migration Principles

**🎯 Zero Breaking Changes Philosophy**
- Maintain compatibility across all supported platforms
- Use capability detection for hardware-specific features
- Preserve existing API contracts during transitions
- Support extreme platform diversity (16MB Dreamcast → Unlimited PC)

**⚡ Feature Classification**
- **Hardware Features**: Platform-specific capabilities requiring detection
- **Programming Features**: Universal conveniences available on all platforms

**🔄 Graceful Evolution**
- Backward compatibility guaranteed for Minor and Patch versions
- Forward compatibility planning for sustainable growth
- Platform constraints respected during feature additions

### Hardware vs Programming Features

#### Hardware Features (Use Capability Detection)
```cpp
// Example: Platform-specific hardware capability
class IGraphicsInterface {
    HALResult<StereoRenderTarget> create_stereo_target(uint32_t width, uint32_t height) {
        if (!supports_capability(HALCapability::StereoRendering)) {
            return HALError::UnsupportedOperation("Stereo rendering not available on this platform");
        }
        return create_stereo_target_impl(width, height);
    }
};
```

Examples:
- **3DS**: Dual monitors (top screen 3D support, bottom screen touch)
- **Dreamcast**: PowerVR2 tile-based rendering
- **Modern PC**: Metal/DirectX 12/Vulkan advanced features
- **Web**: WebGL extensions and capabilities

#### Programming Features (Universal Implementation)
```cpp
// Example: Universal programming convenience
class IMemoryInterface {
    // NEW: Universal batch allocation - works on ALL platforms
    HALResult<std::vector<MemoryAllocation>> allocate_batch(
        const std::vector<AllocationRequest>& requests) {
        
        std::vector<MemoryAllocation> results;
        results.reserve(requests.size());
        
        for (const auto& request : requests) {
            auto result = allocate(request);
            if (!result) {
                // Cleanup on failure
                for (auto& allocation : results) {
                    deallocate(allocation.ptr);
                }
                return result.error();
            }
            results.push_back(result.value());
        }
        
        return results;
    }
};
```

Examples:
- Convenience methods and helper functions
- Enhanced error reporting with more context
- Performance optimization utilities
- Developer debugging and profiling tools
- Build system enhancements

---

## Semantic Versioning Strategy

### Version Format: MAJOR.MINOR.PATCH

#### Major Version (X.0.0) - Breaking Changes
**Scope**: Core architecture changes requiring migration planning
**Migration Impact**: High - Requires careful planning and testing
**Timeline**: 6-12 months between major versions

**Examples**:
- Core interface restructuring
- Driver registration system overhaul
- HALResult paradigm changes
- Build system major transitions (CMake → Buck2)

**Migration Requirements**:
- Full compatibility testing across all platforms
- Driver update procedures
- Build system configuration updates
- Performance validation

#### Minor Version (0.X.0) - New Features
**Scope**: New interfaces, capabilities, and backward-compatible additions
**Migration Impact**: Low - Automatic compatibility maintained
**Timeline**: 2-3 months between minor versions

**Examples**:
- New HAL interfaces (IShaderInterface, ICryptoInterface)
- Hardware capability additions
- Performance monitoring enhancements
- Platform support expansions

**Migration Requirements**:
- Capability detection verification
- Platform-specific testing
- Documentation updates

#### Patch Version (0.0.X) - Bug Fixes
**Scope**: Bug fixes, performance improvements, no API changes
**Migration Impact**: Minimal - Drop-in replacement
**Timeline**: 2-4 weeks between patch versions

**Examples**:
- Memory leak fixes
- Performance optimizations
- Platform-specific bug corrections
- Documentation improvements

**Migration Requirements**:
- Regression testing
- Performance validation

### Version Compatibility Matrix

| Current Version | Target Version | Compatibility | Migration Required |
|-----------------|----------------|---------------|-------------------|
| 1.0.x → 1.1.x  | Minor Upgrade  | Full ✅       | None              |
| 1.1.x → 1.0.x  | Minor Downgrade| Full ✅       | Feature Loss      |
| 1.x.x → 2.0.0  | Major Upgrade  | Breaking ⚠️   | Full Migration    |
| 2.0.0 → 1.x.x  | Major Downgrade| Breaking ❌   | Not Supported     |

---

## Universal API Evolution

### Adding Programming Features

Programming features must work universally across all platforms. These include convenience methods, enhanced error handling, and developer experience improvements.

#### Step-by-Step Universal Feature Addition

**1. Design Phase**
```cpp
// Propose universal enhancement to existing interface
class IMemoryInterface {
    // Existing methods...
    
    // NEW: Enhanced allocation with detailed context
    HALResult<MemoryAllocation> allocate_with_context(
        const AllocationRequest& request,
        const std::string& context_name = "unknown",
        const std::source_location& location = std::source_location::current());
    
    // NEW: Memory usage reporting
    HALResult<MemoryUsageReport> generate_usage_report() const;
    
    // NEW: Allocation debugging helpers
    HALResult<std::vector<AllocationInfo>> get_active_allocations() const;
};
```

**2. Implementation Requirements**
- Must work on ALL platforms (Dreamcast, PSP, Modern PC, Web)
- Cannot depend on platform-specific features
- Should gracefully handle platform constraints

**3. Platform Validation**
```bash
# Test on all supported platforms
make test_memory_interface_universal
./test_dreamcast_memory_features    # 16MB constraint testing
./test_psp_memory_features          # Dual-core considerations
./test_web_memory_features          # Browser limitation testing
./test_macos_memory_features        # Full capability testing
```

#### Universal Feature Implementation Pattern

```cpp
// Universal feature implementation template
namespace flight::hal {

template<typename Interface>
class UniversalFeatureExtension {
public:
    // Universal helper that works on all platforms
    static HALResult<std::vector<typename Interface::ResourceType>> 
    batch_operation(Interface* interface, 
                   const std::vector<typename Interface::RequestType>& requests) {
        
        // Implementation must consider platform constraints
        const auto platform_info = interface->get_platform_info();
        
        // Adjust batch size based on platform memory constraints
        size_t max_batch_size = calculate_max_batch_size(platform_info);
        
        std::vector<typename Interface::ResourceType> results;
        
        // Process in chunks to respect platform limitations
        for (size_t i = 0; i < requests.size(); i += max_batch_size) {
            auto chunk_end = std::min(i + max_batch_size, requests.size());
            auto chunk_result = process_chunk(interface, requests, i, chunk_end);
            
            if (!chunk_result) {
                // Cleanup on failure
                cleanup_results(interface, results);
                return chunk_result.error();
            }
            
            auto& chunk_data = chunk_result.value();
            results.insert(results.end(), chunk_data.begin(), chunk_data.end());
        }
        
        return results;
    }
    
private:
    static size_t calculate_max_batch_size(const PlatformInfo& info) {
        // Respect platform memory constraints
        if (info.total_memory < 32 * 1024 * 1024) {  // < 32MB (Dreamcast-class)
            return 4;   // Very small batches
        } else if (info.total_memory < 512 * 1024 * 1024) {  // < 512MB (PSP-class)
            return 16;  // Small batches
        } else {
            return 64;  // Large batches for modern systems
        }
    }
};

} // namespace flight::hal
```

---

## Hardware-Capability Migration

### Adding Hardware-Specific Features

Hardware features require capability detection and graceful degradation for platforms that don't support them.

#### Hardware Feature Addition Process

**1. Define New Capability**
```cpp
// Add new capability to HALCapability enum
enum class HALCapability : uint32_t {
    // Existing capabilities...
    StereoRendering     = 1 << 15,  // 3DS, VR headsets
    TouchInput          = 1 << 16,  // 3DS bottom screen, mobile devices
    MotionSensors       = 1 << 17,  // PSP, mobile devices
    HapticFeedback      = 1 << 18,  // Modern controllers
    // ...
};
```

**2. Implement Capability Detection**
```cpp
// Platform-specific capability initialization
void PlatformGraphicsDriver::init_platform_capabilities() {
    capability_mask_ = 0;
    
    // Base graphics capabilities (all platforms)
    capability_mask_ |= static_cast<uint32_t>(HALCapability::BasicGraphics);
    
    // Platform-specific capability detection
    #ifdef PLATFORM_3DS
    capability_mask_ |= static_cast<uint32_t>(HALCapability::StereoRendering);
    if (is_bottom_screen_available()) {
        capability_mask_ |= static_cast<uint32_t>(HALCapability::TouchInput);
    }
    #endif
    
    #ifdef PLATFORM_PSP
    if (detect_motion_sensors()) {
        capability_mask_ |= static_cast<uint32_t>(HALCapability::MotionSensors);
    }
    #endif
    
    #ifdef PLATFORM_MACOS
    // Modern platforms support most capabilities
    capability_mask_ |= static_cast<uint32_t>(HALCapability::StereoRendering);
    capability_mask_ |= static_cast<uint32_t>(HALCapability::HapticFeedback);
    #endif
    
    // Web platform limitations
    #ifdef PLATFORM_WEB
    if (webgl_extension_available("WEBGL_stereo_rendering")) {
        capability_mask_ |= static_cast<uint32_t>(HALCapability::StereoRendering);
    }
    #endif
}
```

**3. Implement Feature with Fallback**
```cpp
// Hardware feature with graceful degradation
class IGraphicsInterface {
public:
    HALResult<StereoRenderTarget> create_stereo_render_target(
        uint32_t width, uint32_t height, StereoMode mode) {
        
        // Check for stereo rendering capability
        if (!supports_capability(HALCapability::StereoRendering)) {
            // Fallback: Create standard render target
            LOG_INFO("Stereo rendering not supported, falling back to mono rendering");
            
            auto mono_target = create_render_target(width, height, PixelFormat::RGBA8);
            if (!mono_target) {
                return mono_target.error();
            }
            
            // Wrap mono target as stereo (both eyes use same target)
            StereoRenderTarget stereo_fallback;
            stereo_fallback.left_eye = mono_target.value();
            stereo_fallback.right_eye = mono_target.value();  // Same target
            stereo_fallback.mode = StereoMode::Mono;
            
            return stereo_fallback;
        }
        
        // Platform-specific stereo implementation
        return create_stereo_render_target_impl(width, height, mode);
    }
    
private:
    virtual HALResult<StereoRenderTarget> create_stereo_render_target_impl(
        uint32_t width, uint32_t height, StereoMode mode) = 0;
};
```

#### Platform-Specific Implementation Examples

**3DS Implementation**
```cpp
namespace flight::hal::nintendo3ds {

class Nintendo3DSGraphicsDriver : public IGraphicsInterface {
    HALResult<StereoRenderTarget> create_stereo_render_target_impl(
        uint32_t width, uint32_t height, StereoMode mode) override {
        
        // 3DS-specific stereo rendering setup
        if (mode == StereoMode::SideBySide) {
            // Use 3DS hardware stereo rendering
            auto left_target = create_3ds_render_target(width, height, GPU_RB_RGBA8, GPU_TEX_2D);
            auto right_target = create_3ds_render_target(width, height, GPU_RB_RGBA8, GPU_TEX_2D);
            
            if (!left_target || !right_target) {
                return HALError::AllocationFailed("Failed to create 3DS stereo targets");
            }
            
            StereoRenderTarget stereo_target;
            stereo_target.left_eye = left_target.value();
            stereo_target.right_eye = right_target.value();
            stereo_target.mode = StereoMode::SideBySide;
            
            return stereo_target;
        }
        
        return HALError::InvalidParameter("Unsupported stereo mode for 3DS");
    }
};

} // namespace flight::hal::nintendo3ds
```

**Dreamcast Fallback Implementation**
```cpp
namespace flight::hal::dreamcast {

class DreamcastGraphicsDriver : public IGraphicsInterface {
    HALResult<StereoRenderTarget> create_stereo_render_target_impl(
        uint32_t width, uint32_t height, StereoMode mode) override {
        
        // Dreamcast doesn't support stereo rendering
        // This should never be called due to capability check
        return HALError::UnsupportedOperation(
            "Stereo rendering not supported on Dreamcast hardware");
    }
    
    bool supports_capability(HALCapability capability) const override {
        // Dreamcast capabilities (no stereo support)
        if (capability == HALCapability::StereoRendering) {
            return false;
        }
        return Base::supports_capability(capability);
    }
};

} // namespace flight::hal::dreamcast
```

---

## Interface Migration Patterns

### Adding Methods to Existing Interfaces

When adding new methods to existing HAL interfaces, follow these patterns to maintain compatibility.

#### Pattern 1: Pure Virtual Method Addition (Major Version)

```cpp
// Before (v1.0.0)
class IMemoryInterface : public IHALInterface, public ICapabilityProvider {
public:
    virtual ~IMemoryInterface() = default;
    
    virtual HALResult<MemoryAllocation> allocate(const AllocationRequest& request) = 0;
    virtual HALResult<void> deallocate(void* ptr) = 0;
    virtual HALResult<MemoryStats> get_memory_stats() const = 0;
};

// After (v2.0.0) - BREAKING CHANGE
class IMemoryInterface : public IHALInterface, public ICapabilityProvider {
public:
    virtual ~IMemoryInterface() = default;
    
    // Existing methods
    virtual HALResult<MemoryAllocation> allocate(const AllocationRequest& request) = 0;
    virtual HALResult<void> deallocate(void* ptr) = 0;
    virtual HALResult<MemoryStats> get_memory_stats() const = 0;
    
    // NEW: Pure virtual method - REQUIRES driver implementation
    virtual HALResult<MemoryAllocation> reallocate(void* ptr, size_t new_size) = 0;
};
```

**Migration Steps for Pure Virtual Addition**:
1. Update all existing drivers to implement new method
2. Update interface compliance tests
3. Provide migration guide for third-party drivers
4. Consider backwards compatibility shims

#### Pattern 2: Default Implementation Method Addition (Minor Version)

```cpp
// After (v1.1.0) - NON-BREAKING
class IMemoryInterface : public IHALInterface, public ICapabilityProvider {
public:
    virtual ~IMemoryInterface() = default;
    
    // Existing methods...
    virtual HALResult<MemoryAllocation> allocate(const AllocationRequest& request) = 0;
    virtual HALResult<void> deallocate(void* ptr) = 0;
    virtual HALResult<MemoryStats> get_memory_stats() const = 0;
    
    // NEW: Virtual method with default implementation - NON-BREAKING
    virtual HALResult<MemoryAllocation> reallocate(void* ptr, size_t new_size) {
        // Default implementation using existing methods
        if (!ptr) {
            AllocationRequest request;
            request.size = new_size;
            return allocate(request);
        }
        
        // Simple reallocation strategy
        AllocationRequest new_request;
        new_request.size = new_size;
        auto new_allocation = allocate(new_request);
        
        if (new_allocation) {
            // Copy data (platform-specific implementation needed for efficiency)
            // This is a basic implementation - drivers should override for optimization
            deallocate(ptr);
        }
        
        return new_allocation;
    }
    
    // NEW: Non-virtual convenience method
    HALResult<std::vector<MemoryAllocation>> allocate_batch(
        const std::vector<AllocationRequest>& requests) {
        
        std::vector<MemoryAllocation> results;
        results.reserve(requests.size());
        
        for (const auto& request : requests) {
            auto result = allocate(request);
            if (!result) {
                // Cleanup on failure
                for (auto& allocation : results) {
                    deallocate(allocation.ptr);
                }
                return result.error();
            }
            results.push_back(result.value());
        }
        
        return results;
    }
};
```

#### Pattern 3: Capability-Based Method Addition

```cpp
// Hardware-specific method with capability check
class IGraphicsInterface : public IHALInterface, public ICapabilityProvider {
public:
    // NEW: Hardware-capability method
    HALResult<ComputeShader> create_compute_shader(const ComputeShaderDesc& desc) {
        if (!supports_capability(HALCapability::ComputeShaders)) {
            return HALError::UnsupportedOperation(
                "Compute shaders not supported on this platform");
        }
        return create_compute_shader_impl(desc);
    }
    
protected:
    // Platform-specific implementation (only called if capability supported)
    virtual HALResult<ComputeShader> create_compute_shader_impl(
        const ComputeShaderDesc& desc) {
        return HALError::UnsupportedOperation("Compute shader implementation not provided");
    }
};
```

### Method Signature Evolution

#### Safe Signature Changes (Non-Breaking)

**Adding Optional Parameters**:
```cpp
// Before
virtual HALResult<Texture> create_texture(uint32_t width, uint32_t height, PixelFormat format) = 0;

// After - NON-BREAKING (default parameter)
virtual HALResult<Texture> create_texture(uint32_t width, uint32_t height, PixelFormat format, 
                                         TextureFlags flags = TextureFlags::None) = 0;
```

**Adding Overloads**:
```cpp
// Existing method unchanged
virtual HALResult<Texture> create_texture(uint32_t width, uint32_t height, PixelFormat format) = 0;

// NEW: Additional overload
virtual HALResult<Texture> create_texture(const TextureDesc& desc) {
    // Default implementation using existing method
    return create_texture(desc.width, desc.height, desc.format);
}
```

#### Breaking Signature Changes (Major Version)

**Parameter Type Changes**:
```cpp
// Before (v1.x.x)
virtual HALResult<Buffer> create_buffer(size_t size, BufferUsage usage) = 0;

// After (v2.0.0) - BREAKING
virtual HALResult<Buffer> create_buffer(const BufferDesc& desc) = 0;
```

**Migration Strategy for Breaking Changes**:
```cpp
// Provide backwards compatibility wrapper during transition
class IGraphicsInterface {
public:
    // NEW: Primary method
    virtual HALResult<Buffer> create_buffer(const BufferDesc& desc) = 0;
    
    // DEPRECATED: Backwards compatibility wrapper
    [[deprecated("Use create_buffer(const BufferDesc&) instead")]]
    HALResult<Buffer> create_buffer(size_t size, BufferUsage usage) {
        BufferDesc desc;
        desc.size = size;
        desc.usage = usage;
        return create_buffer(desc);
    }
};
```

---

## Driver Migration Workflows

### Auto-Registration System Updates

The Flight HAL auto-registration system may evolve to support new features or improve performance.

#### Current Registration Pattern
```cpp
// Current driver registration (v1.x.x)
#include "flight/hal/core/driver_auto_registration.hpp"

namespace flight::hal::platform {

class PlatformMemoryDriver : public IMemoryInterface {
public:
    static constexpr int PRIORITY = 10;
    static constexpr const char* DRIVER_NAME = "Platform Memory Driver";
    // Implementation...
};

} // namespace flight::hal::platform

// Auto-register the driver
REGISTER_HAL_DRIVER(flight::hal::IMemoryInterface, 
                   flight::hal::platform::PlatformMemoryDriver);
```

#### Enhanced Registration (Future v2.0.0)
```cpp
// Enhanced registration with metadata (hypothetical v2.0.0)
#include "flight/hal/core/driver_auto_registration_v2.hpp"

namespace flight::hal::platform {

class PlatformMemoryDriver : public IMemoryInterface {
public:
    // Enhanced driver metadata
    static constexpr DriverMetadata METADATA = {
        .name = "Platform Memory Driver",
        .priority = 10,
        .version = "2.0.0",
        .platform_compatibility = PlatformFlags::All,
        .required_capabilities = HALCapability::BasicMemory,
        .optional_capabilities = HALCapability::DMA | HALCapability::VirtualMemory
    };
    
    // Implementation...
};

} // namespace flight::hal::platform

// Enhanced registration macro
REGISTER_HAL_DRIVER_V2(flight::hal::IMemoryInterface, 
                       flight::hal::platform::PlatformMemoryDriver);
```

#### Migration Steps for Registration System

**1. Preparation Phase (Current Version)**
```cpp
// Ensure current drivers follow best practices
namespace flight::hal::platform {

class PlatformMemoryDriver : public IMemoryInterface {
public:
    // Explicit metadata for migration readiness
    static constexpr int PRIORITY = 10;
    static constexpr const char* DRIVER_NAME = "Platform Memory Driver";
    static constexpr const char* DRIVER_VERSION = "1.2.0";
    
    // Capability information for future migration
    static constexpr uint32_t REQUIRED_CAPABILITIES = 
        static_cast<uint32_t>(HALCapability::BasicMemory);
    static constexpr uint32_t OPTIONAL_CAPABILITIES = 
        static_cast<uint32_t>(HALCapability::DMA) | 
        static_cast<uint32_t>(HALCapability::VirtualMemory);
    
    // Implementation...
};

}
```

**2. Transition Phase (v2.0.0)**
```cpp
// Support both old and new registration systems
#ifdef HAL_REGISTRATION_V2
REGISTER_HAL_DRIVER_V2(flight::hal::IMemoryInterface, 
                       flight::hal::platform::PlatformMemoryDriver);
#else
REGISTER_HAL_DRIVER(flight::hal::IMemoryInterface, 
                   flight::hal::platform::PlatformMemoryDriver);
#endif
```

**3. Migration Complete (v2.1.0)**
```cpp
// Remove legacy registration support
REGISTER_HAL_DRIVER_V2(flight::hal::IMemoryInterface, 
                       flight::hal::platform::PlatformMemoryDriver);
```

### Driver Priority Management

When multiple drivers are available for the same interface, the priority system determines selection order.

#### Priority Migration Scenarios

**Scenario 1: New High-Performance Driver**
```cpp
// Existing driver (priority 10)
class StandardMemoryDriver : public IMemoryInterface {
    static constexpr int PRIORITY = 10;
    // Standard implementation...
};

// NEW: High-performance driver (higher priority)
class OptimizedMemoryDriver : public IMemoryInterface {
    static constexpr int PRIORITY = 15;  // Higher priority
    // Optimized implementation with better performance...
};

// Migration result: OptimizedMemoryDriver will be selected automatically
// StandardMemoryDriver remains as fallback
```

**Scenario 2: Platform-Specific Specialization**
```cpp
// Generic cross-platform driver
class GenericGraphicsDriver : public IGraphicsInterface {
    static constexpr int PRIORITY = 5;
    // Cross-platform implementation...
};

// Platform-specific optimized driver
#ifdef PLATFORM_MACOS
class MetalGraphicsDriver : public IGraphicsInterface {
    static constexpr int PRIORITY = 15;  // Higher priority on macOS
    // Metal-optimized implementation...
};
#endif

// Migration: Platform-specific drivers automatically take precedence
```

### HALResult Evolution

The error handling system may evolve to provide richer context and better debugging information.

#### Current HALResult Usage
```cpp
// Current error handling pattern
HALResult<MemoryAllocation> allocate(const AllocationRequest& request) {
    if (request.size == 0) {
        return HALError::InvalidParameter("Size cannot be zero");
    }
    
    void* ptr = platform_allocate(request.size);
    if (!ptr) {
        return HALError::OutOfMemory("Platform allocation failed");
    }
    
    MemoryAllocation allocation;
    allocation.ptr = ptr;
    allocation.size = request.size;
    return allocation;
}
```

#### Enhanced HALResult (Future)
```cpp
// Enhanced error handling with context (hypothetical)
HALResult<MemoryAllocation> allocate(const AllocationRequest& request) {
    auto context = HALContext::current()
        .with_function("allocate")
        .with_interface("IMemoryInterface")
        .with_driver(get_driver_name());
    
    if (request.size == 0) {
        return HALError::InvalidParameter("Size cannot be zero")
            .with_context(context)
            .with_parameter("size", request.size)
            .with_suggestion("Use size > 0");
    }
    
    void* ptr = platform_allocate(request.size);
    if (!ptr) {
        return HALError::OutOfMemory("Platform allocation failed")
            .with_context(context)
            .with_memory_stats(get_memory_stats())
            .with_suggestion("Try smaller allocation or call gc_hint()");
    }
    
    MemoryAllocation allocation;
    allocation.ptr = ptr;
    allocation.size = request.size;
    return allocation;
}
```

---

## Build System Migration

### CMake Configuration Evolution

The Flight HAL build system may evolve to support new platforms, optimization features, or dependency management improvements.

#### Current CMake Structure
```cmake
# Current CMakeLists.txt structure
cmake_minimum_required(VERSION 3.20)
project(FlightHALInterfaces VERSION 1.0.0)

# Platform detection
include(cmake/PlatformDetection.cmake)

# Compiler configuration
include(cmake/CompilerConfiguration.cmake)

# Dependencies
include(cmake/Dependencies.cmake)

# HAL interface libraries
add_subdirectory(src/core)
add_subdirectory(src/interfaces)
add_subdirectory(src/coordination)
add_subdirectory(src/validation)
```

#### Migration Example: Buck2 Integration
```cmake
# Enhanced CMakeLists.txt with Buck2 compatibility
cmake_minimum_required(VERSION 3.20)
project(FlightHALInterfaces VERSION 2.0.0)

# Build system selection
option(USE_BUCK2 "Use Buck2 build system" OFF)

if(USE_BUCK2)
    include(cmake/Buck2Integration.cmake)
    configure_buck2_targets()
else()
    # Traditional CMake build
    include(cmake/TraditionalBuild.cmake)
endif()

# Platform detection (enhanced)
include(cmake/PlatformDetection.cmake)
detect_platform_capabilities()

# Conditional feature configuration
include(cmake/FeatureConfiguration.cmake)
configure_hal_features()
```

#### Migration Steps for Build System

**1. Preparation**
```bash
# Backup current build configuration
cp CMakeLists.txt CMakeLists.txt.backup
cp -r cmake/ cmake_backup/

# Test current build on all platforms
mkdir build_test
cd build_test
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
make test
```

**2. Migration**
```bash
# Update to new build system
git checkout feature/build_system_v2
mkdir build_v2
cd build_v2

# Configure with new options
cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_ENHANCED_FEATURES=ON
make -j$(nproc)

# Validate migration
make test
make hal_compliance_tests
```

**3. Validation**
```bash
# Compare build outputs
diff -r build_test/lib/ build_v2/lib/
diff -r build_test/bin/ build_v2/bin/

# Performance comparison
./build_test/benchmarks/hal_benchmark > old_results.txt
./build_v2/benchmarks/hal_benchmark > new_results.txt
compare_benchmark_results.py old_results.txt new_results.txt
```

### Dependency Management Evolution

#### Current Dependency Pattern
```cmake
# Current dependency management
include(FetchContent)

FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 10.2.1
)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.12.0
)

FetchContent_MakeAvailable(fmt spdlog)
```

#### Enhanced Dependency Management
```cmake
# Enhanced dependency management with version constraints
include(cmake/DependencyManager.cmake)

# Define version constraints and compatibility
define_dependency_constraints(
    fmt VERSION ">=10.2.0,<11.0.0"
    spdlog VERSION ">=1.12.0,<2.0.0"
    gtest VERSION ">=1.14.0,<2.0.0"
)

# Platform-specific dependency handling
if(PLATFORM_DREAMCAST)
    # Use lightweight alternatives for constrained platforms
    use_lightweight_dependencies()
elseif(PLATFORM_WEB)
    # Use Emscripten-compatible versions
    use_emscripten_dependencies()
else()
    # Use full-featured dependencies
    use_standard_dependencies()
endif()

# Automatic dependency resolution
resolve_hal_dependencies()
```

---

## Platform-Specific Considerations

### Memory-Constrained Platforms (Dreamcast, Early PSP)

**Platform Characteristics**:
- Dreamcast: 16MB main RAM, 8MB VRAM
- Early PSP: 32MB total, dual-core architecture
- Strict memory allocation patterns required

#### Migration Considerations for Constrained Platforms

**Memory Usage Optimization**:
```cpp
// Platform-specific memory allocation strategies
namespace flight::hal::migration {

class ConstrainedPlatformMigration {
public:
    static bool validate_memory_requirements(const MigrationPlan& plan) {
        auto platform_info = get_current_platform_info();
        
        if (platform_info.total_memory < 64 * 1024 * 1024) {  // < 64MB
            // Check if migration respects memory constraints
            auto estimated_usage = calculate_migration_memory_usage(plan);
            
            if (estimated_usage > platform_info.total_memory * 0.8) {
                LOG_ERROR("Migration would exceed safe memory limits for constrained platform");
                return false;
            }
            
            // Validate that new features use memory-efficient implementations
            for (const auto& feature : plan.new_features) {
                if (!feature.has_constrained_implementation) {
                    LOG_WARNING("Feature {} lacks constrained platform implementation", 
                               feature.name);
                }
            }
        }
        
        return true;
    }
    
    static MigrationStrategy select_strategy_for_platform() {
        auto platform_info = get_current_platform_info();
        
        if (platform_info.total_memory < 32 * 1024 * 1024) {
            return MigrationStrategy::MinimalFeatures;
        } else if (platform_info.total_memory < 128 * 1024 * 1024) {
            return MigrationStrategy::LimitedFeatures;
        } else {
            return MigrationStrategy::FullFeatures;
        }
    }
};

} // namespace flight::hal::migration
```

**Feature Subset Migration**:
```cpp
// Constrained platform feature selection
enum class PlatformFeatureLevel {
    Minimal,    // Dreamcast, basic embedded
    Limited,    // PSP, advanced embedded
    Standard,   // Modern mobile, low-end PC
    Full        // Modern PC, unlimited resources
};

class PlatformMigrationManager {
public:
    static std::vector<HALCapability> get_supported_capabilities_for_level(
        PlatformFeatureLevel level) {
        
        std::vector<HALCapability> capabilities;
        
        // Base capabilities (all platforms)
        capabilities.push_back(HALCapability::BasicMemory);
        capabilities.push_back(HALCapability::BasicGraphics);
        
        if (level >= PlatformFeatureLevel::Limited) {
            capabilities.push_back(HALCapability::Threading);
            capabilities.push_back(HALCapability::Audio);
        }
        
        if (level >= PlatformFeatureLevel::Standard) {
            capabilities.push_back(HALCapability::Hardware3D);
            capabilities.push_back(HALCapability::Network);
        }
        
        if (level >= PlatformFeatureLevel::Full) {
            capabilities.push_back(HALCapability::ComputeShaders);
            capabilities.push_back(HALCapability::VirtualMemory);
            capabilities.push_back(HALCapability::AdvancedAudio);
        }
        
        return capabilities;
    }
};
```

### High-Performance Platforms (Modern PC, Consoles)

**Platform Characteristics**:
- Unlimited memory and processing power
- Advanced graphics APIs (Metal, DirectX 12, Vulkan)
- Multi-core architectures with complex coordination

#### Performance-Focused Migration

**Advanced Feature Adoption**:
```cpp
// High-performance platform migration optimizations
namespace flight::hal::highperf {

class PerformanceMigrationOptimizer {
public:
    static void optimize_for_platform(const PlatformInfo& info) {
        if (info.performance_tier == PerformanceTier::High) {
            // Enable advanced optimizations
            enable_parallel_driver_loading();
            enable_async_resource_creation();
            enable_multi_threaded_validation();
            
            // Use advanced memory management
            configure_large_page_support();
            enable_memory_prefetching();
        }
    }
    
    static void migrate_to_advanced_apis() {
        // Migrate from legacy APIs to modern equivalents
        auto graphics = get_interface<IGraphicsInterface>();
        
        if (graphics->supports_capability(HALCapability::ModernGraphicsAPI)) {
            // Use advanced rendering pipeline
            enable_bindless_resources();
            enable_gpu_driven_rendering();
            enable_mesh_shaders();
        }
    }
};

} // namespace flight::hal::highperf
```

### Web Platform Considerations

**Platform Characteristics**:
- Browser security restrictions
- WebAssembly limitations
- Asynchronous operation requirements

#### Web-Specific Migration Patterns

**Security Compliance Migration**:
```cpp
// Web platform security considerations
namespace flight::hal::web {

class WebSecurityMigration {
public:
    static bool validate_web_compatibility(const MigrationPlan& plan) {
        for (const auto& feature : plan.new_features) {
            // Check for web-incompatible features
            if (feature.requires_synchronous_file_access ||
                feature.requires_raw_memory_access ||
                feature.requires_native_threads) {
                
                LOG_ERROR("Feature {} not compatible with web platform restrictions", 
                         feature.name);
                return false;
            }
        }
        
        return true;
    }
    
    static void apply_web_specific_migration() {
        // Convert synchronous operations to async
        convert_file_operations_to_async();
        implement_web_worker_threading();
        setup_cors_compliant_networking();
    }
};

} // namespace flight::hal::web
```

---

## Validation and Testing

### Migration Validation Framework

A comprehensive testing strategy ensures migration success across all platforms.

#### Pre-Migration Validation

```cpp
// Pre-migration system validation
namespace flight::hal::validation {

class MigrationValidator {
public:
    struct ValidationResult {
        bool is_valid;
        std::vector<std::string> warnings;
        std::vector<std::string> errors;
        std::optional<MigrationPlan> recommended_plan;
    };
    
    static ValidationResult validate_migration_readiness(
        const std::string& target_version) {
        
        ValidationResult result;
        result.is_valid = true;
        
        // 1. Validate current system state
        auto current_drivers = enumerate_active_drivers();
        for (const auto& driver : current_drivers) {
            if (!driver->is_migration_ready()) {
                result.errors.push_back(
                    fmt::format("Driver {} not ready for migration", 
                               driver->get_driver_name()));
                result.is_valid = false;
            }
        }
        
        // 2. Validate platform compatibility
        auto platform_info = get_current_platform_info();
        if (!is_version_supported_on_platform(target_version, platform_info)) {
            result.errors.push_back(
                fmt::format("Version {} not supported on platform {}", 
                           target_version, platform_info.platform_name));
            result.is_valid = false;
        }
        
        // 3. Validate dependency compatibility
        auto dep_check = validate_dependency_compatibility(target_version);
        if (!dep_check.is_compatible) {
            result.warnings.insert(result.warnings.end(), 
                                  dep_check.warnings.begin(), 
                                  dep_check.warnings.end());
        }
        
        // 4. Generate migration plan
        if (result.is_valid) {
            result.recommended_plan = generate_migration_plan(target_version);
        }
        
        return result;
    }
};

} // namespace flight::hal::validation
```

#### Platform-Specific Test Execution

```bash
#!/bin/bash
# Migration validation test suite

# Function to run platform-specific tests
run_platform_tests() {
    local platform=$1
    local test_config="tests/migration/platform_configs/${platform}.json"
    
    echo "Running migration tests for ${platform}..."
    
    # Pre-migration baseline
    ./tools/migration_helper/capture_baseline \
        --platform=${platform} \
        --output=baselines/${platform}_pre_migration.json
    
    # Run platform-specific compliance tests
    ./tests/compliance/hal_compliance_tests \
        --platform=${platform} \
        --config=${test_config} \
        --migration-mode
    
    # Performance regression tests
    ./benchmarks/migration_benchmarks \
        --platform=${platform} \
        --baseline=baselines/${platform}_pre_migration.json
}

# Test all supported platforms
PLATFORMS=("dreamcast" "psp" "macos" "web" "linux" "windows")

for platform in "${PLATFORMS[@]}"; do
    if is_platform_available "${platform}"; then
        run_platform_tests "${platform}"
    else
        echo "Skipping ${platform} - not available in current environment"
    fi
done

# Cross-platform compatibility tests
echo "Running cross-platform compatibility tests..."
./tests/migration/cross_platform_compatibility_test

# Generate migration report
./tools/migration_helper/generate_migration_report \
    --baselines=baselines/ \
    --output=migration_validation_report.html
```

#### Migration Testing Checklist

```markdown
## Migration Testing Checklist

### Pre-Migration (Required ✅)
- [ ] All drivers report migration readiness
- [ ] Platform compatibility verified
- [ ] Dependency compatibility checked
- [ ] Performance baseline captured
- [ ] Backup strategy confirmed

### During Migration (Required ✅)
- [ ] Interface compliance maintained
- [ ] Platform-specific features operational
- [ ] Memory usage within constraints
- [ ] Performance regressions identified
- [ ] Error handling integrity verified

### Post-Migration (Required ✅)
- [ ] All interfaces functional
- [ ] Platform-specific optimizations active
- [ ] Performance meets or exceeds baseline
- [ ] No memory leaks detected
- [ ] Driver coordination operational

### Platform-Specific (Per Platform)
#### Dreamcast ✅
- [ ] Memory usage < 12MB (safe margin)
- [ ] PowerVR2 optimizations functional
- [ ] KallistiOS integration stable

#### PSP ✅
- [ ] Dual-core coordination operational
- [ ] Media Engine utilization optimal
- [ ] PSPSDK compatibility maintained

#### Modern PC ✅
- [ ] Advanced API features enabled
- [ ] Multi-threading performance optimal
- [ ] Memory management efficient

#### Web ✅
- [ ] Browser security compliance
- [ ] WebAssembly performance acceptable
- [ ] Async operation patterns functional
```

---

## Deprecation Management

### Deprecation Timeline and Communication

The Flight HAL project uses a structured deprecation process to provide sufficient time for migration while maintaining system evolution.

#### Deprecation Phases

**Phase 1: Deprecation Announcement (Version N)**
```cpp
// Mark features as deprecated with clear migration path
class IMemoryInterface {
public:
    // NEW: Preferred method
    virtual HALResult<MemoryAllocation> allocate(const AllocationRequest& request) = 0;
    
    // DEPRECATED: Legacy method with migration guidance
    [[deprecated("Use allocate(const AllocationRequest&) instead. "
                "This method will be removed in version 2.0.0. "
                "See docs/migration_guide.md#memory-interface-migration")]]
    HALResult<MemoryAllocation> allocate_legacy(size_t size, MemoryAlignment alignment) {
        AllocationRequest request;
        request.size = size;
        request.alignment = alignment;
        return allocate(request);
    }
};
```

**Phase 2: Deprecation Warning (Version N+1)**
```cpp
// Enhanced deprecation warnings with migration assistance
class IMemoryInterface {
public:
    virtual HALResult<MemoryAllocation> allocate(const AllocationRequest& request) = 0;
    
    [[deprecated("URGENT: Use allocate(const AllocationRequest&) instead. "
                "This method will be REMOVED in the next major version. "
                "Auto-migration tool available: tools/migration_helper/migrate_memory_calls")]]
    HALResult<MemoryAllocation> allocate_legacy(size_t size, MemoryAlignment alignment) {
        // Log deprecation usage for tracking
        LOG_DEPRECATED("allocate_legacy", "allocate", "2.0.0");
        
        AllocationRequest request;
        request.size = size;
        request.alignment = alignment;
        return allocate(request);
    }
};
```

**Phase 3: Removal (Version N+2)**
```cpp
// Deprecated method removed, compile-time error provides migration guidance
class IMemoryInterface {
public:
    virtual HALResult<MemoryAllocation> allocate(const AllocationRequest& request) = 0;
    
    // Deprecated method removed - compiler error guides migration
    template<typename... Args>
    [[deprecated("allocate_legacy has been removed. Use allocate(const AllocationRequest&). "
                "Run: tools/migration_helper/migrate_memory_calls --auto-fix")]]
    void allocate_legacy(Args&&...) = delete;
};
```

#### Deprecation Communication Strategy

**1. Documentation Updates**
- Update all relevant documentation with migration paths
- Provide code examples for recommended alternatives
- Include timeline for deprecation phases

**2. Migration Tools**
```cpp
// Automated migration tool for deprecated API usage
namespace flight::hal::migration::tools {

class DeprecatedAPIAnalyzer {
public:
    struct DeprecationUsage {
        std::string file_path;
        int line_number;
        std::string deprecated_call;
        std::string recommended_replacement;
        std::string automatic_fix;
    };
    
    static std::vector<DeprecationUsage> scan_codebase(const std::string& path) {
        std::vector<DeprecationUsage> usages;
        
        // Scan for deprecated API usage patterns
        scan_for_pattern(path, R"(\.allocate_legacy\s*\()", [&](const auto& match) {
            DeprecationUsage usage;
            usage.file_path = match.file_path;
            usage.line_number = match.line_number;
            usage.deprecated_call = match.text;
            usage.recommended_replacement = "allocate(AllocationRequest{...})";
            usage.automatic_fix = generate_automatic_fix(match);
            usages.push_back(usage);
        });
        
        return usages;
    }
    
    static bool apply_automatic_fixes(const std::vector<DeprecationUsage>& usages) {
        for (const auto& usage : usages) {
            if (!usage.automatic_fix.empty()) {
                apply_code_fix(usage.file_path, usage.line_number, usage.automatic_fix);
            }
        }
        return true;
    }
};

} // namespace flight::hal::migration::tools
```

**3. Community Communication**
```markdown
# Deprecation Notice: Memory Interface Legacy Methods

## Summary
The legacy memory allocation methods in IMemoryInterface are being deprecated 
in favor of a more flexible request-based API.

## Timeline
- **v1.5.0** (Current): Deprecation warning added
- **v1.6.0** (Next release): Enhanced warnings and migration tools
- **v2.0.0** (Major release): Methods removed

## Migration Path
### Before (Deprecated)
```cpp
auto result = memory->allocate_legacy(1024, MemoryAlignment::Default);
```

### After (Recommended)
```cpp
AllocationRequest request;
request.size = 1024;
request.alignment = MemoryAlignment::Default;
auto result = memory->allocate(request);
```

## Automated Migration
Run the migration tool to automatically update your code:
```bash
./tools/migration_helper/migrate_memory_calls --scan ./src
./tools/migration_helper/migrate_memory_calls --auto-fix ./src
```

## Support
- Migration guide: [docs/migration_guide.md](docs/migration_guide.md)
- Forum discussion: [community.flight-engine.org/deprecations](https://community.flight-engine.org/deprecations)
- Direct support: migration-support@flight-engine.org
```

---

## Troubleshooting Common Issues

### Compilation Errors After Migration

#### Missing Method Implementations
**Problem**: Pure virtual method added to interface, existing drivers don't compile.

**Solution**:
```cpp
// Temporary compatibility shim during transition period
#ifdef FLIGHT_HAL_MIGRATION_COMPATIBILITY

// Provide default implementation during migration
class IMemoryInterface {
public:
    // NEW: Method that would normally be pure virtual
    virtual HALResult<MemoryInfo> get_allocation_info(void* ptr) const {
        // Default implementation for migration compatibility
        return HALError::UnsupportedOperation(
            "get_allocation_info not implemented in legacy driver. "
            "Please update driver to implement this method.");
    }
};

#endif // FLIGHT_HAL_MIGRATION_COMPATIBILITY
```

#### Driver Registration Failures
**Problem**: Driver registration macros changed between versions.

**Solution**:
```cpp
// Support both old and new registration patterns
#if FLIGHT_HAL_VERSION >= 200  // Version 2.0.0+
REGISTER_HAL_DRIVER_V2(IMemoryInterface, PlatformMemoryDriver);
#else
REGISTER_HAL_DRIVER(IMemoryInterface, PlatformMemoryDriver);
#endif
```

### Runtime Issues After Migration

#### Capability Detection Failures
**Problem**: New capabilities not properly detected on existing platforms.

**Diagnosis**:
```cpp
// Debug capability detection issues
void debug_capability_detection() {
    auto drivers = get_all_drivers<IMemoryInterface>();
    
    for (const auto& driver : drivers) {
        LOG_INFO("Driver: {}", driver->get_driver_name());
        LOG_INFO("Capability mask: 0x{:08X}", driver->get_capability_mask());
        
        // Test specific capabilities
        std::vector<HALCapability> test_caps = {
            HALCapability::DMA,
            HALCapability::VirtualMemory,
            HALCapability::ExecutableMemory
        };
        
        for (auto cap : test_caps) {
            bool supported = driver->supports_capability(cap);
            LOG_INFO("  {}: {}", capability_to_string(cap), supported ? "YES" : "NO");
        }
    }
}
```

**Solution**:
```cpp
// Update platform capability detection
void fix_capability_detection() {
    // Ensure platform detection logic is updated
    auto platform_info = detect_platform_capabilities();
    
    // Verify capability cache is refreshed
    clear_capability_cache();
    refresh_platform_capabilities();
    
    // Re-initialize drivers with updated capabilities
    reinitialize_hal_drivers();
}
```

#### Performance Regressions
**Problem**: Significant performance decrease after migration.

**Diagnosis**:
```bash
# Performance regression analysis
./benchmarks/regression_detector \
    --baseline=baselines/pre_migration.json \
    --current=current_performance.json \
    --threshold=5.0  # 5% regression threshold

# Detailed profiling
./tools/performance_profiler/hal_profiler \
    --interface=IMemoryInterface \
    --operations=allocate,deallocate \
    --duration=60s \
    --output=performance_analysis.json
```

**Solution**:
```cpp
// Optimize performance-critical paths
namespace flight::hal::optimization {

class MigrationPerformanceOptimizer {
public:
    static void optimize_memory_interface() {
        // Enable fast-path optimizations
        enable_allocation_caching();
        optimize_capability_checks();
        enable_batch_operations();
    }
    
    static void enable_allocation_caching() {
        // Cache frequently used allocation sizes
        constexpr std::array<size_t, 8> common_sizes = {
            16, 32, 64, 128, 256, 512, 1024, 4096
        };
        
        for (size_t size : common_sizes) {
            pre_allocate_cache_pool(size, 16);  // 16 cached allocations
        }
    }
};

} // namespace flight::hal::optimization
```

### Platform-Specific Issues

#### Dreamcast Memory Constraints
**Problem**: New features cause memory exhaustion on Dreamcast.

**Solution**:
```cpp
// Dreamcast-specific migration adaptations
#ifdef PLATFORM_DREAMCAST

class DreamcastMigrationAdapter {
public:
    static void apply_memory_optimizations() {
        // Reduce feature set for Dreamcast
        disable_non_essential_features();
        
        // Use memory-efficient implementations
        enable_compact_data_structures();
        reduce_debug_overhead();
        
        // Implement aggressive garbage collection
        configure_aggressive_gc();
    }
    
private:
    static void disable_non_essential_features() {
        // Disable memory-intensive features
        disable_capability(HALCapability::ExtensiveLogging);
        disable_capability(HALCapability::DetailedProfiling);
        disable_capability(HALCapability::LargeBufferSupport);
    }
};

#endif // PLATFORM_DREAMCAST
```

#### Web Platform Security Issues
**Problem**: New features violate browser security policies.

**Solution**:
```cpp
// Web-specific security compliance
#ifdef PLATFORM_WEB

class WebSecurityAdapter {
public:
    static void ensure_security_compliance() {
        // Disable features that violate browser security
        disable_raw_memory_access();
        enable_cors_safe_networking();
        use_secure_file_access_patterns();
    }
    
    static void adapt_async_patterns() {
        // Convert blocking operations to async
        convert_file_ops_to_async();
        implement_web_worker_threading();
        setup_message_passing_coordination();
    }
};

#endif // PLATFORM_WEB
```

---

## Migration Examples

### Complete Interface Migration Example

This section provides a real-world example of migrating the IGraphicsInterface to add stereo rendering support.

#### Before Migration (v1.0.0)
```cpp
// Original IGraphicsInterface
class IGraphicsInterface : public IHALInterface, public ICapabilityProvider {
public:
    virtual ~IGraphicsInterface() = default;
    
    // Basic graphics operations
    virtual HALResult<RenderTarget> create_render_target(
        uint32_t width, uint32_t height, PixelFormat format) = 0;
    virtual HALResult<Texture> create_texture(
        uint32_t width, uint32_t height, PixelFormat format) = 0;
    virtual HALResult<void> clear_render_target(
        RenderTarget target, const ClearColor& color) = 0;
};

// Simple platform implementation
class DreamcastGraphicsDriver : public IGraphicsInterface {
public:
    HALResult<RenderTarget> create_render_target(
        uint32_t width, uint32_t height, PixelFormat format) override {
        
        // Dreamcast PowerVR2 implementation
        if (width > 640 || height > 480) {
            return HALError::InvalidParameter("Dreamcast max resolution: 640x480");
        }
        
        auto pvr_target = pvr_create_render_target(width, height, format);
        if (!pvr_target) {
            return HALError::AllocationFailed("PVR render target creation failed");
        }
        
        RenderTarget target;
        target.width = width;
        target.height = height;
        target.format = format;
        target.native_handle = pvr_target;
        
        return target;
    }
    
    // Other method implementations...
};
```

#### Migration Process (v1.1.0 - Add Stereo Support)

**Step 1: Add New Capability**
```cpp
// Updated HALCapability enum
enum class HALCapability : uint32_t {
    // Existing capabilities...
    BasicGraphics       = 1 << 0,
    Hardware3D          = 1 << 1,
    
    // NEW: Stereo rendering capability
    StereoRendering     = 1 << 15,
};
```

**Step 2: Extend Interface (Non-Breaking)**
```cpp
// Enhanced IGraphicsInterface (v1.1.0)
class IGraphicsInterface : public IHALInterface, public ICapabilityProvider {
public:
    virtual ~IGraphicsInterface() = default;
    
    // Existing methods (unchanged)
    virtual HALResult<RenderTarget> create_render_target(
        uint32_t width, uint32_t height, PixelFormat format) = 0;
    virtual HALResult<Texture> create_texture(
        uint32_t width, uint32_t height, PixelFormat format) = 0;
    virtual HALResult<void> clear_render_target(
        RenderTarget target, const ClearColor& color) = 0;
    
    // NEW: Stereo rendering support with graceful fallback
    virtual HALResult<StereoRenderTarget> create_stereo_render_target(
        uint32_t width, uint32_t height, PixelFormat format, StereoMode mode) {
        
        if (!supports_capability(HALCapability::StereoRendering)) {
            // Graceful fallback: create mono target for both eyes
            auto mono_target = create_render_target(width, height, format);
            if (!mono_target) {
                return mono_target.error();
            }
            
            StereoRenderTarget stereo_target;
            stereo_target.left_eye = mono_target.value();
            stereo_target.right_eye = mono_target.value();  // Same target
            stereo_target.mode = StereoMode::Mono;
            
            LOG_INFO("Stereo rendering not supported, using mono fallback");
            return stereo_target;
        }
        
        return create_stereo_render_target_impl(width, height, format, mode);
    }
    
protected:
    // Platform-specific implementation (only override if supported)
    virtual HALResult<StereoRenderTarget> create_stereo_render_target_impl(
        uint32_t width, uint32_t height, PixelFormat format, StereoMode mode) {
        return HALError::UnsupportedOperation("Stereo rendering implementation not provided");
    }
};
```

**Step 3: Update Platform Implementations**

**Dreamcast (No Stereo Support)**
```cpp
// Dreamcast driver - no changes needed!
class DreamcastGraphicsDriver : public IGraphicsInterface {
public:
    // Existing implementation unchanged
    HALResult<RenderTarget> create_render_target(
        uint32_t width, uint32_t height, PixelFormat format) override {
        // Same implementation as before
        // ...
    }
    
    // Capability detection (no stereo support)
    bool supports_capability(HALCapability capability) const override {
        switch (capability) {
            case HALCapability::BasicGraphics:
                return true;
            case HALCapability::StereoRendering:
                return false;  // Dreamcast doesn't support stereo
            default:
                return false;
        }
    }
    
    // No need to implement create_stereo_render_target_impl
    // Default fallback implementation will be used automatically
};
```

**3DS (With Stereo Support)**
```cpp
// 3DS driver - implements stereo rendering
class Nintendo3DSGraphicsDriver : public IGraphicsInterface {
public:
    // Existing methods...
    HALResult<RenderTarget> create_render_target(
        uint32_t width, uint32_t height, PixelFormat format) override {
        // 3DS implementation
        // ...
    }
    
    // Capability detection (stereo supported)
    bool supports_capability(HALCapability capability) const override {
        switch (capability) {
            case HALCapability::BasicGraphics:
            case HALCapability::StereoRendering:
                return true;
            default:
                return false;
        }
    }
    
protected:
    // 3DS-specific stereo implementation
    HALResult<StereoRenderTarget> create_stereo_render_target_impl(
        uint32_t width, uint32_t height, PixelFormat format, StereoMode mode) override {
        
        if (mode != StereoMode::SideBySide) {
            return HALError::InvalidParameter("3DS only supports side-by-side stereo");
        }
        
        // Create separate targets for each eye
        auto left_target = create_3ds_render_target(width, height, format, GPU_EYE_LEFT);
        auto right_target = create_3ds_render_target(width, height, format, GPU_EYE_RIGHT);
        
        if (!left_target || !right_target) {
            return HALError::AllocationFailed("Failed to create 3DS stereo targets");
        }
        
        StereoRenderTarget stereo_target;
        stereo_target.left_eye = left_target.value();
        stereo_target.right_eye = right_target.value();
        stereo_target.mode = StereoMode::SideBySide;
        
        return stereo_target;
    }
};
```

#### After Migration (v1.1.0) - Usage Examples

**Application Code (Works on All Platforms)**
```cpp
// Application using the enhanced interface
void render_stereo_scene(IGraphicsInterface* graphics) {
    // This code works on ALL platforms - stereo where supported, mono fallback elsewhere
    auto stereo_target = graphics->create_stereo_render_target(
        800, 600, PixelFormat::RGBA8, StereoMode::SideBySide);
    
    if (stereo_target) {
        // Render to stereo target (or mono fallback)
        render_eye_view(stereo_target.value().left_eye, EyeType::Left);
        render_eye_view(stereo_target.value().right_eye, EyeType::Right);
        
        // Log what actually happened
        if (stereo_target.value().mode == StereoMode::Mono) {
            LOG_INFO("Rendered in mono mode (platform fallback)");
        } else {
            LOG_INFO("Rendered in true stereo mode");
        }
    } else {
        LOG_ERROR("Failed to create stereo render target: {}", stereo_target.error_message());
    }
}

// Platform capability checking in applications
void check_platform_capabilities(IGraphicsInterface* graphics) {
    std::vector<std::pair<HALCapability, std::string>> capabilities_to_check = {
        {HALCapability::StereoRendering, "Stereo Rendering"},
        {HALCapability::Hardware3D, "Hardware 3D"},
        {HALCapability::ComputeShaders, "Compute Shaders"}
    };
    
    LOG_INFO("Platform capabilities for {}:", graphics->get_driver_name());
    for (const auto& [capability, name] : capabilities_to_check) {
        bool supported = graphics->supports_capability(capability);
        LOG_INFO("  {}: {}", name, supported ? "✅ Supported" : "❌ Not Supported");
    }
}
```

#### Migration Results Summary

**✅ Successful Migration Achievements**:
- **Zero Breaking Changes**: Existing Dreamcast code continues to work unchanged
- **Graceful Degradation**: Stereo applications work on non-stereo platforms
- **Platform Optimization**: 3DS gets native stereo rendering, Dreamcast gets optimized fallback
- **Future Extensibility**: Framework established for additional graphics capabilities

**📊 Platform Compatibility Matrix After Migration**:

| Platform | Stereo Support | Implementation | Performance Impact |
|----------|----------------|----------------|-------------------|
| Dreamcast | ❌ No | Mono fallback | None (optimized fallback) |
| PSP | ❌ No | Mono fallback | None (optimized fallback) |
| 3DS | ✅ Yes | Native hardware | Optimal (hardware stereo) |
| Modern PC | ✅ Yes | API-dependent | Optimal (GPU accelerated) |
| Web | ⚠️ Limited | WebGL extension | Variable (browser dependent) |

### Build System Migration Example

#### Current Build Configuration
```cmake
# Before migration (v1.0.0)
cmake_minimum_required(VERSION 3.20)
project(FlightHALInterfaces VERSION 1.0.0)

# Simple configuration
set(CMAKE_CXX_STANDARD 20)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)

# Single library target
add_library(flight_hal_interfaces
    src/core/hal_capabilities.cpp
    src/core/memory.cpp
    src/core/graphics.cpp
    # ... other sources
)

target_link_libraries(flight_hal_interfaces
    PUBLIC fmt::fmt spdlog::spdlog
)
```

#### Enhanced Build Configuration (v1.1.0)
```cmake
# After migration (v1.1.0) - Enhanced but compatible
cmake_minimum_required(VERSION 3.20)
project(FlightHALInterfaces VERSION 1.1.0)

# Enhanced platform detection
include(cmake/PlatformDetection.cmake)
detect_target_platform()

# Conditional feature configuration
include(cmake/FeatureConfiguration.cmake)
configure_platform_features()

# Modular library structure
add_library(flight_hal_core
    src/core/hal_capabilities.cpp
    src/core/platform_detection.cpp
    src/core/driver_registry.cpp
)

add_library(flight_hal_interfaces
    src/core/memory.cpp
    src/core/graphics.cpp
    src/core/audio.cpp
    # ... other interface implementations
)

# Platform-specific optimizations
if(PLATFORM_DREAMCAST)
    target_compile_definitions(flight_hal_interfaces 
        PRIVATE PLATFORM_DREAMCAST=1)
    target_compile_options(flight_hal_interfaces 
        PRIVATE -Os)  # Optimize for size
elseif(PLATFORM_3DS)
    target_compile_definitions(flight_hal_interfaces 
        PRIVATE PLATFORM_3DS=1 ENABLE_STEREO_RENDERING=1)
endif()

# Backwards compatibility target
add_library(flight_hal_interfaces_compat ALIAS flight_hal_interfaces)
```

#### Migration Command Line
```bash
# Migration procedure
cd flight-hal-interfaces
git checkout v1.0.0

# Create migration branch
git checkout -b migration-to-v1.1.0

# Update build system
git merge origin/v1.1.0-build-system

# Test compilation on all platforms
mkdir build-migration
cd build-migration

# Test Dreamcast build
cmake .. -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/Dreamcast.cmake
make -j4
make test

# Test 3DS build  
cmake .. -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/3DS.cmake
make -j4
make test

# Test modern PC build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
make test

# Validate migration success
echo "Migration to v1.1.0 complete!"
```

---

## 🎯 Migration Success Criteria

### Technical Validation ✅

**Interface Compliance**
- [ ] All existing interfaces maintain backward compatibility
- [ ] New methods provide appropriate fallback implementations
- [ ] Capability detection works correctly across all platforms
- [ ] Error handling remains consistent and informative

**Platform Support**
- [ ] **Dreamcast**: Memory usage within 16MB limit, PowerVR2 optimizations active
- [ ] **PSP**: Dual-core coordination functional, Media Engine utilization optimal  
- [ ] **3DS**: Stereo rendering operational, touch input responsive
- [ ] **Modern PC**: Advanced features enabled, multi-threading performant
- [ ] **Web**: Browser security compliance, WebAssembly performance acceptable

**Performance Requirements**
- [ ] Zero performance regression on existing functionality
- [ ] New features add <5% overhead when unused
- [ ] Platform-specific optimizations demonstrate measurable improvements
- [ ] Memory usage remains within platform constraints

### Migration Process ✅

**Documentation Completeness**
- [ ] All breaking changes documented with migration paths
- [ ] Code examples provided for common migration scenarios
- [ ] Platform-specific considerations clearly outlined
- [ ] Troubleshooting guides address common issues

**Developer Experience**
- [ ] Clear deprecation warnings with actionable guidance
- [ ] Automated migration tools available where applicable
- [ ] Comprehensive testing framework validates migrations
- [ ] Community communication plan executed

**Quality Assurance**
- [ ] Full test suite passes on all supported platforms
- [ ] Performance benchmarks meet or exceed baselines
- [ ] Memory leak detection shows no regressions
- [ ] Stress testing validates system stability

---

## 📚 Additional Resources

### Related Documentation

**Core Architecture**
- [Flight HAL Architecture Guide](docs/architecture/flight_hal_architecture_guide.md) - Strategic design decisions and patterns
- [Enhanced HAL Architecture](docs/architecture/enhanced_hal_architecture.md) - Technical implementation details
- [Platform Detection System](docs/platform_detection_system.md) - Platform identification and capability detection

**Interface Specifications**
- [Memory Interface Design](docs/memory_interface_design.md) - Memory management patterns and constraints
- [Graphics Interface Design](docs/graphics_interface_design.md) - Graphics abstraction and platform optimizations
- [Audio Interface Design](docs/audio_interface_design.md) - Audio system architecture and capabilities

**Testing and Validation**
- [Interface Compliance Testing](docs/interface_compliance_testing_framework.md) - Automated validation procedures
- [Performance Benchmarking](docs/performance_benchmarking_system.md) - Performance measurement and regression detection
- [Stress Testing System](docs/stress_testing_system.md) - System stability and resource exhaustion testing

**Integration and Development**
- [Integration Guidelines](docs/integration_guidelines.md) - Platform team implementation guidance
- [HAL Validation Tools](docs/hal_validation_tools_guide.md) - Development and debugging tools
- [Buck2 Migration Strategy](docs/buck2_migration_strategy.md) - Build system transition planning

### Migration Tools and Scripts

**Automated Migration Helpers**
```bash
# Available migration tools
./tools/migration_helper/hal_version_checker      # Check current version and compatibility
./tools/migration_helper/deprecated_api_scanner   # Find deprecated API usage
./tools/migration_helper/auto_migration_tool      # Automated code updates
./tools/migration_helper/compatibility_validator  # Validate migration results
```

**Build System Migration**
```bash
# Build system transition tools
./tools/build_migration/cmake_to_buck2           # CMake → Buck2 migration
./tools/build_migration/dependency_updater       # Update dependency versions
./tools/build_migration/platform_config_migrator # Update platform configurations
```

**Testing and Validation**
```bash
# Migration validation tools
./tools/validation/migration_test_suite          # Comprehensive migration testing
./tools/validation/performance_regression_check  # Performance impact analysis
./tools/validation/platform_compatibility_test   # Cross-platform validation
```

### Community and Support

**Getting Help**
- **GitHub Issues**: [flight-hal-interfaces/issues](https://github.com/flight-engine/flight-hal-interfaces/issues)
- **Migration Forum**: [community.flight-engine.org/migration](https://community.flight-engine.org/migration)
- **Discord Support**: `#hal-migration` channel
- **Email Support**: `migration-support@flight-engine.org`

**Contributing to Migration Guides**
- Migration guide improvements: Submit PRs to `docs/migration_guide.md`
- Platform-specific guidance: Add to `docs/platform_specific/`
- Migration tools: Contribute to `tools/migration_helper/`
- Test cases: Add migration scenarios to `tests/migration/`

### Version History and Roadmap

**Recent Migration Milestones**
- **v1.0.0 → v1.1.0**: Stereo rendering capability addition
- **v1.1.0 → v1.2.0**: Enhanced error context and debugging
- **v1.2.0 → v1.3.0**: Performance monitoring integration
- **v1.3.0 → v1.4.0**: Advanced platform capability detection

**Upcoming Migration Targets**
- **v1.x.x → v2.0.0**: Driver registration system v2, enhanced metadata
- **v2.0.0 → v2.1.0**: Compute shader interface introduction
- **v2.1.0 → v2.2.0**: Advanced memory management features
- **v2.2.0 → v3.0.0**: Next-generation architecture evolution

---

## 🏆 Conclusion

The Flight HAL Migration and Upgrade Guide provides comprehensive procedures for evolving this sophisticated cross-platform Hardware Abstraction Layer while maintaining compatibility across extreme platform diversity - from 16MB Dreamcast systems to unlimited modern PCs.

### Key Migration Principles Established

**🎯 Zero Breaking Changes Philosophy**: Through careful capability detection and graceful degradation patterns, Flight HAL maintains universal compatibility while enabling platform-specific optimizations.

**⚡ Smart Feature Classification**: The distinction between hardware-level features (requiring capability detection) and programming conveniences (universal implementation) ensures appropriate migration strategies for each type of enhancement.

**🔄 Semantic Versioning Clarity**: Clear version impact classification (Major/Minor/Patch) with specific migration requirements provides predictable upgrade paths for all stakeholders.

**🛡️ Comprehensive Validation**: Multi-layered testing strategies including platform-specific validation, performance regression detection, and automated migration tools ensure successful transitions.

### Migration Success Enablers

**Platform Diversity Support**: Migration procedures respect the unique constraints and capabilities of each target platform, from memory-constrained embedded devices to high-performance modern systems.

**Developer Experience Focus**: Automated migration tools, clear deprecation timelines, and comprehensive documentation minimize migration friction for development teams.

**Community Integration**: Strong communication strategies, community support channels, and collaborative improvement processes ensure ecosystem-wide migration success.

### Future Evolution Framework

This migration guide establishes patterns and procedures that will scale with Flight HAL's continued evolution, supporting the addition of new platforms, capabilities, and architectural improvements while maintaining the project's commitment to universal compatibility and performance excellence.

**The Flight HAL migration framework enables fearless evolution - advancing the system's capabilities while preserving the investment of every platform team and application developer in the ecosystem.**

---

*Migration guide version 1.0.0 - Updated for Flight HAL v1.1.0*  
*Next review: Upon next minor version release*  
*Feedback: [migration-feedback@flight-engine.org](mailto:migration-feedback@flight-engine.org)*
