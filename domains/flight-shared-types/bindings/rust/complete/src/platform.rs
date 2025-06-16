//! Platform types for Flight Shared Types Rust bindings

use crate::{error::MetadataPair, memory::MemorySize};
use serde::{Deserialize, Serialize};

/// Platform constraints
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct PlatformConstraints {
    pub max_memory: MemorySize,
    pub max_cpu_threads: u32,
    pub max_open_files: u32,
    pub network_enabled: bool,
}

/// Platform information
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct PlatformInfo {
    pub id: String,
    pub name: String,
    pub architecture: String,
    pub memory_total: MemorySize,
    pub capabilities: Vec<String>,
    pub constraints: PlatformConstraints,
    pub version: String,
    pub vendor: String,
    pub features: Vec<String>,
}
