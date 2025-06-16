# Flight Component Flattener Team Brief

## 🎯 Mission

You are building the core tool that transforms multiple WebAssembly Component Model components into a single, optimized core WASM binary. This tool is the cornerstone of Flight Core's new architecture, enabling us to run complex component-based applications on retro hardware (PSX, Dreamcast, PSP) with minimal runtime overhead.

## 🚀 What We Need From You

### 1. **Component Parser and Analyzer**
Build a robust parser that can:
- Parse WebAssembly Component Model binaries
- Extract core modules from components
- Build a complete dependency graph
- Identify all imports/exports relationships
- Validate component compatibility

### 2. **Dependency Resolution Engine**
Create a sophisticated dependency resolver that:
- Performs topological sorting of components
- Detects circular dependencies
- Resolves export conflicts using configurable strategies:
  - Namespacing (prefix exports with component names)
  - Priority-based resolution
  - Error on conflict
  - Smart merging for compatible functions

### 3. **Memory Layout Merger**
Implement safe memory merging that:
- Assigns non-overlapping memory segments to each component
- Adds guard pages between segments for safety
- Rebases all memory operations to new offsets
- Supports shared memory regions for inter-component communication
- Maintains memory isolation in debug builds

### 4. **Code Generation and Optimization**
Build the flattening engine that:
- Merges all components into a single module
- Inlines inter-component calls
- Generates efficient trampolines for remaining indirections
- Applies cross-component optimizations
- Preserves debug information when requested

### 5. **Build-Time Toolchain**
Create a complete command-line tool:
```bash
flight-flatten \
  --components physics.wasm,renderer.wasm,audio.wasm \
  --manifest game.yaml \
  --output game.wasm \
  --optimize-level 3 \
  --debug-info
```

## 📋 Key Requirements

### Performance Targets
- Flattening time: < 5 seconds for typical game (10-20 components)
- Output size: No more than 5% overhead vs hand-written module
- Zero runtime overhead for inter-component calls

### Compatibility Requirements
- Support WebAssembly Component Model spec
- Work with components from any language (Rust, C++, AssemblyScript, etc.)
- Handle components with different memory models
- Support both WASI Preview 1 and Preview 2 components

### Debug Support
- Preserve component boundaries in debug builds
- Generate source maps for debugging
- Insert trace points at component boundaries
- Support incremental rebuilds for fast iteration

## 🔧 Technical Constraints

1. **Language**: C++17 (for maximum portability)
2. **Dependencies**: 
   - Binaryen (for WASM manipulation)
   - wasm-tools (for component extraction)
   - YAML-CPP (for configuration)
3. **Binary Size**: Tool itself must be < 50MB
4. **Platform Support**: Must run on Linux, macOS, Windows

## 📊 Success Metrics

1. **Correctness**: Flattened modules pass all component tests
2. **Performance**: 10x faster than runtime component linking
3. **Size**: Output modules fit in retro hardware constraints
4. **Adoption**: Used by all Flight Core game developers

## 🎮 Example Use Case

A game developer creates:
- `physics.wasm` - Physics engine (Rust)
- `renderer.wasm` - Graphics renderer (C++)
- `audio.wasm` - Audio mixer (AssemblyScript)
- `game_logic.wasm` - Game rules (Rust)

Your tool combines these into a single `game.wasm` that:
- Runs on a PSP with 32MB RAM
- Has zero overhead for physics→renderer calls
- Maintains type safety across language boundaries
- Can be debugged with component awareness

## 🚨 Critical Path

This tool blocks:
- Flight Simple Runtime Team (needs flattened modules)
- Flight Build Pipeline Team (needs flattening integration)
- All game development on Flight Core

## 💡 Innovation Opportunities

1. **Predictive Optimization**: Analyze call patterns to optimize hot paths
2. **Automatic Parallelization**: Identify independent components for parallel execution
3. **Size-Optimized Variants**: Generate platform-specific builds (PSX vs modern)
4. **Component Fusion**: Merge compatible components automatically

## 📚 Resources

- [WebAssembly Component Model Spec](https://github.com/WebAssembly/component-model)
- [Binaryen C++ API](https://github.com/WebAssembly/binaryen)
- [wasm-tools Documentation](https://github.com/bytecodealliance/wasm-tools)
- Flight Core Architecture Docs (see `/docs/`)

## 🤝 Collaboration

You'll work closely with:
- **Flight Memory Layout Team**: For memory merging algorithms
- **Flight Debug Instrumentation Team**: For debug support
- **Flight Build Pipeline Team**: For toolchain integration

Remember: This tool enables the dream of modern component-based development on retro hardware. Every optimization matters when targeting a 2MB PSX!
