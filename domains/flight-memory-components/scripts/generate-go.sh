#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$( cd "$SCRIPT_DIR/.." && pwd )"
WIT_DIR="$PROJECT_ROOT/wit"
OUTPUT_DIR="$PROJECT_ROOT/bindings/go"

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Generate Go bindings
echo "  Generating Go bindings from world.wit..."
wit-bindgen go "$WIT_DIR/world.wit" \
    --out-dir "$OUTPUT_DIR" \
    --package memory

# Create go.mod for the bindings
cat > "$OUTPUT_DIR/go.mod" << 'EOF'
module github.com/flight/flight-memory-components/bindings/go

go 1.21

require (
    github.com/bytecodealliance/wasmtime-go v1.0.0
)
EOF

# Create README for Go bindings
cat > "$OUTPUT_DIR/README.md" << 'EOF'
# Flight Memory Components - Go Bindings

This directory contains auto-generated Go bindings for the Flight Memory Components WIT interfaces.

## Usage

Import the package in your Go project:

```go
import (
    memory "github.com/flight/flight-memory-components/bindings/go"
)

func main() {
    // Use memory types
    size := memory.NewMemorySize(1024 * 1024) // 1MB
    
    // Use validators
    if err := memory.ValidateAlignment(addr, 4); err != nil {
        // Handle error
    }
}
```

## Installation

```bash
go get github.com/flight/flight-memory-components/bindings/go
```

## Regenerating

These bindings are auto-generated. To regenerate:
```bash
../scripts/generate-go.sh
```
EOF

# Create example test file
cat > "$OUTPUT_DIR/memory_test.go" << 'EOF'
package memory

import (
    "testing"
)

func TestMemoryTypes(t *testing.T) {
    // Example test - actual implementation depends on generated types
    t.Log("Memory types test placeholder")
}
EOF

echo "  ✓ Go bindings generated in $OUTPUT_DIR"
