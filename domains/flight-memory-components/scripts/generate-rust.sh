#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$( cd "$SCRIPT_DIR/.." && pwd )"
WIT_DIR="$PROJECT_ROOT/wit"
OUTPUT_DIR="$PROJECT_ROOT/bindings/rust"

# Create output directory
mkdir -p "$OUTPUT_DIR/src"

# Generate Rust bindings
echo "  Generating Rust bindings from world.wit..."
wit-bindgen rust "$WIT_DIR/world.wit" \
    --out-dir "$OUTPUT_DIR/src" \
    --std-feature

# Create Cargo.toml
cat > "$OUTPUT_DIR/Cargo.toml" << 'EOF'
[package]
name = "flight-memory-components"
version = "0.1.0"
edition = "2021"
authors = ["Flight Core Contributors"]
description = "Rust bindings for Flight Memory Components"
repository = "https://github.com/flight/flight-memory-components"
license = "MIT"
keywords = ["wasm", "memory", "component-model", "flight"]
categories = ["wasm", "memory-management", "no-std"]

[dependencies]
wit-bindgen = "0.24.0"

[features]
default = ["std"]
std = ["wit-bindgen/std"]

[lib]
name = "flight_memory_components"
path = "src/lib.rs"
EOF

# Create lib.rs that re-exports the generated bindings
cat > "$OUTPUT_DIR/src/lib.rs" << 'EOF'
//! Flight Memory Components - Rust Bindings
//! 
//! This crate provides type-safe memory abstractions for WebAssembly Component Model.
//! 
//! # Example
//! 
//! ```rust
//! use flight_memory_components::{MemorySize, MemoryAddress};
//! 
//! let size = MemorySize::from_bytes(1024 * 1024); // 1MB
//! let addr = MemoryAddress::new(0x1000);
//! ```

#![cfg_attr(not(feature = "std"), no_std)]

// Re-export generated bindings
mod bindings;
pub use bindings::*;

// Additional helper implementations can go here
EOF

# Create README for Rust bindings
cat > "$OUTPUT_DIR/README.md" << 'EOF'
# Flight Memory Components - Rust Bindings

This directory contains auto-generated Rust bindings for the Flight Memory Components WIT interfaces.

## Usage

Add to your `Cargo.toml`:

```toml
[dependencies]
flight-memory-components = { path = "path/to/bindings/rust" }
```

Use in your Rust project:

```rust
use flight_memory_components::{MemorySize, MemoryAddress, validate_alignment};

fn main() {
    // Create memory size
    let size = MemorySize::from_mb(1); // 1MB
    
    // Validate alignment
    match validate_alignment(addr, 4) {
        Ok(_) => println!("Alignment valid"),
        Err(e) => eprintln!("Invalid alignment: {}", e),
    }
}
```

## Features

- `std` (default): Enable standard library support
- `no_std`: Disable standard library for embedded/WASM environments

## Building

```bash
cargo build
cargo build --no-default-features  # For no_std
```

## Regenerating

These bindings are auto-generated. To regenerate:
```bash
../scripts/generate-rust.sh
```
EOF

echo "  ✓ Rust bindings generated in $OUTPUT_DIR"
