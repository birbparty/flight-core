//! Basic usage example for the Flight WIT Processor
//!
//! This example demonstrates how to use the core functionality of the library
//! for parsing and validating WIT files.

use std::path::Path;
use flight_wit_processor::{WitProcessor, parse_and_validate, batch_validate_directory};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    println!("🚀 Flight WIT Processor - Basic Usage Example");
    println!("═══════════════════════════════════════════════");
    
    // Create a WIT processor with default configuration
    let processor = WitProcessor::new();
    
    // Example 1: Parse a single WIT file (if it exists)
    if let Ok(document) = processor.parse_file(Path::new("../wit/memory-types.wit")) {
        println!("\n📄 Parsed WIT file:");
        println!("   Package: {}", document.package_name);
        if let Some(version) = &document.package_version {
            println!("   Version: {}", version);
        }
        println!("   Interfaces: {}", document.interfaces.len());
        println!("   Worlds: {}", document.worlds.len());
        println!("   File size: {} bytes", document.file_size);
        println!("   Parse time: {}ms", document.parse_duration_ms);
        
        // Validate the parsed document
        let validation_report = processor.validate(&document)?;
        println!("\n✅ Validation Results:");
        println!("   Valid: {}", validation_report.is_valid());
        println!("   Errors: {}", validation_report.errors.len());
        println!("   Warnings: {}", validation_report.warnings.len());
        println!("   Info messages: {}", validation_report.info.len());
    } else {
        println!("\n⚠️  No WIT files found in ../wit/ directory");
        println!("   This is expected when running the example standalone");
    }
    
    // Example 2: Parse and validate in one operation
    println!("\n🔍 Example: Parse and validate in one step");
    if let Ok(report) = parse_and_validate(Path::new("../wit/memory-types.wit")) {
        println!("   Processing successful: {}", report.is_success());
        println!("   Package: {}", report.document.package_name);
        println!("   Validation summary: {}", report.validation.summary());
    } else {
        println!("   (No file found - this is expected in standalone mode)");
    }
    
    // Example 3: Batch validation of directory
    println!("\n📁 Example: Batch validation");
    if let Ok(batch_report) = batch_validate_directory(Path::new("../wit")) {
        println!("   Total files: {}", batch_report.stats.total_files);
        println!("   Successful: {}", batch_report.stats.successful_files);
        println!("   Failed: {}", batch_report.stats.failed_files);
        println!("   Total interfaces: {}", batch_report.stats.total_interfaces);
        println!("   Total worlds: {}", batch_report.stats.total_worlds);
        println!("   Processing time: {}ms", batch_report.validation.metrics.duration_ms);
    } else {
        println!("   (No WIT directory found - this is expected in standalone mode)");
    }
    
    // Example 4: Different parser configurations
    println!("\n⚙️  Example: Different parser configurations");
    
    // Memory-constrained parser (for embedded environments)
    let memory_parser = flight_wit_processor::WitParser::memory_constrained();
    println!("   Memory-constrained parser configured for 16MB environments");
    
    // High-performance parser (for CI/CD environments)
    let high_perf_parser = flight_wit_processor::WitParser::high_performance();
    println!("   High-performance parser configured for server environments");
    
    // Custom configuration
    let custom_parser = flight_wit_processor::WitParser::new()
        .with_max_file_size(5 * 1024 * 1024) // 5MB max
        .with_raw_content(false); // Don't store raw content
    println!("   Custom parser with 5MB file limit and no raw content storage");
    
    // Example 5: Different validator configurations
    println!("\n🔍 Example: Different validator configurations");
    
    let strict_validator = flight_wit_processor::WitValidator::strict();
    println!("   Strict validator: enforces all naming conventions and constraints");
    
    let lenient_validator = flight_wit_processor::WitValidator::lenient();
    println!("   Lenient validator: more permissive for development environments");
    
    let custom_validator = flight_wit_processor::WitValidator::new()
        .with_strict_naming(true)
        .with_max_type_depth(5)
        .with_max_function_params(10);
    println!("   Custom validator: strict naming, max 5 type depth, max 10 params");
    
    println!("\n✨ Example completed successfully!");
    println!("\n💡 Next steps:");
    println!("   • Try running with actual WIT files in the ../wit directory");
    println!("   • Experiment with different parser and validator configurations");
    println!("   • Use the CLI tool: cargo run --bin flight-wit -- --help");
    println!("   • Run benchmarks: cargo bench");
    
    Ok(())
}
