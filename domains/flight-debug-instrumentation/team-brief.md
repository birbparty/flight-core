# Flight Debug Instrumentation Team Brief

## 🎯 Mission

You are building the debug instrumentation system that provides visibility into flattened WASM modules during development while compiling to zero-overhead no-ops in production. Your system must preserve component boundaries, track execution flow, and provide actionable debugging information without impacting release performance.

## 🚀 What We Need From You

### 1. **Debug Trace Injection**
Build instrumentation that injects into flattened modules:
- Function entry/exit tracking
- Memory access logging
- Component boundary preservation
- Call stack maintenance
- Performance timing

### 2. **Release Mode Optimization**
Ensure zero overhead in production:
- Replace debug calls with no-ops
- Compile-time elimination
- No binary size bloat
- Predictable performance
- Verifiable removal

### 3. **Component-Aware Debugging**
Maintain component context after flattening:
- Component name tracking
- Original function names
- Source location mapping
- Inter-component call tracking
- Component state inspection

### 4. **Debug Information Generation**
Create rich debugging metadata:
- Source maps for flattened code
- Symbol tables with component info
- Debug data sections in WASM
- Debugging protocol support
- IDE integration formats

### 5. **Runtime Debug Hooks**
Provide runtime debugging capabilities:
- Breakpoint support
- Step debugging across components
- Memory inspection
- Variable watching
- Performance profiling

## 📋 Key Requirements

### Debug Mode Features
- Function call tracing with component context
- Memory access tracking per component
- Timing information for performance analysis
- State snapshots at component boundaries
- Error context with full stack traces

### Release Mode Guarantees
- Zero runtime overhead
- No additional memory usage
- Identical behavior to non-instrumented
- Verifiable through binary comparison
- No security information leaks

### Developer Experience
- Clear, actionable debug output
- Integration with common debuggers
- Visual Studio Code extension support
- Command-line debugging tools
- Real-time performance visualization

## 🔧 Technical Constraints

1. **Language**: C++17 for instrumentation engine
2. **Integration**: Works with Binaryen
3. **Formats**: DWARF, source maps, custom
4. **Performance**: < 10% overhead in debug mode

## 📊 Success Metrics

1. **Adoption**: Used by all Flight Core developers
2. **Performance**: Zero overhead in release
3. **Debugging**: 90% faster bug resolution
4. **Coverage**: All components fully traceable

## 🎮 Example Use Case

Developer debugging a physics bug:
1. Builds with `--debug` flag
2. Instrumentation injects trace points
3. Runs game, bug occurs
4. Debug output shows:
   - Component call sequence
   - Memory access patterns
   - Exact failure point
   - Component state at crash
5. Developer fixes bug using precise information

## 🚨 Critical Path

Your work enables:
- **Developers** to debug complex component interactions
- **QA teams** to provide detailed bug reports
- **Performance teams** to identify bottlenecks
- **Platform teams** to diagnose platform-specific issues

## 💡 Innovation Opportunities

1. **Time-Travel Debugging**: Record and replay execution
2. **Differential Debugging**: Compare debug vs release behavior
3. **AI-Assisted Debugging**: Pattern recognition in traces
4. **Remote Debugging**: Debug on actual hardware

## 🏗️ Architecture Guidelines

### Instrumentation Engine
```cpp
class DebugInstrumenter {
private:
    struct InstrumentationContext {
        ComponentInfo component;
        FunctionInfo function;
        DebugConfig config;
    };
    
public:
    Result<void> instrument_module(
        BinaryenModuleRef module,
        const FlattenerMetadata& metadata,
        const DebugConfig& config
    );
    
    Result<void> add_function_instrumentation(
        BinaryenFunctionRef func,
        const InstrumentationContext& ctx
    );
    
    Result<void> add_memory_instrumentation(
        BinaryenExpressionRef expr,
        const InstrumentationContext& ctx
    );
    
    Result<void> generate_debug_info(
        const InstrumentedModule& module,
        DebugInfoFormat format
    );
};
```

### Debug Function Templates
```cpp
// Debug mode implementations
namespace debug {
    inline void trace_enter(const char* component, const char* function) {
        g_debugger->on_function_enter(component, function);
    }
    
    inline void trace_exit(const char* component, const char* function) {
        g_debugger->on_function_exit(component, function);
    }
    
    inline void trace_memory_read(uint32_t addr, uint32_t size, const char* component) {
        g_debugger->on_memory_read(addr, size, component);
    }
}

// Release mode no-ops
namespace release {
    inline void trace_enter(const char*, const char*) {}
    inline void trace_exit(const char*, const char*) {}
    inline void trace_memory_read(uint32_t, uint32_t, const char*) {}
}
```

### Debug Information Format
```cpp
struct DebugInfo {
    struct ComponentMapping {
        std::string name;
        uint32_t start_offset;
        uint32_t end_offset;
        std::vector<FunctionMapping> functions;
    };
    
    struct FunctionMapping {
        std::string original_name;
        uint32_t wasm_offset;
        std::string source_file;
        uint32_t source_line;
    };
    
    std::vector<ComponentMapping> components;
    std::unordered_map<uint32_t, SourceLocation> pc_to_source;
    std::vector<std::string> source_files;
};
```

### Runtime Debug Protocol
```cpp
class DebuggerInterface {
public:
    // Breakpoint management
    virtual void set_breakpoint(uint32_t address) = 0;
    virtual void remove_breakpoint(uint32_t address) = 0;
    
    // Execution control
    virtual void step_into() = 0;
    virtual void step_over() = 0;
    virtual void step_out() = 0;
    virtual void continue_execution() = 0;
    
    // Inspection
    virtual CallStack get_call_stack() = 0;
    virtual ComponentState inspect_component(const std::string& name) = 0;
    virtual MemoryDump dump_memory(uint32_t addr, uint32_t size) = 0;
    
    // Performance
    virtual ProfileData get_profile_data() = 0;
};
```

## 🤝 Collaboration

You'll work closely with:
- **Flight Component Flattener Team**: Preserve component info
- **Flight Simple Runtime Team**: Integrate debug hooks
- **Developer Experience Team**: IDE integration
- **Platform teams**: Platform-specific debugging

## ⚠️ Special Considerations

### Platform-Specific Debugging
1. **PSX**: Limited memory for debug info
2. **Modern**: Full featured debugging
3. **Web**: Browser DevTools integration
4. **Mobile**: Remote debugging support

### Security Considerations
- Strip all debug info from release builds
- No function names in production
- No source paths in shipped games
- Obfuscate component boundaries

### Performance Impact
- Measure debug overhead precisely
- Provide levels of instrumentation
- Allow selective component debugging
- Minimize memory allocation

## 📚 Key Technologies to Research

1. **Debug Formats**: DWARF, PDB, source maps
2. **Instrumentation**: DynamoRIO, Pin, Frida
3. **Profiling**: Tracy, Optick, Superluminal
4. **Time-Travel**: rr, UDB, TTD
5. **Protocols**: DAP, Chrome DevTools Protocol

Remember: Great debugging tools turn mysterious crashes into obvious fixes!
