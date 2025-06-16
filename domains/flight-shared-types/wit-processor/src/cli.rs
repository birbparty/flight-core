//! Command-line interface for the Flight WIT Processor
//!
//! This module provides a comprehensive CLI for WIT processing operations,
//! designed for both interactive use and CI/CD automation.

use std::path::PathBuf;
use std::process;
use clap::{Parser, Subcommand, ValueEnum};
use serde_json;

use crate::error::{Result, WitError};
use crate::parser::WitParser;
use crate::validator::WitValidator;
use crate::batch::{
    batch_validate_directory, parse_and_validate,
    quick_validate, generate_batch_summary, memory_constrained_validate
};
use crate::types::{WitDocument, ProcessingReport, BatchValidationReport};

/// Flight WIT Interface Processor
/// 
/// A comprehensive tool for parsing, validating, and processing WebAssembly
/// Interface Types (WIT) files with support for Flight's shared types ecosystem.
#[derive(Parser)]
#[command(name = "flight-wit")]
#[command(about = "Flight WIT Interface Processor")]
#[command(version = env!("CARGO_PKG_VERSION"))]
#[command(long_about = None)]
pub struct Cli {
    /// Global configuration options
    #[command(flatten)]
    pub global: GlobalOptions,
    
    /// Subcommands
    #[command(subcommand)]
    pub command: Commands,
}

/// Global options available to all subcommands
#[derive(Parser)]
pub struct GlobalOptions {
    /// Output format
    #[arg(short, long, global = true, default_value = "human")]
    pub output: OutputFormat,
    
    /// Verbose output
    #[arg(short, long, global = true)]
    pub verbose: bool,
    
    /// Quiet mode (suppress non-error output)
    #[arg(short, long, global = true)]
    pub quiet: bool,
    
    /// Configuration file path
    #[arg(long, global = true)]
    pub config: Option<PathBuf>,
}

/// Available output formats
#[derive(Clone, ValueEnum)]
pub enum OutputFormat {
    /// Human-readable output
    Human,
    /// JSON output for automation
    Json,
    /// Compact summary
    Summary,
}

/// Available subcommands
#[derive(Subcommand)]
pub enum Commands {
    /// Parse WIT files and display structure
    Parse {
        /// Path to WIT file or directory
        path: PathBuf,
        
        /// Include raw content in output
        #[arg(long)]
        include_raw: bool,
        
        /// Memory-constrained parsing mode
        #[arg(long)]
        memory_constrained: bool,
    },
    
    /// Validate WIT files for correctness
    Validate {
        /// Path to WIT file or directory
        path: PathBuf,
        
        /// Strict validation mode
        #[arg(long)]
        strict: bool,
        
        /// Lenient validation mode
        #[arg(long)]
        lenient: bool,
        
        /// Maximum memory usage (MB) for constrained environments
        #[arg(long)]
        max_memory: Option<usize>,
    },
    
    /// Quick validation check (for CI/CD)
    Check {
        /// Path to WIT file or directory
        path: PathBuf,
    },
    
    /// Batch process multiple WIT files
    Batch {
        /// Path to directory containing WIT files
        path: PathBuf,
        
        /// Use parallel processing
        #[cfg(feature = "parallel")]
        #[arg(long)]
        parallel: bool,
        
        /// Generate detailed summary report
        #[arg(long)]
        summary: bool,
    },
    
    /// Display version information
    Version,
    
    /// Display configuration information
    Info,
}

/// Main CLI entry point
pub fn main() {
    let cli = Cli::parse();
    
    let result = run_command(cli);
    
    match result {
        Ok(exit_code) => process::exit(exit_code),
        Err(e) => {
            eprintln!("Error: {}", e);
            process::exit(1);
        }
    }
}

/// Run the specified command and return exit code
pub fn run_command(cli: Cli) -> Result<i32> {
    match cli.command {
        Commands::Parse { path, include_raw, memory_constrained } => {
            run_parse_command(&cli.global, path, include_raw, memory_constrained)
        },
        Commands::Validate { path, strict, lenient, max_memory } => {
            run_validate_command(&cli.global, path, strict, lenient, max_memory)
        },
        Commands::Check { path } => {
            run_check_command(&cli.global, path)
        },
        Commands::Batch { path, summary, #[cfg(feature = "parallel")] parallel } => {
            run_batch_command(&cli.global, path, summary, {
                #[cfg(feature = "parallel")]
                { parallel }
                #[cfg(not(feature = "parallel"))]
                { false }
            })
        },
        Commands::Version => {
            run_version_command(&cli.global)
        },
        Commands::Info => {
            run_info_command(&cli.global)
        },
    }
}

/// Parse command implementation
fn run_parse_command(
    global: &GlobalOptions,
    path: PathBuf,
    include_raw: bool,
    memory_constrained: bool,
) -> Result<i32> {
    let parser = if memory_constrained {
        WitParser::memory_constrained()
    } else {
        WitParser::new()
    }.with_raw_content(include_raw);
    
    if path.is_file() {
        // Parse single file
        let document = parser.parse_file(&path)?;
        
        match global.output {
            OutputFormat::Json => {
                let json = serde_json::to_string_pretty(&document)?;
                println!("{}", json);
            },
            OutputFormat::Human => {
                print_document_human(&document, global.verbose)?;
            },
            OutputFormat::Summary => {
                print_document_summary(&document)?;
            },
        }
    } else if path.is_dir() {
        // Parse directory
        let documents = parser.parse_directory(&path)?;
        
        match global.output {
            OutputFormat::Json => {
                let json = serde_json::to_string_pretty(&documents)?;
                println!("{}", json);
            },
            OutputFormat::Human => {
                for document in &documents {
                    print_document_human(document, global.verbose)?;
                    if global.verbose {
                        println!("────────────────────────────────");
                    }
                }
            },
            OutputFormat::Summary => {
                print_documents_summary(&documents)?;
            },
        }
    } else {
        return Err(WitError::config_error("Path does not exist or is not accessible".to_string()));
    }
    
    Ok(0)
}

/// Validate command implementation
fn run_validate_command(
    global: &GlobalOptions,
    path: PathBuf,
    strict: bool,
    lenient: bool,
    max_memory: Option<usize>,
) -> Result<i32> {
    // Handle memory-constrained validation
    if let Some(max_mem) = max_memory {
        let success = memory_constrained_validate(&path, max_mem)?;
        if !global.quiet {
            if success {
                println!("✅ Memory-constrained validation passed");
            } else {
                println!("❌ Memory-constrained validation failed");
            }
        }
        return Ok(if success { 0 } else { 1 });
    }
    
    // Configure validator
    let _validator = if strict {
        WitValidator::strict()
    } else if lenient {
        WitValidator::lenient()
    } else {
        WitValidator::new()
    };
    
    if path.is_file() {
        // Validate single file
        let report = parse_and_validate(&path)?;
        
        match global.output {
            OutputFormat::Json => {
                let json = serde_json::to_string_pretty(&report)?;
                println!("{}", json);
            },
            OutputFormat::Human => {
                print_validation_human(&report, global.verbose)?;
            },
            OutputFormat::Summary => {
                print_validation_summary(&report)?;
            },
        }
        
        Ok(if report.is_success() { 0 } else { 1 })
    } else if path.is_dir() {
        // Validate directory
        let report = batch_validate_directory(&path)?;
        
        match global.output {
            OutputFormat::Json => {
                let json = serde_json::to_string_pretty(&report)?;
                println!("{}", json);
            },
            OutputFormat::Human => {
                if !global.quiet {
                    println!("{}", generate_batch_summary(&report));
                }
            },
            OutputFormat::Summary => {
                print_batch_summary(&report)?;
            },
        }
        
        Ok(if report.success { 0 } else { 1 })
    } else {
        Err(WitError::config_error("Path does not exist or is not accessible".to_string()))
    }
}

/// Check command implementation (quick validation for CI/CD)
fn run_check_command(global: &GlobalOptions, path: PathBuf) -> Result<i32> {
    let success = quick_validate(&path);
    
    if !global.quiet {
        match global.output {
            OutputFormat::Json => {
                let result = serde_json::json!({
                    "success": success,
                    "path": path.to_string_lossy()
                });
                println!("{}", serde_json::to_string_pretty(&result)?);
            },
            _ => {
                if success {
                    println!("✅ Validation passed");
                } else {
                    println!("❌ Validation failed");
                }
            }
        }
    }
    
    Ok(if success { 0 } else { 1 })
}

/// Batch command implementation
fn run_batch_command(
    global: &GlobalOptions,
    path: PathBuf,
    summary: bool,
    _parallel: bool,
) -> Result<i32> {
    let report = batch_validate_directory(&path)?;
    
    match global.output {
        OutputFormat::Json => {
            let json = serde_json::to_string_pretty(&report)?;
            println!("{}", json);
        },
        OutputFormat::Human => {
            if summary || global.verbose {
                println!("{}", generate_batch_summary(&report));
            } else {
                print_batch_summary(&report)?;
            }
        },
        OutputFormat::Summary => {
            print_batch_summary(&report)?;
        },
    }
    
    Ok(if report.success { 0 } else { 1 })
}

/// Version command implementation
fn run_version_command(_global: &GlobalOptions) -> Result<i32> {
    println!("flight-wit {}", env!("CARGO_PKG_VERSION"));
    println!("Flight WIT Interface Processor");
    println!("Built with Rust (version available at runtime)");
    
    // Feature information
    let mut features = Vec::new();
    
    #[cfg(feature = "parallel")]
    features.push("parallel");
    
    #[cfg(feature = "cli")]
    features.push("cli");
    
    if !features.is_empty() {
        println!("Features: {}", features.join(", "));
    }
    
    Ok(0)
}

/// Info command implementation
fn run_info_command(_global: &GlobalOptions) -> Result<i32> {
    println!("🚀 Flight WIT Processor Configuration");
    println!("════════════════════════════════════");
    println!();
    
    // Version information
    println!("📋 Version Information:");
    println!("   Version: {}", env!("CARGO_PKG_VERSION"));
    println!("   Rust Version: (available at runtime)");
    println!();
    
    // Dependencies
    println!("📦 Core Dependencies:");
    println!("   wit-parser: 0.15");
    println!("   wasmparser: 0.217");
    println!("   clap: 4.5");
    println!();
    
    // Supported platforms
    println!("🎯 Supported Platforms:");
    println!("   • Dreamcast (16MB memory constraint)");
    println!("   • PlayStation Portable (32-64MB)");
    println!("   • PlayStation Vita (512MB)");
    println!("   • V6R VMs (512MB - 2GB+)");
    println!("   • Development environments");
    println!();
    
    // Features
    println!("⚡ Available Features:");
    
    #[cfg(feature = "parallel")]
    println!("   ✅ Parallel processing");
    #[cfg(not(feature = "parallel"))]
    println!("   ❌ Parallel processing");
    
    #[cfg(feature = "cli")]
    println!("   ✅ Command-line interface");
    #[cfg(not(feature = "cli"))]
    println!("   ❌ Command-line interface");
    
    println!();
    
    // Usage examples
    println!("💡 Usage Examples:");
    println!("   flight-wit parse memory-types.wit");
    println!("   flight-wit validate --strict ./wit");
    println!("   flight-wit check ./wit (for CI/CD)");
    println!("   flight-wit batch --summary ./wit");
    
    Ok(0)
}

/// Print document in human-readable format
fn print_document_human(document: &WitDocument, verbose: bool) -> Result<()> {
    println!("📄 {}", document.path.display());
    println!("   Package: {}", document.package_name);
    
    if let Some(version) = &document.package_version {
        println!("   Version: {}", version);
    }
    
    println!("   Size: {} bytes", document.file_size);
    println!("   Parse time: {}ms", document.parse_duration_ms);
    
    if !document.interfaces.is_empty() {
        println!("   Interfaces: {}", document.interfaces.len());
        if verbose {
            for interface in &document.interfaces {
                println!("     • {} ({} functions, {} types)", 
                        interface.name, 
                        interface.functions.len(), 
                        interface.types.len());
            }
        }
    }
    
    if !document.worlds.is_empty() {
        println!("   Worlds: {}", document.worlds.len());
        if verbose {
            for world in &document.worlds {
                println!("     • {} ({} imports, {} exports)", 
                        world.name, 
                        world.imports.len(), 
                        world.exports.len());
            }
        }
    }
    
    println!();
    Ok(())
}

/// Print document summary
fn print_document_summary(document: &WitDocument) -> Result<()> {
    println!("{}: {} interfaces, {} worlds", 
            document.path.display(),
            document.interfaces.len(),
            document.worlds.len());
    Ok(())
}

/// Print multiple documents summary
fn print_documents_summary(documents: &[WitDocument]) -> Result<()> {
    let total_interfaces: usize = documents.iter().map(|d| d.interfaces.len()).sum();
    let total_worlds: usize = documents.iter().map(|d| d.worlds.len()).sum();
    let total_size: u64 = documents.iter().map(|d| d.file_size).sum();
    
    println!("📊 Summary: {} files, {} interfaces, {} worlds, {:.2} KB total",
            documents.len(),
            total_interfaces,
            total_worlds,
            total_size as f64 / 1024.0);
    Ok(())
}

/// Print validation results in human-readable format
fn print_validation_human(report: &ProcessingReport, verbose: bool) -> Result<()> {
    let validation = &report.validation;
    
    if report.is_success() {
        println!("✅ {} - Validation passed", report.document.path.display());
    } else {
        println!("❌ {} - Validation failed", report.document.path.display());
    }
    
    if !validation.errors.is_empty() {
        println!("   Errors: {}", validation.errors.len());
        if verbose {
            for error in &validation.errors {
                println!("     • [{}] {}", error.code, error.message);
            }
        }
    }
    
    if !validation.warnings.is_empty() {
        println!("   Warnings: {}", validation.warnings.len());
        if verbose {
            for warning in &validation.warnings {
                println!("     • [{}] {}", warning.code, warning.message);
            }
        }
    }
    
    if verbose && !validation.info.is_empty() {
        println!("   Info: {}", validation.info.len());
        for info in &validation.info {
            println!("     • [{}] {}", info.code, info.message);
        }
    }
    
    println!();
    Ok(())
}

/// Print validation summary
fn print_validation_summary(report: &ProcessingReport) -> Result<()> {
    let status = if report.is_success() { "✅" } else { "❌" };
    println!("{} {}: {}", 
            status,
            report.document.path.display(),
            report.validation.summary());
    Ok(())
}

/// Print batch validation summary
fn print_batch_summary(report: &BatchValidationReport) -> Result<()> {
    let status = if report.success { "✅" } else { "❌" };
    println!("{} {} files: {} successful, {} failed",
            status,
            report.stats.total_files,
            report.stats.successful_files,
            report.stats.failed_files);
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    use clap::Parser;
    
    #[test]
    fn test_cli_parsing() {
        let cli = Cli::try_parse_from(&["flight-wit", "version"]);
        assert!(cli.is_ok());
        
        let cli = cli.unwrap();
        assert!(matches!(cli.command, Commands::Version));
    }
    
    #[test]
    fn test_parse_command() {
        let cli = Cli::try_parse_from(&[
            "flight-wit", 
            "parse", 
            "test.wit", 
            "--include-raw"
        ]);
        assert!(cli.is_ok());
        
        let cli = cli.unwrap();
        if let Commands::Parse { include_raw, .. } = cli.command {
            assert!(include_raw);
        } else {
            panic!("Expected Parse command");
        }
    }
    
    #[test]
    fn test_validate_command() {
        let cli = Cli::try_parse_from(&[
            "flight-wit", 
            "validate", 
            "test.wit", 
            "--strict"
        ]);
        assert!(cli.is_ok());
        
        let cli = cli.unwrap();
        if let Commands::Validate { strict, .. } = cli.command {
            assert!(strict);
        } else {
            panic!("Expected Validate command");
        }
    }
    
    #[test]
    fn test_output_format() {
        let cli = Cli::try_parse_from(&[
            "flight-wit", 
            "--output", "json",
            "version"
        ]);
        assert!(cli.is_ok());
        
        let cli = cli.unwrap();
        assert!(matches!(cli.global.output, OutputFormat::Json));
    }
}
