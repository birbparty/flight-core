# Buck2 Migration Infrastructure for Flight HAL Interfaces

## Overview

This repository has been prepared for future migration to Buck2 build system as part of the "dogfooding" approach to validate the buck2-cpp-cpm migration tool. The infrastructure enables parallel development with CMake while preparing for Buck2 adoption.

## Current Status

### ✅ Completed Infrastructure

#### Core Buck2 Files
- `.buckroot` - Repository root marker
- `.buckconfig` - Buck2 configuration with build settings
- `BUCK` - Root build file with main library targets
- `src/core/BUCK` - Granular core component targets
- `tests/BUCK` - Comprehensive test target definitions
- `third_party/BUCK` - External dependency framework

#### Documentation
- `docs/buck2_migration_strategy.md` - Comprehensive migration plan
- `docs/buck2_setup_guide.md` - Step-by-step setup instructions
- `cmake/Buck2Compatibility.cmake` - CMake patterns that translate to Buck2

#### Build System Features
- **Granular Targets**: Decomposed monolithic library into focused components
- **Test Infrastructure**: Complete test target hierarchy for unit, integration, and stress tests
- **Example Applications**: All examples configured as Buck2 binary targets
- **Dependency Framework**: Structure for external dependency management

### 🚧 Pending Migration Steps

1. **Buck2 Prelude Setup** - Initialize Buck2 prelude submodule
2. **External Dependencies** - Migrate fmt, spdlog, GoogleTest to Buck2 http_archive
3. **Platform Configuration** - Create platform-specific build rules
4. **Performance Validation** - Benchmark Buck2 vs CMake build times

## Buck2 Target Structure

### Library Targets
```
//:flight-hal-interfaces          # Main combined library
//src/core:hal-core               # Core HAL functionality
//src/core:driver-registry        # Driver registration system
//src/core:platform-detection     # Platform detection
//src/core:error-handling         # Error handling framework
//:hal-interfaces                 # Interface definitions  
//:hal-coordination               # Resource coordination
//:hal-platform                  # Platform abstraction
//:hal-validation                 # Validation framework
```

### Example Applications
```
//:enhanced-hal-example           # Basic HAL usage
//:advanced-registration-example  # Driver registration demo
//:platform-detection-example     # Platform detection showcase
//:error-handling-example         # Error handling patterns
//:gpu-audio-coordination-example # Resource coordination demo
```

### Test Targets
```
//tests:all-tests                 # All test suites
//tests:core-tests                # Core functionality tests
//tests:interfaces-tests          # Interface compliance tests
//tests:integration-tests         # Integration test suite
//tests:stress-tests              # Stress testing
//tests:compliance-tests          # Compliance validation
```

## Quick Start

### Prerequisites
```bash
# Install Buck2 (macOS)
brew install facebook/fb/buck2

# Verify installation
buck2 --version
```

### Setup Buck2 Environment
```bash
# Initialize Buck2 prelude
mkdir -p buck2
git submodule add https://github.com/facebook/buck2-prelude.git buck2/prelude
git submodule update --init --recursive
```

### Build Commands
```bash
# Build all targets
buck2 build //...

# Build main library
buck2 build //:flight-hal-interfaces

# Run tests
buck2 test //tests:all-tests

# Build specific example
buck2 build //:enhanced-hal-example
```

## Migration Benefits

### Performance Improvements (Expected)
| Build Type | CMake | Buck2 | Improvement |
|------------|-------|-------|-------------|
| Clean Build | 15-20s | 10-15s | 25-30% |
| Incremental (1 file) | 2-3s | 0.5-1s | 75-80% |
| Incremental (header) | 5-8s | 1-2s | 75-80% |
| With Remote Cache | N/A | 0.1-0.2s | 95% |

### Development Experience
- **Fine-grained Dependencies**: Better modularity and faster incremental builds
- **Hermetic Builds**: Reproducible builds across environments
- **Advanced Caching**: Local and remote caching for team collaboration
- **Query System**: Powerful dependency analysis and build optimization

## Current Build System Status

### CMake (Primary)
- ✅ Fully functional production build system
- ✅ Comprehensive test coverage
- ✅ Cross-platform compatibility
- ✅ CI/CD integration

### Buck2 (Secondary/Validation)
- ✅ Infrastructure prepared
- ⚠️ Requires prelude initialization
- ⚠️ External dependencies need migration
- 🚧 Performance validation pending

## Next Steps

### For Developers
1. **Continue using CMake** for daily development
2. **Experiment with Buck2** using provided infrastructure
3. **Report issues** found during Buck2 testing
4. **Contribute improvements** to Buck2 configuration

### For Migration
1. **Initialize Buck2 prelude** following setup guide
2. **Migrate external dependencies** to Buck2 patterns
3. **Performance benchmarking** against CMake baseline
4. **Team training** on Buck2 workflows

## Documentation

- **Migration Strategy**: [docs/buck2_migration_strategy.md](docs/buck2_migration_strategy.md)
- **Setup Guide**: [docs/buck2_setup_guide.md](docs/buck2_setup_guide.md)
- **CMake Compatibility**: [cmake/Buck2Compatibility.cmake](cmake/Buck2Compatibility.cmake)

## Validation Tool Integration

This infrastructure serves as a validation case for the [buck2-cpp-cpm](https://github.com/punk1290/buck2-cpp-cpm) migration tool, demonstrating:

- **Real-world CMake project** migration patterns
- **Complex dependency structures** and their Buck2 equivalents
- **Testing framework** migration strategies
- **Performance optimization** opportunities

## Support and Resources

- **Buck2 Documentation**: https://buck2.build/
- **Migration Issues**: Check project issues for Buck2-related problems
- **Performance Discussions**: Document build time improvements in issues
- **Team Knowledge Sharing**: Add Buck2 learnings to project wiki

---

*This Buck2 infrastructure enables future migration while maintaining current CMake functionality. The dual build system approach allows gradual adoption and performance validation.*
