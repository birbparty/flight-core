//! WIT validation engine for the Flight WIT Processor
//!
//! This module provides comprehensive validation of WIT documents, checking for
//! syntactic correctness, semantic consistency, and Flight-specific requirements.

use std::collections::{HashMap, HashSet};
use std::time::Instant;

use crate::error::{Result, WitError};
use crate::types::*;

/// WIT validator configuration and state
/// 
/// The validator performs various checks on WIT documents to ensure they meet
/// quality standards and compatibility requirements.
#[derive(Debug, Clone)]
pub struct WitValidator {
    /// Whether to perform strict naming convention checks
    strict_naming: bool,
    
    /// Whether to check for platform-specific constraints
    platform_checks: bool,
    
    /// Whether to validate cross-document dependencies
    dependency_validation: bool,
    
    /// Whether to check for performance-related issues
    performance_checks: bool,
    
    /// Maximum allowed nesting depth for types
    max_type_depth: usize,
    
    /// Maximum allowed number of function parameters
    max_function_params: usize,
    
    /// Platform memory constraints (for validation)
    platform_memory_limits: HashMap<String, u64>,
}

impl WitValidator {
    /// Create a new validator with default configuration
    /// 
    /// Default configuration provides a good balance of validation coverage
    /// without being overly strict for development use.
    pub fn new() -> Self {
        let mut platform_limits = HashMap::new();
        platform_limits.insert("dreamcast".to_string(), 16 * 1024 * 1024); // 16MB
        platform_limits.insert("psp".to_string(), 32 * 1024 * 1024); // 32MB
        platform_limits.insert("vita".to_string(), 512 * 1024 * 1024); // 512MB
        
        Self {
            strict_naming: false,
            platform_checks: true,
            dependency_validation: true,
            performance_checks: true,
            max_type_depth: 10,
            max_function_params: 20,
            platform_memory_limits: platform_limits,
        }
    }
    
    /// Create a strict validator for production use
    /// 
    /// This configuration enforces all possible checks and is suitable
    /// for production environments where quality is paramount.
    pub fn strict() -> Self {
        let mut validator = Self::new();
        validator.strict_naming = true;
        validator.max_type_depth = 5;
        validator.max_function_params = 10;
        validator
    }
    
    /// Create a lenient validator for development
    /// 
    /// This configuration is more permissive and suitable for development
    /// environments where rapid iteration is important.
    pub fn lenient() -> Self {
        let mut validator = Self::new();
        validator.strict_naming = false;
        validator.platform_checks = false;
        validator.performance_checks = false;
        validator.max_type_depth = 20;
        validator.max_function_params = 50;
        validator
    }
    
    /// Configure strict naming checks
    pub fn with_strict_naming(mut self, strict: bool) -> Self {
        self.strict_naming = strict;
        self
    }
    
    /// Configure platform-specific checks
    pub fn with_platform_checks(mut self, enabled: bool) -> Self {
        self.platform_checks = enabled;
        self
    }
    
    /// Configure dependency validation
    pub fn with_dependency_validation(mut self, enabled: bool) -> Self {
        self.dependency_validation = enabled;
        self
    }
    
    /// Configure performance checks
    pub fn with_performance_checks(mut self, enabled: bool) -> Self {
        self.performance_checks = enabled;
        self
    }
    
    /// Set maximum type nesting depth
    pub fn with_max_type_depth(mut self, depth: usize) -> Self {
        self.max_type_depth = depth;
        self
    }
    
    /// Set maximum function parameters
    pub fn with_max_function_params(mut self, params: usize) -> Self {
        self.max_function_params = params;
        self
    }
    
    /// Validate a single WIT document
    /// 
    /// Performs comprehensive validation of the document including syntax,
    /// semantics, naming conventions, and platform constraints.
    /// 
    /// # Arguments
    /// 
    /// * `document` - The WIT document to validate
    /// 
    /// # Returns
    /// 
    /// Returns a ValidationReport containing all issues found during validation.
    pub fn validate(&self, document: &WitDocument) -> Result<ValidationReport> {
        let start_time = Instant::now();
        let mut report = ValidationReport::new();
        
        // Basic document validation
        self.validate_document_structure(document, &mut report)?;
        
        // Package validation
        self.validate_package(document, &mut report)?;
        
        // Interface validation
        for interface in &document.interfaces {
            self.validate_interface(document, interface, &mut report)?;
        }
        
        // World validation
        for world in &document.worlds {
            self.validate_world(document, world, &mut report)?;
        }
        
        // Cross-reference validation
        self.validate_cross_references(document, &mut report)?;
        
        // Platform-specific validation
        if self.platform_checks {
            self.validate_platform_constraints(document, &mut report)?;
        }
        
        // Performance validation
        if self.performance_checks {
            self.validate_performance_characteristics(document, &mut report)?;
        }
        
        // Update metrics
        report.metrics.duration_ms = start_time.elapsed().as_millis() as u64;
        report.metrics.documents_validated = 1;
        report.metrics.interfaces_validated = document.interfaces.len();
        report.metrics.types_validated = document.interfaces.iter()
            .map(|i| i.types.len())
            .sum();
        
        Ok(report)
    }
    
    /// Validate multiple WIT documents together
    /// 
    /// Performs validation across multiple documents, checking for consistency
    /// and proper dependency resolution.
    /// 
    /// # Arguments
    /// 
    /// * `documents` - The WIT documents to validate together
    /// 
    /// # Returns
    /// 
    /// Returns a comprehensive ValidationReport covering all documents.
    pub fn validate_all(&self, documents: &[WitDocument]) -> Result<ValidationReport> {
        let start_time = Instant::now();
        let mut report = ValidationReport::new();
        
        // Validate each document individually
        for document in documents {
            let individual_report = self.validate(document)?;
            
            // Merge results
            for error in individual_report.errors {
                report.add_error(error);
            }
            for warning in individual_report.warnings {
                report.add_warning(warning);
            }
            for info in individual_report.info {
                report.add_info(info);
            }
        }
        
        // Cross-document validation
        if self.dependency_validation && documents.len() > 1 {
            self.validate_cross_document_dependencies(documents, &mut report)?;
        }
        
        // Update metrics
        report.metrics.duration_ms = start_time.elapsed().as_millis() as u64;
        report.metrics.documents_validated = documents.len();
        report.metrics.interfaces_validated = documents.iter()
            .map(|d| d.interfaces.len())
            .sum();
        report.metrics.types_validated = documents.iter()
            .flat_map(|d| &d.interfaces)
            .map(|i| i.types.len())
            .sum();
        
        Ok(report)
    }
    
    /// Validate basic document structure
    fn validate_document_structure(&self, document: &WitDocument, report: &mut ValidationReport) -> Result<()> {
        // Check if document has content
        if document.interfaces.is_empty() && document.worlds.is_empty() {
            report.add_warning(
                ValidationIssue::warning(
                    "EMPTY_DOCUMENT".to_string(),
                    "Document contains no interfaces or worlds".to_string(),
                ).with_location(document.path.clone(), None, None)
            );
        }
        
        // Check file size reasonableness
        if document.file_size > 1024 * 1024 { // 1MB
            report.add_warning(
                ValidationIssue::warning(
                    "LARGE_FILE".to_string(),
                    format!("File size is large ({}KB)", document.file_size / 1024),
                ).with_location(document.path.clone(), None, None)
            );
        }
        
        // Check parse performance
        if document.parse_duration_ms > 1000 { // 1 second
            report.add_info(
                ValidationIssue::info(
                    "SLOW_PARSE".to_string(),
                    format!("File took {}ms to parse", document.parse_duration_ms),
                ).with_location(document.path.clone(), None, None)
            );
        }
        
        Ok(())
    }
    
    /// Validate package information
    fn validate_package(&self, document: &WitDocument, report: &mut ValidationReport) -> Result<()> {
        // Check package name format
        if self.strict_naming {
            if !self.is_valid_package_name(&document.package_name) {
                report.add_error(
                    ValidationIssue::error(
                        "INVALID_PACKAGE_NAME".to_string(),
                        format!("Package name '{}' does not follow naming conventions", document.package_name),
                    ).with_location(document.path.clone(), None, None)
                    .with_context("Package names should be in format 'namespace:name' with kebab-case".to_string())
                );
            }
        }
        
        // Check for version information
        if document.package_version.is_none() {
            report.add_warning(
                ValidationIssue::warning(
                    "MISSING_VERSION".to_string(),
                    "Package does not specify a version".to_string(),
                ).with_location(document.path.clone(), None, None)
                .with_context("Consider adding version information for better dependency management".to_string())
            );
        }
        
        Ok(())
    }
    
    /// Validate an interface
    fn validate_interface(&self, document: &WitDocument, interface: &WitInterface, report: &mut ValidationReport) -> Result<()> {
        // Check interface name
        if self.strict_naming && !self.is_valid_identifier(&interface.name) {
            report.add_error(
                ValidationIssue::error(
                    "INVALID_INTERFACE_NAME".to_string(),
                    format!("Interface name '{}' is not a valid identifier", interface.name),
                ).with_location(document.path.clone(), None, None)
            );
        }
        
        // Check for empty interfaces
        if interface.functions.is_empty() && interface.types.is_empty() {
            report.add_warning(
                ValidationIssue::warning(
                    "EMPTY_INTERFACE".to_string(),
                    format!("Interface '{}' contains no functions or types", interface.name),
                ).with_location(document.path.clone(), None, None)
            );
        }
        
        // Validate functions
        for function in &interface.functions {
            self.validate_function(document, interface, function, report)?;
        }
        
        // Validate types
        for wit_type in &interface.types {
            self.validate_type(document, interface, wit_type, report, 0)?;
        }
        
        // Check for duplicate names
        self.check_interface_duplicates(document, interface, report)?;
        
        Ok(())
    }
    
    /// Validate a function
    fn validate_function(&self, document: &WitDocument, interface: &WitInterface, function: &WitFunction, report: &mut ValidationReport) -> Result<()> {
        // Check function name
        if self.strict_naming && !self.is_valid_identifier(&function.name) {
            report.add_error(
                ValidationIssue::error(
                    "INVALID_FUNCTION_NAME".to_string(),
                    format!("Function name '{}' is not a valid identifier", function.name),
                ).with_location(document.path.clone(), None, None)
            );
        }
        
        // Check parameter count
        if function.parameters.len() > self.max_function_params {
            report.add_warning(
                ValidationIssue::warning(
                    "TOO_MANY_PARAMETERS".to_string(),
                    format!("Function '{}' has {} parameters (max recommended: {})", 
                           function.name, function.parameters.len(), self.max_function_params),
                ).with_location(document.path.clone(), None, None)
                .with_context("Consider grouping parameters into a record type".to_string())
            );
        }
        
        // Check for duplicate parameter names
        let mut param_names = HashSet::new();
        for param in &function.parameters {
            if !param_names.insert(&param.name) {
                report.add_error(
                    ValidationIssue::error(
                        "DUPLICATE_PARAMETER".to_string(),
                        format!("Function '{}' has duplicate parameter '{}'", function.name, param.name),
                    ).with_location(document.path.clone(), None, None)
                );
            }
            
            // Validate parameter names
            if self.strict_naming && !self.is_valid_identifier(&param.name) {
                report.add_error(
                    ValidationIssue::error(
                        "INVALID_PARAMETER_NAME".to_string(),
                        format!("Parameter name '{}' is not a valid identifier", param.name),
                    ).with_location(document.path.clone(), None, None)
                );
            }
        }
        
        Ok(())
    }
    
    /// Validate a type definition
    fn validate_type(&self, document: &WitDocument, interface: &WitInterface, wit_type: &WitType, report: &mut ValidationReport, depth: usize) -> Result<()> {
        // Check nesting depth
        if depth > self.max_type_depth {
            report.add_error(
                ValidationIssue::error(
                    "TYPE_TOO_DEEP".to_string(),
                    format!("Type '{}' exceeds maximum nesting depth of {}", wit_type.name, self.max_type_depth),
                ).with_location(document.path.clone(), None, None)
            );
            return Ok(());
        }
        
        // Check type name
        if self.strict_naming && !self.is_valid_identifier(&wit_type.name) {
            report.add_error(
                ValidationIssue::error(
                    "INVALID_TYPE_NAME".to_string(),
                    format!("Type name '{}' is not a valid identifier", wit_type.name),
                ).with_location(document.path.clone(), None, None)
            );
        }
        
        // Validate type kind
        match &wit_type.kind {
            WitTypeKind::Record(fields) => {
                self.validate_record_type(document, interface, wit_type, fields, report, depth)?;
            },
            WitTypeKind::Variant(cases) => {
                self.validate_variant_type(document, interface, wit_type, cases, report, depth)?;
            },
            WitTypeKind::Enum(values) => {
                self.validate_enum_type(document, interface, wit_type, values, report)?;
            },
            WitTypeKind::List(element_type) => {
                self.validate_type(document, interface, element_type, report, depth + 1)?;
            },
            WitTypeKind::Option(inner_type) => {
                self.validate_type(document, interface, inner_type, report, depth + 1)?;
            },
            WitTypeKind::Result { ok, err } => {
                if let Some(ok_type) = ok {
                    self.validate_type(document, interface, ok_type, report, depth + 1)?;
                }
                if let Some(err_type) = err {
                    self.validate_type(document, interface, err_type, report, depth + 1)?;
                }
            },
            WitTypeKind::Tuple(types) => {
                for tuple_type in types {
                    self.validate_type(document, interface, tuple_type, report, depth + 1)?;
                }
            },
            _ => {} // Other types are fine as-is
        }
        
        Ok(())
    }
    
    /// Validate a record type
    fn validate_record_type(&self, document: &WitDocument, interface: &WitInterface, wit_type: &WitType, fields: &[WitField], report: &mut ValidationReport, depth: usize) -> Result<()> {
        if fields.is_empty() {
            report.add_warning(
                ValidationIssue::warning(
                    "EMPTY_RECORD".to_string(),
                    format!("Record type '{}' has no fields", wit_type.name),
                ).with_location(document.path.clone(), None, None)
            );
        }
        
        // Check for duplicate field names
        let mut field_names = HashSet::new();
        for field in fields {
            if !field_names.insert(&field.name) {
                report.add_error(
                    ValidationIssue::error(
                        "DUPLICATE_FIELD".to_string(),
                        format!("Record '{}' has duplicate field '{}'", wit_type.name, field.name),
                    ).with_location(document.path.clone(), None, None)
                );
            }
            
            // Validate field names
            if self.strict_naming && !self.is_valid_identifier(&field.name) {
                report.add_error(
                    ValidationIssue::error(
                        "INVALID_FIELD_NAME".to_string(),
                        format!("Field name '{}' is not a valid identifier", field.name),
                    ).with_location(document.path.clone(), None, None)
                );
            }
            
            // Recursively validate field types
            self.validate_type(document, interface, &field.field_type, report, depth + 1)?;
        }
        
        Ok(())
    }
    
    /// Validate a variant type
    fn validate_variant_type(&self, document: &WitDocument, interface: &WitInterface, wit_type: &WitType, cases: &[WitVariantCase], report: &mut ValidationReport, depth: usize) -> Result<()> {
        if cases.is_empty() {
            report.add_error(
                ValidationIssue::error(
                    "EMPTY_VARIANT".to_string(),
                    format!("Variant type '{}' has no cases", wit_type.name),
                ).with_location(document.path.clone(), None, None)
            );
        }
        
        // Check for duplicate case names
        let mut case_names = HashSet::new();
        for case in cases {
            if !case_names.insert(&case.name) {
                report.add_error(
                    ValidationIssue::error(
                        "DUPLICATE_CASE".to_string(),
                        format!("Variant '{}' has duplicate case '{}'", wit_type.name, case.name),
                    ).with_location(document.path.clone(), None, None)
                );
            }
            
            // Validate case names
            if self.strict_naming && !self.is_valid_identifier(&case.name) {
                report.add_error(
                    ValidationIssue::error(
                        "INVALID_CASE_NAME".to_string(),
                        format!("Case name '{}' is not a valid identifier", case.name),
                    ).with_location(document.path.clone(), None, None)
                );
            }
            
            // Recursively validate payload types
            if let Some(payload) = &case.payload {
                self.validate_type(document, interface, payload, report, depth + 1)?;
            }
        }
        
        Ok(())
    }
    
    /// Validate an enum type
    fn validate_enum_type(&self, document: &WitDocument, _interface: &WitInterface, wit_type: &WitType, values: &[String], report: &mut ValidationReport) -> Result<()> {
        if values.is_empty() {
            report.add_error(
                ValidationIssue::error(
                    "EMPTY_ENUM".to_string(),
                    format!("Enum type '{}' has no values", wit_type.name),
                ).with_location(document.path.clone(), None, None)
            );
        }
        
        // Check for duplicate values
        let mut value_set = HashSet::new();
        for value in values {
            if !value_set.insert(value) {
                report.add_error(
                    ValidationIssue::error(
                        "DUPLICATE_ENUM_VALUE".to_string(),
                        format!("Enum '{}' has duplicate value '{}'", wit_type.name, value),
                    ).with_location(document.path.clone(), None, None)
                );
            }
            
            // Validate enum value names
            if self.strict_naming && !self.is_valid_identifier(value) {
                report.add_error(
                    ValidationIssue::error(
                        "INVALID_ENUM_VALUE".to_string(),
                        format!("Enum value '{}' is not a valid identifier", value),
                    ).with_location(document.path.clone(), None, None)
                );
            }
        }
        
        Ok(())
    }
    
    /// Validate a world definition
    fn validate_world(&self, document: &WitDocument, world: &WitWorld, report: &mut ValidationReport) -> Result<()> {
        // Check world name
        if self.strict_naming && !self.is_valid_identifier(&world.name) {
            report.add_error(
                ValidationIssue::error(
                    "INVALID_WORLD_NAME".to_string(),
                    format!("World name '{}' is not a valid identifier", world.name),
                ).with_location(document.path.clone(), None, None)
            );
        }
        
        // Check for empty worlds
        if world.imports.is_empty() && world.exports.is_empty() {
            report.add_warning(
                ValidationIssue::warning(
                    "EMPTY_WORLD".to_string(),
                    format!("World '{}' has no imports or exports", world.name),
                ).with_location(document.path.clone(), None, None)
            );
        }
        
        Ok(())
    }
    
    /// Check for duplicate names within an interface
    fn check_interface_duplicates(&self, document: &WitDocument, interface: &WitInterface, report: &mut ValidationReport) -> Result<()> {
        let mut names = HashSet::new();
        
        // Check function names
        for function in &interface.functions {
            if !names.insert(format!("function:{}", function.name)) {
                report.add_error(
                    ValidationIssue::error(
                        "DUPLICATE_NAME".to_string(),
                        format!("Interface '{}' has duplicate name '{}'", interface.name, function.name),
                    ).with_location(document.path.clone(), None, None)
                );
            }
        }
        
        // Check type names
        for wit_type in &interface.types {
            if !names.insert(format!("type:{}", wit_type.name)) {
                report.add_error(
                    ValidationIssue::error(
                        "DUPLICATE_NAME".to_string(),
                        format!("Interface '{}' has duplicate name '{}'", interface.name, wit_type.name),
                    ).with_location(document.path.clone(), None, None)
                );
            }
        }
        
        Ok(())
    }
    
    /// Validate cross-references within a document
    fn validate_cross_references(&self, _document: &WitDocument, _report: &mut ValidationReport) -> Result<()> {
        // TODO: Implement cross-reference validation
        // This would check that all type references resolve correctly
        Ok(())
    }
    
    /// Validate platform-specific constraints
    fn validate_platform_constraints(&self, document: &WitDocument, report: &mut ValidationReport) -> Result<()> {
        // Check for types that might be problematic on constrained platforms
        for interface in &document.interfaces {
            for wit_type in &interface.types {
                if let WitTypeKind::List(_) = &wit_type.kind {
                    report.add_info(
                        ValidationIssue::info(
                            "UNBOUNDED_LIST".to_string(),
                            format!("Type '{}' uses unbounded list, consider size limits for constrained platforms", wit_type.name),
                        ).with_location(document.path.clone(), None, None)
                        .with_context("Dreamcast and PSP have limited memory".to_string())
                    );
                }
            }
        }
        
        Ok(())
    }
    
    /// Validate performance characteristics
    fn validate_performance_characteristics(&self, document: &WitDocument, report: &mut ValidationReport) -> Result<()> {
        // Check for deeply nested types that might impact performance
        for interface in &document.interfaces {
            for wit_type in &interface.types {
                let depth = self.calculate_type_depth(&wit_type.kind);
                if depth > 5 {
                    report.add_warning(
                        ValidationIssue::warning(
                            "DEEP_TYPE_NESTING".to_string(),
                            format!("Type '{}' has nesting depth of {}, which may impact performance", wit_type.name, depth),
                        ).with_location(document.path.clone(), None, None)
                    );
                }
            }
        }
        
        Ok(())
    }
    
    /// Validate dependencies across multiple documents
    fn validate_cross_document_dependencies(&self, _documents: &[WitDocument], _report: &mut ValidationReport) -> Result<()> {
        // TODO: Implement cross-document dependency validation
        // This would check that imports resolve to exports in other documents
        Ok(())
    }
    
    /// Check if a package name is valid
    fn is_valid_package_name(&self, name: &str) -> bool {
        // Package names should be in format "namespace:name"
        if let Some(colon_pos) = name.find(':') {
            let namespace = &name[..colon_pos];
            let package = &name[colon_pos + 1..];
            self.is_valid_identifier(namespace) && self.is_valid_identifier(package)
        } else {
            false
        }
    }
    
    /// Check if an identifier is valid
    fn is_valid_identifier(&self, name: &str) -> bool {
        if name.is_empty() {
            return false;
        }
        
        // Must start with letter or underscore
        let mut chars = name.chars();
        if let Some(first) = chars.next() {
            if !first.is_ascii_alphabetic() && first != '_' {
                return false;
            }
        }
        
        // Rest must be alphanumeric, underscore, or hyphen
        for ch in chars {
            if !ch.is_ascii_alphanumeric() && ch != '_' && ch != '-' {
                return false;
            }
        }
        
        true
    }
    
    /// Calculate the nesting depth of a type
    fn calculate_type_depth(&self, kind: &WitTypeKind) -> usize {
        match kind {
            WitTypeKind::Primitive(_) | WitTypeKind::Reference(_) => 1,
            WitTypeKind::List(inner) | WitTypeKind::Option(inner) => {
                1 + self.calculate_type_depth(&inner.kind)
            },
            WitTypeKind::Result { ok, err } => {
                let ok_depth = ok.as_ref().map(|t| self.calculate_type_depth(&t.kind)).unwrap_or(0);
                let err_depth = err.as_ref().map(|t| self.calculate_type_depth(&t.kind)).unwrap_or(0);
                1 + ok_depth.max(err_depth)
            },
            WitTypeKind::Tuple(types) => {
                1 + types.iter().map(|t| self.calculate_type_depth(&t.kind)).max().unwrap_or(0)
            },
            WitTypeKind::Record(fields) => {
                1 + fields.iter().map(|f| self.calculate_type_depth(&f.field_type.kind)).max().unwrap_or(0)
            },
            WitTypeKind::Variant(cases) => {
                1 + cases.iter()
                    .filter_map(|c| c.payload.as_ref())
                    .map(|t| self.calculate_type_depth(&t.kind))
                    .max()
                    .unwrap_or(0)
            },
            WitTypeKind::Enum(_) => 1,
        }
    }
}

impl Default for WitValidator {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::path::PathBuf;
    
    #[test]
    fn test_validator_creation() {
        let validator = WitValidator::new();
        assert!(validator.platform_checks);
        assert!(validator.dependency_validation);
    }
    
    #[test]
    fn test_strict_validator() {
        let validator = WitValidator::strict();
        assert!(validator.strict_naming);
        assert_eq!(validator.max_type_depth, 5);
    }
    
    #[test]
    fn test_lenient_validator() {
        let validator = WitValidator::lenient();
        assert!(!validator.strict_naming);
        assert!(!validator.platform_checks);
    }
    
    #[test]
    fn test_valid_identifier() {
        let validator = WitValidator::new();
        
        assert!(validator.is_valid_identifier("valid_name"));
        assert!(validator.is_valid_identifier("valid-name"));
        assert!(validator.is_valid_identifier("_underscore"));
        assert!(validator.is_valid_identifier("name123"));
        
        assert!(!validator.is_valid_identifier("123invalid"));
        assert!(!validator.is_valid_identifier("invalid.name"));
        assert!(!validator.is_valid_identifier(""));
        assert!(!validator.is_valid_identifier("invalid space"));
    }
    
    #[test]
    fn test_valid_package_name() {
        let validator = WitValidator::new();
        
        assert!(validator.is_valid_package_name("namespace:package"));
        assert!(validator.is_valid_package_name("my-org:my-package"));
        
        assert!(!validator.is_valid_package_name("invalid"));
        assert!(!validator.is_valid_package_name("namespace:"));
        assert!(!validator.is_valid_package_name(":package"));
    }
}
