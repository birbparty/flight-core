# Flight Shared Types

Universal WebAssembly Component Model types for Flight-Core and V6R integration.

## Overview

This domain provides foundational Component Model types that enable seamless integration between:
- **Flight-Core**: C++17 runtime targeting Dreamcast (16MB) to modern platforms
- **V6R**: TypeScript/Go services for VM resource management

## Critical Path

**V6R VM management is completely blocked** without these memory types. Week 1 delivery is mandatory.

## Architecture

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   WIT Types     │───▶│  Language        │───▶│   Integration   │
│                 │    │  Bindings        │    │                 │
│ • Memory Types  │    │                  │    │ • V6R Services  │
│ • Error Types   │    │ • TypeScript     │    │ • Flight-Core   │
│ • Platform Types│    │ • Go             │    │ • Component     │
│ • Session Types │    │ • Rust           │    │   Model Runtime │
│                 │    │ • C++17          │    │                 │
└─────────────────┘    └──────────────────┘    └─────────────────┘
```

## Quick Start

1. **Install Toolchain**: `wit-bindgen`, `wasm-tools`, `cargo-component`
2. **Generate Bindings**: `./tools/generate-bindings.sh`
3. **Validate Types**: `./tools/validate-wit.sh`
4. **Integrate**: Import generated bindings in target projects

## Directory Structure

```
domains/flight-shared-types/
├── wit/                    # WIT interface definitions
├── bindings/              # Generated language bindings
│   ├── typescript/        # V6R mobile & bridge
│   ├── go/               # V6R API & CLI
│   ├── rust/             # Validation components
│   └── cpp17/            # Flight-Core integration
├── validation/           # WASM validation components
├── src/                  # Implementation helpers
├── tests/                # Cross-language tests
└── tools/                # Development utilities
```

## Platform Support

- **Dreamcast**: 16MB baseline (PlayStation 1 dropped for C++17)
- **PSP**: 32-64MB
- **Modern**: Windows, macOS, Linux, Android, Web
- **V6R VMs**: 512MB - 2GB

## Integration Status

- [ ] Memory Types (Week 1) - **V6R BLOCKING**
- [ ] Error Types (Week 2)
- [ ] Platform Types (Week 2)  
- [ ] Session Types (Week 3)
- [ ] Production Ready (Week 4)
