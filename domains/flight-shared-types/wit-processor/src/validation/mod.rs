//! Advanced validation rules and engines for WIT documents
//!
//! This module implements comprehensive validation rules that go beyond basic
//! syntax checking to ensure semantic correctness, cross-reference validity,
//! and Component Model specification compliance.

pub mod cross_reference;
pub mod breaking_change;
pub mod world_composition;
pub mod wasi_compliance;
pub mod type_system;

use crate::error::{Result, WitError};
use crate::types::*;
use std::collections::{HashMap, HashSet};

/// Enhanced validation rule trait for specialized validators
pub trait AdvancedValidationRule {
    /// Rule name for error reporting
    fn name(&self) -> &'static str;
    
    /// Validate a single document
    fn validate_document(&self, document: &WitDocument) -> Vec<ValidationIssue>;
    
    /// Validate across multiple documents (for cross-document rules)
    fn validate_documents(&self, documents: &[WitDocument]) -> Vec<ValidationIssue> {
        // Default implementation validates each document individually
        documents.iter()
            .flat_map(|doc| self.validate_document(doc))
            .collect()
    }
    
    /// Check if this rule applies to the given document/context
    fn applies_to(&self, document: &WitDocument) -> bool {
        // Default: applies to all documents
        true
    }
}

/// Type reference resolution context
/// 
/// Tracks type definitions across packages and interfaces for validation
#[derive(Debug, Clone)]
pub struct TypeResolutionContext {
    /// Global type registry: package:interface/type -> definition
    type_registry: HashMap<String, WitType>,
    /// Interface registry: package:interface -> interface definition
    interface_registry: HashMap<String, WitInterface>,
    /// Package registry: package -> list of interfaces
    package_registry: HashMap<String, Vec<String>>,
    /// Import resolution map: import name -> actual type path
    import_resolution: HashMap<String, String>,
}

impl TypeResolutionContext {
    /// Create new resolution context
    pub fn new() -> Self {
        Self {
            type_registry: HashMap::new(),
            interface_registry: HashMap::new(),
            package_registry: HashMap::new(),
            import_resolution: HashMap::new(),
        }
    }
    
    /// Build context from multiple documents
    pub fn from_documents(documents: &[WitDocument]) -> Self {
        let mut context = Self::new();
        
        for document in documents {
            context.add_document(document);
        }
        
        context
    }
    
    /// Add document to resolution context
    pub fn add_document(&mut self, document: &WitDocument) {
        // Register package
        let package_name = &document.package_name;
        let mut interfaces = Vec::new();
        
        // Register interfaces and types
        for interface in &document.interfaces {
            let interface_path = format!("{}:{}", package_name, interface.name);
            interfaces.push(interface.name.clone());
            self.interface_registry.insert(interface_path.clone(), interface.clone());
            
            // Register types from this interface
            for wit_type in &interface.types {
                let type_path = format!("{}/{}", interface_path, wit_type.name);
                self.type_registry.insert(type_path, wit_type.clone());
            }
        }
        
        self.package_registry.insert(package_name.clone(), interfaces);
        
        // Process imports to build resolution map
        for import in &document.imports {
            // Parse import like "flight:memory/memory-types.{memory-size, memory-usage-snapshot}"
            if let Some(resolution) = self.parse_import_statement(&import.name) {
                for (imported_name, actual_path) in resolution {
                    self.import_resolution.insert(imported_name, actual_path);
                }
            }
        }
    }
    
    /// Parse import statement to extract type mappings
    fn parse_import_statement(&self, import: &str) -> Option<Vec<(String, String)>> {
        // Parse patterns like "flight:memory/memory-types.{memory-size, memory-usage-snapshot}"
        if let Some(dot_pos) = import.find('.') {
            let interface_path = &import[..dot_pos];
            let types_part = &import[dot_pos + 1..];
            
            if types_part.starts_with('{') && types_part.ends_with('}') {
                let types_list = &types_part[1..types_part.len() - 1];
                let type_names: Vec<&str> = types_list.split(',').map(|s| s.trim()).collect();
                
                let mut mappings = Vec::new();
                for type_name in type_names {
                    let full_path = format!("{}/{}", interface_path, type_name);
                    mappings.push((type_name.to_string(), full_path));
                }
                return Some(mappings);
            }
        }
        None
    }
    
    /// Resolve type reference to actual type definition
    pub fn resolve_type(&self, type_ref: &str) -> Option<&WitType> {
        // First check if it's a direct reference
        if let Some(wit_type) = self.type_registry.get(type_ref) {
            return Some(wit_type);
        }
        
        // Check if it's an imported type
        if let Some(actual_path) = self.import_resolution.get(type_ref) {
            return self.type_registry.get(actual_path);
        }
        
        None
    }
    
    /// Check if interface exists
    pub fn interface_exists(&self, interface_path: &str) -> bool {
        self.interface_registry.contains_key(interface_path)
    }
    
    /// Get interface definition
    pub fn get_interface(&self, interface_path: &str) -> Option<&WitInterface> {
        self.interface_registry.get(interface_path)
    }
    
    /// List all available types
    pub fn list_available_types(&self) -> Vec<String> {
        let mut types: Vec<String> = self.type_registry.keys().cloned().collect();
        types.extend(self.import_resolution.keys().cloned());
        types.sort();
        types
    }
}

impl Default for TypeResolutionContext {
    fn default() -> Self {
        Self::new()
    }
}

/// Enhanced validation rule registry
pub struct AdvancedValidationRuleSet {
    rules: Vec<Box<dyn AdvancedValidationRule>>,
}

impl AdvancedValidationRuleSet {
    /// Create new rule set with all validation rules
    pub fn comprehensive() -> Self {
        let mut rules: Vec<Box<dyn AdvancedValidationRule>> = Vec::new();
        
        // Cross-reference validation rules
        rules.push(Box::new(cross_reference::TypeReferenceValidator::new()));
        rules.push(Box::new(cross_reference::InterfaceConsistencyValidator::new()));
        rules.push(Box::new(cross_reference::ImportExportValidator::new()));
        rules.push(Box::new(cross_reference::CircularDependencyValidator::new()));
        
        // Type system validation rules
        rules.push(Box::new(type_system::TypeDefinitionValidator::new()));
        rules.push(Box::new(type_system::ResourceHandleValidator::new()));
        rules.push(Box::new(type_system::FunctionSignatureValidator::new()));
        
        // World composition validation rules
        rules.push(Box::new(world_composition::WorldStructureValidator::new()));
        rules.push(Box::new(world_composition::InterfaceExportValidator::new()));
        rules.push(Box::new(world_composition::ComponentCompositionValidator::new()));
        
        // WASI compliance validation rules
        rules.push(Box::new(wasi_compliance::WasiWorldValidator::new()));
        rules.push(Box::new(wasi_compliance::WasiInterfaceValidator::new()));
        rules.push(Box::new(wasi_compliance::WasiResourceValidator::new()));
        
        // Breaking change detection rules
        rules.push(Box::new(breaking_change::InterfaceChangeValidator::new()));
        rules.push(Box::new(breaking_change::TypeChangeValidator::new()));
        rules.push(Box::new(breaking_change::WorldChangeValidator::new()));
        
        Self { rules }
    }
    
    /// Create rule set with only cross-reference validation
    pub fn cross_reference_only() -> Self {
        let mut rules: Vec<Box<dyn AdvancedValidationRule>> = Vec::new();
        
        rules.push(Box::new(cross_reference::TypeReferenceValidator::new()));
        rules.push(Box::new(cross_reference::InterfaceConsistencyValidator::new()));
        rules.push(Box::new(cross_reference::ImportExportValidator::new()));
        rules.push(Box::new(cross_reference::CircularDependencyValidator::new()));
        
        Self { rules }
    }
    
    /// Validate single document with all applicable rules
    pub fn validate_document(&self, document: &WitDocument) -> Vec<ValidationIssue> {
        let mut issues = Vec::new();
        
        for rule in &self.rules {
            if rule.applies_to(document) {
                issues.extend(rule.validate_document(document));
            }
        }
        
        issues
    }
    
    /// Validate multiple documents with cross-document rules
    pub fn validate_documents(&self, documents: &[WitDocument]) -> Vec<ValidationIssue> {
        let mut issues = Vec::new();
        
        // First validate each document individually
        for document in documents {
            issues.extend(self.validate_document(document));
        }
        
        // Then run cross-document validation rules
        for rule in &self.rules {
            issues.extend(rule.validate_documents(documents));
        }
        
        issues
    }
}

/// Error codes for advanced validation
pub mod error_codes {
    // Cross-reference validation error codes
    pub const TYPE_REFERENCE_UNDEFINED: &str = "CROSS_REF_001";
    pub const CIRCULAR_DEPENDENCY: &str = "CROSS_REF_002";
    pub const IMPORT_TYPE_MISMATCH: &str = "CROSS_REF_003";
    pub const UNRESOLVED_INTERFACE_DEPENDENCY: &str = "CROSS_REF_004";
    pub const INVALID_TYPE_ALIAS: &str = "CROSS_REF_005";
    pub const INTERFACE_NOT_FOUND: &str = "CROSS_REF_006";
    
    // WASI compliance error codes
    pub const INVALID_WASI_CLI_WORLD: &str = "WASI_001";
    pub const INVALID_WASI_HTTP_WORLD: &str = "WASI_002";
    pub const WASI_INTERFACE_CONTRACT_VIOLATION: &str = "WASI_003";
    pub const INVALID_WASI_RESOURCE_USAGE: &str = "WASI_004";
    pub const WASI_FUNCTION_SIGNATURE_ERROR: &str = "WASI_005";
    
    // World composition error codes
    pub const UNRESOLVED_IMPORT_DEPENDENCY: &str = "WORLD_001";
    pub const CONFLICTING_INTERFACE_DEFINITIONS: &str = "WORLD_002";
    pub const INVALID_COMPONENT_COMPOSITION: &str = "WORLD_003";
    pub const RESOURCE_HANDLE_FLOW_VIOLATION: &str = "WORLD_004";
    pub const WORLD_EXPORT_NOT_FOUND: &str = "WORLD_005";
    
    // Type system error codes
    pub const INVALID_RESOURCE_HANDLE_TYPE: &str = "TYPE_001";
    pub const FUNCTION_SIGNATURE_ERROR: &str = "TYPE_002";
    pub const TYPE_DEFINITION_ERROR: &str = "TYPE_003";
    pub const INVALID_TYPE_COMPOSITION: &str = "TYPE_004";
    
    // Breaking change error codes
    pub const INTERFACE_REMOVED: &str = "BREAKING_001";
    pub const FUNCTION_REMOVED: &str = "BREAKING_002";
    pub const FUNCTION_SIGNATURE_CHANGED: &str = "BREAKING_003";
    pub const TYPE_REMOVED: &str = "BREAKING_004";
    pub const TYPE_DEFINITION_CHANGED: &str = "BREAKING_005";
    pub const WORLD_CONTRACT_CHANGED: &str = "BREAKING_006";
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::path::PathBuf;
    
    #[test]
    fn test_type_resolution_context() {
        let mut context = TypeResolutionContext::new();
        
        let document = WitDocument::new(
            PathBuf::from("test.wit"),
            "test:package".to_string(),
            "content".to_string(),
            100,
            10,
        );
        
        context.add_document(&document);
        assert!(context.package_registry.contains_key("test:package"));
    }
    
    #[test]
    fn test_validation_rule_set() {
        let rule_set = AdvancedValidationRuleSet::cross_reference_only();
        assert!(rule_set.rules.len() > 0);
    }
}
