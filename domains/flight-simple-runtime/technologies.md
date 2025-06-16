# Flight Simple Runtime Technology Stack

## 🔧 Core Technologies

### Programming Language: C++14
**Why C++14 (not C++17):**
- Maximum platform compatibility
- Works with older toolchains (PSX, Dreamcast)
- Still provides modern features
- No filesystem dependency

**Key C++14 Features:**
- `auto` return types
- Generic lambdas
- `std::make_unique` (custom implementation for embedded)
- Binary literals
- Digit separators

### WebAssembly Interpreter: wasm3
**Why wasm3:**
- Written in pure C99
- ~100KB footprint
- Zero dependencies
- Proven on embedded systems
- Fast interpreter performance

**Integration Strategy:**
```cpp
// Embed wasm3 source directly
#include "wasm3/source/wasm3.h"
#include "wasm3/source/m3_api_libc.h"
#include "wasm3/source/m3_api_wasi.h"
```

**Configuration:**
```c
// Platform-specific tuning
#define d_m3MaxFunctionStackHeight   128    // Reduced for embedded
#define d_m3CodePageAlignSize        4096   // Page alignment
#define d_m3EnableCodePageRefCounting 0     // Disable for determinism
#define d_m3ProfilerSlotMask         0      // Disable in release
```

## 🏗️ Build System

### CMake Configuration
```cmake
# Minimal CMake for embedded
cmake_minimum_required(VERSION 3.14)
project(flight-simple-runtime C CXX)

# Platform detection
if(PSX_BUILD)
    set(CMAKE_C_STANDARD 99)
    set(CMAKE_CXX_STANDARD 14)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msoft-float")
endif()

# Embed wasm3
add_subdirectory(third_party/wasm3 EXCLUDE_FROM_ALL)

# Runtime library
add_library(flight-runtime STATIC
    src/runtime.cpp
    src/host_functions.cpp
    src/memory_manager.cpp
)
```

### Platform Toolchains
```cmake
# PSX toolchain
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_C_COMPILER mipsel-none-elf-gcc)
set(CMAKE_CXX_COMPILER mipsel-none-elf-g++)
set(CMAKE_AR mipsel-none-elf-ar)

# Dreamcast toolchain
set(KOS_BASE $ENV{KOS_BASE})
include(${KOS_BASE}/environ.sh)
```

## 🔌 Host Function Implementation

### HAL Mapping Strategy
```cpp
// Direct HAL mapping without abstraction
namespace host_functions {

// Graphics functions
m3ApiRawFunction(hal_clear_screen) {
    m3ApiGetArg(uint32_t, color);
    
    auto* video = get_current_platform()->video_driver();
    video->clear(color);
    
    m3ApiSuccess();
}

// Memory functions
m3ApiRawFunction(hal_alloc) {
    m3ApiGetArg(uint32_t, size);
    m3ApiGetArg(uint32_t, align);
    
    auto* memory = get_current_platform()->memory_driver();
    void* ptr = memory->allocate(size, align);
    
    m3ApiReturn(uint32_t, wasm_ptr(ptr));
}

}
```

### Function Registration
```cpp
struct HostFunctionDef {
    const char* module;
    const char* name;
    const char* signature;
    M3RawCall impl;
};

// Function table
constexpr HostFunctionDef HOST_FUNCTIONS[] = {
    // Graphics
    {"env", "clear_screen", "v(i)", hal_clear_screen},
    {"env", "draw_sprite", "v(iiii)", hal_draw_sprite},
    
    // Audio
    {"env", "play_sound", "v(ii)", hal_play_sound},
    
    // Input
    {"env", "get_input", "i()", hal_get_input},
    
    // Memory
    {"env", "alloc", "i(ii)", hal_alloc},
    {"env", "free", "v(i)", hal_free},
};
```

## 💾 Memory Management

### Linear Memory Setup
```cpp
class WasmMemory {
    uint8_t* base;
    size_t size;
    MemoryLayout layout;
    
public:
    void initialize(const MemoryLayout& layout) {
        // Allocate from platform
        base = platform->allocate_linear_memory(layout.total_size);
        
        // Set up segments
        for (const auto& segment : layout.segments) {
            setup_segment(segment);
        }
    }
    
    // Fast address translation
    inline void* translate(uint32_t wasm_addr) {
        // No bounds check in release
        return base + wasm_addr;
    }
};
```

### Platform Memory Abstraction
```cpp
// Minimal abstraction for different platforms
#ifdef PSX
    #define WASM_MEMORY_BASE 0x80100000  // After kernel
    #define WASM_MEMORY_SIZE (1024 * 1024)  // 1MB
#elif defined(PSP)
    #define WASM_MEMORY_BASE 0x08800000
    #define WASM_MEMORY_SIZE (16 * 1024 * 1024)  // 16MB
#endif
```

## 🚀 Performance Optimizations

### Interpreter Optimizations
```cpp
// Custom wasm3 extensions
namespace wasm3_opt {

// Instruction cache for hot functions
struct InstructionCache {
    std::array<IM3Operation, 1024> cache;
    std::array<uint32_t, 32> hot_functions;
    
    void profile_function(uint32_t func_index);
    void optimize_hot_path(IM3Function func);
};

// Fast dispatch table
alignas(64) M3OpInfo optimized_ops[256] = {
    // Optimized operation implementations
};

}
```

### Platform-Specific Optimizations
```cpp
// PSP: Use VFPU for float operations
#ifdef PSP
    #include <pspfpu.h>
    
    m3ApiRawFunction(fast_vec4_mul) {
        // Use PSP vector unit
        pspFpuSetRoundmode(PSP_FPU_RM_NEAR);
        // ... VFPU operations
    }
#endif

// Modern: Consider JIT
#ifdef MODERN_PLATFORM
    class SelectiveJIT {
        void compile_hot_function(IM3Function func);
    };
#endif
```

## 🐛 Debug Support

### Trace Infrastructure
```cpp
#ifdef DEBUG_BUILD
class RuntimeDebugger {
    struct CallFrame {
        const char* function_name;
        uint64_t enter_time;
        uint32_t wasm_pc;
    };
    
    std::vector<CallFrame> call_stack;
    
public:
    void on_function_enter(IM3Function func);
    void on_function_exit();
    void on_memory_access(uint32_t addr, uint32_t size, bool write);
    void dump_stack_trace();
};
#else
    // No-op in release
    #define on_function_enter(x)
    #define on_function_exit()
#endif
```

### Performance Counters
```cpp
struct RuntimeStats {
    uint64_t total_instructions;
    uint64_t function_calls;
    uint64_t memory_accesses;
    uint64_t cache_hits;
    
    void report() const;
};
```

## 🔗 Platform Integration

### Platform Factory
```cpp
class PlatformRuntime {
public:
    static std::unique_ptr<FlightRuntime> create() {
        #ifdef PSX
            return std::make_unique<PSXRuntime>();
        #elif defined(PSP)
            return std::make_unique<PSPRuntime>();
        #elif defined(DREAMCAST)
            return std::make_unique<DreamcastRuntime>();
        #else
            return std::make_unique<ModernRuntime>();
        #endif
    }
};
```

### Error Handling
```cpp
// No exceptions - use error codes
enum class RuntimeError {
    Success = 0,
    ModuleLoadFailed,
    LinkFailed,
    OutOfMemory,
    StackOverflow,
    IllegalInstruction,
    MemoryAccessViolation
};

template<typename T>
struct Result {
    T value;
    RuntimeError error;
    
    bool is_ok() const { return error == RuntimeError::Success; }
    operator bool() const { return is_ok(); }
};
```

## 📚 Testing Strategy

### Unit Tests (Host Platform)
```cpp
// Test on host before deploying to hardware
TEST(Runtime, LoadModule) {
    FlightRuntime runtime;
    auto result = runtime.load_module(test_wasm);
    ASSERT_TRUE(result.is_ok());
}
```

### Hardware Testing
```cmake
# Build test ROM
add_custom_target(test_rom
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/build_test_rom.sh
    DEPENDS flight-runtime
)
```

## 🔐 Security Considerations

### Memory Isolation
```cpp
// Prevent escape from WASM sandbox
class MemoryValidator {
    bool validate_access(uint32_t addr, uint32_t size) {
        return addr + size <= memory_size;  // Prevent overflow
    }
    
    void trap_on_violation() {
        #ifdef DEBUG_BUILD
            debugger.break_here();
        #else
            platform->panic("Memory violation");
        #endif
    }
};
```

## 📖 Essential References

1. [wasm3 Architecture](https://github.com/wasm3/wasm3/blob/main/docs/Architecture.md)
2. [WebAssembly MVP Spec](https://webassembly.github.io/spec/core/)
3. [PSn00bSDK Documentation](https://github.com/Lameguy64/PSn00bSDK/tree/main/doc)
4. [KallistiOS (Dreamcast)](http://gamedev.allusion.net/docs/kos/)

Remember: Every cycle saved is another particle effect enabled!
