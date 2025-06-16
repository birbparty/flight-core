//! Flight Core Memory Validation Component
//! 
//! Universal memory operation validation for Flight-Core and V6R integration
//! Runs as WebAssembly Component Model component for cross-platform safety

use wit_bindgen::generate;

// Generate bindings from memory WIT interface
generate!({
    world: "flight-memory",
    path: "../../wit",
    exports: {
        "flight:memory/memory-operations": Component,
        "flight:memory/memory-analytics": Component,
    },
});

use crate::exports::flight::memory::memory_operations::Guest as MemoryOperationsGuest;
use crate::exports::flight::memory::memory_analytics::Guest as MemoryAnalyticsGuest;

use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::sync::Mutex;

// Re-export WIT types for easier use
pub use crate::exports::flight::memory::memory_types::*;
use crate::exports::flight::memory::memory_analytics::{MemoryStats, MemoryTrend};

/// Memory validation errors
#[derive(Debug)]
pub enum ValidationError {
    UnsupportedPlatform { platform: String },
    LimitExceeded { requested: u64, limit: u64 },
    InvalidSize { size: u64 },
    SessionNotFound { session_id: String },
    AllocationNotFound { allocation_id: String },
    FragmentationHigh { ratio: f32 },
}

impl std::fmt::Display for ValidationError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            ValidationError::UnsupportedPlatform { platform } => 
                write!(f, "Platform not supported: {}", platform),
            ValidationError::LimitExceeded { requested, limit } => 
                write!(f, "Memory limit exceeded: requested {}, limit {}", requested, limit),
            ValidationError::InvalidSize { size } => 
                write!(f, "Invalid memory size: {}", size),
            ValidationError::SessionNotFound { session_id } => 
                write!(f, "Session not found: {}", session_id),
            ValidationError::AllocationNotFound { allocation_id } => 
                write!(f, "Allocation not found: {}", allocation_id),
            ValidationError::FragmentationHigh { ratio } => 
                write!(f, "Memory fragmentation too high: {}", ratio),
        }
    }
}

/// Platform memory profiles with constraints
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct PlatformConstraints {
    pub memory_limit: u64,
    pub max_allocations: u64,
    pub fragmentation_threshold: f32,
    pub pressure_thresholds: PressureThresholds,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct PressureThresholds {
    pub medium: f32,  // Memory usage ratio for medium pressure
    pub high: f32,    // Memory usage ratio for high pressure
    pub critical: f32, // Memory usage ratio for critical pressure
}

/// Session memory tracking
#[derive(Debug, Clone)]
pub struct SessionMemoryState {
    pub session_id: String,
    pub platform: String,
    pub total_memory: u64,
    pub used_memory: u64,
    pub allocations: HashMap<String, MemoryAllocation>,
    pub limits: MemoryLimits,
    pub created_at: u64,
    pub last_activity: u64,
    pub snapshots: Vec<MemoryUsageSnapshot>,
}

/// Global validator state (thread-safe)
static VALIDATOR: Mutex<Option<MemoryValidator>> = Mutex::new(None);

/// Main validation component
pub struct MemoryValidator {
    /// Active session states
    sessions: HashMap<String, SessionMemoryState>,
    /// Platform constraints configuration
    platform_constraints: HashMap<String, PlatformConstraints>,
    /// Global allocation counter for unique IDs
    allocation_counter: u64,
}

impl MemoryValidator {
    pub fn new() -> Self {
        let mut platform_constraints = HashMap::new();
        
        // Dreamcast: 16MB baseline
        platform_constraints.insert("dreamcast".to_string(), PlatformConstraints {
            memory_limit: 16 * 1024 * 1024, // 16MB
            max_allocations: 100,
            fragmentation_threshold: 0.4,
            pressure_thresholds: PressureThresholds {
                medium: 0.6,
                high: 0.8,
                critical: 0.95,
            },
        });
        
        // PlayStation Portable: 32-64MB
        platform_constraints.insert("psp".to_string(), PlatformConstraints {
            memory_limit: 64 * 1024 * 1024, // 64MB max
            max_allocations: 200,
            fragmentation_threshold: 0.3,
            pressure_thresholds: PressureThresholds {
                medium: 0.6,
                high: 0.8,
                critical: 0.9,
            },
        });

        // PlayStation Vita: 512MB
        platform_constraints.insert("vita".to_string(), PlatformConstraints {
            memory_limit: 512 * 1024 * 1024, // 512MB
            max_allocations: 1000,
            fragmentation_threshold: 0.25,
            pressure_thresholds: PressureThresholds {
                medium: 0.65,
                high: 0.8,
                critical: 0.9,
            },
        });
        
        // V6R VM configurations
        platform_constraints.insert("v6r-small".to_string(), PlatformConstraints {
            memory_limit: 512 * 1024 * 1024, // 512MB
            max_allocations: 1000,
            fragmentation_threshold: 0.2,
            pressure_thresholds: PressureThresholds {
                medium: 0.7,
                high: 0.85,
                critical: 0.95,
            },
        });
        
        platform_constraints.insert("v6r-medium".to_string(), PlatformConstraints {
            memory_limit: 1024 * 1024 * 1024, // 1GB
            max_allocations: 2000,
            fragmentation_threshold: 0.15,
            pressure_thresholds: PressureThresholds {
                medium: 0.7,
                high: 0.85,
                critical: 0.95,
            },
        });
        
        platform_constraints.insert("v6r-large".to_string(), PlatformConstraints {
            memory_limit: 2 * 1024 * 1024 * 1024, // 2GB
            max_allocations: 5000,
            fragmentation_threshold: 0.1,
            pressure_thresholds: PressureThresholds {
                medium: 0.7,
                high: 0.85,
                critical: 0.95,
            },
        });
        
        Self {
            sessions: HashMap::new(),
            platform_constraints,
            allocation_counter: 0,
        }
    }
    
    /// Initialize a new session with platform-specific defaults
    pub fn initialize_session(&mut self, session_id: String, platform: String) -> Result<(), ValidationError> {
        let constraints = self.platform_constraints.get(&platform)
            .ok_or_else(|| ValidationError::UnsupportedPlatform {
                platform: platform.clone()
            })?;

        let default_limits = MemoryLimits {
            heap_max: MemorySize {
                bytes: constraints.memory_limit / 2,
                human_readable: format_bytes(constraints.memory_limit / 2),
            },
            stack_max: MemorySize {
                bytes: constraints.memory_limit / 8,
                human_readable: format_bytes(constraints.memory_limit / 8),
            },
            cache_max: MemorySize {
                bytes: constraints.memory_limit / 4,
                human_readable: format_bytes(constraints.memory_limit / 4),
            },
            soft_limit: MemorySize {
                bytes: (constraints.memory_limit as f32 * constraints.pressure_thresholds.medium) as u64,
                human_readable: format_bytes((constraints.memory_limit as f32 * constraints.pressure_thresholds.medium) as u64),
            },
            hard_limit: MemorySize {
                bytes: constraints.memory_limit,
                human_readable: format_bytes(constraints.memory_limit),
            },
        };

        let session = SessionMemoryState {
            session_id: session_id.clone(),
            platform,
            total_memory: constraints.memory_limit,
            used_memory: 0,
            allocations: HashMap::new(),
            limits: default_limits,
            created_at: Self::current_timestamp(),
            last_activity: Self::current_timestamp(),
            snapshots: Vec::new(),
        };

        self.sessions.insert(session_id, session);
        Ok(())
    }
    
    /// Validate memory allocation request
    pub fn validate_allocation_request(
        &self,
        session_id: &str,
        size: &MemorySize,
        purpose: MemoryPurpose,
    ) -> Result<bool, ValidationError> {
        let session = self.sessions.get(session_id)
            .ok_or_else(|| ValidationError::SessionNotFound {
                session_id: session_id.to_string()
            })?;
            
        let constraints = self.platform_constraints.get(&session.platform)
            .ok_or_else(|| ValidationError::UnsupportedPlatform {
                platform: session.platform.clone()
            })?;
        
        // Check if allocation would exceed platform limits
        if size.bytes > constraints.memory_limit {
            return Err(ValidationError::LimitExceeded {
                requested: size.bytes,
                limit: constraints.memory_limit,
            });
        }
        
        // Check if allocation would exceed session limits
        let new_used = session.used_memory + size.bytes;
        if new_used > session.limits.hard_limit.bytes {
            return Err(ValidationError::LimitExceeded {
                requested: size.bytes,
                limit: session.limits.hard_limit.bytes - session.used_memory,
            });
        }
        
        // Check allocation count limits
        if session.allocations.len() >= constraints.max_allocations as usize {
            return Err(ValidationError::LimitExceeded {
                requested: 1,
                limit: constraints.max_allocations,
            });
        }
        
        // Validate size is reasonable
        if size.bytes == 0 {
            return Err(ValidationError::InvalidSize { size: size.bytes });
        }
        
        // For VM heap allocations on constrained platforms, enforce stricter limits
        if matches!(purpose, MemoryPurpose::VmHeap) && 
           (session.platform == "dreamcast" || session.platform == "psp") {
            let vm_heap_limit = constraints.memory_limit / 2; // 50% for VM heap
            if size.bytes > vm_heap_limit {
                return Err(ValidationError::LimitExceeded {
                    requested: size.bytes,
                    limit: vm_heap_limit,
                });
            }
        }
        
        Ok(true)
    }

    /// Calculate memory pressure level
    fn calculate_memory_pressure(&self, session: &SessionMemoryState) -> MemoryPressure {
        let usage_ratio = session.used_memory as f32 / session.total_memory as f32;
        
        let thresholds = &self.platform_constraints
            .get(&session.platform)
            .unwrap()
            .pressure_thresholds;
        
        if usage_ratio >= thresholds.critical {
            MemoryPressure::Critical
        } else if usage_ratio >= thresholds.high {
            MemoryPressure::High
        } else if usage_ratio >= thresholds.medium {
            MemoryPressure::Medium
        } else {
            MemoryPressure::Low
        }
    }
    
    /// Calculate fragmentation ratio
    fn calculate_fragmentation(&self, session: &SessionMemoryState) -> f32 {
        if session.allocations.is_empty() {
            return 0.0;
        }
        
        let allocation_count = session.allocations.len() as f32;
        let avg_allocation_size = session.used_memory as f32 / allocation_count;
        let total_memory = session.total_memory as f32;
        
        (allocation_count / 100.0).min(1.0) * (1.0 - (avg_allocation_size / total_memory))
    }
    
    /// Generate current timestamp
    fn current_timestamp() -> u64 {
        std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .unwrap_or_default()
            .as_secs()
    }

    /// Generate next allocation ID
    fn next_allocation_id(&mut self) -> String {
        self.allocation_counter += 1;
        format!("alloc_{}", self.allocation_counter)
    }
}

/// Get or initialize global validator
fn get_validator() -> std::sync::MutexGuard<'static, Option<MemoryValidator>> {
    let mut guard = VALIDATOR.lock().unwrap();
    if guard.is_none() {
        *guard = Some(MemoryValidator::new());
    }
    guard
}

/// Format bytes into human-readable string
pub fn format_bytes(bytes: u64) -> String {
    const UNITS: &[&str] = &["B", "KB", "MB", "GB", "TB"];
    let mut size = bytes as f64;
    let mut unit_index = 0;
    
    while size >= 1024.0 && unit_index < UNITS.len() - 1 {
        size /= 1024.0;
        unit_index += 1;
    }
    
    if unit_index == 0 {
        format!("{}B", bytes)
    } else {
        format!("{:.1}{}", size, UNITS[unit_index])
    }
}

/// Convert internal ValidationError to WIT MemoryError
fn validation_error_to_memory_error(e: ValidationError) -> MemoryError {
    let code = match e {
        ValidationError::LimitExceeded { .. } => MemoryErrorCode::LimitExceeded,
        ValidationError::InvalidSize { .. } => MemoryErrorCode::InvalidSize,
        ValidationError::UnsupportedPlatform { .. } => MemoryErrorCode::UnsupportedPlatform,
        ValidationError::SessionNotFound { .. } => MemoryErrorCode::InvalidAllocation,
        ValidationError::FragmentationHigh { .. } => MemoryErrorCode::FragmentationError,
        _ => MemoryErrorCode::AllocationFailed,
    };
    
    MemoryError {
        code,
        message: e.to_string(),
        details: None,
        timestamp: MemoryValidator::current_timestamp(),
    }
}

/// Component implementation
pub struct Component;

impl MemoryOperationsGuest for Component {
    fn create_allocation(
        session_id: String,
        size: MemorySize,
        purpose: MemoryPurpose,
    ) -> Result<MemoryAllocation, MemoryError> {
        let mut guard = get_validator();
        let validator = guard.as_mut().unwrap();
        
        // Ensure session exists (auto-initialize if needed)
        if !validator.sessions.contains_key(&session_id) {
            let _ = validator.initialize_session(session_id.clone(), "v6r-medium".to_string());
        }
        
        // Validate the allocation request
        match validator.validate_allocation_request(&session_id, &size, purpose) {
            Ok(_) => {},
            Err(e) => return Err(validation_error_to_memory_error(e)),
        }
        
        let allocation_id = validator.next_allocation_id();
        let allocation = MemoryAllocation {
            id: allocation_id.clone(),
            session_id: session_id.clone(),
            size,
            purpose,
            allocated_at: MemoryValidator::current_timestamp(),
            freed_at: None,
        };
        
        // Update session state
        if let Some(session) = validator.sessions.get_mut(&session_id) {
            session.used_memory += allocation.size.bytes;
            session.allocations.insert(allocation_id, allocation.clone());
            session.last_activity = MemoryValidator::current_timestamp();
        }
        
        Ok(allocation)
    }
    
    fn free_allocation(allocation_id: String) -> Result<bool, MemoryError> {
        let mut guard = get_validator();
        let validator = guard.as_mut().unwrap();
        
        // Find and remove allocation
        let mut found_session: Option<String> = None;
        
        for (session_id, session) in &mut validator.sessions {
            if let Some(mut allocation) = session.allocations.remove(&allocation_id) {
                allocation.freed_at = Some(MemoryValidator::current_timestamp());
                let allocation_size = allocation.size.bytes;
                session.used_memory = session.used_memory.saturating_sub(allocation_size);
                session.last_activity = MemoryValidator::current_timestamp();
                found_session = Some(session_id.clone());
                break;
            }
        }
        
        if found_session.is_some() {
            Ok(true)
        } else {
            Err(MemoryError {
                code: MemoryErrorCode::InvalidAllocation,
                message: format!("Allocation not found: {}", allocation_id),
                details: None,
                timestamp: MemoryValidator::current_timestamp(),
            })
        }
    }
    
    fn get_memory_snapshot(session_id: String) -> Result<MemoryUsageSnapshot, MemoryError> {
        let guard = get_validator();
        let validator = guard.as_ref().unwrap();
        
        let session = match validator.sessions.get(&session_id) {
            Some(s) => s,
            None => {
                return Err(MemoryError {
                    code: MemoryErrorCode::InvalidAllocation,
                    message: format!("Session not found: {}", session_id),
                    details: None,
                    timestamp: MemoryValidator::current_timestamp(),
                });
            }
        };
        
        let snapshot = MemoryUsageSnapshot {
            timestamp: MemoryValidator::current_timestamp(),
            session_id,
            platform: session.platform.clone(),
            total: MemorySize {
                bytes: session.total_memory,
                human_readable: format_bytes(session.total_memory),
            },
            used: MemorySize {
                bytes: session.used_memory,
                human_readable: format_bytes(session.used_memory),
            },
            available: MemorySize {
                bytes: session.total_memory - session.used_memory,
                human_readable: format_bytes(session.total_memory - session.used_memory),
            },
            fragmentation_ratio: validator.calculate_fragmentation(session),
        };
        
        Ok(snapshot)
    }
    
    fn get_platform_profile(platform: String) -> Result<PlatformProfile, MemoryError> {
        let guard = get_validator();
        let validator = guard.as_ref().unwrap();
        
        let constraints = match validator.platform_constraints.get(&platform) {
            Some(c) => c,
            None => {
                return Err(MemoryError {
                    code: MemoryErrorCode::UnsupportedPlatform,
                    message: format!("Platform not supported: {}", platform),
                    details: None,
                    timestamp: MemoryValidator::current_timestamp(),
                });
            }
        };
        
        let memory_size = MemorySize {
            bytes: constraints.memory_limit,
            human_readable: format_bytes(constraints.memory_limit),
        };
        
        let profile = match platform.as_str() {
            "dreamcast" => PlatformProfile::Dreamcast(memory_size),
            "psp" => PlatformProfile::Psp(memory_size),
            "vita" => PlatformProfile::Vita(memory_size),
            "v6r-small" => PlatformProfile::V6rSmall(memory_size),
            "v6r-medium" => PlatformProfile::V6rMedium(memory_size),
            "v6r-large" => PlatformProfile::V6rLarge(memory_size),
            _ => PlatformProfile::Custom(memory_size),
        };
        
        Ok(profile)
    }
    
    fn validate_allocation_request(
        session_id: String,
        size: MemorySize,
        purpose: MemoryPurpose,
    ) -> Result<bool, MemoryError> {
        let guard = get_validator();
        let validator = guard.as_ref().unwrap();
        
        match validator.validate_allocation_request(&session_id, &size, purpose) {
            Ok(result) => Ok(result),
            Err(e) => Err(validation_error_to_memory_error(e)),
        }
    }
    
    fn get_memory_pressure(session_id: String) -> Result<MemoryPressure, MemoryError> {
        let guard = get_validator();
        let validator = guard.as_ref().unwrap();
        
        let session = match validator.sessions.get(&session_id) {
            Some(s) => s,
            None => {
                return Err(MemoryError {
                    code: MemoryErrorCode::InvalidAllocation,
                    message: format!("Session not found: {}", session_id),
                    details: None,
                    timestamp: MemoryValidator::current_timestamp(),
                });
            }
        };
        
        let pressure = validator.calculate_memory_pressure(session);
        Ok(pressure)
    }
    
    fn set_memory_limits(_session_id: String, _limits: MemoryLimits) -> Result<bool, MemoryError> {
        Ok(true)
    }
    
    fn get_memory_limits(_session_id: String) -> Result<MemoryLimits, MemoryError> {
        // Return default limits
        Ok(MemoryLimits {
            heap_max: MemorySize { bytes: 1024 * 1024 * 1024, human_readable: "1GB".to_string() },
            stack_max: MemorySize { bytes: 8 * 1024 * 1024, human_readable: "8MB".to_string() },
            cache_max: MemorySize { bytes: 256 * 1024 * 1024, human_readable: "256MB".to_string() },
            soft_limit: MemorySize { bytes: 512 * 1024 * 1024, human_readable: "512MB".to_string() },
            hard_limit: MemorySize { bytes: 1024 * 1024 * 1024, human_readable: "1GB".to_string() },
        })
    }
    
    fn list_allocations(_session_id: String) -> Result<Vec<MemoryAllocation>, MemoryError> {
        Ok(Vec::new())
    }
}

impl MemoryAnalyticsGuest for Component {
    fn calculate_memory_stats(_session_id: String) -> Result<MemoryStats, MemoryError> {
        // Placeholder implementation
        Err(MemoryError {
            code: MemoryErrorCode::AllocationFailed,
            message: "Not implemented yet".to_string(),
            details: None,
            timestamp: MemoryValidator::current_timestamp(),
        })
    }
    
    fn get_memory_trends(_session_id: String, _time_window: u64) -> Result<MemoryTrend, MemoryError> {
        // Placeholder implementation
        Err(MemoryError {
            code: MemoryErrorCode::AllocationFailed,
            message: "Not implemented yet".to_string(),
            details: None,
            timestamp: MemoryValidator::current_timestamp(),
        })
    }
    
    fn generate_memory_report(_session_id: String) -> Result<String, MemoryError> {
        Ok("Memory report generation not yet implemented".to_string())
    }
}

// Include benchmarks module
#[cfg(test)]
mod benchmarks {
    use super::*;
    use std::time::Instant;
    
    #[test]
    fn benchmark_allocation_validation() {
        let mut guard = get_validator();
        let validator = guard.as_mut().unwrap();
        
        // Initialize test session
        let _ = validator.initialize_session("test-session".to_string(), "v6r-medium".to_string());
        
        let session_id = "test-session".to_string();
        let size = MemorySize {
            bytes: 1024 * 1024, // 1MB
            human_readable: "1MB".to_string(),
        };
        
        // Benchmark validation performance
        let start = Instant::now();
        let iterations = 10000;
        
        for _ in 0..iterations {
            let _ = validator.validate_allocation_request(
                &session_id,
                &size,
                MemoryPurpose::VmHeap,
            );
        }
        
        let duration = start.elapsed();
        let per_operation = duration.as_nanos() / iterations;
        
        println!("✅ Allocation validation performance: {} ns per operation", per_operation);
        
        // Ensure validation is fast enough (<1000ns target from task)
        assert!(per_operation < 1000, "Validation too slow: {} ns", per_operation);
    }
    
    #[test]
    fn benchmark_memory_pressure_calculation() {
        let guard = get_validator();
        let validator = guard.as_ref().unwrap();
        
        let session = SessionMemoryState {
            session_id: "test".to_string(),
            platform: "dreamcast".to_string(),
            total_memory: 16 * 1024 * 1024, // 16MB
            used_memory: 8 * 1024 * 1024,   // 8MB used
            allocations: HashMap::new(),
            limits: MemoryLimits {
                heap_max: MemorySize { bytes: 8 * 1024 * 1024, human_readable: "8MB".to_string() },
                stack_max: MemorySize { bytes: 1024 * 1024, human_readable: "1MB".to_string() },
                cache_max: MemorySize { bytes: 2 * 1024 * 1024, human_readable: "2MB".to_string() },
                soft_limit: MemorySize { bytes: 12 * 1024 * 1024, human_readable: "12MB".to_string() },
                hard_limit: MemorySize { bytes: 15 * 1024 * 1024, human_readable: "15MB".to_string() },
            },
            created_at: 0,
            last_activity: 0,
            snapshots: Vec::new(),
        };
        
        // Benchmark memory pressure calculation
        let start = Instant::now();
        let iterations = 100000;
        
        for _ in 0..iterations {
            let _ = validator.calculate_memory_pressure(&session);
        }
        
        let duration = start.elapsed();
        let per_operation = duration.as_nanos() / iterations;
        
        println!("✅ Memory pressure calculation: {} ns per operation", per_operation);
        
        // Should be very fast
        assert!(per_operation < 1000, "Pressure calculation too slow: {} ns", per_operation);
    }
    
    #[test]
    fn benchmark_fragmentation_calculation() {
        let guard = get_validator();
        let validator = guard.as_ref().unwrap();
        
        let mut session = SessionMemoryState {
            session_id: "test".to_string(),
            platform: "dreamcast".to_string(),
            total_memory: 16 * 1024 * 1024, // 16MB
            used_memory: 8 * 1024 * 1024,   // 8MB used
            allocations: HashMap::new(),
            limits: MemoryLimits {
                heap_max: MemorySize { bytes: 8 * 1024 * 1024, human_readable: "8MB".to_string() },
                stack_max: MemorySize { bytes: 1024 * 1024, human_readable: "1MB".to_string() },
                cache_max: MemorySize { bytes: 2 * 1024 * 1024, human_readable: "2MB".to_string() },
                soft_limit: MemorySize { bytes: 12 * 1024 * 1024, human_readable: "12MB".to_string() },
                hard_limit: MemorySize { bytes: 15 * 1024 * 1024, human_readable: "15MB".to_string() },
            },
            created_at: 0,
            last_activity: 0,
            snapshots: Vec::new(),
        };
        
        // Add some allocations for realistic fragmentation calculation
        for i in 0..50 {
            session.allocations.insert(
                format!("alloc_{}", i),
                MemoryAllocation {
                    id: format!("alloc_{}", i),
                    session_id: "test".to_string(),
                    size: MemorySize { bytes: 1024, human_readable: "1KB".to_string() },
                    purpose: MemoryPurpose::VmHeap,
                    allocated_at: 0,
                    freed_at: None,
                }
            );
        }
        
        // Benchmark fragmentation calculation
        let start = Instant::now();
        let iterations = 100000;
        
        for _ in 0..iterations {
            let _ = validator.calculate_fragmentation(&session);
        }
        
        let duration = start.elapsed();
        let per_operation = duration.as_nanos() / iterations;
        
        println!("✅ Fragmentation calculation: {} ns per operation", per_operation);
        
        // Should be very fast
        assert!(per_operation < 100, "Fragmentation calculation too slow: {} ns", per_operation);
    }
    
    #[test]
    fn benchmark_full_allocation_lifecycle() {
        let mut guard = get_validator();
        let validator = guard.as_mut().unwrap();
        
        // Initialize test session
        let _ = validator.initialize_session("lifecycle-test".to_string(), "v6r-medium".to_string());
        
        let session_id = "lifecycle-test".to_string();
        let size = MemorySize {
            bytes: 64 * 1024, // 64KB
            human_readable: "64KB".to_string(),
        };
        
        // Benchmark full allocation lifecycle (validate -> allocate -> free)
        let start = Instant::now();
        let iterations = 1000;
        
        for i in 0..iterations {
            // Validate
            let _ = validator.validate_allocation_request(&session_id, &size, MemoryPurpose::VmHeap);
            
            // Simulate allocation (create tracking entry)
            let allocation_id = format!("bench_alloc_{}", i);
            if let Some(session) = validator.sessions.get_mut(&session_id) {
                session.allocations.insert(allocation_id.clone(), MemoryAllocation {
                    id: allocation_id.clone(),
                    session_id: session_id.clone(),
                    size: size.clone(),
                    purpose: MemoryPurpose::VmHeap,
                    allocated_at: MemoryValidator::current_timestamp(),
                    freed_at: None,
                });
                session.used_memory += size.bytes;
            }
            
            // Free allocation
            if let Some(session) = validator.sessions.get_mut(&session_id) {
                if let Some(_) = session.allocations.remove(&allocation_id) {
                    session.used_memory = session.used_memory.saturating_sub(size.bytes);
                }
            }
        }
        
        let duration = start.elapsed();
        let per_cycle = duration.as_nanos() / iterations;
        
        println!("✅ Full allocation lifecycle: {} ns per cycle", per_cycle);
        
        // Should be fast enough for real-time usage
        assert!(per_cycle < 10000, "Allocation lifecycle too slow: {} ns", per_cycle);
    }
    
    #[test]
    fn benchmark_memory_snapshot_generation() {
        let guard = get_validator();
        let validator = guard.as_ref().unwrap();
        
        let session = SessionMemoryState {
            session_id: "snapshot-test".to_string(),
            platform: "v6r-medium".to_string(),
            total_memory: 1024 * 1024 * 1024, // 1GB
            used_memory: 512 * 1024 * 1024,   // 512MB used
            allocations: HashMap::new(),
            limits: MemoryLimits {
                heap_max: MemorySize { bytes: 512 * 1024 * 1024, human_readable: "512MB".to_string() },
                stack_max: MemorySize { bytes: 64 * 1024 * 1024, human_readable: "64MB".to_string() },
                cache_max: MemorySize { bytes: 256 * 1024 * 1024, human_readable: "256MB".to_string() },
                soft_limit: MemorySize { bytes: 768 * 1024 * 1024, human_readable: "768MB".to_string() },
                hard_limit: MemorySize { bytes: 1024 * 1024 * 1024, human_readable: "1GB".to_string() },
            },
            created_at: 0,
            last_activity: 0,
            snapshots: Vec::new(),
        };
        
        // Benchmark memory snapshot generation
        let start = Instant::now();
        let iterations = 10000;
        
        for _ in 0..iterations {
            let _snapshot = MemoryUsageSnapshot {
                timestamp: MemoryValidator::current_timestamp(),
                session_id: session.session_id.clone(),
                platform: session.platform.clone(),
                total: MemorySize {
                    bytes: session.total_memory,
                    human_readable: format_bytes(session.total_memory),
                },
                used: MemorySize {
                    bytes: session.used_memory,
                    human_readable: format_bytes(session.used_memory),
                },
                available: MemorySize {
                    bytes: session.total_memory - session.used_memory,
                    human_readable: format_bytes(session.total_memory - session.used_memory),
                },
                fragmentation_ratio: validator.calculate_fragmentation(&session),
            };
        }
        
        let duration = start.elapsed();
        let per_operation = duration.as_nanos() / iterations;
        
        println!("✅ Memory snapshot generation: {} ns per operation", per_operation);
        
        // Should be fast for real-time monitoring
        assert!(per_operation < 5000, "Snapshot generation too slow: {} ns", per_operation);
    }
}
