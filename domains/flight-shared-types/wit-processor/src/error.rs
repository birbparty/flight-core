//! Error types for the Flight WIT Processor
//!
//! This module defines comprehensive error handling for all WIT processing operations,
//! designed to provide clear diagnostics for both programmatic and human use.

use std::path::PathBuf;
use thiserror::Error;

/// Result type used throughout the WIT processor
pub type Result<T> = std::result::Result<T, WitError>;

/// Main error type for WIT processing operations
/// 
/// This enum covers all possible error conditions that can occur during
/// WIT parsing, validation, and processing operations. Each variant provides
/// detailed context to aid in debugging and error reporting.
#[derive(Error, Debug, Clone, serde::Serialize, serde::Deserialize)]
pub enum WitError {
    /// File I/O related errors
    #[error("Failed to read file '{path}': {message}")]
    FileRead {
        path: PathBuf,
        message: String,
    },

    /// Directory traversal errors
    #[error("Failed to read directory '{path}': {message}")]
    DirectoryRead {
        path: PathBuf,
        message: String,
    },

    /// WIT parsing errors from the underlying parser
    #[error("Failed to parse WIT file '{path}': {message}")]
    ParseError {
        path: PathBuf,
        message: String,
        line: Option<usize>,
        column: Option<usize>,
    },

    /// Validation errors for WIT content
    #[error("Validation failed for '{path}': {message}")]
    ValidationError {
        path: PathBuf,
        message: String,
        details: Vec<String>,
    },

    /// Type-related validation errors
    #[error("Type error in '{interface}': {message}")]
    TypeError {
        interface: String,
        type_name: String,
        message: String,
    },

    /// Interface dependency errors
    #[error("Dependency error: {message}")]
    DependencyError {
        message: String,
        missing_interfaces: Vec<String>,
        circular_dependencies: Vec<String>,
    },

    /// Platform constraint violations
    #[error("Platform constraint violation: {message}")]
    PlatformConstraintError {
        platform: String,
        constraint: String,
        message: String,
    },

    /// Memory usage exceeded limits
    #[error("Memory limit exceeded: {current_mb}MB used, limit is {limit_mb}MB")]
    MemoryLimitExceeded {
        current_mb: usize,
        limit_mb: usize,
    },

    /// Unsupported WIT features
    #[error("Unsupported WIT feature '{feature}' in file '{path}'")]
    UnsupportedFeature {
        feature: String,
        path: PathBuf,
    },

    /// Configuration errors
    #[error("Configuration error: {message}")]
    ConfigError {
        message: String,
    },

    /// Batch processing errors
    #[error("Batch processing failed: {failed_count} of {total_count} files failed")]
    BatchProcessingError {
        failed_count: usize,
        total_count: usize,
        individual_errors: Vec<WitError>,
    },

    /// JSON serialization errors
    #[error("JSON serialization failed: {message}")]
    JsonError {
        message: String,
    },

    /// Generic internal errors
    #[error("Internal error: {message}")]
    Internal {
        message: String,
    },
}

impl WitError {
    /// Create a new parse error with location information
    pub fn parse_error(path: PathBuf, message: String, line: Option<usize>, column: Option<usize>) -> Self {
        Self::ParseError { path, message, line, column }
    }

    /// Create a new validation error with details
    pub fn validation_error(path: PathBuf, message: String, details: Vec<String>) -> Self {
        Self::ValidationError { path, message, details }
    }

    /// Create a new type error
    pub fn type_error(interface: String, type_name: String, message: String) -> Self {
        Self::TypeError { interface, type_name, message }
    }

    /// Create a new dependency error
    pub fn dependency_error(
        message: String,
        missing_interfaces: Vec<String>,
        circular_dependencies: Vec<String>,
    ) -> Self {
        Self::DependencyError {
            message,
            missing_interfaces,
            circular_dependencies,
        }
    }

    /// Create a new platform constraint error
    pub fn platform_constraint_error(platform: String, constraint: String, message: String) -> Self {
        Self::PlatformConstraintError { platform, constraint, message }
    }

    /// Create a memory limit exceeded error
    pub fn memory_limit_exceeded(current_mb: usize, limit_mb: usize) -> Self {
        Self::MemoryLimitExceeded { current_mb, limit_mb }
    }

    /// Create an unsupported feature error
    pub fn unsupported_feature(feature: String, path: PathBuf) -> Self {
        Self::UnsupportedFeature { feature, path }
    }

    /// Create a configuration error
    pub fn config_error(message: String) -> Self {
        Self::ConfigError { message }
    }

    /// Create a batch processing error
    pub fn batch_processing_error(
        failed_count: usize,
        total_count: usize,
        individual_errors: Vec<WitError>,
    ) -> Self {
        Self::BatchProcessingError {
            failed_count,
            total_count,
            individual_errors,
        }
    }

    /// Create an internal error
    pub fn internal(message: String) -> Self {
        Self::Internal { message }
    }

    /// Check if this error is a validation error
    pub fn is_validation_error(&self) -> bool {
        matches!(self, Self::ValidationError { .. } | Self::TypeError { .. })
    }

    /// Check if this error is a file system error
    pub fn is_file_error(&self) -> bool {
        matches!(self, Self::FileRead { .. } | Self::DirectoryRead { .. })
    }

    /// Check if this error is a parsing error
    pub fn is_parse_error(&self) -> bool {
        matches!(self, Self::ParseError { .. })
    }

    /// Get the path associated with this error, if any
    pub fn path(&self) -> Option<&PathBuf> {
        match self {
            Self::FileRead { path, .. }
            | Self::DirectoryRead { path, .. }
            | Self::ParseError { path, .. }
            | Self::ValidationError { path, .. }
            | Self::UnsupportedFeature { path, .. } => Some(path),
            _ => None,
        }
    }

    /// Get all individual errors for batch processing errors
    pub fn individual_errors(&self) -> &[WitError] {
        match self {
            Self::BatchProcessingError { individual_errors, .. } => individual_errors,
            _ => &[],
        }
    }
}


/// Convert from I/O errors with context
impl WitError {
    pub fn from_io_error(err: std::io::Error, path: PathBuf) -> Self {
        Self::FileRead { 
            path, 
            message: err.to_string() 
        }
    }

    pub fn from_dir_error(err: std::io::Error, path: PathBuf) -> Self {
        Self::DirectoryRead { 
            path, 
            message: err.to_string() 
        }
    }
}

impl From<serde_json::Error> for WitError {
    fn from(err: serde_json::Error) -> Self {
        Self::JsonError {
            message: err.to_string(),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::path::PathBuf;

    #[test]
    fn test_error_creation() {
        let err = WitError::parse_error(
            PathBuf::from("test.wit"),
            "Invalid syntax".to_string(),
            Some(10),
            Some(5),
        );
        
        assert!(err.is_parse_error());
        assert_eq!(err.path(), Some(&PathBuf::from("test.wit")));
    }

    #[test]
    fn test_validation_error() {
        let err = WitError::validation_error(
            PathBuf::from("test.wit"),
            "Type mismatch".to_string(),
            vec!["Detail 1".to_string(), "Detail 2".to_string()],
        );
        
        assert!(err.is_validation_error());
    }

    #[test]
    fn test_error_categorization() {
        let parse_err = WitError::parse_error(
            PathBuf::from("test.wit"),
            "Error".to_string(),
            None,
            None,
        );
        
        let validation_err = WitError::validation_error(
            PathBuf::from("test.wit"),
            "Error".to_string(),
            vec![],
        );
        
        assert!(parse_err.is_parse_error());
        assert!(!parse_err.is_validation_error());
        assert!(validation_err.is_validation_error());
        assert!(!validation_err.is_parse_error());
    }
}
