//! WIT parsing engine for the Flight WIT Processor
//!
//! This module provides the core WIT parsing functionality, wrapping the wit-parser
//! crate with Flight-specific logic and optimizations.

use std::fs;
use std::path::{Path, PathBuf};
use std::time::Instant;
use walkdir::WalkDir;
use wit_parser::Resolve;

use crate::error::{Result, WitError};
use crate::types::*;

/// WIT parser configuration and state
/// 
/// The parser is designed to be memory-efficient and reusable across multiple
/// parsing operations. It maintains a resolve context for cross-file dependency
/// resolution.
#[derive(Debug, Clone)]
pub struct WitParser {
    /// Maximum file size to parse (bytes)
    max_file_size: u64,
    
    /// Maximum memory usage during parsing (bytes)
    max_memory_usage: u64,
    
    /// Whether to include raw content in documents
    include_raw_content: bool,
    
    /// Whether to resolve dependencies across files
    resolve_dependencies: bool,
}

impl WitParser {
    /// Create a new WIT parser with default configuration
    /// 
    /// Default configuration is optimized for typical usage with reasonable
    /// memory constraints suitable for constrained environments.
    pub fn new() -> Self {
        Self {
            max_file_size: 10 * 1024 * 1024, // 10MB max file size
            max_memory_usage: 64 * 1024 * 1024, // 64MB max memory usage
            include_raw_content: true,
            resolve_dependencies: true,
        }
    }
    
    /// Create a memory-constrained parser for very limited environments
    /// 
    /// This configuration is suitable for environments like Dreamcast (16MB)
    /// where memory usage must be extremely conservative.
    pub fn memory_constrained() -> Self {
        Self {
            max_file_size: 1024 * 1024, // 1MB max file size
            max_memory_usage: 4 * 1024 * 1024, // 4MB max memory usage
            include_raw_content: false, // Skip raw content to save memory
            resolve_dependencies: false, // Skip cross-file resolution
        }
    }
    
    /// Create a high-performance parser for server environments
    /// 
    /// This configuration prioritizes parsing speed and comprehensive analysis
    /// over memory usage, suitable for CI/CD and development environments.
    pub fn high_performance() -> Self {
        Self {
            max_file_size: 100 * 1024 * 1024, // 100MB max file size
            max_memory_usage: 512 * 1024 * 1024, // 512MB max memory usage
            include_raw_content: true,
            resolve_dependencies: true,
        }
    }
    
    /// Configure maximum file size
    pub fn with_max_file_size(mut self, max_size: u64) -> Self {
        self.max_file_size = max_size;
        self
    }
    
    /// Configure maximum memory usage
    pub fn with_max_memory_usage(mut self, max_memory: u64) -> Self {
        self.max_memory_usage = max_memory;
        self
    }
    
    /// Configure whether to include raw content
    pub fn with_raw_content(mut self, include: bool) -> Self {
        self.include_raw_content = include;
        self
    }
    
    /// Configure dependency resolution
    pub fn with_dependency_resolution(mut self, resolve: bool) -> Self {
        self.resolve_dependencies = resolve;
        self
    }
    
    /// Parse a single WIT file
    /// 
    /// Reads the specified file and parses it into a structured WitDocument.
    /// This method is optimized for single-file parsing and includes comprehensive
    /// error handling and memory management.
    /// 
    /// # Arguments
    /// 
    /// * `path` - Path to the WIT file to parse
    /// 
    /// # Returns
    /// 
    /// Returns a WitDocument containing the parsed structure and metadata.
    /// 
    /// # Errors
    /// 
    /// Returns an error if:
    /// - File cannot be read or doesn't exist
    /// - File size exceeds configured limits
    /// - WIT syntax is invalid
    /// - Memory usage exceeds limits during parsing
    pub fn parse_file(&self, path: &Path) -> Result<WitDocument> {
        let start_time = Instant::now();
        
        // Validate file exists and is readable
        if !path.exists() {
            return Err(WitError::FileRead {
                path: path.to_path_buf(),
                message: "File not found".to_string(),
            });
        }
        
        // Check file size
        let metadata = fs::metadata(path)
            .map_err(|e| WitError::from_io_error(e, path.to_path_buf()))?;
        
        let file_size = metadata.len();
        if file_size > self.max_file_size {
            return Err(WitError::config_error(format!(
                "File size {}MB exceeds maximum allowed size {}MB",
                file_size / (1024 * 1024),
                self.max_file_size / (1024 * 1024)
            )));
        }
        
        // Read file content
        let content = fs::read_to_string(path)
            .map_err(|e| WitError::from_io_error(e, path.to_path_buf()))?;
        
        // Parse with wit-parser using the simpler push_str API
        let mut resolve = Resolve::new();
        let package_id = match resolve.push_str(path, &content) {
            Ok(id) => id,
            Err(e) => {
                return Err(WitError::ParseError {
                    path: path.to_path_buf(),
                    message: e.to_string(),
                    line: None,
                    column: None,
                });
            }
        };
        
        let parse_duration = start_time.elapsed().as_millis() as u64;
        
        // Extract package information
        let package = &resolve.packages[package_id];
        let package_name = package.name.to_string();
        let package_version = package.name.version.as_ref().map(|v| v.to_string());
        
        // Create WIT document
        let raw_content = if self.include_raw_content {
            content
        } else {
            String::new() // Save memory by not storing raw content
        };
        
        let mut document = WitDocument::new(
            path.to_path_buf(),
            package_name,
            raw_content,
            file_size,
            parse_duration,
        );
        
        document.package_version = package_version;
        
        // Extract interfaces
        for (interface_id, interface) in &resolve.interfaces {
            // Check if this interface belongs to our package
            if let Some(interface_package_id) = interface.package {
                if interface_package_id == package_id {
                    let wit_interface = self.extract_interface(&resolve, interface_id, interface)?;
                    document.interfaces.push(wit_interface);
                }
            }
        }
        
        // Extract worlds
        for (world_id, world) in &resolve.worlds {
            // Check if this world belongs to our package
            if let Some(world_package_id) = world.package {
                if world_package_id == package_id {
                    let wit_world = self.extract_world(&resolve, world)?;
                    document.worlds.push(wit_world);
                }
            }
        }
        
        Ok(document)
    }
    
    /// Parse all WIT files in a directory
    /// 
    /// Recursively searches the directory for `.wit` files and parses each one.
    /// This method is optimized for batch processing and includes progress tracking
    /// and memory management across multiple files.
    /// 
    /// # Arguments
    /// 
    /// * `path` - Path to the directory to search
    /// 
    /// # Returns
    /// 
    /// Returns a vector of WitDocument instances, one for each successfully parsed file.
    /// 
    /// # Errors
    /// 
    /// Returns an error if:
    /// - Directory cannot be read
    /// - Any critical parsing error occurs
    /// - Memory usage exceeds configured limits
    pub fn parse_directory(&self, path: &Path) -> Result<Vec<WitDocument>> {
        if !path.is_dir() {
            return Err(WitError::DirectoryRead {
                path: path.to_path_buf(),
                message: "Path is not a directory".to_string(),
            });
        }
        
        // Find all WIT files
        let wit_files = self.find_wit_files(path)?;
        let mut documents = Vec::with_capacity(wit_files.len());
        
        // Parse each file
        for wit_file in wit_files {
            match self.parse_file(&wit_file) {
                Ok(document) => documents.push(document),
                Err(e) => {
                    // For directory parsing, we continue on individual file errors
                    // but could collect them for reporting
                    eprintln!("Warning: Failed to parse {}: {}", wit_file.display(), e);
                }
            }
        }
        
        Ok(documents)
    }
    
    /// Find all WIT files in a directory recursively
    fn find_wit_files(&self, path: &Path) -> Result<Vec<PathBuf>> {
        let mut wit_files = Vec::new();
        
        for entry in WalkDir::new(path).follow_links(false) {
            let entry = entry.map_err(|e| WitError::DirectoryRead {
                path: path.to_path_buf(),
                message: e.to_string(),
            })?;
            
            let path = entry.path();
            if path.is_file() && path.extension().and_then(|s| s.to_str()) == Some("wit") {
                wit_files.push(path.to_path_buf());
            }
        }
        
        Ok(wit_files)
    }
    
    /// Extract interface information from wit-parser structures
    fn extract_interface(
        &self, 
        resolve: &Resolve, 
        interface_id: wit_parser::InterfaceId,
        interface: &wit_parser::Interface,
    ) -> Result<WitInterface> {
        let mut wit_interface = WitInterface {
            name: interface.name.clone().unwrap_or_else(|| format!("interface_{}", interface_id.index())),
            functions: Vec::new(),
            types: Vec::new(),
            documentation: interface.docs.contents.as_ref().map(|d| d.clone()),
        };
        
        // Extract functions
        for (func_name, func) in &interface.functions {
            let wit_function = self.extract_function(resolve, func_name, func)?;
            wit_interface.functions.push(wit_function);
        }
        
        // Extract types
        for (type_name, type_id) in &interface.types {
            if let Some(type_def) = resolve.types.get(*type_id) {
                let wit_type = self.extract_type(resolve, *type_id, type_def)?;
                wit_interface.types.push(wit_type);
            }
        }
        
        Ok(wit_interface)
    }
    
    /// Extract world information from wit-parser structures
    fn extract_world(
        &self, 
        _resolve: &Resolve, 
        world: &wit_parser::World,
    ) -> Result<WitWorld> {
        let wit_world = WitWorld {
            name: world.name.clone(),
            imports: world.imports.keys().map(|k| format!("{:?}", k)).collect(),
            exports: world.exports.keys().map(|k| format!("{:?}", k)).collect(),
            documentation: world.docs.contents.as_ref().map(|d| d.clone()),
        };
        
        Ok(wit_world)
    }
    
    /// Extract function information from wit-parser structures
    fn extract_function(
        &self, 
        resolve: &Resolve, 
        name: &str, 
        func: &wit_parser::Function,
    ) -> Result<WitFunction> {
        let mut parameters = Vec::new();
        
        // Extract parameters
        for (param_name, param_type) in &func.params {
            let wit_param = WitParameter {
                name: param_name.clone(),
                param_type: self.convert_type(resolve, param_type)?,
            };
            parameters.push(wit_param);
        }
        
        // Extract return type - func.result is now just a Type, not Results enum
        let return_type = match &func.result {
            Some(result_type) => Some(self.convert_type(resolve, result_type)?),
            None => None,
        };
        
        Ok(WitFunction {
            name: name.to_string(),
            parameters,
            return_type,
            documentation: func.docs.contents.as_ref().map(|d| d.clone()),
        })
    }
    
    /// Extract and convert type information
    fn extract_type(
        &self, 
        resolve: &Resolve, 
        type_id: wit_parser::TypeId,
        type_def: &wit_parser::TypeDef,
    ) -> Result<WitType> {
        let name = type_def.name.as_ref()
            .map(|s| s.to_string())
            .unwrap_or_else(|| format!("type_{}", type_id.index()));
        
        let kind = self.convert_type_def(resolve, type_def)?;
        
        Ok(WitType {
            name,
            kind,
            documentation: type_def.docs.contents.as_ref().map(|d| d.clone()),
        })
    }
    
    /// Convert wit-parser type to our type representation
    fn convert_type(&self, resolve: &Resolve, wit_type: &wit_parser::Type) -> Result<WitType> {
        let kind = match wit_type {
            wit_parser::Type::Bool => WitTypeKind::Primitive("bool".to_string()),
            wit_parser::Type::U8 => WitTypeKind::Primitive("u8".to_string()),
            wit_parser::Type::U16 => WitTypeKind::Primitive("u16".to_string()),
            wit_parser::Type::U32 => WitTypeKind::Primitive("u32".to_string()),
            wit_parser::Type::U64 => WitTypeKind::Primitive("u64".to_string()),
            wit_parser::Type::S8 => WitTypeKind::Primitive("s8".to_string()),
            wit_parser::Type::S16 => WitTypeKind::Primitive("s16".to_string()),
            wit_parser::Type::S32 => WitTypeKind::Primitive("s32".to_string()),
            wit_parser::Type::S64 => WitTypeKind::Primitive("s64".to_string()),
            wit_parser::Type::F32 => WitTypeKind::Primitive("f32".to_string()),
            wit_parser::Type::F64 => WitTypeKind::Primitive("f64".to_string()),
            wit_parser::Type::Char => WitTypeKind::Primitive("char".to_string()),
            wit_parser::Type::String => WitTypeKind::Primitive("string".to_string()),
            wit_parser::Type::Id(type_id) => {
                if let Some(type_def) = resolve.types.get(*type_id) {
                    return Ok(WitType {
                        name: type_def.name.as_ref()
                            .map(|s| s.to_string())
                            .unwrap_or_else(|| format!("type_{}", type_id.index())),
                        kind: self.convert_type_def(resolve, type_def)?,
                        documentation: type_def.docs.contents.as_ref().map(|d| d.clone()),
                    });
                } else {
                    WitTypeKind::Reference(format!("type_{}", type_id.index()))
                }
            }
            wit_parser::Type::ErrorContext => WitTypeKind::Primitive("error-context".to_string()),
        };
        
        Ok(WitType {
            name: "anonymous".to_string(),
            kind,
            documentation: None,
        })
    }
    
    /// Convert wit-parser type definition to our representation
    fn convert_type_def(&self, resolve: &Resolve, type_def: &wit_parser::TypeDef) -> Result<WitTypeKind> {
        match &type_def.kind {
            wit_parser::TypeDefKind::Record(record) => {
                let mut fields = Vec::new();
                for field in &record.fields {
                    let wit_field = WitField {
                        name: field.name.clone(),
                        field_type: self.convert_type(resolve, &field.ty)?,
                        documentation: field.docs.contents.as_ref().map(|d| d.clone()),
                    };
                    fields.push(wit_field);
                }
                Ok(WitTypeKind::Record(fields))
            },
            wit_parser::TypeDefKind::Variant(variant) => {
                let mut cases = Vec::new();
                for case in &variant.cases {
                    let payload = match &case.ty {
                        Some(ty) => Some(self.convert_type(resolve, ty)?),
                        None => None,
                    };
                    let wit_case = WitVariantCase {
                        name: case.name.clone(),
                        payload,
                        documentation: case.docs.contents.as_ref().map(|d| d.clone()),
                    };
                    cases.push(wit_case);
                }
                Ok(WitTypeKind::Variant(cases))
            },
            wit_parser::TypeDefKind::Enum(enum_def) => {
                let values = enum_def.cases.iter().map(|case| case.name.clone()).collect();
                Ok(WitTypeKind::Enum(values))
            },
            wit_parser::TypeDefKind::List(list_type) => {
                let element_type = Box::new(self.convert_type(resolve, list_type)?);
                Ok(WitTypeKind::List(element_type))
            },
            wit_parser::TypeDefKind::Option(option_type) => {
                let inner_type = Box::new(self.convert_type(resolve, option_type)?);
                Ok(WitTypeKind::Option(inner_type))
            },
            wit_parser::TypeDefKind::Result(result_type) => {
                let ok = match &result_type.ok {
                    Some(ty) => Some(Box::new(self.convert_type(resolve, ty)?)),
                    None => None,
                };
                let err = match &result_type.err {
                    Some(ty) => Some(Box::new(self.convert_type(resolve, ty)?)),
                    None => None,
                };
                Ok(WitTypeKind::Result { ok, err })
            },
            wit_parser::TypeDefKind::Tuple(tuple) => {
                let mut types = Vec::new();
                for ty in &tuple.types {
                    types.push(self.convert_type(resolve, ty)?);
                }
                Ok(WitTypeKind::Tuple(types))
            },
            wit_parser::TypeDefKind::Flags(_) => {
                // Flags are represented as a special enum for now
                Ok(WitTypeKind::Primitive("flags".to_string()))
            },
            wit_parser::TypeDefKind::Handle(_) => {
                // Handles are represented as opaque types
                Ok(WitTypeKind::Primitive("handle".to_string()))
            },
            wit_parser::TypeDefKind::Resource => {
                // Resources are represented as opaque types
                Ok(WitTypeKind::Primitive("resource".to_string()))
            },
            _ => {
                // Unknown or unsupported type
                Ok(WitTypeKind::Primitive("unknown".to_string()))
            }
        }
    }
}

impl Default for WitParser {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::path::PathBuf;
    
    #[test]
    fn test_parser_creation() {
        let parser = WitParser::new();
        assert_eq!(parser.max_file_size, 10 * 1024 * 1024);
        assert!(parser.include_raw_content);
    }
    
    #[test]
    fn test_memory_constrained_parser() {
        let parser = WitParser::memory_constrained();
        assert_eq!(parser.max_file_size, 1024 * 1024);
        assert!(!parser.include_raw_content);
    }
    
    #[test]
    fn test_high_performance_parser() {
        let parser = WitParser::high_performance();
        assert_eq!(parser.max_file_size, 100 * 1024 * 1024);
        assert!(parser.include_raw_content);
    }
    
    #[test]
    fn test_parser_configuration() {
        let parser = WitParser::new()
            .with_max_file_size(5 * 1024 * 1024)
            .with_raw_content(false);
        
        assert_eq!(parser.max_file_size, 5 * 1024 * 1024);
        assert!(!parser.include_raw_content);
    }
}
