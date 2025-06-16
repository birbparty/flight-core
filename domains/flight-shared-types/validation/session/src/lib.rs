//! Flight Session Types Validation
//! 
//! Comprehensive validation for session lifecycle, resource limits, 
//! and configuration across Flight-Core's multi-platform environment

use std::collections::HashMap;
use std::time::{SystemTime, UNIX_EPOCH};
use serde::{Deserialize, Serialize};
use thiserror::Error;

wit_bindgen::generate!({
    world: "session-validation",
    path: "../../wit"
});

/// Session validation errors
#[derive(Error, Debug, Clone, Serialize, Deserialize)]
pub enum SessionValidationError {
    #[error("Invalid session ID: {0}")]
    InvalidSessionId(String),
    #[error("Session state transition not allowed: {from:?} -> {to:?}")]
    InvalidStateTransition { from: SessionState, to: SessionState },
    #[error("Resource limit exceeded: {resource} ({current} > {limit})")]
    ResourceLimitExceeded { resource: String, current: String, limit: String },
    #[error("Session expired at {expired_at}")]
    SessionExpired { expired_at: u64 },
    #[error("Invalid session configuration: {0}")]
    InvalidConfiguration(String),
    #[error("Session not found: {0}")]
    SessionNotFound(String),
    #[error("Platform not supported: {0}")]
    UnsupportedPlatform(String),
    #[error("Invalid metadata: {0}")]
    InvalidMetadata(String),
}

/// Session state machine validator
pub struct SessionStateMachine;

impl SessionStateMachine {
    /// Validates session state transitions
    pub fn validate_transition(from: SessionState, to: SessionState) -> Result<(), SessionValidationError> {
        use SessionState::*;
        
        let valid = match (from, to) {
            // From Initializing
            (Initializing, Active) => true,
            (Initializing, Error) => true,
            (Initializing, Terminated) => true,
            
            // From Active
            (Active, Suspended) => true,
            (Active, Terminating) => true,
            (Active, Error) => true,
            
            // From Suspended
            (Suspended, Active) => true,
            (Suspended, Terminating) => true,
            (Suspended, Error) => true,
            
            // From Terminating
            (Terminating, Terminated) => true,
            (Terminating, Error) => true,
            
            // From Error
            (Error, Active) => true,
            (Error, Terminating) => true,
            (Error, Terminated) => true,
            
            // Same state (no change)
            (s1, s2) if s1 == s2 => true,
            
            // All other transitions are invalid
            _ => false,
        };

        if valid {
            Ok(())
        } else {
            Err(SessionValidationError::InvalidStateTransition { from, to })
        }
    }
    
    /// Gets valid next states for current state
    pub fn get_valid_next_states(current: SessionState) -> Vec<SessionState> {
        use SessionState::*;
        
        match current {
            Initializing => vec![Active, Error, Terminated],
            Active => vec![Suspended, Terminating, Error],
            Suspended => vec![Active, Terminating, Error],
            Terminating => vec![Terminated, Error],
            Terminated => vec![], // Terminal state
            Error => vec![Active, Terminating, Terminated],
        }
    }
}

/// Session validator
pub struct SessionValidator;

impl SessionValidator {
    /// Validates session information
    pub fn validate_session_info(session: &SessionInfo) -> Result<(), SessionValidationError> {
        // Validate session ID
        Self::validate_session_id(&session.id)?;
        
        // Validate timestamps
        Self::validate_timestamps(session.created_at, session.last_activity, session.expires_at)?;
        
        // Validate metadata
        Self::validate_metadata(&session.metadata)?;
        
        // Validate platform
        Self::validate_platform(&session.platform)?;
        
        Ok(())
    }
    
    /// Validates session ID format
    pub fn validate_session_id(session_id: &str) -> Result<(), SessionValidationError> {
        if session_id.is_empty() {
            return Err(SessionValidationError::InvalidSessionId("Session ID cannot be empty".to_string()));
        }
        
        if session_id.len() > 255 {
            return Err(SessionValidationError::InvalidSessionId("Session ID too long (max 255 characters)".to_string()));
        }
        
        // Check for valid characters (alphanumeric, hyphens, underscores)
        if !session_id.chars().all(|c| c.is_alphanumeric() || c == '-' || c == '_') {
            return Err(SessionValidationError::InvalidSessionId("Session ID contains invalid characters".to_string()));
        }
        
        Ok(())
    }
    
    /// Validates session timestamps
    pub fn validate_timestamps(
        created_at: u64,
        last_activity: u64,
        expires_at: Option<u64>
    ) -> Result<(), SessionValidationError> {
        let now = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap()
            .as_secs();
        
        // Last activity cannot be before creation
        if last_activity < created_at {
            return Err(SessionValidationError::InvalidConfiguration(
                "Last activity cannot be before creation time".to_string()
            ));
        }
        
        // Check expiry
        if let Some(expires) = expires_at {
            if expires <= created_at {
                return Err(SessionValidationError::InvalidConfiguration(
                    "Expiry time must be after creation time".to_string()
                ));
            }
            
            if expires <= now {
                return Err(SessionValidationError::SessionExpired { expired_at: expires });
            }
        }
        
        Ok(())
    }
    
    /// Validates session metadata
    pub fn validate_metadata(metadata: &[(String, String)]) -> Result<(), SessionValidationError> {
        let mut keys = std::collections::HashSet::new();
        
        for (key, value) in metadata {
            // Check for duplicate keys
            if !keys.insert(key.clone()) {
                return Err(SessionValidationError::InvalidMetadata(
                    format!("Duplicate metadata key: {}", key)
                ));
            }
            
            // Check key length
            if key.len() > 100 {
                return Err(SessionValidationError::InvalidMetadata(
                    format!("Metadata key too long: {} (max 100 characters)", key)
                ));
            }
            
            // Check value length
            if value.len() > 1000 {
                return Err(SessionValidationError::InvalidMetadata(
                    format!("Metadata value too long for key '{}' (max 1000 characters)", key)
                ));
            }
        }
        
        // Check total metadata count
        if metadata.len() > 50 {
            return Err(SessionValidationError::InvalidMetadata(
                "Too many metadata entries (max 50)".to_string()
            ));
        }
        
        Ok(())
    }
    
    /// Validates platform identifier
    pub fn validate_platform(platform: &str) -> Result<(), SessionValidationError> {
        if platform.is_empty() {
            return Err(SessionValidationError::UnsupportedPlatform("Platform cannot be empty".to_string()));
        }
        
        // List of known platforms
        let known_platforms = [
            "dreamcast", "psp", "vita", "desktop", "mobile", 
            "cloud", "wasm", "custom"
        ];
        
        // Allow known platforms or custom prefixes
        if !known_platforms.iter().any(|&p| platform.starts_with(p)) && !platform.starts_with("custom-") {
            return Err(SessionValidationError::UnsupportedPlatform(
                format!("Unknown platform: {}", platform)
            ));
        }
        
        Ok(())
    }
}

/// Resource limits validator
pub struct ResourceLimitsValidator;

impl ResourceLimitsValidator {
    /// Validates resource limits configuration
    pub fn validate_resource_limits(limits: &ResourceLimits) -> Result<(), SessionValidationError> {
        // Validate memory limits
        if let Some(max_memory) = &limits.max_memory {
            if max_memory.bytes == 0 {
                return Err(SessionValidationError::InvalidConfiguration(
                    "Memory limit cannot be zero".to_string()
                ));
            }
        }
        
        // Validate CPU limits
        if let Some(max_cpu) = limits.max_cpu_percent {
            if max_cpu <= 0.0 || max_cpu > 100.0 {
                return Err(SessionValidationError::InvalidConfiguration(
                    "CPU limit must be between 0.1 and 100.0 percent".to_string()
                ));
            }
        }
        
        // Validate network limits
        if let Some(max_network) = limits.max_network_bps {
            if max_network == 0 {
                return Err(SessionValidationError::InvalidConfiguration(
                    "Network limit cannot be zero".to_string()
                ));
            }
        }
        
        // Validate storage limits
        if let Some(max_storage) = &limits.max_storage {
            if max_storage.bytes == 0 {
                return Err(SessionValidationError::InvalidConfiguration(
                    "Storage limit cannot be zero".to_string()
                ));
            }
        }
        
        // Validate timeout
        if let Some(timeout) = limits.timeout_seconds {
            if timeout == 0 {
                return Err(SessionValidationError::InvalidConfiguration(
                    "Timeout cannot be zero".to_string()
                ));
            }
            
            // Maximum timeout of 30 days
            if timeout > 30 * 24 * 60 * 60 {
                return Err(SessionValidationError::InvalidConfiguration(
                    "Timeout cannot exceed 30 days".to_string()
                ));
            }
        }
        
        Ok(())
    }
    
    /// Checks if resource usage exceeds limits
    pub fn check_resource_usage(
        resources: &SessionResources,
        limits: &ResourceLimits
    ) -> Result<(), SessionValidationError> {
        // Check memory usage
        if let Some(max_memory) = &limits.max_memory {
            if resources.memory.used.bytes > max_memory.bytes {
                return Err(SessionValidationError::ResourceLimitExceeded {
                    resource: "memory".to_string(),
                    current: resources.memory.used.human_readable.clone(),
                    limit: max_memory.human_readable.clone(),
                });
            }
        }
        
        // Check CPU usage
        if let Some(max_cpu) = limits.max_cpu_percent {
            if resources.cpu_usage > max_cpu {
                return Err(SessionValidationError::ResourceLimitExceeded {
                    resource: "cpu".to_string(),
                    current: format!("{:.1}%", resources.cpu_usage),
                    limit: format!("{:.1}%", max_cpu),
                });
            }
        }
        
        // Check network usage
        if let Some(max_network) = limits.max_network_bps {
            if resources.network_usage > max_network {
                return Err(SessionValidationError::ResourceLimitExceeded {
                    resource: "network".to_string(),
                    current: format!("{} bytes/sec", resources.network_usage),
                    limit: format!("{} bytes/sec", max_network),
                });
            }
        }
        
        // Check storage usage
        if let Some(max_storage) = &limits.max_storage {
            if resources.storage_usage.bytes > max_storage.bytes {
                return Err(SessionValidationError::ResourceLimitExceeded {
                    resource: "storage".to_string(),
                    current: resources.storage_usage.human_readable.clone(),
                    limit: max_storage.human_readable.clone(),
                });
            }
        }
        
        // Check connection count
        if let Some(max_connections) = limits.max_connections {
            if resources.connection_count > max_connections {
                return Err(SessionValidationError::ResourceLimitExceeded {
                    resource: "connections".to_string(),
                    current: resources.connection_count.to_string(),
                    limit: max_connections.to_string(),
                });
            }
        }
        
        Ok(())
    }
}

/// Session configuration validator
pub struct SessionConfigValidator;

impl SessionConfigValidator {
    /// Validates session configuration
    pub fn validate_session_config(config: &SessionConfig) -> Result<(), SessionValidationError> {
        // Validate resource limits if present
        if let Some(limits) = &config.resource_limits {
            ResourceLimitsValidator::validate_resource_limits(limits)?;
        }
        
        // Validate environment variables
        Self::validate_environment(&config.environment)?;
        
        // Validate working directory
        if let Some(workdir) = &config.working_directory {
            Self::validate_working_directory(workdir)?;
        }
        
        // Validate custom configuration
        Self::validate_custom_config(&config.custom_config)?;
        
        Ok(())
    }
    
    /// Validates environment variables
    fn validate_environment(env: &[(String, String)]) -> Result<(), SessionValidationError> {
        let mut keys = std::collections::HashSet::new();
        
        for (key, value) in env {
            // Check for duplicate keys
            if !keys.insert(key.clone()) {
                return Err(SessionValidationError::InvalidConfiguration(
                    format!("Duplicate environment variable: {}", key)
                ));
            }
            
            // Check key format (must be valid environment variable name)
            if !key.chars().all(|c| c.is_alphanumeric() || c == '_') || key.starts_with(char::is_numeric) {
                return Err(SessionValidationError::InvalidConfiguration(
                    format!("Invalid environment variable name: {}", key)
                ));
            }
            
            // Check value length
            if value.len() > 4096 {
                return Err(SessionValidationError::InvalidConfiguration(
                    format!("Environment variable value too long: {} (max 4096 characters)", key)
                ));
            }
        }
        
        // Check total environment variable count
        if env.len() > 100 {
            return Err(SessionValidationError::InvalidConfiguration(
                "Too many environment variables (max 100)".to_string()
            ));
        }
        
        Ok(())
    }
    
    /// Validates working directory path
    fn validate_working_directory(workdir: &str) -> Result<(), SessionValidationError> {
        if workdir.is_empty() {
            return Err(SessionValidationError::InvalidConfiguration(
                "Working directory cannot be empty".to_string()
            ));
        }
        
        if workdir.len() > 1000 {
            return Err(SessionValidationError::InvalidConfiguration(
                "Working directory path too long (max 1000 characters)".to_string()
            ));
        }
        
        // Basic path validation (no null bytes)
        if workdir.contains('\0') {
            return Err(SessionValidationError::InvalidConfiguration(
                "Working directory path contains null bytes".to_string()
            ));
        }
        
        Ok(())
    }
    
    /// Validates custom configuration
    fn validate_custom_config(config: &[(String, String)]) -> Result<(), SessionValidationError> {
        let mut keys = std::collections::HashSet::new();
        
        for (key, value) in config {
            // Check for duplicate keys
            if !keys.insert(key.clone()) {
                return Err(SessionValidationError::InvalidConfiguration(
                    format!("Duplicate custom config key: {}", key)
                ));
            }
            
            // Check key length
            if key.len() > 100 {
                return Err(SessionValidationError::InvalidConfiguration(
                    format!("Custom config key too long: {} (max 100 characters)", key)
                ));
            }
            
            // Check value length
            if value.len() > 2000 {
                return Err(SessionValidationError::InvalidConfiguration(
                    format!("Custom config value too long for key '{}' (max 2000 characters)", key)
                ));
            }
        }
        
        // Check total custom config count
        if config.len() > 50 {
            return Err(SessionValidationError::InvalidConfiguration(
                "Too many custom config entries (max 50)".to_string()
            ));
        }
        
        Ok(())
    }
}

/// Session health calculator
pub struct SessionHealthCalculator;

impl SessionHealthCalculator {
    /// Calculates session health based on resource usage and limits
    pub fn calculate_health(
        resources: &SessionResources,
        limits: &Option<ResourceLimits>
    ) -> SessionHealth {
        let mut health_score = 100.0;
        let mut warning_count = 0;
        let mut critical_count = 0;
        
        // Check memory usage
        if let Some(limits) = limits {
            if let Some(max_memory) = &limits.max_memory {
                let memory_usage_percent = (resources.memory.used.bytes as f64 / max_memory.bytes as f64) * 100.0;
                
                if memory_usage_percent > 90.0 {
                    critical_count += 1;
                    health_score -= 30.0;
                } else if memory_usage_percent > 75.0 {
                    warning_count += 1;
                    health_score -= 15.0;
                }
            }
            
            // Check CPU usage
            if let Some(max_cpu) = limits.max_cpu_percent {
                let cpu_usage_percent = resources.cpu_usage;
                
                if cpu_usage_percent > 90.0 {
                    critical_count += 1;
                    health_score -= 25.0;
                } else if cpu_usage_percent > 75.0 {
                    warning_count += 1;
                    health_score -= 10.0;
                }
            }
            
            // Check network usage
            if let Some(max_network) = limits.max_network_bps {
                let network_usage_percent = (resources.network_usage as f64 / max_network as f64) * 100.0;
                
                if network_usage_percent > 90.0 {
                    critical_count += 1;
                    health_score -= 20.0;
                } else if network_usage_percent > 75.0 {
                    warning_count += 1;
                    health_score -= 10.0;
                }
            }
        }
        
        // Check memory fragmentation
        if resources.memory.fragmentation_ratio > 0.8 {
            warning_count += 1;
            health_score -= 15.0;
        }
        
        // Determine final health status
        if critical_count > 0 || health_score <= 30.0 {
            SessionHealth::Critical
        } else if warning_count > 1 || health_score <= 60.0 {
            SessionHealth::Degraded
        } else if warning_count > 0 || health_score <= 85.0 {
            SessionHealth::Warning
        } else {
            SessionHealth::Healthy
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_session_state_transitions() {
        assert!(SessionStateMachine::validate_transition(
            SessionState::Initializing, 
            SessionState::Active
        ).is_ok());
        
        assert!(SessionStateMachine::validate_transition(
            SessionState::Active, 
            SessionState::Initializing
        ).is_err());
    }

    #[test]
    fn test_session_id_validation() {
        assert!(SessionValidator::validate_session_id("valid-session_123").is_ok());
        assert!(SessionValidator::validate_session_id("").is_err());
        assert!(SessionValidator::validate_session_id("invalid@session").is_err());
    }

    #[test]
    fn test_metadata_validation() {
        let valid_metadata = vec![
            ("key1".to_string(), "value1".to_string()),
            ("key2".to_string(), "value2".to_string()),
        ];
        assert!(SessionValidator::validate_metadata(&valid_metadata).is_ok());
        
        let duplicate_keys = vec![
            ("key1".to_string(), "value1".to_string()),
            ("key1".to_string(), "value2".to_string()),
        ];
        assert!(SessionValidator::validate_metadata(&duplicate_keys).is_err());
    }
}
