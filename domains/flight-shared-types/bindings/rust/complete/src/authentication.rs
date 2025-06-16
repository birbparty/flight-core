//! Authentication types for Flight Shared Types Rust bindings

use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct AuthContext {
    pub authenticated: bool,
}
