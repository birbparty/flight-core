//! Performance benchmarks for the Flight WIT Processor
//!
//! These benchmarks measure the performance characteristics of core operations
//! to ensure they meet the requirements for constrained environments.

use criterion::{black_box, criterion_group, criterion_main, Criterion, BenchmarkId};
use std::path::Path;
use tempfile::TempDir;
use std::fs;

use flight_wit_processor::{
    WitProcessor, WitParser, WitValidator, parse_and_validate, batch_validate_directory
};

/// Sample WIT content for benchmarking
const SIMPLE_WIT: &str = r#"
package bench:simple@1.0.0;

interface simple {
    hello: func() -> string;
    add: func(a: u32, b: u32) -> u32;
}

world simple-world {
    export simple;
}
"#;

/// More complex WIT content for benchmarking
const COMPLEX_WIT: &str = r#"
package bench:complex@1.0.0;

interface data-processing {
    process-batch: func(data: list<record-item>) -> result<list<processed-item>, error>;
    validate-input: func(input: input-data) -> result<validated-data, validation-error>;
    transform: func(data: raw-data, config: transform-config) -> transformed-data;
}

interface storage {
    store: func(key: string, value: list<u8>) -> result<storage-id, storage-error>;
    retrieve: func(id: storage-id) -> result<list<u8>, storage-error>;
    delete: func(id: storage-id) -> result<_, storage-error>;
    list: func(prefix: option<string>) -> result<list<storage-id>, storage-error>;
}

record record-item {
    id: u64,
    name: string,
    data: list<u8>,
    metadata: option<metadata>,
    tags: list<string>,
}

record processed-item {
    original-id: u64,
    processed-data: list<u8>,
    processing-time: u64,
    quality-score: f32,
}

record input-data {
    source: string,
    format: data-format,
    compression: option<compression-type>,
    size: u64,
}

record validated-data {
    normalized-source: string,
    verified-format: data-format,
    actual-size: u64,
    checksum: string,
}

record raw-data {
    content: list<u8>,
    encoding: string,
    timestamp: u64,
}

record transform-config {
    output-format: data-format,
    quality: quality-setting,
    compression: option<compression-type>,
    filters: list<filter-config>,
}

record transformed-data {
    result: list<u8>,
    metadata: transform-metadata,
}

record metadata {
    created-at: u64,
    updated-at: u64,
    version: u32,
    attributes: list<tuple<string, string>>,
}

record transform-metadata {
    input-size: u64,
    output-size: u64,
    compression-ratio: f32,
    processing-duration: u64,
}

record filter-config {
    name: string,
    parameters: list<tuple<string, string>>,
    enabled: bool,
}

variant data-format {
    json,
    xml,
    binary,
    csv,
    custom(string),
}

variant compression-type {
    none,
    gzip,
    lz4,
    zstd,
}

variant quality-setting {
    low,
    medium,
    high,
    lossless,
}

variant error {
    invalid-input(string),
    processing-failed(string),
    out-of-memory,
    timeout,
}

variant validation-error {
    invalid-format,
    size-mismatch,
    checksum-failed,
    unsupported-compression,
}

variant storage-error {
    not-found,
    access-denied,
    storage-full,
    corruption-detected,
}

type storage-id = string;

world data-processor {
    export data-processing;
    export storage;
}
"#;

/// Create a temporary WIT file for benchmarking
fn create_benchmark_file(content: &str) -> (TempDir, std::path::PathBuf) {
    let temp_dir = TempDir::new().unwrap();
    let file_path = temp_dir.path().join("benchmark.wit");
    fs::write(&file_path, content).unwrap();
    (temp_dir, file_path)
}

/// Benchmark WIT file parsing
fn bench_parsing(c: &mut Criterion) {
    let mut group = c.benchmark_group("parsing");
    
    let (_temp_dir, simple_file) = create_benchmark_file(SIMPLE_WIT);
    let (_temp_dir2, complex_file) = create_benchmark_file(COMPLEX_WIT);
    
    // Benchmark different parser configurations
    let default_parser = WitParser::new();
    let memory_parser = WitParser::memory_constrained();
    let high_perf_parser = WitParser::high_performance();
    
    group.bench_with_input(
        BenchmarkId::new("simple", "default"),
        &simple_file,
        |b, file| {
            b.iter(|| {
                default_parser.parse_file(black_box(file)).unwrap()
            })
        },
    );
    
    group.bench_with_input(
        BenchmarkId::new("simple", "memory_constrained"),
        &simple_file,
        |b, file| {
            b.iter(|| {
                memory_parser.parse_file(black_box(file)).unwrap()
            })
        },
    );
    
    group.bench_with_input(
        BenchmarkId::new("complex", "default"),
        &complex_file,
        |b, file| {
            b.iter(|| {
                default_parser.parse_file(black_box(file)).unwrap()
            })
        },
    );
    
    group.bench_with_input(
        BenchmarkId::new("complex", "high_performance"),
        &complex_file,
        |b, file| {
            b.iter(|| {
                high_perf_parser.parse_file(black_box(file)).unwrap()
            })
        },
    );
    
    group.finish();
}

/// Benchmark WIT validation
fn bench_validation(c: &mut Criterion) {
    let mut group = c.benchmark_group("validation");
    
    let processor = WitProcessor::new();
    let (_temp_dir, simple_file) = create_benchmark_file(SIMPLE_WIT);
    let (_temp_dir2, complex_file) = create_benchmark_file(COMPLEX_WIT);
    
    let simple_doc = processor.parse_file(&simple_file).unwrap();
    let complex_doc = processor.parse_file(&complex_file).unwrap();
    
    let default_validator = WitValidator::new();
    let strict_validator = WitValidator::strict();
    let lenient_validator = WitValidator::lenient();
    
    group.bench_function("simple_default", |b| {
        b.iter(|| {
            default_validator.validate(black_box(&simple_doc)).unwrap()
        })
    });
    
    group.bench_function("simple_strict", |b| {
        b.iter(|| {
            strict_validator.validate(black_box(&simple_doc)).unwrap()
        })
    });
    
    group.bench_function("complex_default", |b| {
        b.iter(|| {
            default_validator.validate(black_box(&complex_doc)).unwrap()
        })
    });
    
    group.bench_function("complex_lenient", |b| {
        b.iter(|| {
            lenient_validator.validate(black_box(&complex_doc)).unwrap()
        })
    });
    
    group.finish();
}

/// Benchmark end-to-end processing
fn bench_end_to_end(c: &mut Criterion) {
    let mut group = c.benchmark_group("end_to_end");
    
    let (_temp_dir, simple_file) = create_benchmark_file(SIMPLE_WIT);
    let (_temp_dir2, complex_file) = create_benchmark_file(COMPLEX_WIT);
    
    group.bench_function("parse_and_validate_simple", |b| {
        b.iter(|| {
            parse_and_validate(black_box(&simple_file)).unwrap()
        })
    });
    
    group.bench_function("parse_and_validate_complex", |b| {
        b.iter(|| {
            parse_and_validate(black_box(&complex_file)).unwrap()
        })
    });
    
    group.finish();
}

/// Benchmark batch processing
fn bench_batch_processing(c: &mut Criterion) {
    let mut group = c.benchmark_group("batch");
    
    // Create directories with multiple files
    let temp_dir = TempDir::new().unwrap();
    
    // Small batch (5 files)
    for i in 0..5 {
        let file_path = temp_dir.path().join(format!("file_{}.wit", i));
        fs::write(&file_path, SIMPLE_WIT).unwrap();
    }
    
    group.bench_function("batch_5_files", |b| {
        b.iter(|| {
            batch_validate_directory(black_box(temp_dir.path())).unwrap()
        })
    });
    
    // Medium batch (20 files)
    let temp_dir2 = TempDir::new().unwrap();
    for i in 0..20 {
        let file_path = temp_dir2.path().join(format!("file_{}.wit", i));
        let content = if i % 2 == 0 { SIMPLE_WIT } else { COMPLEX_WIT };
        fs::write(&file_path, content).unwrap();
    }
    
    group.bench_function("batch_20_files", |b| {
        b.iter(|| {
            batch_validate_directory(black_box(temp_dir2.path())).unwrap()
        })
    });
    
    group.finish();
}

/// Benchmark memory usage patterns
fn bench_memory_patterns(c: &mut Criterion) {
    let mut group = c.benchmark_group("memory");
    
    let (_temp_dir, simple_file) = create_benchmark_file(SIMPLE_WIT);
    
    // Test memory-constrained vs normal processing
    let normal_processor = WitProcessor::new();
    let memory_processor = WitProcessor::with_config(
        WitParser::memory_constrained(),
        WitValidator::lenient(),
    );
    
    group.bench_function("normal_processor", |b| {
        b.iter(|| {
            normal_processor.process_file(black_box(&simple_file)).unwrap()
        })
    });
    
    group.bench_function("memory_constrained_processor", |b| {
        b.iter(|| {
            memory_processor.process_file(black_box(&simple_file)).unwrap()
        })
    });
    
    group.finish();
}

criterion_group!(
    benches,
    bench_parsing,
    bench_validation,
    bench_end_to_end,
    bench_batch_processing,
    bench_memory_patterns
);
criterion_main!(benches);
