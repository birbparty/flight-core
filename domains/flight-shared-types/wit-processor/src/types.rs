//! Core data types for the Flight WIT Processor
//!
//! This module defines the fundamental data structures used throughout the library
//! for representing parsed WIT documents, validation results, and processing reports.

use std::path::PathBuf;
use std::time::SystemTime;
use serde::{Deserialize, Serialize};
use crate::error::WitError;

/// Represents a parsed WIT document with metadata
/// 
/// This structure contains the parsed representation of a WIT file along with
/// useful metadata for processing and validation.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct WitDocument {
    /// Path to the original WIT file
    pub path: PathBuf,
    
    /// Package name from the WIT file
    pub package_name: String,
    
    /// Package version if specified
    pub package_version: Option<String>,
    
    /// List of interfaces defined in this document
    pub interfaces: Vec<WitInterface>,
    
    /// List of worlds defined in this document
    pub worlds: Vec<WitWorld>,
    
    /// Import dependencies referenced by this document
    pub imports: Vec<WitImport>,
    
    /// Export declarations in this document
    pub exports: Vec<WitExport>,
    
    /// Raw WIT content for reference
    pub raw_content: String,
    
    /// File size in bytes
    pub file_size: u64,
    
    /// When this document was parsed
    pub parsed_at: SystemTime,
    
    /// Parse duration in milliseconds
    pub parse_duration_ms: u64,
}

impl WitDocument {
    /// Create a new WIT document
    pub fn new(
        path: PathBuf,
        package_name: String,
        raw_content: String,
        file_size: u64,
        parse_duration_ms: u64,
    ) -> Self {
        Self {
            path,
            package_name,
            package_version: None,
            interfaces: Vec::new(),
            worlds: Vec::new(),
            imports: Vec::new(),
            exports: Vec::new(),
            raw_content,
            file_size,
            parsed_at: SystemTime::now(),
            parse_duration_ms,
        }
    }
    
    /// Get the file name without extension
    pub fn file_stem(&self) -> Option<&str> {
        self.path.file_stem()?.to_str()
    }
    
    /// Get the number of interfaces defined
    pub fn interface_count(&self) -> usize {
        self.interfaces.len()
    }
    
    /// Get the number of worlds defined
    pub fn world_count(&self) -> usize {
        self.worlds.len()
    }
    
    /// Check if this document has any imports
    pub fn has_imports(&self) -> bool {
        !self.imports.is_empty()
    }
    
    /// Check if this document has any exports
    pub fn has_exports(&self) -> bool {
        !self.exports.is_empty()
    }
    
    /// Get interface by name
    pub fn get_interface(&self, name: &str) -> Option<&WitInterface> {
        self.interfaces.iter().find(|iface| iface.name == name)
    }
    
    /// Get world by name
    pub fn get_world(&self, name: &str) -> Option<&WitWorld> {
        self.worlds.iter().find(|world| world.name == name)
    }
}

/// Represents a WIT interface definition
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct WitInterface {
    /// Interface name
    pub name: String,
    
    /// Functions defined in this interface
    pub functions: Vec<WitFunction>,
    
    /// Types defined in this interface
    pub types: Vec<WitType>,
    
    /// Documentation comments
    pub documentation: Option<String>,
}

/// Represents a WIT world definition
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct WitWorld {
    /// World name
    pub name: String,
    
    /// Imports in this world
    pub imports: Vec<String>,
    
    /// Exports in this world
    pub exports: Vec<String>,
    
    /// Documentation comments
    pub documentation: Option<String>,
}

/// Represents a WIT function definition
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct WitFunction {
    /// Function name
    pub name: String,
    
    /// Parameters
    pub parameters: Vec<WitParameter>,
    
    /// Return type
    pub return_type: Option<WitType>,
    
    /// Documentation comments
    pub documentation: Option<String>,
}

/// Represents a function parameter
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct WitParameter {
    /// Parameter name
    pub name: String,
    
    /// Parameter type
    pub param_type: WitType,
}

/// Represents a WIT type definition
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct WitType {
    /// Type name
    pub name: String,
    
    /// Type kind (record, variant, enum, etc.)
    pub kind: WitTypeKind,
    
    /// Documentation comments
    pub documentation: Option<String>,
}

/// Different kinds of WIT types
#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum WitTypeKind {
    /// Primitive type (u32, string, etc.)
    Primitive(String),
    
    /// Record type with fields
    Record(Vec<WitField>),
    
    /// Variant type with cases
    Variant(Vec<WitVariantCase>),
    
    /// Enum type with values
    Enum(Vec<String>),
    
    /// List type
    List(Box<WitType>),
    
    /// Option type
    Option(Box<WitType>),
    
    /// Result type
    Result {
        ok: Option<Box<WitType>>,
        err: Option<Box<WitType>>,
    },
    
    /// Tuple type
    Tuple(Vec<WitType>),
    
    /// Reference to another type
    Reference(String),
}

/// Represents a record field
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct WitField {
    /// Field name
    pub name: String,
    
    /// Field type
    pub field_type: WitType,
    
    /// Documentation comments
    pub documentation: Option<String>,
}

/// Represents a variant case
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct WitVariantCase {
    /// Case name
    pub name: String,
    
    /// Optional payload type
    pub payload: Option<WitType>,
    
    /// Documentation comments
    pub documentation: Option<String>,
}

/// Represents an import declaration
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct WitImport {
    /// Import name/path
    pub name: String,
    
    /// What is being imported
    pub item: String,
    
    /// Import type (interface, world, etc.)
    pub import_type: ImportType,
}

/// Types of imports
#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum ImportType {
    Interface,
    World,
    Type,
    Function,
}

/// Represents an export declaration
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct WitExport {
    /// Export name
    pub name: String,
    
    /// What is being exported
    pub item: String,
    
    /// Export type
    pub export_type: ExportType,
}

/// Types of exports
#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum ExportType {
    Interface,
    World,
    Type,
    Function,
}

/// Comprehensive validation report for WIT documents
/// 
/// Contains the results of validation checks performed on WIT documents,
/// including errors, warnings, and informational messages.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ValidationReport {
    /// Overall validation result
    pub is_valid: bool,
    
    /// Validation errors that must be fixed
    pub errors: Vec<ValidationIssue>,
    
    /// Warnings that should be addressed
    pub warnings: Vec<ValidationIssue>,
    
    /// Informational messages
    pub info: Vec<ValidationIssue>,
    
    /// Performance metrics
    pub metrics: ValidationMetrics,
    
    /// When validation was performed
    pub validated_at: SystemTime,
}

impl ValidationReport {
    /// Create a new validation report
    pub fn new() -> Self {
        Self {
            is_valid: true,
            errors: Vec::new(),
            warnings: Vec::new(),
            info: Vec::new(),
            metrics: ValidationMetrics::default(),
            validated_at: SystemTime::now(),
        }
    }
    
    /// Add an error to the report
    pub fn add_error(&mut self, issue: ValidationIssue) {
        self.is_valid = false;
        self.errors.push(issue);
    }
    
    /// Add a warning to the report
    pub fn add_warning(&mut self, issue: ValidationIssue) {
        self.warnings.push(issue);
    }
    
    /// Add an info message to the report
    pub fn add_info(&mut self, issue: ValidationIssue) {
        self.info.push(issue);
    }
    
    /// Get total number of issues
    pub fn total_issues(&self) -> usize {
        self.errors.len() + self.warnings.len() + self.info.len()
    }
    
    /// Check if validation passed (no errors)
    pub fn is_valid(&self) -> bool {
        self.is_valid && self.errors.is_empty()
    }
    
    /// Get summary text
    pub fn summary(&self) -> String {
        format!(
            "{} errors, {} warnings, {} info",
            self.errors.len(),
            self.warnings.len(),
            self.info.len()
        )
    }
}

impl Default for ValidationReport {
    fn default() -> Self {
        Self::new()
    }
}

/// Individual validation issue
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ValidationIssue {
    /// Issue severity
    pub severity: IssueSeverity,
    
    /// Issue code for programmatic handling
    pub code: String,
    
    /// Human-readable message
    pub message: String,
    
    /// File path where issue was found
    pub path: Option<PathBuf>,
    
    /// Line number if applicable
    pub line: Option<usize>,
    
    /// Column number if applicable
    pub column: Option<usize>,
    
    /// Additional context or suggestions
    pub context: Option<String>,
}

impl ValidationIssue {
    /// Create a new error issue
    pub fn error(code: String, message: String) -> Self {
        Self {
            severity: IssueSeverity::Error,
            code,
            message,
            path: None,
            line: None,
            column: None,
            context: None,
        }
    }
    
    /// Create a new warning issue
    pub fn warning(code: String, message: String) -> Self {
        Self {
            severity: IssueSeverity::Warning,
            code,
            message,
            path: None,
            line: None,
            column: None,
            context: None,
        }
    }
    
    /// Create a new info issue
    pub fn info(code: String, message: String) -> Self {
        Self {
            severity: IssueSeverity::Info,
            code,
            message,
            path: None,
            line: None,
            column: None,
            context: None,
        }
    }
    
    /// Add location information
    pub fn with_location(mut self, path: PathBuf, line: Option<usize>, column: Option<usize>) -> Self {
        self.path = Some(path);
        self.line = line;
        self.column = column;
        self
    }
    
    /// Add context information
    pub fn with_context(mut self, context: String) -> Self {
        self.context = Some(context);
        self
    }
}

/// Issue severity levels
#[derive(Debug, Clone, Copy, Serialize, Deserialize)]
pub enum IssueSeverity {
    Error,
    Warning,
    Info,
}

/// Validation performance metrics
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ValidationMetrics {
    /// Validation duration in milliseconds
    pub duration_ms: u64,
    
    /// Peak memory usage in bytes
    pub peak_memory_bytes: u64,
    
    /// Number of documents validated
    pub documents_validated: usize,
    
    /// Number of interfaces validated
    pub interfaces_validated: usize,
    
    /// Number of types validated
    pub types_validated: usize,
}

impl Default for ValidationMetrics {
    fn default() -> Self {
        Self {
            duration_ms: 0,
            peak_memory_bytes: 0,
            documents_validated: 0,
            interfaces_validated: 0,
            types_validated: 0,
        }
    }
}

/// Combined processing report for single file operations
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ProcessingReport {
    /// The parsed document
    pub document: WitDocument,
    
    /// Validation results
    pub validation: ValidationReport,
    
    /// Processing successful
    pub success: bool,
}

impl ProcessingReport {
    /// Create a new processing report
    pub fn new(document: WitDocument, validation: ValidationReport) -> Self {
        let success = validation.is_valid();
        Self {
            document,
            validation,
            success,
        }
    }
    
    /// Check if processing was successful
    pub fn is_success(&self) -> bool {
        self.success
    }
}

/// Batch validation report for multiple files
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct BatchValidationReport {
    /// All processed documents
    pub documents: Vec<WitDocument>,
    
    /// Combined validation results
    pub validation: ValidationReport,
    
    /// Individual processing errors
    pub errors: Vec<WitError>,
    
    /// Overall success status
    pub success: bool,
    
    /// Processing statistics
    pub stats: BatchStats,
}

impl BatchValidationReport {
    /// Create a new batch validation report
    pub fn new(documents: Vec<WitDocument>, validation: ValidationReport) -> Self {
        let success = validation.is_valid();
        let stats = BatchStats {
            total_files: documents.len(),
            successful_files: if success { documents.len() } else { 0 },
            failed_files: if success { 0 } else { documents.len() },
            total_interfaces: documents.iter().map(|d| d.interface_count()).sum(),
            total_worlds: documents.iter().map(|d| d.world_count()).sum(),
            total_size_bytes: documents.iter().map(|d| d.file_size).sum(),
        };
        
        Self {
            documents,
            validation,
            errors: Vec::new(),
            success,
            stats,
        }
    }
    
    /// Add a processing error
    pub fn add_error(&mut self, error: WitError) {
        self.success = false;
        self.stats.failed_files += 1;
        if self.stats.successful_files > 0 {
            self.stats.successful_files -= 1;
        }
        self.errors.push(error);
    }
}

/// Batch processing statistics
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct BatchStats {
    /// Total number of files processed
    pub total_files: usize,
    
    /// Number of successfully processed files
    pub successful_files: usize,
    
    /// Number of failed files
    pub failed_files: usize,
    
    /// Total interfaces across all files
    pub total_interfaces: usize,
    
    /// Total worlds across all files
    pub total_worlds: usize,
    
    /// Total size in bytes
    pub total_size_bytes: u64,
}

/// Version compatibility analysis report
/// 
/// Provides comprehensive analysis of compatibility between two WIT versions,
/// including breaking changes, safe additions, and modifications.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct CompatibilityReport {
    /// Overall compatibility status
    pub compatible: bool,
    
    /// Breaking changes that require major version increment
    pub breaking_changes: Vec<BreakingChange>,
    
    /// Safe additions that can be minor version increments
    pub additions: Vec<Addition>,
    
    /// Non-breaking modifications
    pub modifications: Vec<Modification>,
    
    /// Analysis duration in milliseconds
    pub analysis_duration_ms: u64,
    
    /// When compatibility analysis was performed
    pub analyzed_at: SystemTime,
}

impl CompatibilityReport {
    /// Create a new compatibility report
    pub fn new() -> Self {
        Self {
            compatible: true,
            breaking_changes: Vec::new(),
            additions: Vec::new(),
            modifications: Vec::new(),
            analysis_duration_ms: 0,
            analyzed_at: SystemTime::now(),
        }
    }
    
    /// Add a breaking change
    pub fn add_breaking_change(&mut self, change: BreakingChange) {
        self.compatible = false;
        self.breaking_changes.push(change);
    }
    
    /// Add a safe addition
    pub fn add_addition(&mut self, addition: Addition) {
        self.additions.push(addition);
    }
    
    /// Add a modification
    pub fn add_modification(&mut self, modification: Modification) {
        self.modifications.push(modification);
    }
    
    /// Check if there are any breaking changes
    pub fn has_breaking_changes(&self) -> bool {
        !self.breaking_changes.is_empty()
    }
    
    /// Get summary of changes
    pub fn summary(&self) -> String {
        format!(
            "{} breaking changes, {} additions, {} modifications",
            self.breaking_changes.len(),
            self.additions.len(),
            self.modifications.len()
        )
    }
}

impl Default for CompatibilityReport {
    fn default() -> Self {
        Self::new()
    }
}

/// Types of breaking changes that require major version increments
#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum BreakingChange {
    /// Removed interface
    RemovedInterface(String),
    
    /// Removed function from interface
    RemovedFunction {
        interface: String,
        function: String,
    },
    
    /// Changed function signature
    ChangedFunctionSignature {
        interface: String,
        function: String,
        change: SignatureChange,
    },
    
    /// Removed type definition
    RemovedType(String),
    
    /// Modified type definition in breaking way
    ModifiedTypeDefinition {
        type_name: String,
        change: TypeChange,
    },
    
    /// Removed enum variant
    RemovedEnumVariant {
        enum_name: String,
        variant: String,
    },
    
    /// Removed record field
    RemovedRecordField {
        record_name: String,
        field: String,
    },
    
    /// Changed record field type
    ChangedRecordFieldType {
        record_name: String,
        field: String,
        old_type: String,
        new_type: String,
    },
}

/// Safe additions that don't break compatibility
#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum Addition {
    /// Added new interface
    AddedInterface(String),
    
    /// Added new function to interface
    AddedFunction {
        interface: String,
        function: String,
    },
    
    /// Added new type definition
    AddedType(String),
    
    /// Added enum variant
    AddedEnumVariant {
        enum_name: String,
        variant: String,
    },
    
    /// Added optional record field
    AddedOptionalRecordField {
        record_name: String,
        field: String,
    },
    
    /// Added world
    AddedWorld(String),
}

/// Non-breaking modifications
#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum Modification {
    /// Updated documentation
    UpdatedDocumentation {
        item: String,
        item_type: String,
    },
    
    /// Reordered enum variants (if order doesn't matter)
    ReorderedEnumVariants {
        enum_name: String,
    },
    
    /// Reordered record fields
    ReorderedRecordFields {
        record_name: String,
    },
}

/// Details about function signature changes
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SignatureChange {
    /// Changed parameter types
    pub parameter_changes: Vec<ParameterChange>,
    
    /// Changed return type
    pub return_type_change: Option<ReturnTypeChange>,
}

/// Parameter change details
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ParameterChange {
    /// Parameter name
    pub name: String,
    
    /// Type of change
    pub change_type: ParameterChangeType,
}

/// Types of parameter changes
#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum ParameterChangeType {
    /// Parameter was removed
    Removed,
    
    /// Parameter type was changed
    TypeChanged {
        old_type: String,
        new_type: String,
    },
    
    /// Parameter was added
    Added,
}

/// Return type change details
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ReturnTypeChange {
    /// Old return type
    pub old_type: Option<String>,
    
    /// New return type
    pub new_type: Option<String>,
}

/// Type definition change details
#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum TypeChange {
    /// Kind of type changed (e.g., record to variant)
    KindChanged {
        old_kind: String,
        new_kind: String,
    },
    
    /// Record structure changed
    RecordChanged {
        field_changes: Vec<FieldChange>,
    },
    
    /// Variant structure changed
    VariantChanged {
        case_changes: Vec<CaseChange>,
    },
    
    /// Enum values changed
    EnumChanged {
        value_changes: Vec<EnumValueChange>,
    },
}

/// Record field change details
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct FieldChange {
    /// Field name
    pub name: String,
    
    /// Type of change
    pub change_type: FieldChangeType,
}

/// Types of field changes
#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum FieldChangeType {
    /// Field was removed
    Removed,
    
    /// Field type was changed
    TypeChanged {
        old_type: String,
        new_type: String,
    },
    
    /// Field was added
    Added,
}

/// Variant case change details
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct CaseChange {
    /// Case name
    pub name: String,
    
    /// Type of change
    pub change_type: CaseChangeType,
}

/// Types of case changes
#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum CaseChangeType {
    /// Case was removed
    Removed,
    
    /// Case payload was changed
    PayloadChanged {
        old_payload: Option<String>,
        new_payload: Option<String>,
    },
    
    /// Case was added
    Added,
}

/// Enum value change details
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct EnumValueChange {
    /// Value name
    pub name: String,
    
    /// Type of change
    pub change_type: EnumValueChangeType,
}

/// Types of enum value changes
#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum EnumValueChangeType {
    /// Value was removed
    Removed,
    
    /// Value was added
    Added,
}

/// Type dependency graph for analysis
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct TypeDependencyGraph {
    /// Nodes in the dependency graph (type names)
    pub nodes: Vec<String>,
    
    /// Edges representing dependencies (from -> to)
    pub edges: Vec<DependencyEdge>,
    
    /// Circular dependencies detected
    pub circular_dependencies: Vec<CircularDependency>,
    
    /// Type usage patterns
    pub usage_patterns: Vec<TypeUsagePattern>,
}

impl TypeDependencyGraph {
    /// Create a new empty dependency graph
    pub fn new() -> Self {
        Self {
            nodes: Vec::new(),
            edges: Vec::new(),
            circular_dependencies: Vec::new(),
            usage_patterns: Vec::new(),
        }
    }
    
    /// Add a dependency edge
    pub fn add_dependency(&mut self, from: String, to: String, dependency_type: DependencyType) {
        if !self.nodes.contains(&from) {
            self.nodes.push(from.clone());
        }
        if !self.nodes.contains(&to) {
            self.nodes.push(to.clone());
        }
        
        self.edges.push(DependencyEdge {
            from,
            to,
            dependency_type,
        });
    }
    
    /// Check if the graph has circular dependencies
    pub fn has_circular_dependencies(&self) -> bool {
        !self.circular_dependencies.is_empty()
    }
    
    /// Get all dependencies of a type
    pub fn get_dependencies(&self, type_name: &str) -> Vec<&DependencyEdge> {
        self.edges.iter()
            .filter(|edge| edge.from == type_name)
            .collect()
    }
    
    /// Get all types that depend on this type
    pub fn get_dependents(&self, type_name: &str) -> Vec<&DependencyEdge> {
        self.edges.iter()
            .filter(|edge| edge.to == type_name)
            .collect()
    }
}

impl Default for TypeDependencyGraph {
    fn default() -> Self {
        Self::new()
    }
}

/// Dependency edge in the type graph
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct DependencyEdge {
    /// Source type
    pub from: String,
    
    /// Target type
    pub to: String,
    
    /// Type of dependency
    pub dependency_type: DependencyType,
}

/// Types of dependencies between types
#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum DependencyType {
    /// Field type dependency (record field uses another type)
    FieldType,
    
    /// Variant payload dependency
    VariantPayload,
    
    /// List element type
    ListElement,
    
    /// Option inner type
    OptionInner,
    
    /// Result ok/err type
    ResultType,
    
    /// Tuple element type
    TupleElement,
    
    /// Function parameter type
    FunctionParameter,
    
    /// Function return type
    FunctionReturn,
}

/// Circular dependency information
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct CircularDependency {
    /// Types involved in the cycle
    pub cycle: Vec<String>,
    
    /// Severity of the circular dependency
    pub severity: CircularDependencySeverity,
}

/// Severity levels for circular dependencies
#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum CircularDependencySeverity {
    /// Error - must be resolved
    Error,
    
    /// Warning - should be reviewed
    Warning,
    
    /// Info - acceptable pattern
    Info,
}

/// Type usage pattern analysis
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct TypeUsagePattern {
    /// Type name
    pub type_name: String,
    
    /// How many times this type is used
    pub usage_count: usize,
    
    /// Contexts where this type is used
    pub usage_contexts: Vec<UsageContext>,
    
    /// Whether this type is exported
    pub is_exported: bool,
    
    /// Whether this type is only used internally
    pub is_internal_only: bool,
}

/// Context where a type is used
#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum UsageContext {
    /// Used as a record field
    RecordField {
        record_name: String,
        field_name: String,
    },
    
    /// Used as variant payload
    VariantPayload {
        variant_name: String,
        case_name: String,
    },
    
    /// Used as function parameter
    FunctionParameter {
        interface_name: String,
        function_name: String,
        parameter_name: String,
    },
    
    /// Used as function return type
    FunctionReturn {
        interface_name: String,
        function_name: String,
    },
    
    /// Used in type alias or composition
    TypeComposition {
        parent_type: String,
    },
}

/// Type usage analysis report
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct TypeUsageReport {
    /// All type usage patterns
    pub patterns: Vec<TypeUsagePattern>,
    
    /// Most frequently used types
    pub most_used_types: Vec<String>,
    
    /// Unused types
    pub unused_types: Vec<String>,
    
    /// Types only used internally
    pub internal_only_types: Vec<String>,
    
    /// Analysis metrics
    pub metrics: TypeUsageMetrics,
}

impl TypeUsageReport {
    /// Create a new type usage report
    pub fn new() -> Self {
        Self {
            patterns: Vec::new(),
            most_used_types: Vec::new(),
            unused_types: Vec::new(),
            internal_only_types: Vec::new(),
            metrics: TypeUsageMetrics::default(),
        }
    }
    
    /// Get usage pattern for a specific type
    pub fn get_usage_pattern(&self, type_name: &str) -> Option<&TypeUsagePattern> {
        self.patterns.iter().find(|p| p.type_name == type_name)
    }
}

impl Default for TypeUsageReport {
    fn default() -> Self {
        Self::new()
    }
}

/// Metrics for type usage analysis
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct TypeUsageMetrics {
    /// Total number of types analyzed
    pub total_types: usize,
    
    /// Number of used types
    pub used_types: usize,
    
    /// Number of unused types
    pub unused_types: usize,
    
    /// Average usage count per type
    pub average_usage_count: f64,
    
    /// Maximum usage count
    pub max_usage_count: usize,
    
    /// Analysis duration in milliseconds
    pub analysis_duration_ms: u64,
}

impl Default for TypeUsageMetrics {
    fn default() -> Self {
        Self {
            total_types: 0,
            used_types: 0,
            unused_types: 0,
            average_usage_count: 0.0,
            max_usage_count: 0,
            analysis_duration_ms: 0,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::path::PathBuf;
    
    #[test]
    fn test_wit_document_creation() {
        let doc = WitDocument::new(
            PathBuf::from("test.wit"),
            "test:package".to_string(),
            "package test:package@1.0.0;".to_string(),
            100,
            10,
        );
        
        assert_eq!(doc.package_name, "test:package");
        assert_eq!(doc.interface_count(), 0);
        assert_eq!(doc.world_count(), 0);
    }
    
    #[test]
    fn test_validation_report() {
        let mut report = ValidationReport::new();
        assert!(report.is_valid());
        
        report.add_error(ValidationIssue::error(
            "TEST001".to_string(),
            "Test error".to_string(),
        ));
        
        assert!(!report.is_valid());
        assert_eq!(report.errors.len(), 1);
    }
    
    #[test]
    fn test_processing_report() {
        let doc = WitDocument::new(
            PathBuf::from("test.wit"),
            "test:package".to_string(),
            "content".to_string(),
            100,
            10,
        );
        
        let validation = ValidationReport::new();
        let report = ProcessingReport::new(doc, validation);
        
        assert!(report.is_success());
    }
    
    #[test]
    fn test_compatibility_report() {
        let mut report = CompatibilityReport::new();
        assert!(report.compatible);
        assert!(!report.has_breaking_changes());
        
        report.add_breaking_change(BreakingChange::RemovedInterface("test".to_string()));
        
        assert!(!report.compatible);
        assert!(report.has_breaking_changes());
        assert_eq!(report.breaking_changes.len(), 1);
    }
    
    #[test]
    fn test_type_dependency_graph() {
        let mut graph = TypeDependencyGraph::new();
        
        graph.add_dependency(
            "RecordA".to_string(),
            "RecordB".to_string(),
            DependencyType::FieldType,
        );
        
        assert_eq!(graph.nodes.len(), 2);
        assert_eq!(graph.edges.len(), 1);
        
        let deps = graph.get_dependencies("RecordA");
        assert_eq!(deps.len(), 1);
        assert_eq!(deps[0].to, "RecordB");
    }
    
    #[test]
    fn test_type_usage_report() {
        let report = TypeUsageReport::new();
        assert_eq!(report.patterns.len(), 0);
        assert_eq!(report.unused_types.len(), 0);
    }
}
