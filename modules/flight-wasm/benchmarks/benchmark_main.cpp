// =============================================================================
// Flight WASM Foundation - Benchmark Main Entry Point
// Mission Critical Performance Measurement System
// =============================================================================

#include <benchmark/benchmark.h>
#include <flight/wasm/wasm.hpp>
#include <chrono>
#include <iostream>
#include <iomanip>

using namespace flight::wasm;

// =============================================================================
// Platform-Specific Performance Constants
// =============================================================================

namespace flight_wasm_bench_constants {
    
#ifdef __DREAMCAST__
    // Dreamcast SH-4 @ 200MHz
    constexpr auto CPU_FREQUENCY = 200000000ULL; // Hz
    constexpr auto CYCLES_PER_NANOSECOND = 0.2;
    constexpr size_t L1_CACHE_SIZE = 8 * 1024; // 8KB
    constexpr size_t MEMORY_BANDWIDTH = 800ULL * 1024 * 1024; // 800MB/s
#elif defined(__PSP__)
    // PSP MIPS @ 333MHz  
    constexpr auto CPU_FREQUENCY = 333000000ULL; // Hz
    constexpr auto CYCLES_PER_NANOSECOND = 0.333;
    constexpr size_t L1_CACHE_SIZE = 16 * 1024; // 16KB
    constexpr size_t MEMORY_BANDWIDTH = 2700ULL * 1024 * 1024; // 2.7GB/s
#elif defined(__vita__)
    // PS Vita ARM Cortex-A9 @ 444MHz
    constexpr auto CPU_FREQUENCY = 444000000ULL; // Hz
    constexpr auto CYCLES_PER_NANOSECOND = 0.444;
    constexpr size_t L1_CACHE_SIZE = 32 * 1024; // 32KB
    constexpr size_t MEMORY_BANDWIDTH = 6400ULL * 1024 * 1024; // 6.4GB/s
#else
    // Modern desktop/server hardware
    constexpr auto CPU_FREQUENCY = 3000000000ULL; // 3GHz baseline
    constexpr auto CYCLES_PER_NANOSECOND = 3.0;
    constexpr size_t L1_CACHE_SIZE = 64 * 1024; // 64KB
    constexpr size_t MEMORY_BANDWIDTH = 50000ULL * 1024 * 1024; // 50GB/s
#endif

    // Performance targets derived from WebAssembly specification requirements
    constexpr auto MAX_VALUE_CONSTRUCTION_TIME = std::chrono::nanoseconds(1);  // <1 CPU cycle
    constexpr auto MAX_TYPE_CHECK_TIME = std::chrono::nanoseconds(1);         // <1 CPU cycle
    constexpr auto MAX_TYPE_CONVERSION_TIME = std::chrono::nanoseconds(5);    // <5 CPU cycles
    
    // Binary parsing performance targets  
    constexpr auto MIN_PARSING_THROUGHPUT = 100 * 1024 * 1024; // 100MB/s
    constexpr auto MAX_LEB128_DECODE_TIME = std::chrono::nanoseconds(10);
    constexpr auto MAX_SECTION_PARSE_OVERHEAD = 0.05; // 5% overhead
    
    // Memory efficiency targets
    constexpr size_t MAX_VALUE_SIZE = 16; // bytes
    constexpr size_t MAX_INSTRUCTION_SIZE = 32; // bytes
    constexpr size_t MAX_MODULE_OVERHEAD = 1024; // bytes base overhead
}

// =============================================================================
// Benchmark Configuration and Setup
// =============================================================================

void configure_benchmark_defaults() {
    std::cout << "=== Flight WASM Benchmark Configuration ===" << std::endl;
    std::cout << "CPU Frequency: " << flight_wasm_bench_constants::CPU_FREQUENCY / 1000000 << " MHz" << std::endl;
    std::cout << "L1 Cache Size: " << flight_wasm_bench_constants::L1_CACHE_SIZE / 1024 << " KB" << std::endl;
    std::cout << "Memory Bandwidth: " << flight_wasm_bench_constants::MEMORY_BANDWIDTH / (1024*1024) << " MB/s" << std::endl;
    std::cout << "Cycles per Nanosecond: " << flight_wasm_bench_constants::CYCLES_PER_NANOSECOND << std::endl;
    std::cout << "===========================================" << std::endl;
}

// =============================================================================
// Performance Target Validation
// =============================================================================

bool validate_performance_target(const std::string& name, double time_ns) {
    // Value construction and type checking targets
    if (name.find("ValueConstruction") != std::string::npos ||
        name.find("TypeChecking") != std::string::npos) {
        return time_ns <= flight_wasm_bench_constants::MAX_VALUE_CONSTRUCTION_TIME.count();
    }
    
    // Type conversion targets
    if (name.find("TypeConversion") != std::string::npos) {
        return time_ns <= flight_wasm_bench_constants::MAX_TYPE_CONVERSION_TIME.count();
    }
    
    // LEB128 decoding targets
    if (name.find("LEB128") != std::string::npos) {
        return time_ns <= flight_wasm_bench_constants::MAX_LEB128_DECODE_TIME.count();
    }
    
    // Parsing throughput targets (checked separately in benchmark implementation)
    if (name.find("ModuleParsing") != std::string::npos) {
        // Throughput validation is handled in the benchmark itself
        return true;
    }
    
    // Default: assume target met for non-critical benchmarks
    return true;
}

// =============================================================================
// Simple Performance Reporter
// =============================================================================

void print_benchmark_summary() {
    std::cout << "\n=================================================" << std::endl;
    std::cout << "🚀 Flight WASM Benchmark Execution Complete!" << std::endl;
    std::cout << "For detailed performance validation, run:" << std::endl;
    std::cout << "  cmake --build . --target run-benchmarks-detailed" << std::endl;
    std::cout << "  python3 scripts/validate_performance.py benchmark_results.json" << std::endl;
    std::cout << "=================================================" << std::endl;
}

// =============================================================================
// Main Benchmark Entry Point
// =============================================================================

int main(int argc, char** argv) {
    // Configure benchmark defaults based on platform
    configure_benchmark_defaults();
    
    // Initialize benchmark framework
    ::benchmark::Initialize(&argc, argv);
    
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return 1;
    }
    
    std::cout << "\n=== Flight WASM Performance Benchmark Results ===" << std::endl;
    
    // Run all benchmarks
    ::benchmark::RunSpecifiedBenchmarks();
    
    // Print summary
    print_benchmark_summary();
    
    return 0;
}
