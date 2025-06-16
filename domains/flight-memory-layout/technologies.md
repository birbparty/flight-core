# Flight Memory Layout Technology Stack

## 🔧 Core Technologies

### Programming Language: C++17 (Header-Only)
**Why Header-Only:**
- Zero link-time dependencies
- Easy integration into any project
- Template-based for compile-time optimization
- Single-header amalgamation option

**Key C++17 Features:**
- `constexpr` for compile-time layout calculation
- `std::optional` for nullable segments
- `std::variant` for platform-specific attributes
- `if constexpr` for platform branching
- Class template argument deduction (CTAD)

## 🧮 Memory Allocation Algorithms

### Primary: Two-Level Segregated Fit (TLSF)
**Why TLSF:**
- O(1) allocation and deallocation
- Bounded fragmentation (< 25% worst case)
- Deterministic performance
- Perfect for real-time systems

**Implementation:**
```cpp
template<size_t MaxSize = 1024*1024*1024>
class TLSFAllocator {
    static constexpr size_t FL_INDEX_MAX = 30;
    static constexpr size_t SL_INDEX_COUNT = 32;
    
    struct Block {
        size_t size;
        Block* next_free;
        Block* prev_free;
        Block* next_phys;
        Block* prev_phys;
    };
    
    Block* blocks[FL_INDEX_MAX][SL_INDEX_COUNT];
    
public:
    constexpr void* allocate(size_t size, size_t align);
    constexpr void deallocate(void* ptr);
};
```

### Secondary: Buddy Allocator
**Use Cases:**
- Power-of-2 sized allocations
- Simple implementation for constrained platforms
- Good for VRAM allocation

### Fallback: Linear Allocator
**Use Cases:**
- Initialization phase
- Temporary allocations
- Stack-like allocation patterns

## 🏗️ Memory Layout Strategies

### 1. **First-Fit with Guard Pages**
```cpp
struct GuardedLayout {
    static constexpr size_t GUARD_SIZE = 4096;
    
    struct Segment {
        uintptr_t base;
        size_t size;
        uintptr_t guard_start;
    };
    
    std::vector<Segment> layout_components(
        const std::vector<ComponentMemoryReq>& reqs);
};
```

### 2. **Best-Fit Packing**
**Algorithm:** Modified bin packing
```cpp
class BestFitPacker {
    struct Bin {
        size_t capacity;
        size_t used;
        std::vector<ComponentId> contents;
    };
    
    std::vector<Bin> pack(
        const std::vector<ComponentMemoryReq>& components,
        size_t bin_size);
};
```

### 3. **Cache-Aware Layout**
**Purpose:** Optimize for cache performance
```cpp
class CacheAwareLayout {
    static constexpr size_t CACHE_LINE = 64;
    static constexpr size_t L1_SIZE = 32 * 1024;
    
    void color_memory(MemorySegment& segment);
    void separate_hot_cold(std::vector<MemoryRegion>& regions);
};
```

## 🔄 Memory Rebasing Engine

### Binaryen Integration
```cpp
#include <binaryen-c.h>

class MemoryRebaseEngine {
    BinaryenModuleRef module;
    
public:
    void rebase_memory_instructions(
        uint32_t old_base,
        uint32_t new_base
    );
    
    void update_data_segments(
        const MemorySegment& segment
    );
    
    void inject_bounds_checks(
        const MemorySegment& segment,
        bool debug_mode
    );
};
```

### Instruction Pattern Matching
```cpp
enum class MemoryInstruction {
    I32Load, I64Load, F32Load, F64Load,
    I32Store, I64Store, F32Store, F64Store,
    I32Load8S, I32Load8U, I32Load16S, I32Load16U,
    // ... etc
};

class InstructionRewriter {
    std::unordered_map<MemoryInstruction, RewriteRule> rules;
    
    void rewrite_instruction(
        BinaryenExpressionRef expr,
        int32_t offset_delta
    );
};
```

## 🛡️ Safety Mechanisms

### Static Analysis
```cpp
class MemoryAccessAnalyzer {
    struct AccessPattern {
        uint32_t min_offset;
        uint32_t max_offset;
        bool is_bounded;
        bool is_linear;
    };
    
    AccessPattern analyze_function(
        BinaryenFunctionRef func
    );
    
    bool validate_safety(
        const AccessPattern& pattern,
        const MemorySegment& segment
    );
};
```

### Runtime Bounds Checking
```cpp
template<bool DebugMode>
class BoundsChecker {
    inline bool check_access(
        uint32_t addr,
        uint32_t size,
        const MemorySegment& segment
    ) {
        if constexpr (DebugMode) {
            return addr >= segment.base && 
                   (addr + size) <= (segment.base + segment.size);
        }
        return true; // No-op in release
    }
};
```

## 📊 Platform-Specific Implementations

### PSX Memory Layout
```cpp
namespace PSX {
    constexpr uint32_t MAIN_RAM_START = 0x80000000;
    constexpr uint32_t MAIN_RAM_SIZE = 2 * 1024 * 1024;
    constexpr uint32_t SCRATCHPAD_START = 0x1F800000;
    constexpr uint32_t SCRATCHPAD_SIZE = 1024;
    constexpr uint32_t VRAM_START = 0x00000000;
    constexpr uint32_t VRAM_SIZE = 1024 * 1024;
    
    class PSXMemoryLayout {
        // Specialized allocator for 2MB constraint
        using Allocator = LinearAllocator<MAIN_RAM_SIZE>;
    };
}
```

### Modern Platform Layout
```cpp
namespace Modern {
    class VirtualMemoryLayout {
        // Use OS virtual memory APIs
        void* reserve_address_space(size_t size);
        void commit_memory(void* addr, size_t size);
        void add_guard_page(void* addr);
        
        // ASLR support
        uintptr_t randomize_base_address();
    };
}
```

## 🧪 Testing Infrastructure

### Property-Based Testing
```cpp
// Using rapidcheck or similar
RC_GTEST_PROP(MemoryLayout, NoOverlap, 
    (std::vector<size_t> sizes)) {
    
    MemoryLayout layout;
    auto segments = layout.allocate_segments(sizes);
    
    // Check no overlaps
    for (size_t i = 0; i < segments.size(); ++i) {
        for (size_t j = i + 1; j < segments.size(); ++j) {
            RC_ASSERT(!segments_overlap(segments[i], segments[j]));
        }
    }
}
```

### Fuzzing Support
```cpp
extern "C" int LLVMFuzzerTestOneInput(
    const uint8_t* data, 
    size_t size) {
    
    // Parse allocation requests from fuzz input
    std::vector<AllocationRequest> requests = 
        parse_requests(data, size);
    
    // Try to allocate
    MemoryLayout layout;
    try {
        layout.process_requests(requests);
    } catch (...) {
        // Should handle gracefully
    }
    
    return 0;
}
```

## 🚀 Optimization Techniques

### Compile-Time Layout
```cpp
template<size_t... ComponentSizes>
class StaticLayout {
    static constexpr size_t total_size = 
        (ComponentSizes + ... + 0);
    
    static constexpr auto calculate_offsets() {
        std::array<size_t, sizeof...(ComponentSizes)> offsets{};
        // Compile-time offset calculation
        return offsets;
    }
    
public:
    static constexpr auto offsets = calculate_offsets();
};
```

### SIMD Memory Operations
```cpp
#include <immintrin.h>

class SIMDMemoryOps {
    void fast_clear(void* ptr, size_t size) {
        __m256i zero = _mm256_setzero_si256();
        __m256i* dest = static_cast<__m256i*>(ptr);
        
        for (size_t i = 0; i < size / 32; ++i) {
            _mm256_stream_si256(&dest[i], zero);
        }
    }
};
```

## 📚 Key References

### Academic Papers
1. **"TLSF: A New Dynamic Memory Allocator for Real-Time Systems"** - Masmano et al.
2. **"Dynamic Storage Allocation: A Survey and Critical Review"** - Wilson et al.
3. **"Memory Allocation for Long-Running Server Applications"** - Berger et al.

### Implementation References
1. **jemalloc** - Advanced allocation strategies
2. **dlmalloc** - Doug Lea's allocator
3. **mimalloc** - Microsoft's allocator
4. **PSn00bSDK** - PSX memory management

## 🔐 Security Considerations

### Guard Page Implementation
```cpp
class GuardPageManager {
    // Platform-specific guard page setup
    #ifdef _WIN32
        void setup_guard_page(void* addr) {
            DWORD old;
            VirtualProtect(addr, 4096, PAGE_NOACCESS, &old);
        }
    #else
        void setup_guard_page(void* addr) {
            mprotect(addr, 4096, PROT_NONE);
        }
    #endif
};
```

### Address Space Layout
```cpp
class ASLRLayout {
    std::random_device rd;
    std::mt19937 gen{rd()};
    
    uintptr_t randomize_offset(
        uintptr_t base,
        size_t range
    ) {
        std::uniform_int_distribution<> dis(0, range);
        return base + (dis(gen) & ~0xFFF); // Page aligned
    }
};
```

## 🎯 Best Practices

1. **Always use `constexpr`** where possible for compile-time optimization
2. **Template on allocation size** when size is known at compile time
3. **Provide both debug and release** variants with zero overhead
4. **Test with address sanitizer** and valgrind
5. **Profile on actual hardware** - emulators lie about performance
6. **Document memory layouts** with clear diagrams

Remember: On retro hardware, every byte and every cycle counts. The difference between 30fps and 60fps might be your memory layout!
