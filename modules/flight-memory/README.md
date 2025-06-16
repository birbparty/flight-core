# Flight Memory Module

## Purpose and Scope

The `flight-memory` module provides platform-independent memory management for Flight. It offers various allocation strategies optimized for different use cases, from high-performance linear allocation to flexible pool-based systems, all while maintaining minimal overhead for embedded platforms.

## Public API Overview

### Allocators
- `Allocator`: Base interface for all memory allocators
- `LinearAllocator`: Fast bump allocator with bulk deallocation only
- `StackAllocator`: LIFO allocation pattern for temporary memory
- `MemoryPool`: Fixed-size block allocator for uniform objects

### Memory Management
- `MemoryStats`: Real-time memory usage statistics
- `AllocationStrategy`: Enumeration of available allocation strategies

## Usage Examples

```cpp
#include <flight/memory/memory.hpp>

using namespace flight::memory;

// Create a linear allocator with 1MB backing storage
LinearAllocator allocator(1024 * 1024);

// Allocate memory
void* ptr = allocator.allocate(256, 16); // 256 bytes, 16-byte aligned

// Get memory statistics
size_t used = allocator.get_used_memory();
size_t total = allocator.get_total_memory();

// Reset allocator (deallocates everything)
allocator.reset();
```

## Dependencies

This module has no dependencies on other Flight modules. It is used by:
- `flight-runtime`: For execution stack and heap management
- All other modules for dynamic memory allocation

## Build Instructions

This module is built as part of the Flight project:

```bash
# From project root
cmake -B build
cmake --build build --target flight-memory
```

To run tests:
```bash
cmake --build build --target flight-memory-tests
ctest -R flight-memory
```

## Architecture Notes

- Header-only allocator interfaces for zero overhead
- Lock-free statistics tracking using atomics
- Custom allocators for embedded platforms without malloc
- Memory alignment utilities for SIMD operations
- Compile-time configurable allocation strategies

## Performance Characteristics

### Linear Allocator
- O(1) allocation time
- No fragmentation
- Bulk deallocation only
- Ideal for per-frame allocations

### Stack Allocator
- O(1) allocation and deallocation
- LIFO deallocation required
- Perfect for recursive algorithms

### Memory Pool
- O(1) allocation and deallocation
- No fragmentation for fixed-size blocks
- Ideal for frequently allocated objects
