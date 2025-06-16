# Buck2 Setup Guide for Flight HAL Interfaces

## Overview

This guide provides step-by-step instructions for setting up Buck2 build system for the Flight HAL Interfaces project. This setup enables a "dogfooding" approach where we validate the buck2-cpp-cpm migration tool on our own codebase.

## Prerequisites

### System Requirements
- **Operating System**: macOS, Linux, or Windows with WSL2
- **Buck2**: Version 2024-01-01 or later
- **C++ Compiler**: 
  - macOS: Xcode Command Line Tools (Apple Clang 14+)
  - Linux: GCC 11+ or Clang 14+
  - Windows: MSVC 2022 or Clang 14+
- **Git**: For Buck2 prelude submodule management

### Installing Buck2

#### macOS (Homebrew)
```bash
brew install facebook/fb/buck2
```

#### Linux (from releases)
```bash
# Download latest release from GitHub
curl -L https://github.com/facebook/buck2/releases/latest/download/buck2-x86_64-unknown-linux-gnu.zst -o buck2.zst
zstd -d buck2.zst
chmod +x buck2
sudo mv buck2 /usr/local/bin/
```

#### Windows (via Chocolatey)
```powershell
choco install buck2
```

#### Verify Installation
```bash
buck2 --version
```

## Project Setup

### 1. Buck2 Infrastructure Files

The following files have been created for Buck2 integration:

- `.buckroot` - Marks the repository root
- `.buckconfig` - Buck2 configuration with build settings
- `BUCK` - Root build file with main library targets
- `src/core/BUCK` - Core component build rules
- `tests/BUCK` - Test target definitions
- `third_party/BUCK` - External dependency placeholders

### 2. Buck2 Prelude Setup

Buck2 requires a prelude for build rules. Set up the prelude:

```bash
# Create buck2 directory
mkdir -p buck2

# Clone the Buck2 prelude as a submodule
git submodule add https://github.com/facebook/buck2-prelude.git buck2/prelude

# Initialize and update submodules
git submodule update --init --recursive
```

### 3. Platform Configuration

Create platform-specific configurations:

```bash
mkdir -p platforms
```

Create `platforms/BUCK`:
```python
platform(
    name = "default",
    constraint_values = [
        "prelude//os:linux",
        "prelude//cpu:x86_64",
    ],
)

platform(
    name = "macos",
    constraint_values = [
        "prelude//os:macos", 
        "prelude//cpu:x86_64",
    ],
)

platform(
    name = "windows",
    constraint_values = [
        "prelude//os:windows",
        "prelude//cpu:x86_64", 
    ],
)
```

## Build Commands

### Basic Build Commands

```bash
# Build all targets
buck2 build //...

# Build specific library
buck2 build //:flight-hal-interfaces

# Build core component
buck2 build //src/core:hal-core

# Build examples
buck2 build //examples:enhanced-hal-example
```

### Test Commands

```bash
# Run all tests
buck2 test //tests:all-tests

# Run specific test suite
buck2 test //tests:core-tests

# Run tests with coverage
buck2 test //tests:all-tests --coverage
```

### Development Commands

```bash
# Build with debug information
buck2 build //:flight-hal-interfaces --config=buck2.execution_platforms=prelude//platforms:default

# Show build graph
buck2 query "deps(//:flight-hal-interfaces)"

# Show reverse dependencies
buck2 query "rdeps(//..., //src/core:hal-core)"

# Clean build artifacts
buck2 clean
```

## Performance Monitoring

### Build Performance Metrics

Buck2 provides detailed build performance information:

```bash
# Build with timing information
buck2 build //... --show-output

# Profile build performance
buck2 build //... --profile

# Show cache statistics
buck2 status --show-cache-dir
```

### Expected Performance Improvements

Based on similar projects migrated to Buck2:

| Build Type | CMake Baseline | Buck2 Target | Improvement |
|------------|----------------|--------------|-------------|
| Clean Build | 15-20s | 10-15s | 25-30% |
| Incremental (1 file) | 2-3s | 0.5-1s | 75-80% |
| Incremental (header) | 5-8s | 1-2s | 75-80% |
| With Remote Cache | N/A | 0.1-0.2s | 95% |

## Troubleshooting

### Common Issues

#### 1. Prelude Not Found
```
Error: Cell `prelude` was not found
```

**Solution**: Ensure Buck2 prelude is properly initialized:
```bash
git submodule update --init --recursive
```

#### 2. Compiler Not Found
```
Error: C++ compiler not found
```

**Solution**: Install appropriate compiler and update `.buckconfig`:
```ini
[cxx]
cxx = /usr/bin/clang++
cxxflags = -std=c++17
```

#### 3. Header Files Not Found
```
Error: header 'flight/hal/core/...' not found
```

**Solution**: Verify header paths in BUCK files match actual file locations.

### Debug Build Issues

Enable verbose logging:
```bash
buck2 build //... -v 3
```

Show build commands being executed:
```bash
buck2 build //... --show-full-output
```

## Development Workflow

### 1. Making Changes

After modifying source files:
```bash
# Incremental build (very fast)
buck2 build //:flight-hal-interfaces

# Run affected tests
buck2 test $(buck2 query "rdeps(//tests:..., $(buck2 query 'changed_since("HEAD~1")'))")
```

### 2. Adding New Components

When adding new source files:

1. Add sources to appropriate `BUCK` file
2. Update dependencies if needed
3. Test the build:
```bash
buck2 build //src/new-component:...
```

### 3. Dependency Management

To add external dependencies:

1. Update `third_party/BUCK` with `http_archive` rule
2. Create corresponding `.BUILD` file
3. Add dependency to consuming targets

## CI/CD Integration

### GitHub Actions

Example workflow for Buck2 builds:

```yaml
name: Buck2 Build
on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v3
      with:
        submodules: recursive
    
    - name: Install Buck2
      run: |
        curl -L https://github.com/facebook/buck2/releases/latest/download/buck2-x86_64-unknown-linux-gnu.zst -o buck2.zst
        zstd -d buck2.zst
        chmod +x buck2
        sudo mv buck2 /usr/local/bin/
    
    - name: Build
      run: buck2 build //...
    
    - name: Test
      run: buck2 test //tests:all-tests
```

## Migration Strategy

### Phase 1: Parallel Development
- Maintain CMake as primary build system
- Use Buck2 for validation and performance testing
- Train team on Buck2 workflows

### Phase 2: Gradual Adoption
- Migrate development workflows to Buck2
- Update CI/CD pipelines
- Performance comparison and optimization

### Phase 3: Full Migration
- Make Buck2 the primary build system
- Archive CMake configuration
- Document lessons learned

## Resources

- [Buck2 Official Documentation](https://buck2.build/)
- [Buck2 C++ Rules](https://buck2.build/docs/api/rules/#cxx)
- [Flight HAL Buck2 Migration Strategy](./buck2_migration_strategy.md)
- [buck2-cpp-cpm Migration Tool](https://github.com/punk1290/buck2-cpp-cpm)

## Support

For Buck2-specific issues:
- Check [Buck2 GitHub Issues](https://github.com/facebook/buck2/issues)
- Join [Buck2 Discord](https://discord.gg/buck2)

For project-specific questions:
- Review [Migration Strategy Document](./buck2_migration_strategy.md)
- Check project documentation in `docs/`
