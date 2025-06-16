//! Complete Rust Bindings for Flight Shared Types
//! Optimized for Component Model validation and Flight-Core integration

use std::collections::HashMap;
use std::sync::{Arc, Mutex};
use std::time::{SystemTime, UNIX_EPOCH};

use serde::{Deserialize, Serialize};
use thiserror::Error;
use uuid::Uuid;

pub mod memory;
pub mod error;
pub mod platform;
pub mod component;
pub mod authentication;
pub mod realtime;
pub mod session;
pub mod pagination;

// Re-export all types for convenience
pub use memory::*;
pub use error::*;
pub use platform::*;
pub use component::*;
pub use authentication::*;
pub use realtime::*;
pub use session::*;
pub use pagination::*;

/// Flight Result type using Rust's native Result
pub type FlightResult<T> = Result<T, FlightError>;

/// Flight-Core Component Model Integration Manager
/// 
/// Provides zero-cost abstractions for Flight-Core HAL integration
pub struct FlightCoreIntegration {
    platform_info: Option<PlatformInfo>,
    components: Arc<Mutex<HashMap<String, ComponentInfo>>>,
    auth_context: Option<AuthContext>,
    memory_manager: Arc<Mutex<MemoryManager>>,
    error_manager: Arc<Mutex<ErrorManager>>,
}

impl FlightCoreIntegration {
    /// Create a new Flight-Core integration instance
    pub fn new() -> Self {
        Self {
            platform_info: None,
            components: Arc::new(Mutex::new(HashMap::new())),
            auth_context: None,
            memory_manager: Arc::new(Mutex::new(MemoryManager::new())),
            error_manager: Arc::new(Mutex::new(ErrorManager::new())),
        }
    }

    /// Initialize with platform information
    pub fn initialize(&mut self, platform_id: &str) -> FlightResult<()> {
        let platform_info = self.get_platform_info(platform_id)?;
        self.platform_info = Some(platform_info);
        Ok(())
    }

    /// Create a new component instance
    pub fn create_component(
        &self,
        name: &str,
        world: &str,
        platform: &str,
        session_id: Option<String>,
    ) -> FlightResult<ComponentId> {
        let mut components = self.components.lock().unwrap();
        let component_id = ComponentId::new();
        
        let component_info = ComponentInfo {
            id: component_id.clone(),
            name: name.to_string(),
            version: "1.0.0".to_string(),
            state: ComponentState::Loaded,
            world: WorldName(world.to_string()),
            platform: platform.to_string(),
            session_id,
            created_at: current_timestamp(),
            last_activity: current_timestamp(),
            memory_usage: MemoryUsageSnapshot::default(),
            metadata: vec![
                MetadataPair {
                    key: "created_by".to_string(),
                    value: "rust-integration".to_string(),
                },
            ],
        };

        components.insert(component_id.0.clone(), component_info);
        Ok(component_id)
    }

    /// Get platform information for the given platform ID
    fn get_platform_info(&self, platform_id: &str) -> FlightResult<PlatformInfo> {
        match platform_id {
            "dreamcast" => Ok(PlatformInfo {
                id: platform_id.to_string(),
                name: "Sega Dreamcast".to_string(),
                architecture: "SH-4".to_string(),
                memory_total: MemorySize {
                    bytes: 16 * 1024 * 1024, // 16MB
                    human_readable: "16MB".to_string(),
                },
                capabilities: vec!["basic-graphics".to_string(), "audio".to_string()],
                constraints: PlatformConstraints {
                    max_memory: MemorySize {
                        bytes: 16 * 1024 * 1024,
                        human_readable: "16MB".to_string(),
                    },
                    max_cpu_threads: 1,
                    max_open_files: 32,
                    network_enabled: false,
                },
                version: "1.0".to_string(),
                vendor: "Sega".to_string(),
                features: vec!["component-model".to_string()],
            }),
            "vita" => Ok(PlatformInfo {
                id: platform_id.to_string(),
                name: "PlayStation Vita".to_string(),
                architecture: "ARM Cortex-A9".to_string(),
                memory_total: MemorySize {
                    bytes: 512 * 1024 * 1024, // 512MB
                    human_readable: "512MB".to_string(),
                },
                capabilities: vec!["graphics".to_string(), "audio".to_string(), "network".to_string()],
                constraints: PlatformConstraints {
                    max_memory: MemorySize {
                        bytes: 512 * 1024 * 1024,
                        human_readable: "512MB".to_string(),
                    },
                    max_cpu_threads: 4,
                    max_open_files: 1024,
                    network_enabled: true,
                },
                version: "3.60".to_string(),
                vendor: "Sony".to_string(),
                features: vec!["component-model".to_string(), "real-time".to_string()],
            }),
            _ => Err(FlightError::platform_error(
                format!("Unknown platform: {}", platform_id),
                Some(platform_id.to_string()),
            )),
        }
    }
}

impl Default for FlightCoreIntegration {
    fn default() -> Self {
        Self::new()
    }
}

/// Memory Manager for Rust bindings
#[derive(Debug)]
pub struct MemoryManager {
    allocations: HashMap<String, MemoryAllocation>,
    limits: HashMap<String, MemoryLimits>,
    profiles: HashMap<String, PlatformProfile>,
}

impl MemoryManager {
    pub fn new() -> Self {
        Self {
            allocations: HashMap::new(),
            limits: HashMap::new(),
            profiles: HashMap::new(),
        }
    }

    pub fn create_allocation(
        &mut self,
        session_id: String,
        size: MemorySize,
        purpose: MemoryPurpose,
    ) -> FlightResult<MemoryAllocation> {
        let allocation_id = Uuid::new_v4().to_string();
        let allocation = MemoryAllocation {
            id: allocation_id.clone(),
            session_id: session_id.clone(),
            size,
            purpose,
            allocated_at: current_timestamp(),
            freed_at: None,
        };

        self.allocations.insert(allocation_id, allocation.clone());
        Ok(allocation)
    }

    pub fn get_memory_snapshot(&self, session_id: &str) -> FlightResult<MemoryUsageSnapshot> {
        let mut total_allocated = 0u64;
        let mut active_allocations = 0u32;

        for allocation in self.allocations.values() {
            if allocation.session_id == session_id && allocation.freed_at.is_none() {
                total_allocated += allocation.size.bytes;
                active_allocations += 1;
            }
        }

        // Default to 1GB total if no profile set
        let total_memory = MemorySize {
            bytes: 1024 * 1024 * 1024,
            human_readable: "1GB".to_string(),
        };

        let used = MemorySize {
            bytes: total_allocated,
            human_readable: format_bytes(total_allocated),
        };

        let available = MemorySize {
            bytes: total_memory.bytes.saturating_sub(total_allocated),
            human_readable: format_bytes(total_memory.bytes.saturating_sub(total_allocated)),
        };

        Ok(MemoryUsageSnapshot {
            timestamp: current_timestamp(),
            session_id: session_id.to_string(),
            platform: "rust-platform".to_string(),
            total: total_memory,
            used,
            available,
            fragmentation_ratio: if active_allocations > 100 {
                (active_allocations as f32 / 1000.0).min(0.3)
            } else {
                0.05
            },
        })
    }
}

impl Default for MemoryManager {
    fn default() -> Self {
        Self::new()
    }
}

/// Error Manager for Rust bindings
#[derive(Debug)]
pub struct ErrorManager {
    error_history: Vec<FlightError>,
}

impl ErrorManager {
    pub fn new() -> Self {
        Self {
            error_history: Vec::new(),
        }
    }

    pub fn create_error(
        &mut self,
        severity: ErrorSeverity,
        category: ErrorCategory,
        message: String,
        context: ErrorContext,
    ) -> FlightError {
        let error = FlightError {
            id: Uuid::new_v4().to_string(),
            severity,
            category,
            message,
            details: None,
            context,
            timestamp: current_timestamp(),
            cause: None,
        };

        self.error_history.push(error.clone());
        error
    }

    pub fn create_simple_error(
        &mut self,
        severity: ErrorSeverity,
        category: ErrorCategory,
        message: String,
        source: String,
        operation: String,
    ) -> FlightError {
        let context = ErrorContext {
            source,
            operation,
            session_id: None,
            user_id: None,
            platform: None,
            service_id: None,
            metadata: Vec::new(),
        };

        self.create_error(severity, category, message, context)
    }
}

impl Default for ErrorManager {
    fn default() -> Self {
        Self::new()
    }
}

// Utility functions
fn current_timestamp() -> u64 {
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap()
        .as_secs()
}

fn format_bytes(bytes: u64) -> String {
    const UNIT: u64 = 1024;
    if bytes < UNIT {
        return format!("{}B", bytes);
    }
    
    let mut div = UNIT;
    let mut exp = 0;
    let mut n = bytes / UNIT;
    
    while n >= UNIT && exp < 4 {
        div *= UNIT;
        exp += 1;
        n /= UNIT;
    }
    
    let units = ["K", "M", "G", "T", "P"];
    format!("{:.1}{}B", bytes as f64 / div as f64, units[exp])
}

// Component Model integration traits
#[cfg(feature = "component-model")]
pub trait ComponentModelIntegration {
    fn export_component(&self) -> FlightResult<Vec<u8>>;
    fn import_component(&mut self, data: &[u8]) -> FlightResult<()>;
}

#[cfg(feature = "component-model")]
impl ComponentModelIntegration for FlightCoreIntegration {
    fn export_component(&self) -> FlightResult<Vec<u8>> {
        // Serialize the current state for Component Model export
        let state = serde_json::to_vec(&self.platform_info)
            .map_err(|e| FlightError::serialization_error(e.to_string()))?;
        Ok(state)
    }

    fn import_component(&mut self, data: &[u8]) -> FlightResult<()> {
        // Deserialize Component Model import
        let platform_info: PlatformInfo = serde_json::from_slice(data)
            .map_err(|e| FlightError::serialization_error(e.to_string()))?;
        self.platform_info = Some(platform_info);
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_flight_integration() {
        let mut integration = FlightCoreIntegration::new();
        assert!(integration.initialize("dreamcast").is_ok());
    }

    #[test]
    fn test_memory_manager() {
        let mut manager = MemoryManager::new();
        let size = MemorySize {
            bytes: 1024,
            human_readable: "1KB".to_string(),
        };
        
        let allocation = manager.create_allocation(
            "test-session".to_string(),
            size,
            MemoryPurpose::ComponentStack,
        );
        
        assert!(allocation.is_ok());
    }

    #[test]
    fn test_error_manager() {
        let mut manager = ErrorManager::new();
        let error = manager.create_simple_error(
            ErrorSeverity::Error,
            ErrorCategory::Component,
            "Test error".to_string(),
            "test".to_string(),
            "test_operation".to_string(),
        );
        
        assert_eq!(error.message, "Test error");
        assert_eq!(error.severity, ErrorSeverity::Error);
    }
}
