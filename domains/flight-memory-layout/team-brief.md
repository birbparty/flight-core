# Flight Memory Layout Team Brief

## 🎯 Mission

You are building the memory layout algorithms and utilities that enable safe merging of multiple WebAssembly component memory spaces into a single, unified memory layout. Your work is critical for allowing components written in different languages to coexist in the same memory space while maintaining isolation and safety.

## 🚀 What We Need From You

### 1. **Segmented Memory Allocator**
Build a sophisticated memory allocator that:
- Assigns non-overlapping memory segments to components
- Maintains configurable guard pages between segments
- Aligns segments to platform-specific boundaries
- Tracks ownership and access permissions per segment
- Supports both static and dynamic allocation strategies

### 2. **Memory Rebasing Engine**
Create utilities to rebase memory operations:
- Analyze all memory instructions in WASM modules
- Rewrite memory offsets to new base addresses
- Handle both direct and indirect memory access
- Preserve alignment requirements
- Generate relocation tables for debugging

### 3. **Shared Memory Protocols**
Implement safe shared memory regions:
- Define shared memory allocation API
- Implement access control lists
- Create synchronization primitives
- Handle platform-specific shared memory (PSX VRAM, etc.)
- Provide zero-copy inter-component communication

### 4. **Platform Memory Profiles**
Create memory layouts optimized for each platform:
- PSX: 2MB main RAM + 1MB VRAM layout
- PSP: 32/64MB with Media Engine RAM
- Dreamcast: 16MB main + 8MB VRAM
- Modern: Virtual memory with large address spaces
- Respect platform-specific memory regions

### 5. **Memory Safety Validation**
Build comprehensive validation tools:
- Static analysis of memory access patterns
- Runtime bounds checking (debug mode)
- Memory leak detection
- Fragmentation analysis
- Access violation detection

## 📋 Key Requirements

### Performance Targets
- Memory allocation: < 100 nanoseconds
- Rebasing operation: < 1ms per MB of code
- Zero overhead in release builds
- Minimal fragmentation (< 5% waste)

### Safety Requirements
- No component can access another's memory (unless shared)
- Guard pages must trap access violations
- All allocations must be aligned correctly
- Memory exhaustion handled gracefully

### Platform Constraints
- Must work with linear memory model
- Support 32-bit and 64-bit address spaces
- Handle both MMU and non-MMU systems
- Respect platform memory maps

## 🔧 Technical Constraints

1. **Language**: C++17 (header-only library preferred)
2. **Dependencies**: Minimal (only STL)
3. **Integration**: Must work with Binaryen
4. **Testing**: Comprehensive unit tests required

## 📊 Success Metrics

1. **Safety**: Zero memory corruption bugs
2. **Efficiency**: < 5% memory overhead
3. **Performance**: Meets all timing targets
4. **Compatibility**: Works on all target platforms

## 🎮 Example Use Case

A game has these components:
- Physics engine: needs 4MB workspace
- Renderer: needs 8MB for buffers
- Audio mixer: needs 2MB for samples
- Game logic: needs 1MB heap

Your system must:
1. Allocate these in a 16MB PSP memory space
2. Add guard pages between each
3. Rebase all memory operations
4. Provide shared buffer for physics→renderer

Result: Safe, efficient memory layout that maximizes available RAM.

## 🚨 Critical Path

Your work blocks:
- **Flight Component Flattener Team** (needs memory merging)
- **Flight Simple Runtime Team** (needs memory layout)
- **All platform teams** (need platform-specific layouts)

## 💡 Innovation Opportunities

1. **Smart Packing**: AI-driven memory layout optimization
2. **Compression**: Transparent memory compression for retro platforms
3. **Hot/Cold Separation**: Frequently accessed data in fast memory
4. **Predictive Allocation**: Pre-allocate based on usage patterns

## 📚 Key Algorithms to Research

1. **Buddy Allocator**: For power-of-2 allocations
2. **TLSF (Two-Level Segregated Fit)**: Real-time allocation
3. **Address Space Layout Randomization**: Security through randomization
4. **Memory Coloring**: Cache optimization through careful placement

## 🏗️ Architecture Guidelines

### Core Interface
```cpp
class MemoryLayout {
public:
    // Allocate segment for component
    Result<MemorySegment> allocate_segment(
        ComponentId id,
        size_t size,
        size_t alignment,
        MemoryFlags flags
    );
    
    // Create shared region
    Result<SharedRegion> create_shared_region(
        const std::vector<ComponentId>& sharers,
        size_t size,
        AccessPermissions perms
    );
    
    // Rebase module memory operations
    Result<void> rebase_module(
        BinaryenModuleRef module,
        const MemorySegment& segment
    );
    
    // Validate memory safety
    Result<ValidationReport> validate_layout() const;
};
```

### Memory Segment Structure
```cpp
struct MemorySegment {
    uint32_t base_address;
    uint32_t size;
    uint32_t guard_size;
    ComponentId owner;
    MemoryFlags flags;
    
    // Platform-specific attributes
    std::optional<VRAMRegion> vram_mapping;
    std::optional<DMAChannel> dma_access;
};
```

## 🤝 Collaboration

You'll work closely with:
- **Flight Component Flattener Team**: Primary consumer
- **Flight Memory Components Team**: For type definitions
- **Platform HAL Teams**: For platform constraints

## ⚠️ Special Considerations

1. **Retro Hardware**: Some platforms have no MMU
2. **Fixed Memory Maps**: PSX has hardcoded memory regions
3. **Alignment Hell**: Different platforms need different alignments
4. **DMA Requirements**: Some memory must be DMA-accessible

Remember: On a 2MB PSX, every byte counts. Your algorithms determine whether complex games are even possible on retro hardware!
