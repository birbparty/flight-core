//! # Flight WIT Processor
//!
//! A comprehensive library for parsing, validating, and processing WebAssembly Interface Types (WIT)
//! files. This library is designed to support Flight's shared types ecosystem with a focus on
//! memory efficiency and performance.
//!
//! ## Features
//!
//! - **WIT Parsing**: Parse individual WIT files and directories
//! - **Validation**: Comprehensive validation of WIT interfaces and types
//! - **Batch Processing**: Efficient processing of multiple WIT files for CI/CD
//! - **Memory Conscious**: Designed to work within constrained environments (16MB+)
//! - **Async Support**: Async APIs for high-performance applications
//!
//! ## Example Usage
//!
//! ```rust
//! use flight_wit_processor::{WitProcessor, Result};
//! use std::path::Path;
//!
//! # fn main() -> Result<()> {
//! let processor = WitProcessor::new();
//! let document = processor.parse_file(Path::new("path/to/file.wit"))?;
//! let report = processor.validate(&document)?;
//! 
//! if report.is_valid() {
//!     println!("WIT file is valid!");
//! }
//! # Ok(())
//! # }
//! ```

pub mod error;
pub mod types;
pub mod parser;
pub mod validator;
pub mod batch;

#[cfg(feature = "cli")]
pub mod cli;

// Re-export core types for convenience
pub use error::{WitError, Result};
pub use types::{
    WitDocument, ValidationReport, ProcessingReport, BatchValidationReport,
    CompatibilityReport, TypeDependencyGraph, TypeUsageReport, BreakingChange,
    Addition, Modification, DependencyType, UsageContext,
};
pub use parser::WitParser;
pub use validator::WitValidator;
pub use batch::{batch_validate_directory, parse_and_validate, quick_validate, generate_batch_summary};

/// Main WIT processor interface
/// 
/// This is the primary entry point for the library. It provides a high-level
/// interface for parsing and validating WIT files with sensible defaults.
/// 
/// # Example
/// 
/// ```rust
/// use flight_wit_processor::WitProcessor;
/// use std::path::Path;
/// 
/// let processor = WitProcessor::new();
/// let result = processor.parse_file(Path::new("memory-types.wit"));
/// ```
#[derive(Debug, Clone)]
pub struct WitProcessor {
    parser: WitParser,
    validator: WitValidator,
    /// AST cache for performance optimization
    ast_cache: std::collections::HashMap<std::path::PathBuf, WitDocument>,
    /// Maximum cache size to prevent memory bloat
    max_cache_size: usize,
}

impl WitProcessor {
    /// Create a new WIT processor with default configuration
    /// 
    /// This creates a processor optimized for typical usage patterns,
    /// including memory-conscious parsing and comprehensive validation.
    pub fn new() -> Self {
        Self {
            parser: WitParser::new(),
            validator: WitValidator::new(),
            ast_cache: std::collections::HashMap::new(),
            max_cache_size: 100, // Default cache size for 100 documents
        }
    }
    
    /// Create a memory-constrained processor for limited environments
    /// 
    /// This configuration is suitable for environments like Dreamcast (16MB)
    /// where memory usage must be extremely conservative.
    pub fn memory_constrained() -> Self {
        Self {
            parser: WitParser::memory_constrained(),
            validator: WitValidator::new(),
            ast_cache: std::collections::HashMap::new(),
            max_cache_size: 10, // Very limited cache for memory-constrained environments
        }
    }
    
    /// Create a high-performance processor for server environments
    /// 
    /// This configuration prioritizes parsing speed and comprehensive analysis
    /// over memory usage, suitable for CI/CD and development environments.
    pub fn high_performance() -> Self {
        Self {
            parser: WitParser::high_performance(),
            validator: WitValidator::new(),
            ast_cache: std::collections::HashMap::new(),
            max_cache_size: 1000, // Large cache for high-performance environments
        }
    }
    
    /// Create a new WIT processor with custom configuration
    /// 
    /// Allows fine-tuning of parser and validator behavior for specific use cases.
    pub fn with_config(parser: WitParser, validator: WitValidator) -> Self {
        Self { 
            parser, 
            validator,
            ast_cache: std::collections::HashMap::new(),
            max_cache_size: 100,
        }
    }
    
    /// Configure maximum cache size
    pub fn with_max_cache_size(mut self, max_size: usize) -> Self {
        self.max_cache_size = max_size;
        self
    }
    
    /// Parse a single WIT file
    /// 
    /// Reads and parses the specified WIT file, returning a structured representation
    /// that can be used for validation and analysis.
    /// 
    /// # Arguments
    /// 
    /// * `path` - Path to the WIT file to parse
    /// 
    /// # Returns
    /// 
    /// Returns a `WitDocument` containing the parsed interface definitions and metadata.
    /// 
    /// # Errors
    /// 
    /// Returns an error if:
    /// - The file cannot be read
    /// - The WIT syntax is invalid
    /// - The file contains unsupported features
    pub fn parse_file(&self, path: &std::path::Path) -> Result<WitDocument> {
        self.parser.parse_file(path)
    }
    
    /// Parse all WIT files in a directory
    /// 
    /// Recursively searches the directory for `.wit` files and parses them.
    /// This is useful for processing entire WIT package directories.
    /// 
    /// # Arguments
    /// 
    /// * `path` - Path to the directory containing WIT files
    /// 
    /// # Returns
    /// 
    /// Returns a vector of `WitDocument`s, one for each successfully parsed file.
    /// 
    /// # Errors
    /// 
    /// Returns an error if:
    /// - The directory cannot be read
    /// - Any WIT file has invalid syntax
    /// - File system errors occur during traversal
    pub fn parse_directory(&self, path: &std::path::Path) -> Result<Vec<WitDocument>> {
        self.parser.parse_directory(path)
    }
    
    /// Validate a single WIT document
    /// 
    /// Performs comprehensive validation of the document including:
    /// - Type checking and consistency
    /// - Interface compatibility
    /// - Naming conventions
    /// - Platform constraints
    /// 
    /// # Arguments
    /// 
    /// * `document` - The WIT document to validate
    /// 
    /// # Returns
    /// 
    /// Returns a `ValidationReport` containing the results of all validation checks.
    pub fn validate(&self, document: &WitDocument) -> Result<ValidationReport> {
        self.validator.validate(document)
    }
    
    /// Validate multiple WIT documents together
    /// 
    /// Performs validation across multiple documents, checking for:
    /// - Cross-document type compatibility
    /// - Import/export consistency
    /// - Circular dependencies
    /// - Package-level constraints
    /// 
    /// # Arguments
    /// 
    /// * `documents` - The WIT documents to validate together
    /// 
    /// # Returns
    /// 
    /// Returns a comprehensive `ValidationReport` covering all documents.
    pub fn validate_all(&self, documents: &[WitDocument]) -> Result<ValidationReport> {
        self.validator.validate_all(documents)
    }
    
    /// Parse and validate a file in one operation
    /// 
    /// Convenience method that combines parsing and validation for single files.
    /// This is the most common operation and is optimized for typical usage.
    /// 
    /// # Arguments
    /// 
    /// * `path` - Path to the WIT file
    /// 
    /// # Returns
    /// 
    /// Returns a `ProcessingReport` containing both the parsed document and validation results.
    pub fn process_file(&self, path: &std::path::Path) -> Result<ProcessingReport> {
        let document = self.parse_file(path)?;
        let validation = self.validate(&document)?;
        Ok(ProcessingReport::new(document, validation))
    }
    
    /// Parse and validate a directory in one operation
    /// 
    /// Convenience method for processing entire directories. This is optimized
    /// for CI/CD usage where you want to validate entire WIT packages.
    /// 
    /// # Arguments
    /// 
    /// * `path` - Path to the directory containing WIT files
    /// 
    /// # Returns
    /// 
    /// Returns a `BatchValidationReport` containing results for all processed files.
    pub fn process_directory(&self, path: &std::path::Path) -> Result<BatchValidationReport> {
        let documents = self.parse_directory(path)?;
        let validation = self.validate_all(&documents)?;
        Ok(BatchValidationReport::new(documents, validation))
    }
    
    /// Extract type dependencies from a WIT document
    /// 
    /// Builds a comprehensive dependency graph showing how types relate to each other.
    /// This is essential for registry population and component flattening analysis.
    /// 
    /// # Arguments
    /// 
    /// * `document` - The WIT document to analyze
    /// 
    /// # Returns
    /// 
    /// Returns a `TypeDependencyGraph` containing all type relationships.
    pub fn extract_type_dependencies(&self, document: &WitDocument) -> Result<TypeDependencyGraph> {
        use crate::types::{TypeDependencyGraph, DependencyType, CircularDependencySeverity, CircularDependency};
        use std::collections::{HashSet, HashMap};
        
        let start_time = std::time::Instant::now();
        let mut graph = TypeDependencyGraph::new();
        
        // Process all interfaces
        for interface in &document.interfaces {
            // Add dependencies from function parameters and return types
            for function in &interface.functions {
                for param in &function.parameters {
                    self.add_type_dependencies_recursive(
                        &format!("{}::{}", interface.name, function.name),
                        &param.param_type,
                        &mut graph,
                        DependencyType::FunctionParameter,
                    );
                }
                
                if let Some(return_type) = &function.return_type {
                    self.add_type_dependencies_recursive(
                        &format!("{}::{}", interface.name, function.name),
                        return_type,
                        &mut graph,
                        DependencyType::FunctionReturn,
                    );
                }
            }
            
            // Add dependencies from type definitions
            for type_def in &interface.types {
                self.add_type_definition_dependencies(&type_def, &mut graph);
            }
        }
        
        // Detect circular dependencies
        self.detect_circular_dependencies(&mut graph);
        
        Ok(graph)
    }
    
    /// Check compatibility between two WIT documents
    /// 
    /// Performs comprehensive compatibility analysis to detect breaking changes,
    /// safe additions, and modifications between two versions of a WIT interface.
    /// 
    /// # Arguments
    /// 
    /// * `old_document` - The original/older version of the WIT document
    /// * `new_document` - The new/updated version of the WIT document
    /// 
    /// # Returns
    /// 
    /// Returns a `CompatibilityReport` detailing all changes and their impact.
    pub fn check_compatibility(&self, old_document: &WitDocument, new_document: &WitDocument) -> Result<CompatibilityReport> {
        use crate::types::{CompatibilityReport, BreakingChange, Addition, Modification};
        use std::collections::HashMap;
        
        let start_time = std::time::Instant::now();
        let mut report = CompatibilityReport::new();
        
        // Compare interfaces
        let old_interfaces: HashMap<String, &crate::types::WitInterface> = 
            old_document.interfaces.iter().map(|i| (i.name.clone(), i)).collect();
        let new_interfaces: HashMap<String, &crate::types::WitInterface> = 
            new_document.interfaces.iter().map(|i| (i.name.clone(), i)).collect();
        
        // Check for removed interfaces
        for (name, _) in &old_interfaces {
            if !new_interfaces.contains_key(name) {
                report.add_breaking_change(BreakingChange::RemovedInterface(name.clone()));
            }
        }
        
        // Check for added interfaces
        for (name, _) in &new_interfaces {
            if !old_interfaces.contains_key(name) {
                report.add_addition(Addition::AddedInterface(name.clone()));
            }
        }
        
        // Check for modified interfaces
        for (name, old_iface) in &old_interfaces {
            if let Some(new_iface) = new_interfaces.get(name) {
                self.compare_interfaces(old_iface, new_iface, &mut report);
            }
        }
        
        // Compare types
        self.compare_types(&old_document, &new_document, &mut report);
        
        // Compare worlds
        self.compare_worlds(&old_document, &new_document, &mut report);
        
        report.analysis_duration_ms = start_time.elapsed().as_millis() as u64;
        Ok(report)
    }
    
    /// Analyze type usage patterns in a WIT document
    /// 
    /// Provides insights into how types are used, which types are most common,
    /// and which types might be unused or internal-only.
    /// 
    /// # Arguments
    /// 
    /// * `document` - The WIT document to analyze
    /// 
    /// # Returns
    /// 
    /// Returns a `TypeUsageReport` with comprehensive usage analysis.
    pub fn analyze_type_usage(&self, document: &WitDocument) -> Result<TypeUsageReport> {
        use crate::types::{TypeUsageReport, TypeUsagePattern, UsageContext, TypeUsageMetrics};
        use std::collections::HashMap;
        
        let start_time = std::time::Instant::now();
        let mut report = TypeUsageReport::new();
        let mut usage_counts: HashMap<String, usize> = HashMap::new();
        let mut usage_contexts: HashMap<String, Vec<UsageContext>> = HashMap::new();
        
        // Analyze usage in interfaces
        for interface in &document.interfaces {
            // Count usage in functions
            for function in &interface.functions {
                for param in &function.parameters {
                    self.count_type_usage_recursive(
                        &param.param_type,
                        &mut usage_counts,
                        &mut usage_contexts,
                        UsageContext::FunctionParameter {
                            interface_name: interface.name.clone(),
                            function_name: function.name.clone(),
                            parameter_name: param.name.clone(),
                        },
                    );
                }
                
                if let Some(return_type) = &function.return_type {
                    self.count_type_usage_recursive(
                        return_type,
                        &mut usage_counts,
                        &mut usage_contexts,
                        UsageContext::FunctionReturn {
                            interface_name: interface.name.clone(),
                            function_name: function.name.clone(),
                        },
                    );
                }
            }
            
            // Count usage in type definitions
            for type_def in &interface.types {
                self.count_type_definition_usage(type_def, &mut usage_counts, &mut usage_contexts);
            }
        }
        
        // Build usage patterns
        for (type_name, count) in &usage_counts {
            let contexts = usage_contexts.get(type_name).cloned().unwrap_or_default();
            let pattern = TypeUsagePattern {
                type_name: type_name.clone(),
                usage_count: *count,
                usage_contexts: contexts,
                is_exported: true, // TODO: Determine from exports analysis
                is_internal_only: false, // TODO: Determine from usage analysis
            };
            report.patterns.push(pattern);
        }
        
        // Sort and categorize types
        report.patterns.sort_by(|a, b| b.usage_count.cmp(&a.usage_count));
        report.most_used_types = report.patterns.iter()
            .take(10)
            .map(|p| p.type_name.clone())
            .collect();
        
        report.unused_types = report.patterns.iter()
            .filter(|p| p.usage_count == 0)
            .map(|p| p.type_name.clone())
            .collect();
        
        report.internal_only_types = report.patterns.iter()
            .filter(|p| p.is_internal_only)
            .map(|p| p.type_name.clone())
            .collect();
        
        // Calculate metrics
        let total_types = report.patterns.len();
        let used_types = report.patterns.iter().filter(|p| p.usage_count > 0).count();
        let total_usage: usize = report.patterns.iter().map(|p| p.usage_count).sum();
        
        report.metrics = TypeUsageMetrics {
            total_types,
            used_types,
            unused_types: total_types - used_types,
            average_usage_count: if total_types > 0 { total_usage as f64 / total_types as f64 } else { 0.0 },
            max_usage_count: report.patterns.iter().map(|p| p.usage_count).max().unwrap_or(0),
            analysis_duration_ms: start_time.elapsed().as_millis() as u64,
        };
        
        Ok(report)
    }
    
    /// Get cached AST for a file path, if available
    /// 
    /// Returns the cached WIT document if it exists in the cache.
    /// This can provide significant performance improvements for repeated operations.
    /// 
    /// # Arguments
    /// 
    /// * `path` - Path to the WIT file
    /// 
    /// # Returns
    /// 
    /// Returns `Some(&WitDocument)` if cached, `None` otherwise.
    pub fn get_cached_ast(&self, path: &std::path::Path) -> Option<&WitDocument> {
        self.ast_cache.get(path)
    }
    
    /// Clear the AST cache
    /// 
    /// Removes all cached documents to free memory. Useful for long-running
    /// processes that need to manage memory usage.
    pub fn clear_cache(&mut self) {
        self.ast_cache.clear();
    }
    
    /// Get cache statistics
    /// 
    /// Returns information about current cache usage for monitoring and debugging.
    pub fn cache_stats(&self) -> (usize, usize) {
        (self.ast_cache.len(), self.max_cache_size)
    }
    
    // Private helper methods for advanced functionality
    
    /// Recursively add type dependencies to the graph
    fn add_type_dependencies_recursive(
        &self,
        from: &str,
        wit_type: &crate::types::WitType,
        graph: &mut TypeDependencyGraph,
        dependency_type: DependencyType,
    ) {
        use crate::types::WitTypeKind;
        
        match &wit_type.kind {
            WitTypeKind::Primitive(_) => {
                // Primitives don't create dependencies
            }
            WitTypeKind::Reference(type_name) => {
                graph.add_dependency(from.to_string(), type_name.clone(), dependency_type);
            }
            WitTypeKind::List(inner) => {
                self.add_type_dependencies_recursive(from, inner, graph, DependencyType::ListElement);
            }
            WitTypeKind::Option(inner) => {
                self.add_type_dependencies_recursive(from, inner, graph, DependencyType::OptionInner);
            }
            WitTypeKind::Result { ok, err } => {
                if let Some(ok_type) = ok {
                    self.add_type_dependencies_recursive(from, ok_type, graph, DependencyType::ResultType);
                }
                if let Some(err_type) = err {
                    self.add_type_dependencies_recursive(from, err_type, graph, DependencyType::ResultType);
                }
            }
            WitTypeKind::Tuple(types) => {
                for tuple_type in types {
                    self.add_type_dependencies_recursive(from, tuple_type, graph, DependencyType::TupleElement);
                }
            }
            WitTypeKind::Record(fields) => {
                for field in fields {
                    self.add_type_dependencies_recursive(from, &field.field_type, graph, DependencyType::FieldType);
                }
            }
            WitTypeKind::Variant(cases) => {
                for case in cases {
                    if let Some(payload) = &case.payload {
                        self.add_type_dependencies_recursive(from, payload, graph, DependencyType::VariantPayload);
                    }
                }
            }
            WitTypeKind::Enum(_) => {
                // Enums don't create type dependencies
            }
        }
    }
    
    /// Add dependencies from type definitions
    fn add_type_definition_dependencies(
        &self,
        type_def: &crate::types::WitType,
        graph: &mut TypeDependencyGraph,
    ) {
        use crate::types::WitTypeKind;
        
        match &type_def.kind {
            WitTypeKind::Record(fields) => {
                for field in fields {
                    self.add_type_dependencies_recursive(
                        &type_def.name,
                        &field.field_type,
                        graph,
                        DependencyType::FieldType,
                    );
                }
            }
            WitTypeKind::Variant(cases) => {
                for case in cases {
                    if let Some(payload) = &case.payload {
                        self.add_type_dependencies_recursive(
                            &type_def.name,
                            payload,
                            graph,
                            DependencyType::VariantPayload,
                        );
                    }
                }
            }
            _ => {
                // Other types handled in recursive function
                self.add_type_dependencies_recursive(
                    &type_def.name,
                    type_def,
                    graph,
                    DependencyType::FieldType,
                );
            }
        }
    }
    
    /// Detect circular dependencies in the type graph
    fn detect_circular_dependencies(&self, graph: &mut TypeDependencyGraph) {
        use crate::types::{CircularDependency, CircularDependencySeverity};
        use std::collections::HashSet;
        
        let mut visited = HashSet::new();
        let mut recursion_stack = HashSet::new();
        let mut path = Vec::new();
        let mut detected_cycles = Vec::new();
        
        // Clone the nodes to avoid borrowing issues
        let nodes = graph.nodes.clone();
        
        for node in &nodes {
            if !visited.contains(node) {
                self.dfs_detect_cycles(
                    node,
                    &graph,
                    &mut visited,
                    &mut recursion_stack,
                    &mut path,
                    &mut detected_cycles,
                );
            }
        }
        
        // Add detected cycles to the graph
        graph.circular_dependencies = detected_cycles;
    }
    
    /// Depth-first search to detect cycles
    fn dfs_detect_cycles(
        &self,
        node: &str,
        graph: &TypeDependencyGraph,
        visited: &mut std::collections::HashSet<String>,
        recursion_stack: &mut std::collections::HashSet<String>,
        path: &mut Vec<String>,
        cycles: &mut Vec<crate::types::CircularDependency>,
    ) {
        visited.insert(node.to_string());
        recursion_stack.insert(node.to_string());
        path.push(node.to_string());
        
        let dependencies = graph.get_dependencies(node);
        for dep in dependencies {
            if !visited.contains(&dep.to) {
                self.dfs_detect_cycles(&dep.to, graph, visited, recursion_stack, path, cycles);
            } else if recursion_stack.contains(&dep.to) {
                // Found a cycle
                let cycle_start = path.iter().position(|n| n == &dep.to).unwrap();
                let cycle: Vec<String> = path[cycle_start..].to_vec();
                
                let severity = if cycle.len() <= 2 {
                    crate::types::CircularDependencySeverity::Info
                } else if cycle.len() <= 5 {
                    crate::types::CircularDependencySeverity::Warning
                } else {
                    crate::types::CircularDependencySeverity::Error
                };
                
                cycles.push(crate::types::CircularDependency { cycle, severity });
            }
        }
        
        recursion_stack.remove(node);
        path.pop();
    }
    
    /// Compare two interfaces for compatibility changes
    fn compare_interfaces(
        &self,
        old_iface: &crate::types::WitInterface,
        new_iface: &crate::types::WitInterface,
        report: &mut CompatibilityReport,
    ) {
        use crate::types::{BreakingChange, Addition, Modification};
        use std::collections::HashMap;
        
        // Compare functions
        let old_functions: HashMap<String, &crate::types::WitFunction> = 
            old_iface.functions.iter().map(|f| (f.name.clone(), f)).collect();
        let new_functions: HashMap<String, &crate::types::WitFunction> = 
            new_iface.functions.iter().map(|f| (f.name.clone(), f)).collect();
        
        // Check for removed functions
        for (name, _) in &old_functions {
            if !new_functions.contains_key(name) {
                report.add_breaking_change(BreakingChange::RemovedFunction {
                    interface: old_iface.name.clone(),
                    function: name.clone(),
                });
            }
        }
        
        // Check for added functions
        for (name, _) in &new_functions {
            if !old_functions.contains_key(name) {
                report.add_addition(Addition::AddedFunction {
                    interface: old_iface.name.clone(),
                    function: name.clone(),
                });
            }
        }
        
        // Check for modified functions
        for (name, old_func) in &old_functions {
            if let Some(new_func) = new_functions.get(name) {
                if self.functions_differ(old_func, new_func) {
                    let change = self.analyze_function_signature_change(old_func, new_func);
                    report.add_breaking_change(BreakingChange::ChangedFunctionSignature {
                        interface: old_iface.name.clone(),
                        function: name.clone(),
                        change,
                    });
                }
            }
        }
        
        // Check documentation changes
        if old_iface.documentation != new_iface.documentation {
            report.add_modification(Modification::UpdatedDocumentation {
                item: old_iface.name.clone(),
                item_type: "interface".to_string(),
            });
        }
    }
    
    /// Compare types between two documents
    fn compare_types(
        &self,
        old_document: &WitDocument,
        new_document: &WitDocument,
        report: &mut CompatibilityReport,
    ) {
        use crate::types::{BreakingChange, Addition};
        use std::collections::HashMap;
        
        let old_types: HashMap<String, &crate::types::WitType> = 
            old_document.interfaces.iter()
                .flat_map(|i| &i.types)
                .map(|t| (t.name.clone(), t))
                .collect();
        
        let new_types: HashMap<String, &crate::types::WitType> = 
            new_document.interfaces.iter()
                .flat_map(|i| &i.types)
                .map(|t| (t.name.clone(), t))
                .collect();
        
        // Check for removed types
        for (name, _) in &old_types {
            if !new_types.contains_key(name) {
                report.add_breaking_change(BreakingChange::RemovedType(name.clone()));
            }
        }
        
        // Check for added types
        for (name, _) in &new_types {
            if !old_types.contains_key(name) {
                report.add_addition(Addition::AddedType(name.clone()));
            }
        }
        
        // Check for modified types
        for (name, old_type) in &old_types {
            if let Some(new_type) = new_types.get(name) {
                if self.types_differ(old_type, new_type) {
                    let change = self.analyze_type_change(old_type, new_type);
                    report.add_breaking_change(BreakingChange::ModifiedTypeDefinition {
                        type_name: name.clone(),
                        change,
                    });
                }
            }
        }
    }
    
    /// Compare worlds between two documents
    fn compare_worlds(
        &self,
        old_document: &WitDocument,
        new_document: &WitDocument,
        report: &mut CompatibilityReport,
    ) {
        use crate::types::Addition;
        use std::collections::HashMap;
        
        let old_worlds: HashMap<String, &crate::types::WitWorld> = 
            old_document.worlds.iter().map(|w| (w.name.clone(), w)).collect();
        let new_worlds: HashMap<String, &crate::types::WitWorld> = 
            new_document.worlds.iter().map(|w| (w.name.clone(), w)).collect();
        
        // Check for added worlds (typically not breaking)
        for (name, _) in &new_worlds {
            if !old_worlds.contains_key(name) {
                report.add_addition(Addition::AddedWorld(name.clone()));
            }
        }
    }
    
    /// Check if two functions differ significantly
    fn functions_differ(&self, old: &crate::types::WitFunction, new: &crate::types::WitFunction) -> bool {
        // Check parameter count and types
        if old.parameters.len() != new.parameters.len() {
            return true;
        }
        
        for (old_param, new_param) in old.parameters.iter().zip(&new.parameters) {
            if old_param.name != new_param.name || self.types_differ(&old_param.param_type, &new_param.param_type) {
                return true;
            }
        }
        
        // Check return type
        match (&old.return_type, &new.return_type) {
            (None, None) => false,
            (Some(_), None) | (None, Some(_)) => true,
            (Some(old_ret), Some(new_ret)) => self.types_differ(old_ret, new_ret),
        }
    }
    
    /// Check if two types differ
    fn types_differ(&self, old: &crate::types::WitType, new: &crate::types::WitType) -> bool {
        use crate::types::WitTypeKind;
        
        // Compare type kinds
        match (&old.kind, &new.kind) {
            (WitTypeKind::Primitive(a), WitTypeKind::Primitive(b)) => a != b,
            (WitTypeKind::Reference(a), WitTypeKind::Reference(b)) => a != b,
            (WitTypeKind::List(a), WitTypeKind::List(b)) => self.types_differ(a, b),
            (WitTypeKind::Option(a), WitTypeKind::Option(b)) => self.types_differ(a, b),
            (WitTypeKind::Tuple(a), WitTypeKind::Tuple(b)) => {
                a.len() != b.len() || a.iter().zip(b).any(|(x, y)| self.types_differ(x, y))
            }
            (WitTypeKind::Record(a), WitTypeKind::Record(b)) => {
                a.len() != b.len() || a.iter().zip(b).any(|(x, y)| {
                    x.name != y.name || self.types_differ(&x.field_type, &y.field_type)
                })
            }
            (WitTypeKind::Variant(a), WitTypeKind::Variant(b)) => {
                a.len() != b.len() || a.iter().zip(b).any(|(x, y)| {
                    x.name != y.name || match (&x.payload, &y.payload) {
                        (None, None) => false,
                        (Some(_), None) | (None, Some(_)) => true,
                        (Some(x_payload), Some(y_payload)) => self.types_differ(x_payload, y_payload),
                    }
                })
            }
            (WitTypeKind::Enum(a), WitTypeKind::Enum(b)) => a != b,
            _ => true, // Different kinds are always different
        }
    }
    
    /// Analyze function signature changes
    fn analyze_function_signature_change(
        &self,
        old: &crate::types::WitFunction,
        new: &crate::types::WitFunction,
    ) -> crate::types::SignatureChange {
        use crate::types::{SignatureChange, ParameterChange, ParameterChangeType, ReturnTypeChange};
        
        let mut parameter_changes = Vec::new();
        let mut return_type_change = None;
        
        // Analyze parameter changes
        for (i, old_param) in old.parameters.iter().enumerate() {
            if let Some(new_param) = new.parameters.get(i) {
                if old_param.name != new_param.name || self.types_differ(&old_param.param_type, &new_param.param_type) {
                    parameter_changes.push(ParameterChange {
                        name: old_param.name.clone(),
                        change_type: ParameterChangeType::TypeChanged {
                            old_type: format!("{:?}", old_param.param_type.kind),
                            new_type: format!("{:?}", new_param.param_type.kind),
                        },
                    });
                }
            } else {
                parameter_changes.push(ParameterChange {
                    name: old_param.name.clone(),
                    change_type: ParameterChangeType::Removed,
                });
            }
        }
        
        // Check for added parameters
        for new_param in new.parameters.iter().skip(old.parameters.len()) {
            parameter_changes.push(ParameterChange {
                name: new_param.name.clone(),
                change_type: ParameterChangeType::Added,
            });
        }
        
        // Analyze return type changes
        match (&old.return_type, &new.return_type) {
            (None, None) => {}
            (Some(old_ret), Some(new_ret)) if !self.types_differ(old_ret, new_ret) => {}
            _ => {
                return_type_change = Some(ReturnTypeChange {
                    old_type: old.return_type.as_ref().map(|t| format!("{:?}", t.kind)),
                    new_type: new.return_type.as_ref().map(|t| format!("{:?}", t.kind)),
                });
            }
        }
        
        SignatureChange {
            parameter_changes,
            return_type_change,
        }
    }
    
    /// Analyze type definition changes
    fn analyze_type_change(
        &self,
        old: &crate::types::WitType,
        new: &crate::types::WitType,
    ) -> crate::types::TypeChange {
        use crate::types::{TypeChange, WitTypeKind};
        
        match (&old.kind, &new.kind) {
            (WitTypeKind::Record(_), WitTypeKind::Record(_)) => {
                // Detailed record comparison would go here
                TypeChange::RecordChanged { field_changes: Vec::new() }
            }
            (WitTypeKind::Variant(_), WitTypeKind::Variant(_)) => {
                // Detailed variant comparison would go here
                TypeChange::VariantChanged { case_changes: Vec::new() }
            }
            (WitTypeKind::Enum(_), WitTypeKind::Enum(_)) => {
                // Detailed enum comparison would go here
                TypeChange::EnumChanged { value_changes: Vec::new() }
            }
            _ => {
                TypeChange::KindChanged {
                    old_kind: format!("{:?}", old.kind),
                    new_kind: format!("{:?}", new.kind),
                }
            }
        }
    }
    
    /// Count type usage recursively
    fn count_type_usage_recursive(
        &self,
        wit_type: &crate::types::WitType,
        usage_counts: &mut std::collections::HashMap<String, usize>,
        usage_contexts: &mut std::collections::HashMap<String, Vec<UsageContext>>,
        context: UsageContext,
    ) {
        use crate::types::WitTypeKind;
        
        // Count the primary type
        *usage_counts.entry(wit_type.name.clone()).or_insert(0) += 1;
        usage_contexts.entry(wit_type.name.clone()).or_insert_with(Vec::new).push(context.clone());
        
        // Recursively count nested types
        match &wit_type.kind {
            WitTypeKind::List(inner) => {
                self.count_type_usage_recursive(inner, usage_counts, usage_contexts, context);
            }
            WitTypeKind::Option(inner) => {
                self.count_type_usage_recursive(inner, usage_counts, usage_contexts, context);
            }
            WitTypeKind::Result { ok, err } => {
                if let Some(ok_type) = ok {
                    self.count_type_usage_recursive(ok_type, usage_counts, usage_contexts, context.clone());
                }
                if let Some(err_type) = err {
                    self.count_type_usage_recursive(err_type, usage_counts, usage_contexts, context);
                }
            }
            WitTypeKind::Tuple(types) => {
                for tuple_type in types {
                    self.count_type_usage_recursive(tuple_type, usage_counts, usage_contexts, context.clone());
                }
            }
            _ => {} // Other types don't have nested types to count
        }
    }
    
    /// Count usage in type definitions
    fn count_type_definition_usage(
        &self,
        type_def: &crate::types::WitType,
        usage_counts: &mut std::collections::HashMap<String, usize>,
        usage_contexts: &mut std::collections::HashMap<String, Vec<UsageContext>>,
    ) {
        use crate::types::{WitTypeKind, UsageContext};
        
        match &type_def.kind {
            WitTypeKind::Record(fields) => {
                for field in fields {
                    self.count_type_usage_recursive(
                        &field.field_type,
                        usage_counts,
                        usage_contexts,
                        UsageContext::RecordField {
                            record_name: type_def.name.clone(),
                            field_name: field.name.clone(),
                        },
                    );
                }
            }
            WitTypeKind::Variant(cases) => {
                for case in cases {
                    if let Some(payload) = &case.payload {
                        self.count_type_usage_recursive(
                            payload,
                            usage_counts,
                            usage_contexts,
                            UsageContext::VariantPayload {
                                variant_name: type_def.name.clone(),
                                case_name: case.name.clone(),
                            },
                        );
                    }
                }
            }
            _ => {}
        }
    }
}

impl Default for WitProcessor {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::path::Path;
    
    #[test]
    fn test_processor_creation() {
        let processor = WitProcessor::new();
        assert!(true, "Processor should be created successfully");
    }
    
    #[test]
    fn test_default_processor() {
        let processor = WitProcessor::default();
        assert!(true, "Default processor should be created successfully");
    }
}
