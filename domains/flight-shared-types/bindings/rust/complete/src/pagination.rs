//! Pagination types for Flight Shared Types Rust bindings

use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ListRequest {
    pub page: u32,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ListResponse {
    pub items: Vec<String>,
}
