#!/usr/bin/env python3
# =============================================================================
# Flight WASM Foundation - Performance Validation Script
# Automated Performance Target Validation and Regression Detection
# =============================================================================

import json
import sys
import argparse
from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass
from pathlib import Path
import statistics

@dataclass
class PerformanceTarget:
    """Defines performance targets for benchmarks."""
    max_time_ns: Optional[float] = None
    min_throughput_mbps: Optional[float] = None
    max_memory_bytes: Optional[int] = None
    max_regression_percent: float = 5.0
    description: str = ""

@dataclass
class BenchmarkResult:
    """Represents a single benchmark result."""
    name: str
    real_time_ns: float
    cpu_time_ns: float
    iterations: int
    bytes_per_second: Optional[float] = None
    items_per_second: Optional[float] = None
    label: str = ""
    counters: Dict[str, float] = None

class PerformanceValidator:
    """Validates benchmark results against performance targets."""
    
    def __init__(self):
        self.targets = self._define_performance_targets()
        self.failures = []
        self.warnings = []
        self.successes = []
    
    def _define_performance_targets(self) -> Dict[str, PerformanceTarget]:
        """Define performance targets for each benchmark category."""
        return {
            # Value construction and type checking (< 1 CPU cycle)
            'ValueConstruction': PerformanceTarget(
                max_time_ns=1.0,
                description="Value construction must be < 1 CPU cycle"
            ),
            'TypeChecking': PerformanceTarget(
                max_time_ns=1.0,
                description="Type checking must be < 1 CPU cycle"
            ),
            
            # Type conversion (< 5 CPU cycles)
            'TypeConversion': PerformanceTarget(
                max_time_ns=5.0,
                description="Type conversion must be < 5 CPU cycles"
            ),
            
            # LEB128 decoding (< 10 nanoseconds)
            'LEB128': PerformanceTarget(
                max_time_ns=10.0,
                description="LEB128 decoding must be < 10ns"
            ),
            
            # Magic number validation (< 1 nanosecond)
            'MagicNumber': PerformanceTarget(
                max_time_ns=1.0,
                description="Magic number validation must be < 1ns"
            ),
            
            # Module parsing throughput (> 100MB/s)
            'ModuleParsing': PerformanceTarget(
                min_throughput_mbps=100.0,
                description="Module parsing must achieve > 100MB/s"
            ),
            
            # UTF-8 validation
            'UTF8_Validation_ASCII': PerformanceTarget(
                max_time_ns=20.0,
                description="ASCII UTF-8 validation must be < 20ns"
            ),
            'UTF8_Validation_Multibyte': PerformanceTarget(
                max_time_ns=50.0,
                description="Multibyte UTF-8 validation must be < 50ns"
            ),
            
            # Error handling
            'ErrorConstruction': PerformanceTarget(
                max_time_ns=50.0,
                description="Error construction must be < 50ns"
            ),
            'Expected_Success': PerformanceTarget(
                max_time_ns=1.0,
                description="Expected<T> success path must be < 1ns"
            ),
            
            # Platform operations
            'PlatformDetection': PerformanceTarget(
                max_time_ns=100.0,
                description="Platform detection must be < 100ns"
            ),
            'ByteSwap': PerformanceTarget(
                max_time_ns=2.0,
                description="Byte swapping must be < 2ns"
            ),
            
            # Memory constraints
            'ZeroAllocation': PerformanceTarget(
                max_memory_bytes=0,
                description="Core operations must perform zero allocations"
            ),
            'ValueMemoryLayout': PerformanceTarget(
                max_memory_bytes=16,  # bytes per value
                description="Value storage must be <= 16 bytes per value"
            )
        }
    
    def validate_benchmark_results(self, results_file: Path) -> bool:
        """Validate all benchmark results against targets."""
        try:
            with open(results_file, 'r') as f:
                data = json.load(f)
        except (FileNotFoundError, json.JSONDecodeError) as e:
            print(f"Error loading benchmark results: {e}")
            return False
        
        if 'benchmarks' not in data:
            print("Invalid benchmark results format: missing 'benchmarks' key")
            return False
        
        print("🚀 Flight WASM Performance Validation")
        print("=" * 50)
        
        all_passed = True
        
        for benchmark_data in data['benchmarks']:
            result = self._parse_benchmark_result(benchmark_data)
            if result:
                passed = self._validate_single_benchmark(result)
                if not passed:
                    all_passed = False
        
        self._print_summary()
        return all_passed
    
    def _parse_benchmark_result(self, data: dict) -> Optional[BenchmarkResult]:
        """Parse a single benchmark result from JSON."""
        try:
            return BenchmarkResult(
                name=data['name'],
                real_time_ns=data['real_time'],
                cpu_time_ns=data['cpu_time'],
                iterations=data['iterations'],
                bytes_per_second=data.get('bytes_per_second'),
                items_per_second=data.get('items_per_second'),
                label=data.get('label', ''),
                counters=data.get('counters', {})
            )
        except KeyError as e:
            print(f"Warning: Malformed benchmark result, missing key: {e}")
            return None
    
    def _validate_single_benchmark(self, result: BenchmarkResult) -> bool:
        """Validate a single benchmark result against targets."""
        # Find matching target
        target = self._find_matching_target(result.name)
        if not target:
            # No specific target defined, skip validation
            return True
        
        passed = True
        
        # Validate timing targets
        if target.max_time_ns is not None:
            if result.real_time_ns > target.max_time_ns:
                self.failures.append(
                    f"❌ {result.name}: {result.real_time_ns:.2f}ns > {target.max_time_ns}ns target"
                )
                passed = False
            else:
                self.successes.append(
                    f"✅ {result.name}: {result.real_time_ns:.2f}ns (target: <{target.max_time_ns}ns)"
                )
        
        # Validate throughput targets
        if target.min_throughput_mbps is not None and result.bytes_per_second:
            throughput_mbps = result.bytes_per_second / (1024 * 1024)
            if throughput_mbps < target.min_throughput_mbps:
                self.failures.append(
                    f"❌ {result.name}: {throughput_mbps:.1f}MB/s < {target.min_throughput_mbps}MB/s target"
                )
                passed = False
            else:
                self.successes.append(
                    f"✅ {result.name}: {throughput_mbps:.1f}MB/s (target: >{target.min_throughput_mbps}MB/s)"
                )
        
        # Validate memory targets
        if target.max_memory_bytes is not None:
            memory_used = self._extract_memory_usage(result)
            if memory_used is not None and memory_used > target.max_memory_bytes:
                self.failures.append(
                    f"❌ {result.name}: {memory_used} bytes > {target.max_memory_bytes} bytes target"
                )
                passed = False
            elif memory_used is not None:
                self.successes.append(
                    f"✅ {result.name}: {memory_used} bytes (target: <={target.max_memory_bytes} bytes)"
                )
        
        return passed
    
    def _find_matching_target(self, benchmark_name: str) -> Optional[PerformanceTarget]:
        """Find the best matching performance target for a benchmark."""
        # Try exact match first
        if benchmark_name in self.targets:
            return self.targets[benchmark_name]
        
        # Try substring matching
        for target_key, target in self.targets.items():
            if target_key in benchmark_name:
                return target
        
        return None
    
    def _extract_memory_usage(self, result: BenchmarkResult) -> Optional[int]:
        """Extract memory usage from benchmark counters."""
        if not result.counters:
            return None
        
        # Check various counter names for memory usage
        memory_keys = [
            'allocations_bytes',
            'memory_used_bytes', 
            'memory_leak_bytes',
            'bytes_per_value'
        ]
        
        for key in memory_keys:
            if key in result.counters:
                return int(result.counters[key])
        
        return None
    
    def _print_summary(self):
        """Print validation summary."""
        print(f"\n📊 Validation Summary")
        print(f"{'=' * 30}")
        print(f"✅ Passed: {len(self.successes)}")
        print(f"❌ Failed: {len(self.failures)}")
        print(f"⚠️  Warnings: {len(self.warnings)}")
        
        if self.failures:
            print(f"\n🔥 Performance Target Failures:")
            for failure in self.failures:
                print(f"  {failure}")
        
        if self.warnings:
            print(f"\n⚠️  Performance Warnings:")
            for warning in self.warnings:
                print(f"  {warning}")
        
        if not self.failures:
            print(f"\n🎉 ALL PERFORMANCE TARGETS MET!")
            print(f"Flight WASM is ready for high-performance WebAssembly processing! 🚀")
        else:
            print(f"\n💪 Performance optimization needed before release.")
            print(f"Please address the failed targets above.")

class RegressionAnalyzer:
    """Analyzes performance regressions over time."""
    
    def __init__(self, baseline_dir: Path):
        self.baseline_dir = baseline_dir
        self.baseline_dir.mkdir(exist_ok=True)
    
    def analyze_regressions(self, results_file: Path) -> bool:
        """Analyze performance regressions against historical baselines."""
        print(f"\n📈 Regression Analysis")
        print(f"{'=' * 30}")
        
        try:
            with open(results_file, 'r') as f:
                data = json.load(f)
        except (FileNotFoundError, json.JSONDecodeError) as e:
            print(f"Error loading benchmark results: {e}")
            return False
        
        regression_count = 0
        improvement_count = 0
        
        for benchmark_data in data['benchmarks']:
            name = benchmark_data['name']
            current_time = benchmark_data['real_time']
            
            baseline_time = self._load_baseline(name)
            if baseline_time is None:
                # No baseline, save current as baseline
                self._save_baseline(name, current_time)
                continue
            
            regression_percent = (current_time - baseline_time) / baseline_time * 100
            
            if regression_percent > 5.0:  # 5% regression threshold
                print(f"📉 REGRESSION: {name} is {regression_percent:.1f}% slower")
                regression_count += 1
            elif regression_percent < -15.0:  # 15% improvement threshold
                print(f"📈 IMPROVEMENT: {name} is {-regression_percent:.1f}% faster")
                self._save_baseline(name, current_time)  # Update baseline
                improvement_count += 1
            else:
                print(f"✅ STABLE: {name} ({regression_percent:+.1f}%)")
        
        print(f"\nRegression Summary:")
        print(f"  Regressions: {regression_count}")
        print(f"  Improvements: {improvement_count}")
        
        return regression_count == 0
    
    def _load_baseline(self, benchmark_name: str) -> Optional[float]:
        """Load baseline performance for a benchmark."""
        baseline_file = self.baseline_dir / f"{benchmark_name}.baseline"
        if not baseline_file.exists():
            return None
        
        try:
            with open(baseline_file, 'r') as f:
                return float(f.read().strip())
        except (ValueError, IOError):
            return None
    
    def _save_baseline(self, benchmark_name: str, time_ns: float):
        """Save baseline performance for a benchmark."""
        baseline_file = self.baseline_dir / f"{benchmark_name}.baseline"
        with open(baseline_file, 'w') as f:
            f.write(str(time_ns))

def main():
    """Main entry point for performance validation."""
    parser = argparse.ArgumentParser(
        description="Validate Flight WASM benchmark performance targets"
    )
    parser.add_argument(
        "results_file",
        type=Path,
        help="JSON file containing benchmark results"
    )
    parser.add_argument(
        "--baseline-dir",
        type=Path,
        default=Path("baselines"),
        help="Directory containing performance baselines"
    )
    parser.add_argument(
        "--regression-analysis",
        action="store_true",
        help="Perform regression analysis against baselines"
    )
    parser.add_argument(
        "--strict",
        action="store_true", 
        help="Exit with error code if any targets fail"
    )
    
    args = parser.parse_args()
    
    if not args.results_file.exists():
        print(f"Error: Benchmark results file not found: {args.results_file}")
        sys.exit(1)
    
    # Validate performance targets
    validator = PerformanceValidator()
    targets_passed = validator.validate_benchmark_results(args.results_file)
    
    # Analyze regressions if requested
    regressions_passed = True
    if args.regression_analysis:
        analyzer = RegressionAnalyzer(args.baseline_dir)
        regressions_passed = analyzer.analyze_regressions(args.results_file)
    
    # Exit with appropriate code
    if args.strict and (not targets_passed or not regressions_passed):
        print(f"\n❌ Performance validation failed!")
        sys.exit(1)
    else:
        print(f"\n✅ Performance validation completed.")
        sys.exit(0)

if __name__ == "__main__":
    main()
