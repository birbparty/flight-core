# Flight Component Flattener Technology Stack

## 🔧 Core Technologies

### Programming Language: C++17
**Why C++17:**
- Maximum portability across development platforms
- Direct integration with Binaryen's C++ API
- Fine-grained memory control
- No runtime overhead
- Mature tooling ecosystem

**Key C++17 Features to Use:**
- `std::filesystem` for cross-platform file handling
- `std::optional` for safe nullable types
- `std::variant` for discriminated unions
- Structured bindings for cleaner code
- `if constexpr` for compile-time branching

### WebAssembly Manipulation: Binaryen
**Purpose:** Core WASM transformation engine

**Key APIs:**
```cpp
#include <binaryen-c.h>
// Or use C++ API:
#include <wasm.h>
#include <wasm-builder.h>
#include <wasm-interpreter.h>
```

**Use Cases:**
- Parse and validate WASM modules
- Merge multiple modules
- Optimize across module boundaries
- Rewrite memory operations
- Generate new WASM binaries

**Documentation:** https://github.com/WebAssembly/binaryen/wiki/API

### Component Extraction: wasm-tools
**Purpose:** Handle Component Model specifics

**Key Commands:**
```bash
# Extract core module from component
wasm-tools component extract core.wasm -o extracted.wasm

# Validate components
wasm-tools validate component.wasm

# Print component structure
wasm-tools print component.wasm
```

**Integration Strategy:**
- Use as subprocess for component operations
- Parse output for dependency information
- Validate components before processing

### Configuration: YAML-CPP
**Purpose:** Parse manifest files and configuration

**Example Usage:**
```cpp
#include <yaml-cpp/yaml.h>

YAML::Node config = YAML::LoadFile("game.yaml");
auto components = config["components"].as<std::vector<std::string>>();
```

**Manifest Format:**
```yaml
components:
  - path: physics.wasm
    name: physics
    priority: 1
  - path: renderer.wasm
    name: renderer
    priority: 2
dependencies:
  physics: []
  renderer: [physics]
```

## 🏗️ Build System

### CMake (3.14+)
**Project Structure:**
```cmake
cmake_minimum_required(VERSION 3.14)
project(flight-flatten VERSION 1.0.0)

# Use CPM for dependencies
include(cmake/CPM.cmake)

# Add Binaryen
CPMAddPackage(
  NAME binaryen
  GITHUB_REPOSITORY WebAssembly/binaryen
  VERSION 116
)

# Add YAML-CPP
CPMAddPackage(
  NAME yaml-cpp
  GITHUB_REPOSITORY jbeder/yaml-cpp
  VERSION 0.7.0
)
```

### Dependency Management: CPM.cmake
**Why CPM:**
- Header-only, easy to vendor
- Automatic dependency fetching
- Git submodule alternative
- Cache support for faster builds

## 📊 Data Structures & Algorithms

### Graph Library: Custom Implementation
**Why Custom:**
- Lightweight (no Boost dependency)
- Tailored for component dependencies
- Easy to optimize for our use case

**Key Classes:**
```cpp
template<typename Node>
class DependencyGraph {
    std::unordered_map<Node, std::vector<Node>> adjacency;
    std::unordered_map<Node, int> in_degree;
    
public:
    std::vector<Node> topological_sort();
    bool has_cycle() const;
    std::vector<Node> get_dependents(const Node& n);
};
```

### Memory Layout: Custom Allocator
**Design Pattern:**
```cpp
class SegmentAllocator {
    struct Segment {
        uint32_t base;
        uint32_t size;
        uint32_t guard_size;
    };
    
    std::vector<Segment> segments;
    uint32_t current_offset = 0;
    
public:
    Segment allocate(uint32_t size, uint32_t alignment);
    void add_guard_page();
};
```

## 🧪 Testing Framework

### Google Test (GTest)
**Why GTest:**
- Industry standard
- Great CMake integration
- Excellent mocking support
- Parallel test execution

**Test Structure:**
```cpp
TEST(ComponentFlattener, MergesTwoComponents) {
    auto comp1 = load_component("test1.wasm");
    auto comp2 = load_component("test2.wasm");
    
    ComponentFlattener flattener;
    auto result = flattener.flatten({comp1, comp2});
    
    ASSERT_TRUE(result.is_success());
    EXPECT_EQ(result.exports().size(), 2);
}
```

### Fuzzing: libFuzzer
**Purpose:** Find edge cases in parser
```cpp
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    try {
        ComponentParser parser;
        parser.parse(data, size);
    } catch (...) {
        // Catch all exceptions, we're looking for crashes
    }
    return 0;
}
```

## 🔍 Debugging & Profiling

### Debugging Tools
- **LLDB/GDB:** Standard debuggers
- **Valgrind:** Memory leak detection
- **AddressSanitizer:** Built into Clang/GCC
- **wasm-decompile:** Inspect generated WASM

### Profiling Tools
- **perf:** Linux performance analysis
- **Instruments:** macOS profiling
- **Tracy Profiler:** Real-time profiling
- **Binaryen's --time-passes:** Optimization timing

## 🚀 Performance Optimization

### Parallel Processing: std::execution
```cpp
#include <execution>
#include <algorithm>

// Parallel component parsing
std::for_each(std::execution::par_unseq,
    components.begin(), components.end(),
    [](auto& comp) { comp.parse(); });
```

### Memory Mapping: mmap/MapViewOfFile
**Purpose:** Efficient large file handling
```cpp
class MemoryMappedFile {
    void* data;
    size_t size;
    
public:
    std::span<const uint8_t> get_bytes() const;
};
```

## 📚 Additional Libraries

### CLI Parsing: CLI11
**Purpose:** Command-line interface
```cpp
CLI::App app{"Flight Component Flattener"};
std::vector<std::string> components;
app.add_option("-c,--components", components, "Component files")
   ->required();
```

### JSON Output: nlohmann/json
**Purpose:** Build manifests and debug info
```cpp
nlohmann::json manifest;
manifest["version"] = "1.0";
manifest["components"] = component_list;
manifest["memory_layout"] = layout_info;
```

### Logging: spdlog
**Purpose:** Structured logging
```cpp
spdlog::info("Flattening {} components", components.size());
spdlog::debug("Memory layout: base={:x}, size={}", base, size);
```

## 🔐 Security Considerations

### Input Validation
- Validate all WASM inputs with Binaryen
- Check component signatures
- Verify memory bounds
- Sanitize file paths

### Resource Limits
```cpp
struct ResourceLimits {
    size_t max_component_size = 100 * 1024 * 1024;  // 100MB
    size_t max_total_memory = 1024 * 1024 * 1024;   // 1GB
    size_t max_components = 100;
    std::chrono::seconds max_processing_time{30};
};
```

## 🎯 Best Practices

1. **Error Handling:** Use `Result<T, Error>` pattern
2. **Memory Safety:** RAII everywhere, no naked pointers
3. **Testing:** Minimum 80% code coverage
4. **Documentation:** Doxygen comments on all public APIs
5. **Code Style:** Follow LLVM coding standards
6. **Versioning:** Semantic versioning for releases

## 📖 Essential Reading

1. [WebAssembly Component Model Spec](https://github.com/WebAssembly/component-model)
2. [Binaryen Internals](https://github.com/WebAssembly/binaryen/blob/main/src/README.md)
3. [wasm-tools Documentation](https://github.com/bytecodealliance/wasm-tools/tree/main/docs)
4. [Modern C++ Best Practices](https://github.com/cpp-best-practices/cppbestpractices)

Remember: Every millisecond counts when targeting retro hardware. Profile early, optimize often!
