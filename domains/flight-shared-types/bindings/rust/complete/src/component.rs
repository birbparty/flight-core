//! Component types for Flight Shared Types Rust bindings

use crate::{error::MetadataPair, memory::MemoryUsageSnapshot};
use serde::{Deserialize, Serialize};

/// Component identifier
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ComponentId(pub String);

impl ComponentId {
    pub fn new() -> Self {
        Self(uuid::Uuid::new_v4().to_string())
    }
}

/// World name
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct WorldName(pub String);

/// Component state
#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum ComponentState {
    Loaded,
    Instantiating,
    Instantiated,
    Running,
    Suspended,
    Terminating,
    Terminated,
    Error,
}

/// Component information
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ComponentInfo {
    pub id: ComponentId,
    pub name: String,
    pub version: String,
    pub state: ComponentState,
    pub world: WorldName,
    pub platform: String,
    pub session_id: Option<String>,
    pub created_at: u64,
    pub last_activity: u64,
    pub memory_usage: MemoryUsageSnapshot,
    pub metadata: Vec<MetadataPair>,
}
