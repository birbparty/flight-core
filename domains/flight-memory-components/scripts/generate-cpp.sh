#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$( cd "$SCRIPT_DIR/.." && pwd )"
WIT_DIR="$PROJECT_ROOT/wit"
OUTPUT_DIR="$PROJECT_ROOT/bindings/cpp"

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Generate C++ bindings
echo "  Generating C++ bindings from world.wit..."
wit-bindgen cpp "$WIT_DIR/world.wit" \
    --out-dir "$OUTPUT_DIR" \
    --namespace flight::memory \
    --std c++17

# Create CMakeLists.txt for the bindings
cat > "$OUTPUT_DIR/CMakeLists.txt" << 'EOF'
cmake_minimum_required(VERSION 3.10)
project(flight_memory_components)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add library
add_library(flight_memory_components INTERFACE)
target_include_directories(flight_memory_components INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Installation rules
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        DESTINATION include/flight/memory
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")
EOF

# Create README for C++ bindings
cat > "$OUTPUT_DIR/README.md" << 'EOF'
# Flight Memory Components - C++ Bindings

This directory contains auto-generated C++ bindings for the Flight Memory Components WIT interfaces.

## Usage

Include the headers in your C++ project:

```cpp
#include <flight/memory/types.hpp>
#include <flight/memory/validators.hpp>
```

## Building

These are header-only bindings. Simply include them in your project.

For CMake projects:
```cmake
add_subdirectory(path/to/flight-memory-components/bindings/cpp)
target_link_libraries(your_target PRIVATE flight_memory_components)
```

## Regenerating

These bindings are auto-generated. To regenerate:
```bash
../scripts/generate-cpp.sh
```
EOF

echo "  ✓ C++ bindings generated in $OUTPUT_DIR"
