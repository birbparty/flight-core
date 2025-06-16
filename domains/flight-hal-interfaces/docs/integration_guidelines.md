# 🚀 Flight HAL Integration Guidelines for Platform Teams

## Overview

This comprehensive guide enables platform implementation teams to successfully integrate Flight HAL drivers across all target platforms. The Flight HAL architecture is inspired by RetroArch's proven driver system and supports extreme platform diversity - from Sega Dreamcast (16MB RAM) to modern gaming PCs.

## Target Platform Teams

- **macOS HAL Team**: Native API integration, Metal optimization
- **Dreamcast HAL Team**: Memory constraints, KallistiOS integration  
- **PSP HAL Team**: Dual-core architecture, PSPSDK patterns
- **Web HAL Team**: Emscripten constraints, browser limitations

---

## 📋 Table of Contents

1. [Universal Driver Implementation Patterns](#universal-driver-implementation-patterns)
2. [Platform-Specific Implementation Guides](#platform-specific-implementation-guides)
3. [Testing and Validation Procedures](#testing-and-validation-procedures)
4. [Performance Optimization Guidelines](#performance-optimization-guidelines)
5. [Troubleshooting and Common Issues](#troubleshooting-and-common-issues)
6. [Reference Implementations](#reference-implementations)

---

## Universal Driver Implementation Patterns

### 1. Base Driver Architecture

All Flight HAL drivers follow a consistent inheritance pattern:

```cpp
#include "flight/hal/interfaces/memory.hpp"
#include "flight/hal/core/driver_auto_registration.hpp"
#include "flight/hal/core/hal_capabilities.hpp"

namespace flight::hal::platform {

class PlatformMemoryDriver : public IMemoryInterface {
public:
    // Driver metadata
    static constexpr int PRIORITY = 10;
    static constexpr const char* DRIVER_NAME = "Platform Memory Driver";
    
    // Constructor/Destructor
    PlatformMemoryDriver();
    ~PlatformMemoryDriver() override;
    
    // Core Interface Implementation
    HALResult<MemoryAllocation> allocate(const AllocationRequest& request) override;
    HALResult<void> deallocate(void* ptr) override;
    HALResult<MemoryStats> get_memory_stats() const override;
    
    // IHALInterface implementation
    std::string_view get_driver_name() const override;
    int get_priority() const override;
    HALResult<void> initialize() override;
    HALResult<void> shutdown() override;
    bool is_active() const override;
    bool is_available() const override;
    
    // ICapabilityProvider implementation
    bool supports_capability(HALCapability capability) const override;
    uint32_t get_capability_mask() const override;
    PerformanceTier get_performance_tier() const override;
    const PlatformInfo& get_platform_info() const override;

private:
    void init_platform_capabilities();
    void init_platform_constraints();
    
    bool initialized_ = false;
    uint32_t capability_mask_;
    std::unique_ptr<PlatformInfo> platform_info_;
};

} // namespace flight::hal::platform
```

### 2. Auto-Registration System

Use the `REGISTER_HAL_DRIVER` macro for automatic driver discovery:

```cpp
// Unconditional registration
REGISTER_HAL_DRIVER(flight::hal::IMemoryInterface, 
                   flight::hal::platform::PlatformMemoryDriver);

// Conditional registration based on platform
#ifdef PLATFORM_MACOS
REGISTER_HAL_DRIVER(flight::hal::IMemoryInterface, 
                   flight::hal::macos::MacOSMemoryDriver);
#endif

// Conditional registration with priority
REGISTER_HAL_DRIVER_WITH_PRIORITY(flight::hal::IGraphicsInterface,
                                 flight::hal::platform::PlatformGraphicsDriver,
                                 15);
```

### 3. HALResult Error Handling Pattern

All operations must use `HALResult<T>` for comprehensive error reporting:

```cpp
HALResult<MemoryAllocation> PlatformMemoryDriver::allocate(const AllocationRequest& request) {
    // Validate parameters
    if (request.size == 0) {
        return HALError::InvalidParameter("Allocation size cannot be zero");
    }
    
    if (request.size > max_allocation_size_) {
        return HALError::InsufficientResources(
            "Requested size exceeds platform maximum");
    }
    
    // Platform-specific allocation logic
    void* ptr = platform_allocate(request.size, request.alignment);
    if (!ptr) {
        return HALError::AllocationFailed("Platform allocation failed");
    }
    
    // Create allocation metadata
    MemoryAllocation allocation;
    allocation.ptr = ptr;
    allocation.size = request.size;
    allocation.alignment = request.alignment;
    allocation.flags = request.flags;
    
    return allocation;
}

// Usage pattern with error handling
auto result = memory_driver->allocate(request);
if (result) {
    auto allocation = result.value();
    // Use allocation.ptr, allocation.size, etc.
} else {
    LOG_ERROR("Memory allocation failed: {}", result.error_message());
    // Handle error appropriately
}
```

### 4. Capability Detection Implementation

Implement robust capability detection for graceful degradation:

```cpp
void PlatformMemoryDriver::init_platform_capabilities() {
    capability_mask_ = 0;
    
    // Check platform-specific capabilities
    if (platform_supports_dma()) {
        capability_mask_ |= static_cast<uint32_t>(HALCapability::DMA);
    }
    
    if (platform_supports_executable_memory()) {
        capability_mask_ |= static_cast<uint32_t>(HALCapability::ExecutableMemory);
    }
    
    if (platform_supports_threading()) {
        capability_mask_ |= static_cast<uint32_t>(HALCapability::Threading);
    }
    
    // Set performance tier based on platform constraints
    if (total_memory_ < 32 * 1024 * 1024) {  // Less than 32MB
        performance_tier_ = PerformanceTier::Minimal;
    } else if (total_memory_ < 256 * 1024 * 1024) {  // Less than 256MB
        performance_tier_ = PerformanceTier::Limited;
    } else {
        performance_tier_ = PerformanceTier::High;
    }
}

bool PlatformMemoryDriver::supports_capability(HALCapability capability) const {
    return (capability_mask_ & static_cast<uint32_t>(capability)) != 0;
}
```

### 5. Resource Management Patterns

Implement proper resource cleanup and tracking:

```cpp
class PlatformMemoryDriver : public IMemoryInterface {
private:
    struct AllocationEntry {
        size_t size;
        MemoryAlignment alignment;
        MemoryFlags flags;
        std::chrono::steady_clock::time_point allocated_at;
    };
    
    mutable std::mutex allocations_mutex_;
    std::unordered_map<void*, AllocationEntry> allocations_;
    std::atomic<size_t> total_allocated_{0};
    std::atomic<size_t> peak_allocated_{0};

public:
    HALResult<void> deallocate(void* ptr) override {
        if (!ptr) {
            return HALError::InvalidParameter("Cannot deallocate null pointer");
        }
        
        std::lock_guard<std::mutex> lock(allocations_mutex_);
        auto it = allocations_.find(ptr);
        if (it == allocations_.end()) {
            return HALError::InvalidParameter("Pointer not found in allocation table");
        }
        
        size_t size = it->second.size;
        allocations_.erase(it);
        
        platform_deallocate(ptr);
        total_allocated_.fetch_sub(size);
        
        return HALResult<void>::success();
    }
    
    ~PlatformMemoryDriver() {
        // Verify all allocations have been freed
        std::lock_guard<std::mutex> lock(allocations_mutex_);
        if (!allocations_.empty()) {
            LOG_WARNING("Memory driver destroyed with {} outstanding allocations", 
                       allocations_.size());
            
            // Clean up remaining allocations
            for (const auto& [ptr, entry] : allocations_) {
                platform_deallocate(ptr);
            }
        }
    }
};
```

---

## Platform-Specific Implementation Guides

### macOS HAL Team Integration Guide

#### Native API Integration Patterns

```cpp
#include <Metal/Metal.h>
#include <Foundation/Foundation.h>
#include <CoreFoundation/CoreFoundation.h>

namespace flight::hal::macos {

class MacOSMemoryDriver : public IMemoryInterface {
private:
    vm_size_t page_size_;
    mach_port_t mach_task_;
    
public:
    HALResult<void> initialize() override {
        // Get system page size
        if (host_page_size(mach_host_self(), &page_size_) != KERN_SUCCESS) {
            return HALError::InitializationFailed("Failed to get system page size");
        }
        
        mach_task_ = mach_task_self();
        
        // Initialize platform capabilities
        init_platform_capabilities();
        
        return HALResult<void>::success();
    }
    
private:
    void init_platform_capabilities() {
        // macOS supports all advanced capabilities
        capability_mask_ = 
            static_cast<uint32_t>(HALCapability::Threading) |
            static_cast<uint32_t>(HALCapability::VirtualMemory) |
            static_cast<uint32_t>(HALCapability::MemoryMapping) |
            static_cast<uint32_t>(HALCapability::DMA) |
            static_cast<uint32_t>(HALCapability::ExecutableMemory) |
            static_cast<uint32_t>(HALCapability::Hardware3D);
        
        performance_tier_ = PerformanceTier::High;
        
        // Initialize platform info
        platform_info_ = std::make_unique<PlatformInfo>();
        platform_info_->platform_name = "macOS";
        platform_info_->architecture = "x86_64"; // or "arm64" for Apple Silicon
        platform_info_->total_memory = get_system_memory_size();
    }
    
    size_t get_system_memory_size() const {
        int mib[2] = {CTL_HW, HW_MEMSIZE};
        uint64_t memory_size;
        size_t length = sizeof(memory_size);
        
        if (sysctl(mib, 2, &memory_size, &length, nullptr, 0) == 0) {
            return static_cast<size_t>(memory_size);
        }
        
        return 0; // Fallback if sysctl fails
    }
};

} // namespace flight::hal::macos
```

#### Metal Graphics Integration

```cpp
namespace flight::hal::macos {

class MacOSGraphicsDriver : public IGraphicsInterface {
private:
    id<MTLDevice> metal_device_;
    id<MTLCommandQueue> command_queue_;
    
public:
    HALResult<void> initialize() override {
        // Get default Metal device
        metal_device_ = MTLCreateSystemDefaultDevice();
        if (!metal_device_) {
            return HALError::InitializationFailed("No Metal device available");
        }
        
        // Create command queue
        command_queue_ = [metal_device_ newCommandQueue];
        if (!command_queue_) {
            return HALError::InitializationFailed("Failed to create Metal command queue");
        }
        
        return HALResult<void>::success();
    }
    
    HALResult<GraphicsBuffer> create_buffer(size_t size, BufferUsage usage) override {
        MTLResourceOptions options = MTLResourceStorageModeShared;
        
        // Choose appropriate storage mode based on usage
        switch (usage) {
            case BufferUsage::Static:
                options = MTLResourceStorageModePrivate;
                break;
            case BufferUsage::Dynamic:
                options = MTLResourceStorageModeShared;
                break;
            case BufferUsage::Staging:
                options = MTLResourceStorageModeShared;
                break;
        }
        
        id<MTLBuffer> buffer = [metal_device_ newBufferWithLength:size options:options];
        if (!buffer) {
            return HALError::AllocationFailed("Failed to create Metal buffer");
        }
        
        GraphicsBuffer result;
        result.native_handle = (__bridge void*)buffer;
        result.size = size;
        result.usage = usage;
        
        return result;
    }
};

} // namespace flight::hal::macos
```

### Dreamcast HAL Team Integration Guide

#### KallistiOS Integration Patterns

```cpp
#include <kos.h>
#include <dc/pvr.h>
#include <dc/maple.h>

namespace flight::hal::dreamcast {

class DreamcastMemoryDriver : public IMemoryInterface {
private:
    static constexpr size_t DREAMCAST_MAIN_RAM = 16 * 1024 * 1024;  // 16MB
    static constexpr size_t DREAMCAST_VRAM = 8 * 1024 * 1024;       // 8MB VRAM
    static constexpr size_t MAX_ALLOCATION = 1 * 1024 * 1024;       // 1MB max per allocation
    
    // Custom allocator for extreme memory efficiency
    std::unique_ptr<PoolAllocator> main_pool_;
    std::unique_ptr<LinearAllocator> scratch_allocator_;
    
public:
    HALResult<void> initialize() override {
        // Initialize memory pools with Dreamcast constraints
        main_pool_ = std::make_unique<PoolAllocator>(
            DREAMCAST_MAIN_RAM * 0.8,  // Use 80% of RAM for main pool
            64,                         // 64-byte alignment for SH-4 efficiency
            4096                        // 4KB minimum block size
        );
        
        scratch_allocator_ = std::make_unique<LinearAllocator>(
            DREAMCAST_MAIN_RAM * 0.1   // 10% for scratch/temporary allocations
        );
        
        init_platform_capabilities();
        return HALResult<void>::success();
    }
    
private:
    void init_platform_capabilities() {
        // Dreamcast has minimal capabilities
        capability_mask_ = 
            static_cast<uint32_t>(HALCapability::BasicMemory);
        
        // No threading, DMA, or virtual memory support
        performance_tier_ = PerformanceTier::Minimal;
        
        platform_info_ = std::make_unique<PlatformInfo>();
        platform_info_->platform_name = "Sega Dreamcast";
        platform_info_->architecture = "SH-4";
        platform_info_->total_memory = DREAMCAST_MAIN_RAM;
        platform_info_->memory_alignment = 32;  // SH-4 cache line size
    }
    
public:
    HALResult<MemoryAllocation> allocate(const AllocationRequest& request) override {
        // Strict size limits for Dreamcast
        if (request.size > MAX_ALLOCATION) {
            return HALError::InsufficientResources(
                "Allocation exceeds Dreamcast limit (1MB)");
        }
        
        // Use pool allocator for main allocations
        void* ptr = main_pool_->allocate(request.size, request.alignment);
        if (!ptr && request.flags & MemoryFlags::AllowScratch) {
            // Fallback to scratch allocator
            ptr = scratch_allocator_->allocate(request.size, request.alignment);
        }
        
        if (!ptr) {
            return HALError::OutOfMemory("Dreamcast memory exhausted");
        }
        
        MemoryAllocation allocation;
        allocation.ptr = ptr;
        allocation.size = request.size;
        allocation.alignment = request.alignment;
        allocation.flags = request.flags;
        
        return allocation;
    }
};

} // namespace flight::hal::dreamcast
```

#### PowerVR2 Graphics Integration

```cpp
namespace flight::hal::dreamcast {

class DreamcastGraphicsDriver : public IGraphicsInterface {
private:
    static constexpr size_t PVR_TEXTURE_RAM = 8 * 1024 * 1024;  // 8MB VRAM
    pvr_init_params_t pvr_params_;
    
public:
    HALResult<void> initialize() override {
        // Initialize PowerVR2 with conservative settings
        pvr_params_.opb_sizes[PVR_BINSIZE_0] = PVR_BINSIZE_32;
        pvr_params_.opb_sizes[PVR_BINSIZE_1] = PVR_BINSIZE_0;
        pvr_params_.vertex_buf_size = 512 * 1024;  // 512KB vertex buffer
        pvr_params_.dma_enabled = 0;               // Disable DMA for stability
        pvr_params_.fsaa_enabled = 0;              // Disable FSAA
        pvr_params_.autosort_disabled = 0;
        
        if (pvr_init(&pvr_params_) < 0) {
            return HALError::InitializationFailed("PowerVR2 initialization failed");
        }
        
        return HALResult<void>::success();
    }
    
    HALResult<RenderTarget> create_render_target(uint32_t width, uint32_t height, 
                                                PixelFormat format) override {
        // Dreamcast limitations: 640x480 maximum, 16-bit color recommended
        if (width > 640 || height > 480) {
            return HALError::InvalidParameter("Dreamcast maximum resolution is 640x480");
        }
        
        if (format != PixelFormat::RGB565 && format != PixelFormat::ARGB1555) {
            return HALError::UnsupportedFormat("Only 16-bit formats supported on Dreamcast");
        }
        
        RenderTarget target;
        target.width = width;
        target.height = height;
        target.format = format;
        target.native_handle = nullptr;  // Dreamcast uses single framebuffer
        
        return target;
    }
};

} // namespace flight::hal::dreamcast
```

### PSP HAL Team Integration Guide

#### PSPSDK Integration Patterns

```cpp
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>

namespace flight::hal::psp {

class PSPMemoryDriver : public IMemoryInterface {
private:
    static constexpr size_t PSP_MAIN_RAM = 32 * 1024 * 1024;   // 32MB main RAM
    static constexpr size_t PSP_VRAM = 2 * 1024 * 1024;        // 2MB VRAM
    static constexpr size_t MEDIA_ENGINE_RAM = 2 * 1024 * 1024; // 2MB ME RAM
    
    // Dual allocator strategy for dual-core architecture
    std::unique_ptr<PoolAllocator> main_pool_;
    std::unique_ptr<PoolAllocator> media_engine_pool_;
    
public:
    HALResult<void> initialize() override {
        // Initialize main CPU memory pool
        main_pool_ = std::make_unique<PoolAllocator>(
            PSP_MAIN_RAM * 0.7,        // 70% for main pool
            16,                        // 16-byte alignment for MIPS
            1024                       // 1KB minimum block size
        );
        
        // Initialize Media Engine memory pool if available
        if (sceKernelDevkitVersion() >= 0x02000000) {  // PSP-2000+ has ME
            media_engine_pool_ = std::make_unique<PoolAllocator>(
                MEDIA_ENGINE_RAM * 0.8,
                16,
                512
            );
        }
        
        init_platform_capabilities();
        return HALResult<void>::success();
    }
    
private:
    void init_platform_capabilities() {
        capability_mask_ = 
            static_cast<uint32_t>(HALCapability::BasicMemory) |
            static_cast<uint32_t>(HALCapability::Hardware3D);
        
        // Check for dual-core support (PSP-2000+)
        if (sceKernelDevkitVersion() >= 0x02000000) {
            capability_mask_ |= static_cast<uint32_t>(HALCapability::DualCore);
        }
        
        performance_tier_ = PerformanceTier::Limited;
        
        platform_info_ = std::make_unique<PlatformInfo>();
        platform_info_->platform_name = "Sony PSP";
        platform_info_->architecture = "MIPS R4000";
        platform_info_->total_memory = PSP_MAIN_RAM;
        platform_info_->memory_alignment = 16;
    }
    
public:
    HALResult<MemoryAllocation> allocate(const AllocationRequest& request) override {
        void* ptr = nullptr;
        
        // Use Media Engine pool for audio/video processing if available
        if (request.flags & MemoryFlags::MediaProcessing && media_engine_pool_) {
            ptr = media_engine_pool_->allocate(request.size, request.alignment);
        }
        
        // Fallback to main pool
        if (!ptr) {
            ptr = main_pool_->allocate(request.size, request.alignment);
        }
        
        if (!ptr) {
            return HALError::OutOfMemory("PSP memory exhausted");
        }
        
        MemoryAllocation allocation;
        allocation.ptr = ptr;
        allocation.size = request.size;
        allocation.alignment = request.alignment;
        allocation.flags = request.flags;
        
        return allocation;
    }
};

} // namespace flight::hal::psp
```

### Web HAL Team Integration Guide

#### Emscripten Integration Patterns

```cpp
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/bind.h>
#endif

namespace flight::hal::web {

class WebMemoryDriver : public IMemoryInterface {
private:
    static constexpr size_t WEB_MEMORY_LIMIT = 50 * 1024 * 1024;  // 50MB browser limit
    static constexpr size_t GC_THRESHOLD = 40 * 1024 * 1024;      // Trigger GC at 40MB
    
    std::atomic<size_t> total_allocated_{0};
    std::atomic<size_t> gc_counter_{0};
    
public:
    HALResult<void> initialize() override {
        // Check for WebAssembly support
        #ifdef __EMSCRIPTEN__
        if (!emscripten_has_asyncify()) {
            LOG_WARNING("Asyncify not available - some features may be limited");
        }
        #endif
        
        init_platform_capabilities();
        return HALResult<void>::success();
    }
    
private:
    void init_platform_capabilities() {
        capability_mask_ = 
            static_cast<uint32_t>(HALCapability::BasicMemory) |
            static_cast<uint32_t>(HALCapability::WebGL) |
            static_cast<uint32_t>(HALCapability::WebAudio) |
            static_cast<uint32_t>(HALCapability::SandboxRestrictions);
        
        performance_tier_ = PerformanceTier::Standard;
        
        platform_info_ = std::make_unique<PlatformInfo>();
        platform_info_->platform_name = "Web Browser";
        platform_info_->architecture = "WebAssembly";
        platform_info_->total_memory = WEB_MEMORY_LIMIT;
        platform_info_->memory_alignment = 8;  // WebAssembly alignment
    }
    
public:
    HALResult<MemoryAllocation> allocate(const AllocationRequest& request) override {
        // Check browser memory limits
        size_t new_total = total_allocated_.load() + request.size;
        if (new_total > WEB_MEMORY_LIMIT) {
            return HALError::InsufficientResources("Browser memory limit exceeded");
        }
        
        // Trigger garbage collection hint if approaching limit
        if (new_total > GC_THRESHOLD && (gc_counter_++ % 10) == 0) {
            #ifdef __EMSCRIPTEN__
            emscripten_force_exit(0); // This is a placeholder - use proper GC hint
            #endif
        }
        
        void* ptr = std::aligned_alloc(
            static_cast<size_t>(request.alignment), 
            request.size
        );
        
        if (!ptr) {
            return HALError::OutOfMemory("Web allocation failed");
        }
        
        total_allocated_.fetch_add(request.size);
        
        MemoryAllocation allocation;
        allocation.ptr = ptr;
        allocation.size = request.size;
        allocation.alignment = request.alignment;
        allocation.flags = request.flags;
        
        return allocation;
    }
    
    HALResult<void> deallocate(void* ptr) override {
        if (!ptr) {
            return HALError::InvalidParameter("Cannot deallocate null pointer");
        }
        
        // Note: In real implementation, track allocation sizes
        std::free(ptr);
        
        return HALResult<void>::success();
    }
};

} // namespace flight::hal::web
```

---

## Testing and Validation Procedures

### Platform-Specific Test Execution

Each platform team should execute the complete test suite with platform-appropriate configurations:

```bash
# macOS Testing
cmake -DFLIGHT_HAL_PLATFORM=macOS -DFLIGHT_HAL_MAX_MEMORY_TEST=1073741824 ..
make hal_compliance_tests
./hal_compliance_tests --platform=macos

# Dreamcast Testing  
cmake -DFLIGHT_HAL_PLATFORM=Dreamcast -DFLIGHT_HAL_MAX_MEMORY_TEST=1048576 ..
make hal_compliance_tests
./hal_compliance_tests --platform=dreamcast --memory-limit=16MB

# PSP Testing
cmake -DFLIGHT_HAL_PLATFORM=PSP -DFLIGHT_HAL_MAX_MEMORY_TEST=2097152 ..
make hal_compliance_tests  
./hal_compliance_tests --platform=psp --dual-core-tests

# Web Testing
emcmake cmake -DFLIGHT_HAL_PLATFORM=Web -DFLIGHT_HAL_MAX_MEMORY_TEST=52428800 ..
emmake make hal_compliance_tests
node hal_compliance_tests.js --platform=web --browser-limits
```

### Compliance Test Requirements

All drivers must pass these minimum compliance requirements:

1. **Basic Functionality Tests**
   - Interface contract validation
   - Parameter validation
   - Error handling verification
   - Resource cleanup validation

2. **Performance Tests** (Platform-specific SLAs)
   - **macOS**: < 1ms allocation latency, > 1GB/s throughput
   - **Dreamcast**: < 5ms allocation latency, memory efficiency > 90%
   - **PSP**: < 2ms allocation latency, dual-core coordination
   - **Web**: < 3ms allocation latency, GC-friendly patterns

3. **Stress Tests**
   - Memory pressure scenarios
   - Concurrent allocation/deallocation
   - Resource exhaustion recovery
   - Long-running stability

### Integration Test Strategy

```cpp
// Platform-specific integration test example
TEST_F(PlatformIntegrationTest, CrossDriverCoordination) {
    auto memory = platform.get_interface<IMemoryInterface>();
    auto graphics = platform.get_interface<IGraphicsInterface>();
    
    ASSERT_TRUE(memory);
    ASSERT_TRUE(graphics);
    
    // Test resource sharing between drivers
    auto mem_result = memory->allocate(1024 * 1024, MemoryAlignment::GPU);
    ASSERT_TRUE(mem_result);
    
    auto buffer_result = graphics->create_buffer_from_memory(
        mem_result.value().ptr, 
        mem_result.value().size
    );
    ASSERT_TRUE(buffer_result);
    
    // Verify coordination worked correctly
    EXPECT_TRUE(graphics->is_valid_buffer(buffer_result.value()));
}
```

---

## Performance Optimization Guidelines

### Platform-Specific Memory Strategies

#### macOS Optimization
```cpp
// Use mmap for large allocations
void* allocate_large_macos(size_t size) {
    void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, 
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) return nullptr;
    
    // Use madvise for performance hints
    madvise(ptr, size, MADV_SEQUENTIAL);
    return ptr;
}

// Use dispatch queues for async operations
dispatch_queue_t memory_queue = dispatch_queue_create(
    "com.flight.memory", DISPATCH_QUEUE_CONCURRENT);
```

#### Dreamcast Optimization
```cpp
// Use SH-4 cache-friendly allocation patterns
void* allocate_cache_aligned_dreamcast(size_t size) {
    // Align to 32-byte cache lines
    size_t aligned_size = (size + 31) & ~31;
    
    // Use P2 area for uncached access when needed
    if (size > 64 * 1024) {  // Large allocations
        return (void*)((uintptr_t)malloc(aligned_size) | 0xA0000000);
    }
    
    return memalign(32, aligned_size);
}
```

#### PSP Optimization
```cpp
// Use Media Engine for parallel processing
void setup_media_engine_processing() {
    if (media_engine_available()) {
        // Allocate shared memory for ME communication
        void* shared_mem = allocate_uncached_memory(64 * 1024);
        
        // Setup ME program for audio/video processing
        load_me_program("audio_processor.prx");
    }
}
```

#### Web Optimization  
```cpp
// Use SharedArrayBuffer when available
bool setup_shared_memory_web() {
    #ifdef __EMSCRIPTEN__
    EM_ASM({
        if (typeof SharedArrayBuffer !== 'undefined') {
            Module.sharedMemory = new SharedArrayBuffer(1024 * 1024);
            return true;
        }
        return false;
    });
    #endif
    return false;
}
```

### Threading Model Adaptations

```cpp
// Platform-specific threading strategies
namespace flight::hal::threading {

class PlatformThreadManager {
public:
    static std::unique_ptr<PlatformThreadManager> create() {
        #ifdef PLATFORM_MACOS
        return std::make_unique<MacOSThreadManager>();
        #elif defined(PLATFORM_DREAMCAST)
        return std::make_unique<SingleThreadManager>();  // No threading
        #elif defined(PLATFORM_PSP)
        return std::make_unique<PSPDualCoreManager>();
        #elif defined(PLATFORM_WEB)
        return std::make_unique<WebWorkerManager>();
        #endif
    }
};

// macOS: Grand Central Dispatch optimization
class MacOSThreadManager {
    dispatch_queue_t concurrent_queue_;
    dispatch_group_t operation_group_;
    
public:
    void execute_parallel(std::vector<std::function<void()>> tasks) {
        for (auto& task : tasks) {
            dispatch_group_async(operation_group_, concurrent_queue_, ^{
                task();
            });
        }
        dispatch_group_wait(operation_group_, DISPATCH_TIME_FOREVER);
    }
};

// PSP: Dual-core coordination
class PSPDualCoreManager {
public:
    void execute_on_media_engine(std::function<void()> task) {
        // Load and execute task on Media Engine
        sceKernelUtilsMt19937Init(&mt_ctx_, time(nullptr));
        // Media Engine task execution logic
    }
};

} // namespace flight::hal::threading
```

### Graphics Pipeline Optimization

```cpp
// Platform-specific graphics optimization patterns
namespace flight::hal::graphics {

// macOS Metal optimization
void optimize_metal_pipeline(id<MTLRenderPipelineState> pipeline) {
    // Use tile-based rendering optimization
    MTLRenderPipelineDescriptor* descriptor = [[MTLRenderPipelineDescriptor alloc] init];
    descriptor.rasterSampleCount = 1;  // Disable MSAA for performance
    descriptor.supportIndirectCommandBuffers = YES;
}

// Dreamcast PowerVR2 optimization  
void optimize_pvr2_pipeline() {
    // Use modifier volumes sparingly
    pvr_modifier_vol_t mod_vol;
    mod_vol.flags = PVR_MODIFIER_CHEAP;  // Use cheap modifier
    
    // Batch polygon submissions
    pvr_list_begin(PVR_LIST_OP_POLY);
    // Submit all opaque polygons at once
    pvr_list_finish();
}

// PSP Graphics Engine optimization
void optimize_psp_graphics() {
    // Use VRAM for frequently accessed textures
    sceGuStart(GU_DIRECT, display_list_);
    sceGuDrawBuffer(GU_PSM_5650, vram_base_, FRAME_BUFFER_WIDTH);
    sceGuDispBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, vram_base_ + FRAME_SIZE, FRAME_BUFFER_WIDTH);
}

// Web WebGL optimization
void optimize_webgl_pipeline() {
    // Use instanced rendering when available
    if (webgl_context_->getExtension("ANGLE_instanced_arrays")) {
        // Enable instanced rendering for better performance
    }
    
    // Minimize state changes
    batch_draw_calls();
}

} // namespace flight::hal::graphics
```

---

## Troubleshooting and Common Issues

### Common Implementation Problems

#### 1. Memory Allocation Failures

**Problem**: Drivers failing to allocate memory on constrained platforms.

**Solution**:
```cpp
// Always check platform constraints before allocation
HALResult<MemoryAllocation> safe_allocate(size_t size) {
    auto platform_info = get_platform_info();
    
    // Check against platform maximum
    if (size > platform_info.max_allocation_size) {
        return HALError::InsufficientResources(
            "Allocation exceeds platform limit");
    }
    
    // Check available memory
    auto stats = get_memory_stats();
    if (stats && (stats.value().available_memory < size * 2)) {
        // Try garbage collection first
        gc_hint();
        
        // Re-check after GC
        auto new_stats = get_memory_stats();
        if (new_stats && (new_stats.value().available_memory < size)) {
            return HALError::OutOfMemory("Insufficient memory after GC");
        }
    }
    
    return platform_allocate_impl(size);
}
```

#### 2. Capability Detection Issues

**Problem**: Incorrect capability reporting leading to runtime failures.

**Solution**:
```cpp
// Always verify capabilities at runtime
bool verify_capability_implementation() {
    // Test each reported capability
    if (supports_capability(HALCapability::DMA)) {
        // Actually test DMA functionality
        auto result = test_dma_allocation();
        if (!result) {
            LOG_ERROR("DMA capability reported but test failed");
            // Remove from capability mask
            capability_mask_ &= ~static_cast<uint32_t>(HALCapability::DMA);
            return false;
        }
    }
    
    return true;
}
```

#### 3. Threading Coordination Problems

**Problem**: Deadlocks or race conditions in multi-threaded environments.

**Solution**:
```cpp
// Use consistent lock ordering
class ThreadSafeDriver {
private:
    mutable std::mutex mutex_a_;  // Always acquire first
    mutable std::mutex mutex_b_;  // Always acquire second
    
public:
    void safe_operation() {
        std::lock_guard<std::mutex> lock_a(mutex_a_);
        std::lock_guard<std::mutex> lock_b(mutex_b_);
        // Critical section
    }
    
    // Use RAII for cleanup
    class ScopedResource {
        Driver* driver_;
    public:
        ScopedResource(Driver* d) : driver_(d) { driver_->acquire(); }
        ~ScopedResource() { driver_->release(); }
    };
};
```

### Platform-Specific Debugging

#### macOS Debugging
```bash
# Use Instruments for performance profiling
instruments -t "Metal System Trace" -D trace.trace ./hal_test

# Debug memory issues with AddressSanitizer
cmake -DCMAKE_BUILD_TYPE=Debug -DSANITIZE_ADDRESS=ON ..
make hal_test && ./hal_test

# Use Xcode's Metal Debugger
# Set METAL_DEBUG_ERROR_MODE=2 environment variable
```

#### Dreamcast Debugging
```bash
# Use dcload for debugging over network
make && dc-tool -t 192.168.1.100 -x hal_test.elf

# Enable debug symbols
kos-cc -g -O0 -o hal_test.elf *.o

# Use GDB with dc-tool
dc-tool-gdb -g -t 192.168.1.100 -x hal_test.elf
```

#### PSP Debugging
```bash
# Use PSPLINK for debugging
pspsh -p 4000  # Connect to PSP
reset          # Reset PSP
debug hal_test.prx  # Load and debug

# Enable verbose logging
sceKernelPrintf("Debug: %s\n", debug_message);
```

#### Web Debugging  
```bash
# Use browser developer tools
# Enable WebAssembly debugging
emcc -g4 --source-map-base http://localhost:8000/ 

# Use Chrome DevTools for WebGL debugging
# Install WebGL Inspector extension
```

### Performance Debugging Tools

```cpp
// Built-in performance monitoring
class PerformanceProfiler {
    std::chrono::high_resolution_clock::time_point start_;
    std::string operation_name_;
    
public:
    PerformanceProfiler(const std::string& name) 
        : operation_name_(name)
        , start_(std::chrono::high_resolution_clock::now()) {
    }
    
    ~PerformanceProfiler() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end - start_).count();
        
        LOG_PERF("{} took {} microseconds", operation_name_, duration);
        
        // Check against platform SLA
        if (duration > get_platform_sla_microseconds()) {
            LOG_WARNING("{} exceeded SLA: {} > {} microseconds", 
                       operation_name_, duration, get_platform_sla_microseconds());
        }
    }
};

#define PROFILE_OPERATION(name) PerformanceProfiler prof(name)
```

---

## Reference Implementations

### Complete Memory Driver Example

Here's a complete, production-ready memory driver implementation:

```cpp
// File: drivers/reference/reference_memory_driver.hpp
#pragma once

#include "flight/hal/interfaces/memory.hpp"
#include "flight/hal/core/driver_auto_registration.hpp"
#include <unordered_map>
#include <mutex>
#include <atomic>

namespace flight::hal::reference {

class ReferenceMemoryDriver : public IMemoryInterface {
public:
    static constexpr int PRIORITY = 5;
    static constexpr const char* DRIVER_NAME = "Reference Memory Driver";
    
    ReferenceMemoryDriver();
    ~ReferenceMemoryDriver() override;
    
    // Core allocation methods
    HALResult<MemoryAllocation> allocate(const AllocationRequest& request) override;
    HALResult<void> deallocate(void* ptr) override;
    HALResult<MemoryAllocation> reallocate(void* ptr, size_t new_size) override;
    
    // Memory information
    HALResult<MemoryStats> get_memory_stats() const override;
    HALResult<MemoryStats> get_memory_stats(MemoryType type) const override;
    bool is_valid_pointer(void* ptr) const override;
    HALResult<MemoryAllocation> get_allocation_info(void* ptr) const override;
    
    // Capability queries
    bool supports_alignment(MemoryAlignment alignment) const override;
    bool supports_memory_type(MemoryType type) const override;
    size_t get_max_allocation_size() const override;
    
    // Memory management
    HALResult<void> defragment(DefragmentationCallback callback = nullptr) override;
    HALResult<void> gc_hint() override;
    HALResult<size_t> trim_memory() override;
    
    // IHALInterface implementation
    std::string_view get_driver_name() const override;
    int get_priority() const override;
    HALResult<void> initialize() override;
    HALResult<void> shutdown() override;
    bool is_active() const override;
    bool is_available() const override;
    std::string_view get_version() const override;
    
    // ICapabilityProvider implementation
    bool supports_capability(HALCapability capability) const override;
    uint32_t get_capability_mask() const override;
    PerformanceTier get_performance_tier() const override;
    const PlatformInfo& get_platform_info() const override;

private:
    struct AllocationEntry {
        size_t size;
        MemoryAlignment alignment;
        MemoryFlags flags;
        MemoryType type;
        std::chrono::steady_clock::time_point allocated_at;
        bool valid;
    };
    
    void init_platform_capabilities();
    void* platform_allocate(size_t size, MemoryAlignment alignment);
    void platform_deallocate(void* ptr);
    size_t calculate_aligned_size(size_t size, MemoryAlignment alignment) const;
    bool exceeds_platform_limits(size_t size) const;
    
    // Thread-safe allocation tracking
    mutable std::mutex allocations_mutex_;
    std::unordered_map<void*, AllocationEntry> allocations_;
    
    // Performance statistics
    std::atomic<size_t> total_allocated_{0};
    std::atomic<size_t> peak_allocated_{0};
    std::atomic<size_t> allocation_count_{0};
    std::atomic<size_t> deallocation_count_{0};
    
    // Driver state
    bool initialized_ = false;
    bool active_ = false;
    uint32_t capability_mask_;
    PerformanceTier performance_tier_;
    std::unique_ptr<PlatformInfo> platform_info_;
    
    // Platform constraints
    size_t max_allocation_size_;
    size_t total_available_memory_;
};

} // namespace flight::hal::reference

// Auto-register the reference driver
REGISTER_HAL_DRIVER(flight::hal::IMemoryInterface, 
                   flight::hal::reference::ReferenceMemoryDriver);
```

### Quick Start Template

Teams can use this template to quickly bootstrap a new driver:

```cpp
// File: drivers/platform/platform_INTERFACE_driver.hpp
#pragma once

#include "flight/hal/interfaces/INTERFACE.hpp"
#include "flight/hal/core/driver_auto_registration.hpp"

namespace flight::hal::PLATFORM {

class PlatformINTERFACEDriver : public IINTERFACE {
public:
    static constexpr int PRIORITY = 10;
    static constexpr const char* DRIVER_NAME = "Platform INTERFACE Driver";
    
    // TODO: Implement required interface methods
    
    // IHALInterface implementation
    std::string_view get_driver_name() const override {
        return DRIVER_NAME;
    }
    
    int get_priority() const override {
        return PRIORITY;
    }
    
    HALResult<void> initialize() override {
        // TODO: Platform-specific initialization
        return HALResult<void>::success();
    }
    
    HALResult<void> shutdown() override {
        // TODO: Platform-specific cleanup
        return HALResult<void>::success();
    }
    
    bool is_active() const override {
        return initialized_;
    }
    
    bool is_available() const override {
        // TODO: Check platform availability
        return true;
    }
    
    std::string_view get_version() const override {
        return "1.0.0";
    }
    
    // ICapabilityProvider implementation
    bool supports_capability(HALCapability capability) const override {
        return (capability_mask_ & static_cast<uint32_t>(capability)) != 0;
    }
    
    uint32_t get_capability_mask() const override {
        return capability_mask_;
    }
    
    PerformanceTier get_performance_tier() const override {
        return performance_tier_;
    }
    
    const PlatformInfo& get_platform_info() const override {
        return *platform_info_;
    }

private:
    void init_platform_capabilities() {
        // TODO: Initialize platform-specific capabilities
        capability_mask_ = 0;
        performance_tier_ = PerformanceTier::Standard;
        
        platform_info_ = std::make_unique<PlatformInfo>();
        platform_info_->platform_name = "PLATFORM";
        platform_info_->architecture = "ARCHITECTURE";
        platform_info_->total_memory = 0; // TODO: Get actual memory size
    }
    
    bool initialized_ = false;
    uint32_t capability_mask_;
    PerformanceTier performance_tier_;
    std::unique_ptr<PlatformInfo> platform_info_;
};

} // namespace flight::hal::PLATFORM

// Auto-register the driver
REGISTER_HAL_DRIVER(flight::hal::IINTERFACE, 
                   flight::hal::PLATFORM::PlatformINTERFACEDriver);
```

---

## 🎯 Success Criteria Checklist

### Universal Implementation Requirements
- ✅ **Consistent inheritance pattern** from HAL interfaces
- ✅ **Auto-registration** using `REGISTER_HAL_DRIVER` macros
- ✅ **HALResult error handling** for all operations
- ✅ **Capability detection** with graceful degradation
- ✅ **Resource management** with proper cleanup
- ✅ **Performance monitoring** with platform-appropriate SLAs

### Platform-Specific Integration
- ✅ **macOS**: Metal API integration, Core Foundation patterns
- ✅ **Dreamcast**: KallistiOS integration, 16MB memory constraints
- ✅ **PSP**: PSPSDK patterns, dual-core optimization
- ✅ **Web**: Emscripten compatibility, browser security compliance

### Testing and Validation
- ✅ **Compliance testing** with platform-specific configurations
- ✅ **Performance benchmarking** against platform SLAs
- ✅ **Stress testing** for resource exhaustion scenarios
- ✅ **Integration testing** for cross-driver coordination

### Documentation and Support
- ✅ **Comprehensive implementation guides** for each platform
- ✅ **Reference implementations** and templates
- ✅ **Troubleshooting guides** with platform-specific debugging
- ✅ **Performance optimization** strategies and patterns

---

## 📞 Support and Resources

### Getting Help
- **Documentation**: Refer to interface-specific design documents
- **Examples**: Study mock drivers and comprehensive examples
- **Testing**: Use compliance validation tools for verification
- **Performance**: Leverage benchmarking framework for optimization

### Additional Resources
- [Flight HAL Architecture](docs/architecture/enhanced_hal_architecture.md)
- [Interface Compliance Testing](docs/interface_compliance_testing_framework.md)
- [Performance Benchmarking](docs/performance_benchmarking_system.md)
- [Validation Tools Guide](docs/hal_validation_tools_guide.md)

---

**Ready to integrate? Choose your platform-specific guide above and start implementing world-class HAL drivers for Flight's revolutionary cross-platform engine!**
