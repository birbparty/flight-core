//! Cross-reference validation rules for WIT documents
//!
//! This module implements validation rules that check type references, interface
//! dependencies, import/export consistency, and circular dependency detection
//! across WIT documents and packages.

use crate::types::*;
use crate::validation::{AdvancedValidationRule, TypeResolutionContext, error_codes};
use std::collections::{HashMap, HashSet, VecDeque};

/// Validates that all type references resolve to actual type definitions
pub struct TypeReferenceValidator {
    context: Option<TypeResolutionContext>,
}

impl TypeReferenceValidator {
    pub fn new() -> Self {
        Self { context: None }
    }
    
    /// Check if a type reference is valid within the given context
    fn validate_type_reference(
        &self,
        type_ref: &str,
        context: &TypeResolutionContext,
        source_location: &str,
    ) -> Option<ValidationIssue> {
        // Skip primitive types
        if self.is_primitive_type(type_ref) {
            return None;
        }
        
        // Check if type can be resolved
        if context.resolve_type(type_ref).is_none() {
            let available_types = context.list_available_types();
            let suggestion = self.suggest_similar_type(type_ref, &available_types);
            
            let mut issue = ValidationIssue::error(
                error_codes::TYPE_REFERENCE_UNDEFINED.to_string(),
                format!("Undefined type reference '{}' in {}", type_ref, source_location),
            );
            
            if let Some(suggestion) = suggestion {
                issue = issue.with_context(format!("Did you mean '{}'?", suggestion));
            } else {
                issue = issue.with_context(format!("Available types: {}", available_types.join(", ")));
            }
            
            Some(issue)
        } else {
            None
        }
    }
    
    /// Check if a type name refers to a primitive type
    fn is_primitive_type(&self, type_name: &str) -> bool {
        matches!(type_name,
            "bool" | "u8" | "u16" | "u32" | "u64" | "s8" | "s16" | "s32" | "s64" |
            "f32" | "f64" | "char" | "string"
        )
    }
    
    /// Suggest similar type name if reference is close to an existing type
    fn suggest_similar_type(&self, type_ref: &str, available_types: &[String]) -> Option<String> {
        let mut best_match = None;
        let mut best_distance = usize::MAX;
        
        for available_type in available_types {
            let distance = edit_distance(type_ref, available_type);
            if distance < best_distance && distance <= 3 {
                best_distance = distance;
                best_match = Some(available_type.clone());
            }
        }
        
        best_match
    }
    
    /// Recursively validate type references in a WitType
    fn validate_wit_type_references(
        &self,
        wit_type: &WitType,
        context: &TypeResolutionContext,
        source_location: &str,
    ) -> Vec<ValidationIssue> {
        let mut issues = Vec::new();
        
        match &wit_type.kind {
            WitTypeKind::Reference(type_name) => {
                if let Some(issue) = self.validate_type_reference(type_name, context, source_location) {
                    issues.push(issue);
                }
            }
            WitTypeKind::List(inner_type) => {
                issues.extend(self.validate_wit_type_references(inner_type, context, source_location));
            }
            WitTypeKind::Option(inner_type) => {
                issues.extend(self.validate_wit_type_references(inner_type, context, source_location));
            }
            WitTypeKind::Result { ok, err } => {
                if let Some(ok_type) = ok {
                    issues.extend(self.validate_wit_type_references(ok_type, context, source_location));
                }
                if let Some(err_type) = err {
                    issues.extend(self.validate_wit_type_references(err_type, context, source_location));
                }
            }
            WitTypeKind::Tuple(types) => {
                for tuple_type in types {
                    issues.extend(self.validate_wit_type_references(tuple_type, context, source_location));
                }
            }
            WitTypeKind::Record(fields) => {
                for field in fields {
                    let field_location = format!("{}::{}", source_location, field.name);
                    issues.extend(self.validate_wit_type_references(&field.field_type, context, &field_location));
                }
            }
            WitTypeKind::Variant(cases) => {
                for case in cases {
                    if let Some(payload) = &case.payload {
                        let case_location = format!("{}::{}", source_location, case.name);
                        issues.extend(self.validate_wit_type_references(payload, context, &case_location));
                    }
                }
            }
            _ => {} // Primitives and enums don't have type references
        }
        
        issues
    }
}

impl AdvancedValidationRule for TypeReferenceValidator {
    fn name(&self) -> &'static str {
        "TypeReferenceValidator"
    }
    
    fn validate_document(&self, document: &WitDocument) -> Vec<ValidationIssue> {
        // For single document validation, create a local context
        let context = TypeResolutionContext::from_documents(&[document.clone()]);
        let mut issues = Vec::new();
        
        // Validate type references in interfaces
        for interface in &document.interfaces {
            // Validate function parameter and return types
            for function in &interface.functions {
                let function_location = format!("interface {}, function {}", interface.name, function.name);
                
                // Validate parameter types
                for param in &function.parameters {
                    let param_location = format!("{}, parameter {}", function_location, param.name);
                    issues.extend(self.validate_wit_type_references(&param.param_type, &context, &param_location));
                }
                
                // Validate return type
                if let Some(return_type) = &function.return_type {
                    let return_location = format!("{}, return type", function_location);
                    issues.extend(self.validate_wit_type_references(return_type, &context, &return_location));
                }
            }
            
            // Validate type definitions
            for wit_type in &interface.types {
                let type_location = format!("interface {}, type {}", interface.name, wit_type.name);
                issues.extend(self.validate_wit_type_references(wit_type, &context, &type_location));
            }
        }
        
        issues
    }
    
    fn validate_documents(&self, documents: &[WitDocument]) -> Vec<ValidationIssue> {
        // For multi-document validation, create a comprehensive context
        let context = TypeResolutionContext::from_documents(documents);
        let mut issues = Vec::new();
        
        for document in documents {
            // Validate type references in interfaces
            for interface in &document.interfaces {
                // Validate function parameter and return types
                for function in &interface.functions {
                    let function_location = format!("{}::{}, function {}", document.package_name, interface.name, function.name);
                    
                    // Validate parameter types
                    for param in &function.parameters {
                        let param_location = format!("{}, parameter {}", function_location, param.name);
                        issues.extend(self.validate_wit_type_references(&param.param_type, &context, &param_location));
                    }
                    
                    // Validate return type
                    if let Some(return_type) = &function.return_type {
                        let return_location = format!("{}, return type", function_location);
                        issues.extend(self.validate_wit_type_references(return_type, &context, &return_location));
                    }
                }
                
                // Validate type definitions
                for wit_type in &interface.types {
                    let type_location = format!("{}::{}, type {}", document.package_name, interface.name, wit_type.name);
                    issues.extend(self.validate_wit_type_references(wit_type, &context, &type_location));
                }
            }
        }
        
        issues
    }
}

/// Validates consistency between interface imports and exports
pub struct InterfaceConsistencyValidator;

impl InterfaceConsistencyValidator {
    pub fn new() -> Self {
        Self
    }
}

impl AdvancedValidationRule for InterfaceConsistencyValidator {
    fn name(&self) -> &'static str {
        "InterfaceConsistencyValidator"
    }
    
    fn validate_document(&self, document: &WitDocument) -> Vec<ValidationIssue> {
        let mut issues = Vec::new();
        
        // Check that exported interfaces are actually defined
        for export in &document.exports {
            if export.export_type == ExportType::Interface {
                let interface_exists = document.interfaces.iter()
                    .any(|iface| iface.name == export.item);
                
                if !interface_exists {
                    issues.push(ValidationIssue::error(
                        error_codes::INTERFACE_NOT_FOUND.to_string(),
                        format!("Exported interface '{}' is not defined in the document", export.item),
                    ).with_context("Define the interface or remove the export declaration".to_string()));
                }
            }
        }
        
        // Check for interface name conflicts
        let mut interface_names = HashSet::new();
        for interface in &document.interfaces {
            if !interface_names.insert(&interface.name) {
                issues.push(ValidationIssue::error(
                    error_codes::CONFLICTING_INTERFACE_DEFINITIONS.to_string(),
                    format!("Duplicate interface definition '{}'", interface.name),
                ));
            }
        }
        
        issues
    }
    
    fn validate_documents(&self, documents: &[WitDocument]) -> Vec<ValidationIssue> {
        let mut issues = Vec::new();
        
        // Check cross-document interface consistency
        let mut global_interfaces: HashMap<String, &WitDocument> = HashMap::new();
        
        for document in documents {
            for interface in &document.interfaces {
                let interface_path = format!("{}:{}", document.package_name, interface.name);
                
                if let Some(existing_doc) = global_interfaces.get(&interface_path) {
                    if existing_doc.package_name != document.package_name {
                        issues.push(ValidationIssue::error(
                            error_codes::CONFLICTING_INTERFACE_DEFINITIONS.to_string(),
                            format!("Interface '{}' is defined in multiple packages: '{}' and '{}'", 
                                   interface.name, existing_doc.package_name, document.package_name),
                        ));
                    }
                } else {
                    global_interfaces.insert(interface_path, document);
                }
            }
        }
        
        issues
    }
}

/// Validates import and export declarations
pub struct ImportExportValidator;

impl ImportExportValidator {
    pub fn new() -> Self {
        Self
    }
}

impl AdvancedValidationRule for ImportExportValidator {
    fn name(&self) -> &'static str {
        "ImportExportValidator"
    }
    
    fn validate_document(&self, document: &WitDocument) -> Vec<ValidationIssue> {
        let mut issues = Vec::new();
        
        // Validate import syntax
        for import in &document.imports {
            if let Err(error) = self.validate_import_syntax(&import.name) {
                issues.push(ValidationIssue::error(
                    error_codes::IMPORT_TYPE_MISMATCH.to_string(),
                    format!("Invalid import syntax '{}': {}", import.name, error),
                ));
            }
        }
        
        issues
    }
    
    fn validate_documents(&self, documents: &[WitDocument]) -> Vec<ValidationIssue> {
        let mut issues = Vec::new();
        let context = TypeResolutionContext::from_documents(documents);
        
        // Validate that all imports can be resolved
        for document in documents {
            for import in &document.imports {
                match import.import_type {
                    ImportType::Interface => {
                        if !context.interface_exists(&import.item) {
                            issues.push(ValidationIssue::error(
                                error_codes::UNRESOLVED_INTERFACE_DEPENDENCY.to_string(),
                                format!("Unresolved interface import '{}' in package '{}'", 
                                       import.item, document.package_name),
                            ));
                        }
                    }
                    ImportType::Type => {
                        if context.resolve_type(&import.item).is_none() {
                            issues.push(ValidationIssue::error(
                                error_codes::TYPE_REFERENCE_UNDEFINED.to_string(),
                                format!("Unresolved type import '{}' in package '{}'", 
                                       import.item, document.package_name),
                            ));
                        }
                    }
                    _ => {} // Other import types handled elsewhere
                }
            }
        }
        
        issues
    }
}

impl ImportExportValidator {
    fn validate_import_syntax(&self, import_name: &str) -> Result<(), String> {
        // Validate import patterns like "flight:memory/memory-types.{memory-size, memory-usage-snapshot}"
        if import_name.contains('{') {
            if !import_name.ends_with('}') {
                return Err("Import list must end with '}'".to_string());
            }
            
            let parts: Vec<&str> = import_name.splitn(2, '.').collect();
            if parts.len() != 2 {
                return Err("Import list must have format 'package/interface.{types}'".to_string());
            }
            
            let types_part = parts[1];
            if !types_part.starts_with('{') {
                return Err("Type list must start with '{'".to_string());
            }
            
            let types_content = &types_part[1..types_part.len()-1];
            if types_content.trim().is_empty() {
                return Err("Type list cannot be empty".to_string());
            }
        }
        
        Ok(())
    }
}

/// Detects circular dependencies in type definitions
pub struct CircularDependencyValidator;

impl CircularDependencyValidator {
    pub fn new() -> Self {
        Self
    }
    
    /// Build dependency graph for types
    fn build_dependency_graph(&self, documents: &[WitDocument]) -> HashMap<String, HashSet<String>> {
        let mut graph: HashMap<String, HashSet<String>> = HashMap::new();
        
        for document in documents {
            for interface in &document.interfaces {
                for wit_type in &interface.types {
                    let type_path = format!("{}:{}/{}", document.package_name, interface.name, wit_type.name);
                    let dependencies = self.extract_type_dependencies(wit_type);
                    graph.insert(type_path, dependencies);
                }
            }
        }
        
        graph
    }
    
    /// Extract all type dependencies from a type definition
    fn extract_type_dependencies(&self, wit_type: &WitType) -> HashSet<String> {
        let mut dependencies = HashSet::new();
        self.collect_dependencies_recursive(&wit_type.kind, &mut dependencies);
        dependencies
    }
    
    /// Recursively collect type dependencies
    fn collect_dependencies_recursive(&self, kind: &WitTypeKind, dependencies: &mut HashSet<String>) {
        match kind {
            WitTypeKind::Reference(type_name) => {
                dependencies.insert(type_name.clone());
            }
            WitTypeKind::List(inner) => {
                self.collect_dependencies_recursive(&inner.kind, dependencies);
            }
            WitTypeKind::Option(inner) => {
                self.collect_dependencies_recursive(&inner.kind, dependencies);
            }
            WitTypeKind::Result { ok, err } => {
                if let Some(ok_type) = ok {
                    self.collect_dependencies_recursive(&ok_type.kind, dependencies);
                }
                if let Some(err_type) = err {
                    self.collect_dependencies_recursive(&err_type.kind, dependencies);
                }
            }
            WitTypeKind::Tuple(types) => {
                for tuple_type in types {
                    self.collect_dependencies_recursive(&tuple_type.kind, dependencies);
                }
            }
            WitTypeKind::Record(fields) => {
                for field in fields {
                    self.collect_dependencies_recursive(&field.field_type.kind, dependencies);
                }
            }
            WitTypeKind::Variant(cases) => {
                for case in cases {
                    if let Some(payload) = &case.payload {
                        self.collect_dependencies_recursive(&payload.kind, dependencies);
                    }
                }
            }
            _ => {} // Primitives and enums don't create dependencies
        }
    }
    
    /// Detect circular dependencies using depth-first search
    fn detect_cycles(&self, graph: &HashMap<String, HashSet<String>>) -> Vec<Vec<String>> {
        let mut visited = HashSet::new();
        let mut rec_stack = HashSet::new();
        let mut cycles = Vec::new();
        
        for node in graph.keys() {
            if !visited.contains(node) {
                let mut path = Vec::new();
                self.dfs_detect_cycle(node, graph, &mut visited, &mut rec_stack, &mut path, &mut cycles);
            }
        }
        
        cycles
    }
    
    /// Depth-first search for cycle detection
    fn dfs_detect_cycle(
        &self,
        node: &str,
        graph: &HashMap<String, HashSet<String>>,
        visited: &mut HashSet<String>,
        rec_stack: &mut HashSet<String>,
        path: &mut Vec<String>,
        cycles: &mut Vec<Vec<String>>,
    ) {
        visited.insert(node.to_string());
        rec_stack.insert(node.to_string());
        path.push(node.to_string());
        
        if let Some(dependencies) = graph.get(node) {
            for dep in dependencies {
                if !visited.contains(dep) {
                    self.dfs_detect_cycle(dep, graph, visited, rec_stack, path, cycles);
                } else if rec_stack.contains(dep) {
                    // Found a cycle
                    if let Some(cycle_start) = path.iter().position(|n| n == dep) {
                        let cycle = path[cycle_start..].to_vec();
                        cycles.push(cycle);
                    }
                }
            }
        }
        
        rec_stack.remove(node);
        path.pop();
    }
}

impl AdvancedValidationRule for CircularDependencyValidator {
    fn name(&self) -> &'static str {
        "CircularDependencyValidator"
    }
    
    fn validate_document(&self, document: &WitDocument) -> Vec<ValidationIssue> {
        // Single document validation - check for internal cycles
        self.validate_documents(&[document.clone()])
    }
    
    fn validate_documents(&self, documents: &[WitDocument]) -> Vec<ValidationIssue> {
        let mut issues = Vec::new();
        
        let graph = self.build_dependency_graph(documents);
        let cycles = self.detect_cycles(&graph);
        
        for cycle in cycles {
            if cycle.len() > 1 {
                issues.push(ValidationIssue::error(
                    error_codes::CIRCULAR_DEPENDENCY.to_string(),
                    format!("Circular dependency detected: {}", cycle.join(" -> ")),
                ).with_context("Circular dependencies are not allowed in the Component Model".to_string()));
            }
        }
        
        issues
    }
}

/// Simple edit distance calculation for type name suggestions
fn edit_distance(s1: &str, s2: &str) -> usize {
    let len1 = s1.len();
    let len2 = s2.len();
    let mut matrix = vec![vec![0; len2 + 1]; len1 + 1];
    
    for i in 0..=len1 {
        matrix[i][0] = i;
    }
    for j in 0..=len2 {
        matrix[0][j] = j;
    }
    
    for i in 1..=len1 {
        for j in 1..=len2 {
            let cost = if s1.chars().nth(i - 1) == s2.chars().nth(j - 1) { 0 } else { 1 };
            matrix[i][j] = (matrix[i - 1][j] + 1)
                .min(matrix[i][j - 1] + 1)
                .min(matrix[i - 1][j - 1] + cost);
        }
    }
    
    matrix[len1][len2]
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::path::PathBuf;
    
    #[test]
    fn test_type_reference_validator() {
        let validator = TypeReferenceValidator::new();
        assert_eq!(validator.name(), "TypeReferenceValidator");
    }
    
    #[test]
    fn test_primitive_type_detection() {
        let validator = TypeReferenceValidator::new();
        assert!(validator.is_primitive_type("u32"));
        assert!(validator.is_primitive_type("string"));
        assert!(!validator.is_primitive_type("memory-size"));
    }
    
    #[test]
    fn test_edit_distance() {
        assert_eq!(edit_distance("test", "test"), 0);
        assert_eq!(edit_distance("test", "best"), 1);
        assert_eq!(edit_distance("memory-size", "memory-sice"), 1);
    }
    
    #[test]
    fn test_circular_dependency_validator() {
        let validator = CircularDependencyValidator::new();
        assert_eq!(validator.name(), "CircularDependencyValidator");
    }
    
    #[test]
    fn test_import_syntax_validation() {
        let validator = ImportExportValidator::new();
        assert!(validator.validate_import_syntax("flight:memory/memory-types.{memory-size}").is_ok());
        assert!(validator.validate_import_syntax("invalid{syntax").is_err());
    }
}
