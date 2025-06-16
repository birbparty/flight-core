#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$( cd "$SCRIPT_DIR/.." && pwd )"
WIT_DIR="$PROJECT_ROOT/wit"
OUTPUT_DIR="$PROJECT_ROOT/bindings/typescript"

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Check if jco is installed
if ! command -v jco &> /dev/null; then
    echo "  ⚠️  jco not found. TypeScript bindings require @bytecodealliance/jco"
    echo "     Install with: npm install -g @bytecodealliance/jco"
    exit 1
fi

# Generate TypeScript bindings
echo "  Generating TypeScript bindings from world.wit..."
jco types "$WIT_DIR/world.wit" \
    --out-dir "$OUTPUT_DIR" \
    --world-name flight-memory

# Create package.json
cat > "$OUTPUT_DIR/package.json" << 'EOF'
{
  "name": "@flight/memory-components",
  "version": "0.1.0",
  "description": "TypeScript bindings for Flight Memory Components",
  "main": "index.js",
  "types": "index.d.ts",
  "repository": {
    "type": "git",
    "url": "https://github.com/flight/flight-memory-components.git"
  },
  "keywords": [
    "webassembly",
    "memory",
    "component-model",
    "flight"
  ],
  "author": "Flight Core Contributors",
  "license": "MIT",
  "files": [
    "*.js",
    "*.d.ts",
    "*.ts",
    "README.md"
  ],
  "publishConfig": {
    "access": "public"
  }
}
EOF

# Create tsconfig.json
cat > "$OUTPUT_DIR/tsconfig.json" << 'EOF'
{
  "compilerOptions": {
    "target": "ES2020",
    "module": "ESNext",
    "lib": ["ES2020"],
    "declaration": true,
    "outDir": "./",
    "strict": true,
    "esModuleInterop": true,
    "skipLibCheck": true,
    "forceConsistentCasingInFileNames": true,
    "moduleResolution": "node"
  },
  "include": ["*.ts"],
  "exclude": ["node_modules"]
}
EOF

# Create README for TypeScript bindings
cat > "$OUTPUT_DIR/README.md" << 'EOF'
# Flight Memory Components - TypeScript Bindings

This directory contains auto-generated TypeScript bindings for the Flight Memory Components WIT interfaces.

## Usage

Install the package:

```bash
npm install @flight/memory-components
```

Use in your TypeScript project:

```typescript
import { MemorySize, MemoryAddress, validateAlignment } from '@flight/memory-components';

// Create memory size
const size = new MemorySize(1024 * 1024); // 1MB

// Validate alignment
try {
  validateAlignment(address, 4);
} catch (error) {
  console.error('Invalid alignment:', error);
}
```

## Building

```bash
npm run build
```

## Regenerating

These bindings are auto-generated. To regenerate:
```bash
../scripts/generate-typescript.sh
```
EOF

# Create a simple index.ts re-export file
cat > "$OUTPUT_DIR/index.ts" << 'EOF'
// Re-export all generated types and functions
export * from './flight-memory';
EOF

echo "  ✓ TypeScript bindings generated in $OUTPUT_DIR"
