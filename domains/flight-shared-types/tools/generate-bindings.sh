#!/bin/bash
# Binding generation tool for all target languages (Updated for wit-bindgen 0.42.1 + jco)

set -euo pipefail

WIT_DIR="/Users/punk1290/git/flight/domains/flight-shared-types/wit"
BINDINGS_DIR="/Users/punk1290/git/flight/domains/flight-shared-types/bindings"

echo "🔄 Generating language bindings..."

for wit_file in "$WIT_DIR"/*.wit; do
    if [ -f "$wit_file" ]; then
        name=$(basename "$wit_file" .wit)
        echo "Generating bindings for $name..."
        
        # TypeScript (using jco for better type support)
        mkdir -p "$BINDINGS_DIR/typescript/$name"
        jco types "$wit_file" --out-dir "$BINDINGS_DIR/typescript/$name"
        
        # Rust (using wit-bindgen 0.42.1)
        mkdir -p "$BINDINGS_DIR/rust/$name"
        wit-bindgen rust "$wit_file" --out-dir "$BINDINGS_DIR/rust/$name"
        
        # C (using wit-bindgen 0.42.1)
        mkdir -p "$BINDINGS_DIR/cpp17/$name"
        wit-bindgen c "$wit_file" --out-dir "$BINDINGS_DIR/cpp17/$name"
        
        # Note: Go support has moved to separate repository
        # For Go bindings, use: https://github.com/bytecodealliance/go-modules
        
        echo "✅ Generated bindings for $name (Rust, TypeScript, C)"
    fi
done

echo "✅ All bindings generated with updated toolchain"
