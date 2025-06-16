//! Error types for Flight Shared Types Rust bindings

use serde::{Deserialize, Serialize};
use std::fmt;

/// Error severity levels
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum ErrorSeverity {
    Info,
    Warning,
    Error,
    Critical,
    Fatal,
}

impl fmt::Display for ErrorSeverity {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            ErrorSeverity::Info => write!(f, "info"),
            ErrorSeverity::Warning => write!(f, "warning"),
            ErrorSeverity::Error => write!(f, "error"),
            ErrorSeverity::Critical => write!(f, "critical"),
            ErrorSeverity::Fatal => write!(f, "fatal"),
        }
    }
}

/// Error categories for systematic error handling
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum ErrorCategory {
    Memory,
    Platform,
    Network,
    Validation,
    Security,
    Component,
    ServiceIntegration,
    FlightSystem,
    Application,
    Unknown,
}

impl fmt::Display for ErrorCategory {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            ErrorCategory::Memory => write!(f, "memory"),
            ErrorCategory::Platform => write!(f, "platform"),
            ErrorCategory::Network => write!(f, "network"),
            ErrorCategory::Validation => write!(f, "validation"),
            ErrorCategory::Security => write!(f, "security"),
            ErrorCategory::Component => write!(f, "component"),
            ErrorCategory::ServiceIntegration => write!(f, "service-integration"),
            ErrorCategory::FlightSystem => write!(f, "flight-system"),
            ErrorCategory::Application => write!(f, "application"),
            ErrorCategory::Unknown => write!(f, "unknown"),
        }
    }
}

/// Metadata key-value pair
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct MetadataPair {
    pub key: String,
    pub value: String,
}

/// Error context for debugging and service integration
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ErrorContext {
    pub source: String,
    pub operation: String,
    pub session_id: Option<String>,
    pub user_id: Option<String>,
    pub platform: Option<String>,
    pub service_id: Option<String>,
    pub metadata: Vec<MetadataPair>,
}

/// Main error type for Flight systems
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct FlightError {
    pub id: String,
    pub severity: ErrorSeverity,
    pub category: ErrorCategory,
    pub message: String,
    pub details: Option<String>,
    pub context: ErrorContext,
    pub timestamp: u64,
    pub cause: Option<String>,
}

impl fmt::Display for FlightError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        if let Some(details) = &self.details {
            write!(f, "[{}/{}] {}: {}", self.severity, self.category, self.message, details)
        } else {
            write!(f, "[{}/{}] {}", self.severity, self.category, self.message)
        }
    }
}

impl std::error::Error for FlightError {}

impl FlightError {
    /// Create a new error
    pub fn new(
        severity: ErrorSeverity,
        category: ErrorCategory,
        message: String,
        source: String,
        operation: String,
    ) -> Self {
        Self {
            id: uuid::Uuid::new_v4().to_string(),
            severity,
            category,
            message,
            details: None,
            context: ErrorContext {
                source,
                operation,
                session_id: None,
                user_id: None,
                platform: None,
                service_id: None,
                metadata: Vec::new(),
            },
            timestamp: std::time::SystemTime::now()
                .duration_since(std::time::UNIX_EPOCH)
                .unwrap()
                .as_secs(),
            cause: None,
        }
    }

    /// Create a platform error
    pub fn platform_error(message: String, details: Option<String>) -> Self {
        let mut error = Self::new(
            ErrorSeverity::Error,
            ErrorCategory::Platform,
            message,
            "platform".to_string(),
            "platform_operation".to_string(),
        );
        error.details = details;
        error
    }

    /// Create a memory error
    pub fn memory_error(message: String, details: Option<String>) -> Self {
        let mut error = Self::new(
            ErrorSeverity::Error,
            ErrorCategory::Memory,
            message,
            "memory".to_string(),
            "memory_operation".to_string(),
        );
        error.details = details;
        error
    }

    /// Create a component error
    pub fn component_error(message: String, details: Option<String>) -> Self {
        let mut error = Self::new(
            ErrorSeverity::Error,
            ErrorCategory::Component,
            message,
            "component".to_string(),
            "component_operation".to_string(),
        );
        error.details = details;
        error
    }

    /// Create a serialization error
    pub fn serialization_error(message: String) -> Self {
        Self::new(
            ErrorSeverity::Error,
            ErrorCategory::Validation,
            message,
            "serialization".to_string(),
            "serialize".to_string(),
        )
    }

    /// Add details to an error
    pub fn with_details(mut self, details: String) -> Self {
        self.details = Some(details);
        self
    }

    /// Add metadata to an error
    pub fn with_metadata(mut self, key: String, value: String) -> Self {
        self.context.metadata.push(MetadataPair { key, value });
        self
    }
}
