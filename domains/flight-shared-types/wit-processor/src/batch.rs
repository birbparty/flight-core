//! Batch processing operations for the Flight WIT Processor
//!
//! This module provides high-level functions for processing multiple WIT files
//! efficiently, with a focus on CI/CD integration and automation workflows.

use std::path::Path;
use std::time::Instant;

#[cfg(feature = "parallel")]
use rayon::prelude::*;

use crate::error::{Result, WitError};
use crate::types::{WitDocument, ValidationReport, ProcessingReport, BatchValidationReport};
use crate::parser::WitParser;
use crate::validator::WitValidator;

/// Batch validate all WIT files in a directory
/// 
/// This is a high-level convenience function for CI/CD workflows that need to
/// validate entire directories of WIT files. It provides comprehensive reporting
/// and handles errors gracefully.
/// 
/// # Arguments
/// 
/// * `path` - Path to the directory containing WIT files
/// 
/// # Returns
/// 
/// Returns a BatchValidationReport containing results for all processed files.
/// Individual file errors are collected rather than causing the entire operation to fail.
/// 
/// # Example
/// 
/// ```rust
/// use flight_wit_processor::batch_validate_directory;
/// use std::path::Path;
/// 
/// let report = batch_validate_directory(Path::new("./wit"))?;
/// if report.success {
///     println!("All {} files validated successfully!", report.stats.total_files);
/// } else {
///     println!("Validation failed: {} errors", report.errors.len());
/// }
/// # Ok::<(), Box<dyn std::error::Error>>(())
/// ```
pub fn batch_validate_directory(path: &Path) -> Result<BatchValidationReport> {
    let parser = WitParser::new();
    let validator = WitValidator::new();
    
    batch_validate_directory_with_config(path, &parser, &validator)
}

/// Batch validate directory with custom configuration
/// 
/// Provides the same functionality as `batch_validate_directory` but allows
/// customization of parser and validator behavior.
/// 
/// # Arguments
/// 
/// * `path` - Path to the directory containing WIT files
/// * `parser` - Configured WIT parser
/// * `validator` - Configured WIT validator
/// 
/// # Returns
/// 
/// Returns a BatchValidationReport containing results for all processed files.
pub fn batch_validate_directory_with_config(
    path: &Path, 
    parser: &WitParser, 
    validator: &WitValidator
) -> Result<BatchValidationReport> {
    let start_time = Instant::now();
    
    // Parse all WIT files in the directory
    let documents = match parser.parse_directory(path) {
        Ok(docs) => docs,
        Err(e) => {
            // If we can't even parse the directory, return early with error
            let mut report = BatchValidationReport::new(Vec::new(), ValidationReport::new());
            report.add_error(e);
            return Ok(report);
        }
    };
    
    // Validate all documents together
    let validation_report = match validator.validate_all(&documents) {
        Ok(report) => report,
        Err(e) => {
            let mut report = BatchValidationReport::new(documents, ValidationReport::new());
            report.add_error(e);
            return Ok(report);
        }
    };
    
    let mut batch_report = BatchValidationReport::new(documents, validation_report);
    
    // Update timing information
    let duration = start_time.elapsed();
    batch_report.validation.metrics.duration_ms = duration.as_millis() as u64;
    
    Ok(batch_report)
}

/// Parse and validate a single file, returning comprehensive results
/// 
/// This function combines parsing and validation into a single operation,
/// providing detailed reporting suitable for both programmatic use and human consumption.
/// 
/// # Arguments
/// 
/// * `path` - Path to the WIT file to process
/// 
/// # Returns
/// 
/// Returns a ProcessingReport containing both the parsed document and validation results.
/// 
/// # Example
/// 
/// ```rust
/// use flight_wit_processor::parse_and_validate;
/// use std::path::Path;
/// 
/// let report = parse_and_validate(Path::new("memory-types.wit"))?;
/// if report.is_success() {
///     println!("File processed successfully!");
///     println!("Found {} interfaces", report.document.interface_count());
/// } else {
///     println!("Processing failed: {}", report.validation.summary());
/// }
/// # Ok::<(), Box<dyn std::error::Error>>(())
/// ```
pub fn parse_and_validate(path: &Path) -> Result<ProcessingReport> {
    let parser = WitParser::new();
    let validator = WitValidator::new();
    
    parse_and_validate_with_config(path, &parser, &validator)
}

/// Parse and validate a file with custom configuration
/// 
/// Provides the same functionality as `parse_and_validate` but allows
/// customization of parser and validator behavior.
/// 
/// # Arguments
/// 
/// * `path` - Path to the WIT file to process
/// * `parser` - Configured WIT parser
/// * `validator` - Configured WIT validator
/// 
/// # Returns
/// 
/// Returns a ProcessingReport containing both the parsed document and validation results.
pub fn parse_and_validate_with_config(
    path: &Path, 
    parser: &WitParser, 
    validator: &WitValidator
) -> Result<ProcessingReport> {
    // Parse the file
    let document = parser.parse_file(path)?;
    
    // Validate the document
    let validation_report = validator.validate(&document)?;
    
    Ok(ProcessingReport::new(document, validation_report))
}

/// Batch process multiple files in parallel (requires "parallel" feature)
/// 
/// This function processes multiple WIT files concurrently, which can significantly
/// improve performance when processing large numbers of files. The level of parallelism
/// is automatically determined based on available CPU cores.
/// 
/// # Arguments
/// 
/// * `paths` - Iterator of paths to WIT files to process
/// * `parser` - Configured WIT parser (must be thread-safe)
/// * `validator` - Configured WIT validator (must be thread-safe)
/// 
/// # Returns
/// 
/// Returns a vector of ProcessingReport instances, one for each successfully processed file.
/// Files that fail to process are omitted from the results, but errors are logged.
/// 
/// # Note
/// 
/// This function is only available when the "parallel" feature is enabled.
#[cfg(feature = "parallel")]
pub fn batch_process_parallel<I>(
    paths: I,
    parser: &WitParser,
    validator: &WitValidator,
) -> Vec<ProcessingReport>
where
    I: IntoParallelIterator,
    I::Item: AsRef<Path> + Send,
{
    paths
        .into_par_iter()
        .filter_map(|path| {
            match parse_and_validate_with_config(path.as_ref(), parser, validator) {
                Ok(report) => Some(report),
                Err(e) => {
                    eprintln!("Warning: Failed to process {}: {}", path.as_ref().display(), e);
                    None
                }
            }
        })
        .collect()
}

/// Batch process multiple files sequentially
/// 
/// This function processes multiple WIT files one at a time, which is more memory-efficient
/// than parallel processing but potentially slower for large numbers of files.
/// 
/// # Arguments
/// 
/// * `paths` - Iterator of paths to WIT files to process
/// * `parser` - Configured WIT parser
/// * `validator` - Configured WIT validator
/// 
/// # Returns
/// 
/// Returns a vector of ProcessingReport instances, one for each successfully processed file.
/// Files that fail to process are omitted from the results, but errors are logged.
pub fn batch_process_sequential<I>(
    paths: I,
    parser: &WitParser,
    validator: &WitValidator,
) -> Vec<ProcessingReport>
where
    I: IntoIterator,
    I::Item: AsRef<Path>,
{
    paths
        .into_iter()
        .filter_map(|path| {
            match parse_and_validate_with_config(path.as_ref(), parser, validator) {
                Ok(report) => Some(report),
                Err(e) => {
                    eprintln!("Warning: Failed to process {}: {}", path.as_ref().display(), e);
                    None
                }
            }
        })
        .collect()
}

/// Quick validation check for CI/CD pipelines
/// 
/// This function provides a fast validation check suitable for CI/CD pipelines where
/// you just need to know if validation passes or fails, without detailed reporting.
/// 
/// # Arguments
/// 
/// * `path` - Path to the directory or file to validate
/// 
/// # Returns
/// 
/// Returns `true` if all validations pass, `false` otherwise.
/// Errors are printed to stderr for debugging.
/// 
/// # Example
/// 
/// ```rust
/// use flight_wit_processor::quick_validate;
/// use std::path::Path;
/// 
/// if quick_validate(Path::new("./wit")) {
///     println!("✓ All WIT files are valid");
///     std::process::exit(0);
/// } else {
///     println!("✗ Validation failed");
///     std::process::exit(1);
/// }
/// ```
pub fn quick_validate(path: &Path) -> bool {
    if path.is_file() {
        // Single file validation
        match parse_and_validate(path) {
            Ok(report) => {
                if !report.is_success() {
                    eprintln!("Validation failed for {}: {}", path.display(), report.validation.summary());
                }
                report.is_success()
            },
            Err(e) => {
                eprintln!("Error processing {}: {}", path.display(), e);
                false
            }
        }
    } else if path.is_dir() {
        // Directory validation
        match batch_validate_directory(path) {
            Ok(report) => {
                if !report.success {
                    eprintln!("Batch validation failed: {} errors", report.errors.len());
                    for error in &report.errors {
                        eprintln!("  - {}", error);
                    }
                }
                report.success
            },
            Err(e) => {
                eprintln!("Error processing directory {}: {}", path.display(), e);
                false
            }
        }
    } else {
        eprintln!("Path does not exist or is not accessible: {}", path.display());
        false
    }
}

/// Memory-constrained batch processing for embedded environments
/// 
/// This function is optimized for processing WIT files in memory-constrained
/// environments like embedded systems. It processes files one at a time and
/// uses minimal memory.
/// 
/// # Arguments
/// 
/// * `path` - Path to the directory containing WIT files
/// * `max_memory_mb` - Maximum memory usage in megabytes
/// 
/// # Returns
/// 
/// Returns a simplified validation result indicating success or failure.
/// Detailed reporting is minimal to conserve memory.
pub fn memory_constrained_validate(path: &Path, max_memory_mb: usize) -> Result<bool> {
    let parser = WitParser::memory_constrained()
        .with_max_memory_usage((max_memory_mb * 1024 * 1024) as u64);
    let validator = WitValidator::lenient(); // Use lenient validation to save memory
    
    // Process files one at a time to minimize memory usage
    let wit_files = parser.parse_directory(path)?;
    
    for document in wit_files {
        let validation = validator.validate(&document)?;
        if !validation.is_valid() {
            return Ok(false);
        }
    }
    
    Ok(true)
}

/// Generate a summary report for batch validation results
/// 
/// Creates a human-readable summary of batch validation results, suitable for
/// displaying in CI/CD logs or development environments.
/// 
/// # Arguments
/// 
/// * `report` - The batch validation report to summarize
/// 
/// # Returns
/// 
/// Returns a formatted string containing the summary.
/// 
/// # Example
/// 
/// ```rust
/// use flight_wit_processor::{batch_validate_directory, generate_batch_summary};
/// use std::path::Path;
/// 
/// let report = batch_validate_directory(Path::new("./wit"))?;
/// let summary = generate_batch_summary(&report);
/// println!("{}", summary);
/// # Ok::<(), Box<dyn std::error::Error>>(())
/// ```
pub fn generate_batch_summary(report: &BatchValidationReport) -> String {
    let mut summary = String::new();
    
    summary.push_str(&format!("📊 Batch Validation Summary\n"));
    summary.push_str(&format!("═══════════════════════════\n\n"));
    
    // Overall status
    if report.success {
        summary.push_str("✅ Status: PASSED\n");
    } else {
        summary.push_str("❌ Status: FAILED\n");
    }
    
    // File statistics
    summary.push_str(&format!("📁 Files: {} total, {} successful, {} failed\n",
        report.stats.total_files,
        report.stats.successful_files,
        report.stats.failed_files
    ));
    
    // Content statistics
    summary.push_str(&format!("🔧 Interfaces: {}\n", report.stats.total_interfaces));
    summary.push_str(&format!("🌍 Worlds: {}\n", report.stats.total_worlds));
    summary.push_str(&format!("💾 Total Size: {:.2} KB\n", report.stats.total_size_bytes as f64 / 1024.0));
    
    // Validation results
    summary.push_str(&format!("\n📋 Validation Results:\n"));
    summary.push_str(&format!("   Errors: {}\n", report.validation.errors.len()));
    summary.push_str(&format!("   Warnings: {}\n", report.validation.warnings.len()));
    summary.push_str(&format!("   Info: {}\n", report.validation.info.len()));
    
    // Performance metrics
    summary.push_str(&format!("\n⚡ Performance:\n"));
    summary.push_str(&format!("   Duration: {}ms\n", report.validation.metrics.duration_ms));
    summary.push_str(&format!("   Peak Memory: {:.2} MB\n", 
        report.validation.metrics.peak_memory_bytes as f64 / (1024.0 * 1024.0)
    ));
    
    // Error details (if any)
    if !report.errors.is_empty() {
        summary.push_str(&format!("\n🚨 Processing Errors:\n"));
        for (i, error) in report.errors.iter().enumerate() {
            summary.push_str(&format!("   {}. {}\n", i + 1, error));
        }
    }
    
    // Validation issue details (if any)
    if !report.validation.errors.is_empty() {
        summary.push_str(&format!("\n❌ Validation Errors:\n"));
        for (i, error) in report.validation.errors.iter().enumerate() {
            summary.push_str(&format!("   {}. [{}] {}\n", i + 1, error.code, error.message));
            if let Some(path) = &error.path {
                summary.push_str(&format!("      File: {}\n", path.display()));
            }
        }
    }
    
    summary
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::path::PathBuf;
    use tempfile::TempDir;
    use std::fs;
    
    #[test]
    fn test_quick_validate_nonexistent() {
        let result = quick_validate(Path::new("/nonexistent/path"));
        assert!(!result);
    }
    
    #[test]
    fn test_memory_constrained_validate() {
        let temp_dir = TempDir::new().unwrap();
        
        // Create a simple WIT file for testing
        let wit_content = r#"
package test:memory@1.0.0;

interface test {
    test-function: func() -> string;
}
"#;
        
        let wit_file = temp_dir.path().join("test.wit");
        fs::write(&wit_file, wit_content).unwrap();
        
        let result = memory_constrained_validate(temp_dir.path(), 16);
        assert!(result.is_ok());
    }
    
    #[test]
    fn test_batch_summary_generation() {
        let documents = vec![];
        let validation = ValidationReport::new();
        let report = BatchValidationReport::new(documents, validation);
        
        let summary = generate_batch_summary(&report);
        assert!(summary.contains("Batch Validation Summary"));
        assert!(summary.contains("Status: PASSED"));
    }
    
    #[cfg(feature = "parallel")]
    #[test]
    fn test_parallel_processing() {
        let paths: Vec<PathBuf> = vec![];
        let parser = WitParser::new();
        let validator = WitValidator::new();
        
        let results = batch_process_parallel(paths, &parser, &validator);
        assert_eq!(results.len(), 0);
    }
}
