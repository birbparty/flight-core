# Flight WASM Performance Benchmark Framework

Mission-critical performance measurement system for Flight WASM with automated regression detection and platform-specific optimization validation.

## 🚀 Overview

This benchmark framework provides comprehensive performance validation for Flight WASM across multiple embedded platforms including Dreamcast, PSP, PS Vita, and modern desktop systems. It enforces strict performance targets required for real-time WebAssembly processing.

## 📊 Performance Targets

### Core Operations (Sub-nanosecond Requirements)
- **Value Construction**: < 1 CPU cycle (< 1ns on modern hardware)
- **Type Checking**: < 1 CPU cycle (< 1ns on modern hardware)  
- **Type Conversion**: < 5 CPU cycles (< 5ns on modern hardware)
- **Endianness Operations**: < 2 CPU cycles (< 2ns on modern hardware)

### Binary Parsing Performance
- **LEB128 Decoding**: < 10ns per operation
- **Module Parsing Throughput**: > 100MB/s sustained
- **Magic Number Validation**: < 1ns (compile-time preferred)
- **UTF-8 Validation**: < 20ns ASCII, < 50ns multibyte

### Memory Efficiency
- **Zero Allocations**: Core value operations must perform zero allocations
- **Value Storage**: ≤ 16 bytes per value (optimal packing)
- **Memory Leak Detection**: Automated leak validation
- **Platform Constraints**: Automatic validation for embedded targets

## 🔧 Building and Running

### Build Benchmarks
```bash
# Configure with benchmarks enabled
cmake -B build-benchmarks -S . -DCMAKE_BUILD_TYPE=Release -DFLIGHT_WASM_BUILD_BENCHMARKS=ON

# Build the benchmark executable
cmake --build build-benchmarks --target flight-wasm-benchmarks
```

### Run Performance Tests
```bash
# Basic benchmark execution
cd build-benchmarks
./benchmarks/flight-wasm-benchmarks

# Generate JSON results for validation
./benchmarks/flight-wasm-benchmarks --benchmark_format=json --benchmark_out=results.json

# Run performance validation
python3 ../benchmarks/scripts/validate_performance.py results.json
```

### Advanced Options
```bash
# Quick validation run
./benchmarks/flight-wasm-benchmarks --benchmark_min_time=0.1s

# Detailed profiling
./benchmarks/flight-wasm-benchmarks --benchmark_repetitions=10 --benchmark_report_aggregates_only=true

# Filter specific benchmarks
./benchmarks/flight-wasm-benchmarks --benchmark_filter="Value.*"
```

## 📁 Framework Structure

```
benchmarks/
├── CMakeLists.txt              # Google Benchmark integration
├── benchmark_main.cpp          # Main entry point with platform detection
├── scripts/
│   └── validate_performance.py # Automated performance validation
├── types/
│   └── benchmark_values.cpp    # Value type performance tests
├── binary/
│   └── benchmark_parser.cpp    # Binary parsing performance tests
├── utilities/
│   ├── benchmark_error.cpp     # Error handling performance tests
│   └── benchmark_platform.cpp  # Platform detection performance tests
└── performance/
    ├── benchmark_regression.cpp # Regression detection tests
    └── benchmark_memory.cpp     # Memory efficiency tests
```

## 🎯 Platform-Specific Targets

### Dreamcast (SH-4 @ 200MHz)
- **Memory Budget**: Very conservative (26MB total RAM)
- **CPU Performance**: 200MHz single-core SH-4
- **Optimization Focus**: Minimal memory usage and cache efficiency

### PlayStation Portable (MIPS @ 333MHz)
- **Memory Budget**: Moderate (32MB RAM)
- **CPU Performance**: 333MHz MIPS R4000
- **Optimization Focus**: MIPS-specific optimizations

### PlayStation Vita (ARM Cortex-A9 @ 444MHz)
- **Memory Budget**: Large (512MB RAM)
- **CPU Performance**: 444MHz quad-core ARM with NEON
- **Optimization Focus**: SIMD vectorization and multi-core

### Desktop/Server
- **Memory Budget**: Unlimited for development
- **CPU Performance**: Modern x86_64 with aggressive optimizations
- **Optimization Focus**: Maximum throughput and native CPU features

## 📈 Regression Detection

The framework includes automated regression detection with:

- **5% Performance Regression Threshold**: Alerts on any slowdown > 5%
- **10% Memory Regression Threshold**: Alerts on memory usage increases > 10%
- **15% Improvement Recognition**: Automatically updates baselines for significant improvements
- **Git Commit Tracking**: Links performance changes to specific commits
- **Platform-Specific Baselines**: Separate baselines for each target platform

### Baseline Management
```bash
# Performance baselines are stored in build-benchmarks/baselines/
# Each benchmark maintains separate baselines per platform

# View current baselines
ls build-benchmarks/baselines/

# Reset all baselines (use with caution)
rm -rf build-benchmarks/baselines/
```

## 🔬 Continuous Integration

The benchmark framework integrates with CI/CD systems:

```yaml
# Example GitHub Actions integration
- name: Run Performance Benchmarks
  run: |
    cmake --build build --target flight-wasm-benchmarks
    cd build
    ./benchmarks/flight-wasm-benchmarks --benchmark_format=json --benchmark_out=results.json
    python3 ../benchmarks/scripts/validate_performance.py results.json --strict
```

The `--strict` flag causes the validation script to exit with error code 1 if any performance targets are missed, failing the CI build.

## 🛠️ Development Guidelines

### Adding New Benchmarks

1. **Choose the Appropriate Category**: Place benchmarks in the correct subdirectory
2. **Follow Naming Convention**: Use `BM_CategoryName_SpecificOperation` format
3. **Set Performance Labels**: Use labels like `target_10ns` to specify expected performance
4. **Include DoNotOptimize**: Prevent compiler from optimizing away benchmark code
5. **Add Platform Guards**: Use `#ifdef` for platform-specific benchmarks

Example:
```cpp
static void BM_NewFeature_Operation(benchmark::State& state) {
    for (auto _ : state) {
        // Your benchmark code here
        auto result = expensive_operation();
        benchmark::DoNotOptimize(result);
    }
    state.SetLabel("target_50ns");
}
BENCHMARK(BM_NewFeature_Operation);
```

### Performance Target Guidelines

- **Sub-nanosecond**: Core operations that happen millions of times
- **1-10ns**: Frequent operations like LEB128 decoding
- **10-100ns**: Moderate frequency operations like section parsing
- **100ns-1μs**: Infrequent operations like module validation
- **>1μs**: Acceptable for one-time initialization only

## 🚨 Performance Alerts

The framework provides several levels of performance alerts:

- **🔥 CATASTROPHIC REGRESSION**: >25% slowdown - requires immediate attention
- **⚠️ PERFORMANCE REGRESSION**: >5% slowdown - investigate and optimize
- **🧠 MEMORY REGRESSION**: >10% memory increase - check for leaks
- **🚀 PERFORMANCE IMPROVEMENT**: >15% speedup - updates baseline automatically

## 📊 Interpreting Results

### Benchmark Output Interpretation
- **Time**: Real-world execution time (what users experience)
- **CPU Time**: Pure CPU execution time (excludes I/O waits)
- **Iterations**: Number of times the operation was executed
- **Bytes/Second**: Throughput for parsing operations
- **Items/Second**: Processing rate for batch operations

### Performance Targets Status
- **✅ Target Met**: Operation performed within acceptable limits
- **⚠️ Target Missed**: Operation exceeded target but within tolerance
- **❌ Target Failed**: Operation significantly exceeded target - optimization needed

## 🔄 Future Enhancements

As Flight WASM development progresses, the placeholder benchmarks will be replaced with real implementation tests:

- **Value System**: Real Value class performance validation
- **Binary Parser**: Actual WebAssembly module parsing benchmarks
- **Instruction Execution**: Instruction dispatch and execution performance
- **Memory Management**: Real allocation tracking and optimization
- **Cross-Platform**: Full embedded platform validation

The framework is designed to scale with the project and provide continuous performance validation throughout the development lifecycle.
