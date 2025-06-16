# Flight WASM Module

## Purpose and Scope

The `flight-wasm` module provides the foundational WebAssembly types, values, and instruction definitions for the Flight runtime. This module serves as the core building block for WebAssembly execution, defining the fundamental data structures and constants required by the runtime.

## Public API Overview

### Core Types
- `ValueType`: Enumeration of WebAssembly value types (i32, i64, f32, f64, v128, funcref, externref)
- `Value`: Representation of WebAssembly values
- `Type`: Type definitions for functions, tables, memories, and globals
- `Instruction`: WebAssembly instruction representations

### Constants
- `WASM_BINARY_MAGIC`: Magic number for WebAssembly binary format (0x6d736100)
- `WASM_BINARY_VERSION`: Current WebAssembly binary version (1)

## Usage Examples

```cpp
#include <flight/wasm/wasm.hpp>

using namespace flight::wasm;

// Check value type
ValueType type = ValueType::I32;

// Future usage (once implementation is complete):
// Value val = Value::from_i32(42);
// Instruction inst = Instruction::i32_const(42);
```

## Dependencies

This module has no dependencies on other Flight modules. It provides the base types used by:
- `flight-runtime`: Uses WASM types for execution
- `flight-component`: Extends WASM types for Component Model

## Build Instructions

This module is built as part of the Flight project:

```bash
# From project root
cmake -B build
cmake --build build --target flight-wasm
```

To run tests:
```bash
cmake --build build --target flight-wasm-tests
ctest -R flight-wasm
```

## Architecture Notes

- Header-only types for zero-overhead abstractions
- Platform-independent implementations
- No external dependencies for maximum portability
- Designed for embedded systems with minimal memory footprint
