# Buck2 Migration Strategy for Flight HAL Interfaces

## Overview

This document outlines the strategy for migrating the Flight HAL Interfaces project from CMake to Buck2 build system, following the "dogfooding" approach where we validate the buck2-cpp-cpm migration tool on our own codebase.

## Migration Benefits

### Performance Improvements
- **Advanced Caching**: Buck2's sophisticated caching system reduces build times by up to 95%
- **Incremental Builds**: Efficient incremental compilation with fine-grained dependency tracking
- **Remote Caching**: Shared build artifacts across development team
- **Parallel Execution**: Improved parallelization compared to make-based systems

### Development Experience
- **Fast Feedback**: Sub-second builds for single file changes
- **Deterministic Builds**: Hermetic builds ensure consistency across environments
- **Target Granularity**: Fine-grained build targets enable better modularity

## Current Project Analysis

### Buck2-Friendly Characteristics
✅ **Modern CMake Structure**: Target-based approach translates well to Buck2
✅ **Clear Module Organization**: Distinct libraries (core, interfaces, coordination, validation)
✅ **Explicit Dependencies**: Well-defined dependency relationships
✅ **Comprehensive Testing**: Structured test organization supports Buck2 test targets

### Areas Requiring Preparation
⚠️ **Monolithic Library**: Current single library needs decomposition into granular targets
⚠️ **External Dependencies**: FetchContent dependencies need mapping to Buck2 patterns
⚠️ **Platform Detection**: CMake's platform logic needs translation to Buck2 constraints

## Migration Plan

### Phase 1: Infrastructure Setup ✅
- [x] Create `.buckroot` file to mark repository root
- [x] Configure `.buckconfig` with proper cell configuration
- [x] Create root `BUCK` file with main library targets
- [x] Document migration strategy and benefits

### Phase 2: Granular Target Decomposition
- [ ] Refactor monolithic CMake library into smaller, focused libraries
- [ ] Create component-level `BUCK` files for each major module
- [ ] Implement explicit inter-component dependencies
- [ ] Update examples to use granular targets

### Phase 3: Dependency Management
- [ ] Map external dependencies from FetchContent to Buck2 patterns
- [ ] Create `third_party/` directory with external dependency BUILD files
- [ ] Implement version pinning and hash verification
- [ ] Document dependency update procedures

### Phase 4: Platform Configuration
- [ ] Translate CMake platform detection to Buck2 constraints
- [ ] Create platform-specific compiler configurations
- [ ] Implement cross-platform build rules
- [ ] Test on multiple platforms (macOS, Linux, Windows)

### Phase 5: Testing and Validation
- [ ] Migrate test targets to Buck2 `cxx_test` rules
- [ ] Implement benchmark targets for performance comparison
- [ ] Validate build correctness against CMake baseline
- [ ] Document performance improvements

## Buck2 Project Structure

```
flight-hal-interfaces/
├── .buckroot                    # Buck2 repository root marker
├── .buckconfig                  # Buck2 configuration
├── BUCK                         # Root build file
├── buck2/                       # Buck2 infrastructure
│   └── prelude/                 # Buck2 prelude (git submodule)
├── third_party/                 # External dependencies
│   ├── BUCK                     # Third-party build rules
│   └── BUILD_DEPS.md           # Dependency documentation
├── src/
│   ├── core/BUCK               # Core module build rules
│   ├── coordination/BUCK       # Coordination module build rules
│   ├── platform/BUCK           # Platform module build rules
│   └── validation/BUCK         # Validation module build rules
├── examples/
│   └── BUCK                    # Example executables
├── tests/
│   ├── unit/BUCK               # Unit test rules
│   ├── integration/BUCK        # Integration test rules
│   └── stress/BUCK             # Stress test rules
└── tools/
    └── BUCK                    # Tool executables
```

## Target Organization Strategy

### Core Libraries
- `//src/core:hal-core` - Core HAL functionality
- `//src/interfaces:hal-interfaces` - Interface definitions
- `//src/coordination:hal-coordination` - Resource coordination
- `//src/platform:hal-platform` - Platform abstraction
- `//src/validation:hal-validation` - Validation framework

### Combined Targets
- `//:flight-hal-interfaces` - Main library combining all components
- `//:flight-hal-dev` - Development target with additional tools

### Example Applications
- `//examples:enhanced-hal-example` - Basic HAL usage demonstration
- `//examples:platform-detection-example` - Platform detection showcase
- `//examples:coordination-example` - Resource coordination example

## External Dependencies

### Current CMake Dependencies
- `fmt` - String formatting library
- `spdlog` - Logging framework
- `GoogleTest` - Unit testing framework

### Buck2 Migration Strategy
```python
# third_party/BUCK
http_archive(
    name = "fmt",
    urls = ["https://github.com/fmtlib/fmt/archive/10.2.1.tar.gz"],
    sha256 = "...",
    strip_prefix = "fmt-10.2.1",
    build_file = "//third_party:fmt.BUILD",
)
```

## Performance Comparison Framework

### Metrics to Track
- **Clean Build Time**: Full rebuild from scratch
- **Incremental Build Time**: Single file change rebuild
- **Cache Hit Rate**: Percentage of cached vs rebuilt targets
- **Memory Usage**: Peak memory during builds
- **Parallelization Efficiency**: CPU utilization during builds

### Baseline Measurements (CMake)
- Clean build: ~15-20 seconds
- Incremental (single .cpp): ~2-3 seconds
- Incremental (header change): ~5-8 seconds

### Expected Buck2 Improvements
- Clean build: ~10-15 seconds (25-30% improvement)
- Incremental (single .cpp): ~0.5-1 second (75-80% improvement)
- Incremental (header): ~1-2 seconds (75-80% improvement)
- With remote cache: ~0.1-0.2 seconds (95% improvement)

## Risk Mitigation

### Parallel Development
- Maintain CMake as primary build system during migration
- Use Buck2 as secondary validation system
- Gradual team adoption based on comfort level

### Rollback Strategy
- Keep CMake files functional throughout migration
- Document any CMake-specific workarounds
- Maintain build system feature parity

### Validation Approach
- Binary compatibility verification between CMake and Buck2 builds
- Performance regression testing
- Cross-platform build verification

## Timeline

### Week 1-2: Foundation
- Complete Buck2 infrastructure setup
- Begin granular target decomposition
- Start team training on Buck2 concepts

### Week 3-4: Core Migration
- Migrate core library targets
- Implement dependency management
- Create platform-specific configurations

### Week 5-6: Testing and Examples
- Migrate test targets
- Update example applications
- Performance baseline establishment

### Week 7-8: Validation and Documentation
- Cross-platform testing
- Performance comparison
- Migration guide completion

## Success Criteria

### Technical Milestones
- [ ] All library targets build successfully with Buck2
- [ ] All tests pass with Buck2 builds
- [ ] Build performance improvements demonstrated
- [ ] Cross-platform compatibility verified

### Team Adoption
- [ ] Development team trained on Buck2 workflows
- [ ] CI/CD pipeline updated to support Buck2
- [ ] Documentation updated with Buck2 procedures

### Performance Goals
- [ ] 25% improvement in clean build times
- [ ] 75% improvement in incremental build times
- [ ] 95% improvement with remote caching enabled

## References

- [Buck2 Official Documentation](https://buck2.build/)
- [Buck2 C++ Rules Reference](https://buck2.build/docs/api/rules/#cxx)
- [buck2-cpp-cpm Migration Tool](https://github.com/punk1290/buck2-cpp-cpm)
- [Meta's Buck2 Migration Experience](https://engineering.fb.com/2023/04/06/open-source/buck2-open-source-large-scale-build-system/)
