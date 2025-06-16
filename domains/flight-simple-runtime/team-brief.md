# Flight Simple Runtime Team Brief

## 🎯 Mission

You are building a lightweight WebAssembly runtime based on wasm3 that executes flattened WASM modules on retro hardware. This runtime replaces the complex Component Model runtime with a simple, efficient interpreter that can run on platforms with as little as 2MB of RAM.

## 🚀 What We Need From You

### 1. **wasm3 Integration**
Embed and configure wasm3 for optimal performance:
- Integrate wasm3 as the core interpreter
- Configure for minimal memory usage
- Optimize for target platform constraints
- Handle platform-specific quirks (endianness, alignment)
- Provide clean C++ wrapper API

### 2. **Host Function Bridge**
Create efficient host function bindings:
- Map HAL interfaces to WASM imports
- Implement fast function dispatch
- Handle type conversions (no complex lifting/lowering)
- Support both sync and async operations
- Minimize call overhead

### 3. **Memory Management Integration**
Work with flattened memory layouts:
- Initialize memory segments from layout
- Set up guard pages (where supported)
- Handle shared memory regions
- Implement memory access validation (debug mode)
- Support platform-specific memory (VRAM, scratchpad)

### 4. **Platform HAL Integration**
Seamlessly integrate with Flight Core HAL:
- Use HAL drivers for all platform operations
- Map WASI-like imports to HAL calls
- Handle platform capabilities gracefully
- Support all target platforms (PSX to modern)

### 5. **Minimal Runtime API**
Provide a simple, efficient API:
```cpp
class FlightRuntime {
    Result<void> load_module(std::span<const uint8_t> wasm);
    Result<void> run();
    Result<Value> call_export(const std::string& name, std::span<const Value> args);
};
```

## 📋 Key Requirements

### Performance Targets
- Startup time: < 100ms on PSP
- Function call overhead: < 1μs
- Memory overhead: < 500KB
- Zero heap allocations after init

### Size Constraints
- Runtime binary: < 200KB
- Memory footprint: < 1MB total
- Works in 2MB PSX environment

### Compatibility Requirements
- Execute flattened WASM modules
- Support WASM MVP features only
- Handle platform differences transparently
- No threading, no exceptions

## 🔧 Technical Constraints

1. **Language**: C++14 (for maximum compatibility)
2. **Dependencies**: 
   - wasm3 (embedded)
   - Flight Core HAL
   - No STL on some platforms
3. **Platforms**: All Flight Core targets

## 📊 Success Metrics

1. **Performance**: Runs games at 60fps on PSP
2. **Size**: Fits in retro hardware constraints
3. **Reliability**: Zero crashes, deterministic execution
4. **Compatibility**: Runs all flattened modules

## 🎮 Example Use Case

A flattened game module needs:
- Memory initialization (15MB on PSP)
- Host functions for graphics/audio
- 60Hz game loop
- Input handling

Your runtime must:
1. Load the module in < 100ms
2. Set up memory layout
3. Link host functions
4. Run game loop with minimal overhead

## 🚨 Critical Path

Your work blocks:
- **All game developers** (need runtime to test)
- **Platform teams** (need runtime for validation)
- **Debug tool developers** (need runtime hooks)

## 💡 Innovation Opportunities

1. **JIT Compilation**: Selective hot-path compilation
2. **Instruction Caching**: Reuse decoded instructions
3. **Fast Dispatch**: Optimized function tables
4. **Memory Prefetching**: Predict access patterns

## 🏗️ Architecture Guidelines

### Core Runtime Structure
```cpp
class SimplifiedRuntime {
private:
    // wasm3 core
    IM3Environment env;
    IM3Runtime runtime;
    IM3Module module;
    
    // Memory layout
    MemoryLayout memory_layout;
    
    // Host functions
    HostFunctionRegistry host_functions;
    
    // Platform HAL
    std::unique_ptr<Platform> platform;
    
public:
    // Simple API
    Result<void> initialize(const RuntimeConfig& config);
    Result<void> load_module(std::span<const uint8_t> wasm);
    Result<void> link_host_functions();
    Result<void> run();
};
```

### Host Function Registration
```cpp
struct HostFunction {
    const char* module_name;
    const char* function_name;
    const char* signature;
    M3RawCall implementation;
};

// Example: Graphics function
m3ApiRawFunction(draw_sprite) {
    m3ApiGetArg(uint32_t, x);
    m3ApiGetArg(uint32_t, y);
    m3ApiGetArg(uint32_t, sprite_id);
    
    // Direct HAL call
    g_platform->video()->draw_sprite(x, y, sprite_id);
    
    m3ApiSuccess();
}
```

### Memory Setup
```cpp
class RuntimeMemory {
    uint8_t* base_address;
    size_t total_size;
    std::vector<MemorySegment> segments;
    
public:
    void setup_from_layout(const MemoryLayout& layout);
    void* get_host_pointer(uint32_t wasm_addr);
    bool validate_access(uint32_t addr, size_t size);
};
```

## 🤝 Collaboration

You'll work closely with:
- **Flight Component Flattener Team**: Consume their output
- **Flight Memory Layout Team**: Use their memory setup
- **Platform HAL teams**: Integrate with their drivers

## ⚠️ Special Considerations

### Retro Platform Quirks
1. **PSX**: No floating point unit
2. **Dreamcast**: Different endianness
3. **PSP**: Separate Media Engine memory
4. **GBA**: Thumb instruction set

### wasm3 Configuration
```c
#define M3_COMPILE_OPCODES 1
#define M3_MAX_FUNCTION_STACK_HEIGHT 128
#define M3_OPTIMIZE_WASM 1
#define M3_USE_OPTIMIZATION_PASSES 1
#define M3_LITTLE_ENDIAN 1  // Platform specific
```

### Debug Support
In debug builds, provide:
- Function call tracing
- Memory access logging
- Performance counters
- Stack traces on error

## 📚 Key References

1. [wasm3 Documentation](https://github.com/wasm3/wasm3)
2. [WebAssembly MVP Spec](https://webassembly.github.io/spec/core/)
3. Flight Core HAL Documentation
4. Platform-specific guides

Remember: This runtime enables modern game development on 30-year-old hardware. Every instruction counts!
