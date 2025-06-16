//! Memory types for Flight Shared Types Rust bindings

use crate::error::MetadataPair;
use serde::{Deserialize, Serialize};

/// Memory size representation
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct MemorySize {
    pub bytes: u64,
    pub human_readable: String,
}

impl Default for MemorySize {
    fn default() -> Self {
        Self {
            bytes: 0,
            human_readable: "0B".to_string(),
        }
    }
}

/// Memory usage snapshot
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct MemoryUsageSnapshot {
    pub timestamp: u64,
    pub session_id: String,
    pub platform: String,
    pub total: MemorySize,
    pub used: MemorySize,
    pub available: MemorySize,
    pub fragmentation_ratio: f32,
}

impl Default for MemoryUsageSnapshot {
    fn default() -> Self {
        Self {
            timestamp: 0,
            session_id: String::new(),
            platform: String::new(),
            total: MemorySize::default(),
            used: MemorySize::default(),
            available: MemorySize::default(),
            fragmentation_ratio: 0.0,
        }
    }
}

/// Memory allocation purposes
#[derive(Debug, Clone, Copy, Serialize, Deserialize)]
pub enum MemoryPurpose {
    VmHeap,
    ComponentStack,
    AssetCache,
    JitCodeCache,
    SystemReserved,
    WasmLinear,
    NetworkBuffers,
    Temporary,
}

/// Memory allocation record
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct MemoryAllocation {
    pub id: String,
    pub session_id: String,
    pub size: MemorySize,
    pub purpose: MemoryPurpose,
    pub allocated_at: u64,
    pub freed_at: Option<u64>,
}

/// Platform memory profile
#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum PlatformProfile {
    Dreamcast(MemorySize),
    Psp(MemorySize),
    Vita(MemorySize),
    V6rSmall(MemorySize),
    V6rMedium(MemorySize),
    V6rLarge(MemorySize),
    Custom(MemorySize),
}

/// Memory limits configuration
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct MemoryLimits {
    pub heap_max: MemorySize,
    pub stack_max: MemorySize,
    pub cache_max: MemorySize,
    pub soft_limit: MemorySize,
    pub hard_limit: MemorySize,
}
