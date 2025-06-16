# Flight Debug Instrumentation Technology Stack

## 🔧 Core Technologies

### Programming Language: C++17
**Why C++17:**
- Direct integration with Binaryen
- Zero-overhead abstractions
- Compile-time debug/release switching
- Template metaprogramming for instrumentation
- Compatible with all platforms

**Key C++17 Features:**
- `if constexpr` for compile-time debug elimination
- `std::string_view` for efficient string handling
- Fold expressions for variadic templates
- Structured bindings
- `[[maybe_unused]]` for debug-only parameters

## 🏗️ WASM Instrumentation

### Binaryen
**Purpose:** Core WASM manipulation engine

**Key APIs:**
```cpp
#include <binaryen-c.h>
#include <ir/module-utils.h>
#include <pass.h>
#include <wasm-builder.h>

// Custom instrumentation pass
class DebugInstrumentationPass : public WalkerPass<PostWalker<DebugInstrumentationPass>> {
    void visitCall(Call* curr) {
        // Inject debug tracing
        injectFunctionTrace(curr);
    }
    
    void visitLoad(Load* curr) {
        // Track memory reads
        injectMemoryTrace(curr, false);
    }
    
    void visitStore(Store* curr) {
        // Track memory writes
        injectMemoryTrace(curr, true);
    }
};
```

### WASM Binary Format
```cpp
// Direct binary manipulation for minimal overhead
class WasmBinaryInstrumenter {
    void inject_debug_section(std::vector<uint8_t>& wasm) {
        // Custom debug section format
        DebugSection debug;
        debug.add_component_map(components);
        debug.add_source_map(sources);
        
        // Inject as custom section
        insert_custom_section(wasm, "flight_debug", debug.serialize());
    }
};
```

## 🛡️ Debug/Release Mode Switching

### Compile-Time Elimination
```cpp
template<bool IsDebug>
class Instrumenter {
    static constexpr bool debug_mode = IsDebug;
    
    void instrument_function(BinaryenFunctionRef func) {
        if constexpr (debug_mode) {
            // Full instrumentation
            add_entry_trace(func);
            add_exit_trace(func);
            add_timing(func);
        } else {
            // No-op in release
        }
    }
};

// Specialized for release mode
template<>
class Instrumenter<false> {
    void instrument_function(BinaryenFunctionRef) {}
};
```

### Macro-Based Switching
```cpp
#ifdef DEBUG_BUILD
    #define DEBUG_TRACE(component, func) \
        debug::trace_enter(component, func)
    #define DEBUG_TRACE_EXIT(component, func) \
        debug::trace_exit(component, func)
#else
    #define DEBUG_TRACE(component, func) ((void)0)
    #define DEBUG_TRACE_EXIT(component, func) ((void)0)
#endif
```

## 📊 Debug Information Formats

### Source Maps
**Purpose:** Map flattened code to original components

```cpp
#include <source_map.h>

class SourceMapGenerator {
    sourcemap::SourceMap map;
    
public:
    void add_mapping(
        uint32_t generated_line,
        uint32_t generated_column,
        const std::string& source_file,
        uint32_t original_line,
        uint32_t original_column,
        const std::string& component_name
    ) {
        map.addMapping(
            generated_line, generated_column,
            source_file,
            original_line, original_column,
            component_name
        );
    }
    
    std::string serialize() {
        return map.toString();
    }
};
```

### DWARF Debug Information
**Purpose:** Full debugging support on native platforms

```cpp
#include <llvm/DebugInfo/DWARF/DWARFContext.h>

class DWARFGenerator {
    void generate_debug_info(
        const InstrumentedModule& module,
        llvm::MCContext& context
    ) {
        // Generate .debug_info section
        auto* debug_info = create_debug_info_section(context);
        
        // Add compilation units per component
        for (const auto& component : module.components) {
            add_compilation_unit(debug_info, component);
        }
        
        // Generate .debug_line for source mapping
        generate_line_table(module);
    }
};
```

### Custom Flight Debug Format
```cpp
// Compact format for retro platforms
struct FlightDebugFormat {
    struct Header {
        uint32_t magic = 0x464C4442; // 'FLDB'
        uint16_t version = 1;
        uint16_t flags;
        uint32_t component_count;
        uint32_t function_count;
    };
    
    struct ComponentInfo {
        uint32_t name_offset;
        uint32_t start_pc;
        uint32_t end_pc;
        uint16_t function_count;
    };
    
    struct FunctionInfo {
        uint32_t name_offset;
        uint32_t start_pc;
        uint16_t component_index;
    };
};
```

## 🔍 Runtime Debug Support

### Debug API Interface
```cpp
class IDebugger {
public:
    // Function tracing
    virtual void on_function_enter(
        const char* component,
        const char* function,
        uint32_t pc
    ) = 0;
    
    virtual void on_function_exit(
        const char* component,
        const char* function,
        uint32_t pc,
        uint64_t duration_ns
    ) = 0;
    
    // Memory tracking
    virtual void on_memory_access(
        uint32_t address,
        uint32_t size,
        bool is_write,
        const char* component
    ) = 0;
    
    // Breakpoints
    virtual bool should_break(uint32_t pc) = 0;
};
```

### Minimal Debug Runtime
```cpp
// Lightweight debugger for embedded platforms
class MinimalDebugger : public IDebugger {
    struct CallFrame {
        const char* component;
        const char* function;
        uint64_t enter_time;
    };
    
    // Fixed-size stack for embedded
    std::array<CallFrame, 32> call_stack;
    uint8_t stack_depth = 0;
    
    // Simple ring buffer for traces
    RingBuffer<TraceEvent, 1024> traces;
    
public:
    void on_function_enter(...) override {
        if (stack_depth < call_stack.size()) {
            call_stack[stack_depth++] = {
                component, function, 
                get_time_ns()
            };
        }
    }
};
```

## 🚀 Performance Profiling

### Sampling Profiler
```cpp
class SamplingProfiler {
    struct Sample {
        uint32_t pc;
        const char* component;
        const char* function;
        uint64_t timestamp;
    };
    
    CircularBuffer<Sample, 10000> samples;
    
    void take_sample() {
        auto pc = get_current_pc();
        auto [component, function] = 
            debug_info.lookup(pc);
        
        samples.push({
            pc, component, function,
            get_time_ns()
        });
    }
};
```

### Instrumentation-Based Profiler
```cpp
class InstrumentationProfiler {
    struct FunctionStats {
        uint64_t call_count = 0;
        uint64_t total_time = 0;
        uint64_t min_time = UINT64_MAX;
        uint64_t max_time = 0;
    };
    
    std::unordered_map<std::string, FunctionStats> stats;
    
    void record_function_time(
        const std::string& key,
        uint64_t duration
    ) {
        auto& stat = stats[key];
        stat.call_count++;
        stat.total_time += duration;
        stat.min_time = std::min(stat.min_time, duration);
        stat.max_time = std::max(stat.max_time, duration);
    }
};
```

## 🔗 IDE Integration

### Debug Adapter Protocol (DAP)
```cpp
#include <dap/protocol.h>
#include <dap/session.h>

class FlightDebugAdapter : public dap::Session {
    void on_initialize(const dap::InitializeRequest& req) {
        // Return capabilities
        dap::InitializeResponse response;
        response.capabilities.supportsConfigurationDoneRequest = true;
        response.capabilities.supportsSetVariable = false;
        response.capabilities.supportsStepInTargetsRequest = true;
        send(response);
    }
    
    void on_set_breakpoints(const dap::SetBreakpointsRequest& req) {
        // Map source lines to WASM PCs
        for (const auto& bp : req.breakpoints) {
            auto pc = source_map.line_to_pc(
                req.source.path, bp.line
            );
            runtime->set_breakpoint(pc);
        }
    }
};
```

### VS Code Extension Integration
```typescript
// Extension API for Flight debugging
export interface FlightDebugSession {
    // Component-aware stepping
    stepIntoComponent(): Promise<void>;
    stepOutOfComponent(): Promise<void>;
    
    // Memory inspection
    inspectComponentMemory(component: string): Promise<MemoryView>;
    
    // Performance
    getProfileData(): Promise<ProfileData>;
}
```

## 📚 Testing & Validation

### Instrumentation Testing
```cpp
TEST(Instrumentation, ZeroOverheadRelease) {
    // Compile same module in debug and release
    auto debug_module = compile_with_debug(source);
    auto release_module = compile_release(source);
    
    // Binary should be identical
    EXPECT_EQ(
        get_code_section(release_module),
        get_code_section(uninstrumented_module)
    );
}

TEST(Instrumentation, DebugPreservesSemantics) {
    // Run with and without instrumentation
    auto normal_result = run_uninstrumented(input);
    auto debug_result = run_instrumented(input);
    
    // Results must match
    EXPECT_EQ(normal_result, debug_result);
}
```

## 🔐 Security Considerations

### Debug Info Stripping
```cpp
class DebugStripper {
    void strip_debug_info(BinaryenModuleRef module) {
        // Remove all debug custom sections
        remove_custom_section(module, "flight_debug");
        remove_custom_section(module, "sourceMappingURL");
        remove_custom_section(module, "name");
        
        // Strip function names
        anonymize_functions(module);
        
        // Remove any debug imports
        remove_debug_imports(module);
    }
};
```

## 📖 Essential References

1. [Binaryen C++ API](https://github.com/WebAssembly/binaryen/wiki/Binaryen-C++-API)
2. [DWARF 5 Standard](https://dwarfstd.org/dwarf5std.html)
3. [Source Map Spec](https://sourcemaps.info/spec.html)
4. [Debug Adapter Protocol](https://microsoft.github.io/debug-adapter-protocol/)
5. [WASM Custom Sections](https://webassembly.github.io/spec/core/binary/modules.html#custom-section)

Remember: The best debugger is invisible in production and indispensable in development!
