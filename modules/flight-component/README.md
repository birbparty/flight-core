# Flight Component Module

## Purpose and Scope

The `flight-component` module implements the WebAssembly Component Model for Flight. It provides types, operations, and the Canonical ABI necessary to support component-based WebAssembly applications with rich interface types and resource management.

## Public API Overview

### Core Types
- `Component`: Represents a WebAssembly component
- `Interface`: Defines component interfaces with typed imports/exports
- `Resource`: Managed resources with automatic cleanup
- `CanonicalABI`: Implementation of the Component Model ABI

### Type System
- `TypeKind`: Comprehensive type enumeration (primitives, strings, lists, records, variants, etc.)
- `InterfaceType`: Rich type definitions for component interfaces
- `ResourceHandle<T>`: Type-safe handles for resource management

## Usage Examples

```cpp
#include <flight/component/component.hpp>

using namespace flight::component;

// Define an interface type
InterfaceType stringType{TypeKind::String, "string", {}};
InterfaceType listType{TypeKind::List, "list", {stringType}};

// Future usage (once implementation is complete):
// auto component = Component::from_bytes(component_bytes);
// auto instance = component.instantiate();
// auto result = instance.call_export("process", {"hello", "world"});
```

## Dependencies

- `flight-wasm`: Core WebAssembly types
- `flight-runtime`: Execution engine for components

## Build Instructions

This module is built as part of the Flight project:

```bash
# From project root
cmake -B build
cmake --build build --target flight-component
```

To run tests:
```bash
cmake --build build --target flight-component-tests
ctest -R flight-component
```

## Architecture Notes

- Full Component Model type system implementation
- Canonical ABI for cross-language interoperability
- Resource lifetime management with RAII
- Support for interface composition and linking
- Efficient representation of interface types

## Component Model Features

- **Interface Types**: Rich type system beyond core WASM types
- **Resources**: First-class support for managed resources
- **Composition**: Link multiple components together
- **Cross-Language**: Seamless interop between different source languages
- **Type Safety**: Strong typing across component boundaries
