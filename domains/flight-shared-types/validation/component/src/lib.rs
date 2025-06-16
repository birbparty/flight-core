//! Component Model Infrastructure Validation
//!
//! This module provides comprehensive validation for Flight Component Model infrastructure,
//! ensuring correctness across Flight-Core and V6R platforms. It validates component lifecycle
//! management, resource allocation, inter-component communication, and platform-specific features.

use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::time::{SystemTime, UNIX_EPOCH};

// Re-export foundation types
pub use flight_memory_validation::{MemorySize, MemoryUsageSnapshot};
pub use flight_error_validation::{FlightError, FlightResult};
pub use flight_platform_validation::PlatformInfo;
pub use flight_session_validation::SessionInfo;

/// Component identification types
pub type ComponentId = String;
pub type InstanceId = String;
pub type WorldName = String;
pub type InterfaceName = String;

/// Component lifecycle states
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub enum ComponentState {
    /// Component binary loaded but not instantiated
    Loaded,
    /// Component instantiating (calling constructors)
    Instantiating,
    /// Component fully instantiated and ready
    Instantiated,
    /// Component actively executing
    Running,
    /// Component execution suspended
    Suspended,
    /// Component shutting down (calling destructors)
    Terminating,
    /// Component terminated and resources released
    Terminated,
    /// Component in error state
    Error,
}

/// Component metadata and information
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ComponentInfo {
    /// Unique component identifier
    pub id: ComponentId,
    /// Component name
    pub name: String,
    /// Component version
    pub version: String,
    /// Current lifecycle state
    pub state: ComponentState,
    /// World interface this component implements
    pub world: WorldName,
    /// Platform where component runs
    pub platform: String,
    /// Session this component belongs to
    pub session_id: Option<String>,
    /// Component creation time (Unix timestamp)
    pub created_at: u64,
    /// Last activity timestamp (Unix timestamp)
    pub last_activity: u64,
    /// Memory usage by this component
    pub memory_usage: MemoryUsageSnapshot,
    /// Component-specific metadata
    pub metadata: Vec<(String, String)>,
}

/// Execution priority levels
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub enum ExecutionPriority {
    /// Low priority background tasks
    Low,
    /// Normal priority execution
    Normal,
    /// High priority interactive tasks
    High,
    /// Critical priority system tasks
    Critical,
}

/// Platform execution modes
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub enum ExecutionMode {
    /// Single-threaded execution (Dreamcast)
    SingleThreaded,
    /// Multi-threaded execution
    MultiThreaded,
    /// Async/await execution
    AsyncExecution,
    /// Real-time execution
    RealTime,
}

/// Component execution context
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ExecutionContext {
    /// Component being executed
    pub component: ComponentId,
    /// Current execution stack depth
    pub stack_depth: u32,
    /// Available memory
    pub available_memory: MemorySize,
    /// CPU time allocated (milliseconds)
    pub cpu_time_ms: u32,
    /// Execution priority
    pub priority: ExecutionPriority,
    /// Platform execution mode
    pub execution_mode: ExecutionMode,
}

/// Flight-Core component patterns validation
pub struct FlightCoreComponentPatterns;

impl FlightCoreComponentPatterns {
    /// Create HAL component configuration for a specific platform
    pub fn create_hal_component_config(platform: &str) -> ComponentInfo {
        let (memory_total, memory_used) = match platform {
            "dreamcast" => (16 * 1024 * 1024, 2 * 1024 * 1024), // 16MB total, 2MB used
            "psp" => (32 * 1024 * 1024, 4 * 1024 * 1024),       // 32MB total, 4MB used
            "vita" => (512 * 1024 * 1024, 64 * 1024 * 1024),    // 512MB total, 64MB used
            _ => (1024 * 1024 * 1024, 128 * 1024 * 1024),       // 1GB total, 128MB used (default)
        };

        let memory_usage = MemoryUsageSnapshot {
            timestamp: SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_secs(),
            session_id: format!("{}-hal-session", platform),
            platform: platform.to_string(),
            total: MemorySize {
                bytes: memory_total,
                human_readable: Self::format_memory_size(memory_total),
            },
            used: MemorySize {
                bytes: memory_used,
                human_readable: Self::format_memory_size(memory_used),
            },
            available: MemorySize {
                bytes: memory_total - memory_used,
                human_readable: Self::format_memory_size(memory_total - memory_used),
            },
            fragmentation_ratio: 0.05,
        };

        ComponentInfo {
            id: format!("flight-hal-{}", platform),
            name: format!("Flight HAL - {}", platform.to_uppercase()),
            version: "1.0.0".to_string(),
            state: ComponentState::Instantiated,
            world: "flight:hal-world".to_string(),
            platform: platform.to_string(),
            session_id: Some(format!("{}-hal-session", platform)),
            created_at: SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_secs(),
            last_activity: SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_secs(),
            memory_usage,
            metadata: vec![
                ("component_type".to_string(), "hal".to_string()),
                ("platform_specific".to_string(), "true".to_string()),
                ("language".to_string(), "c++17".to_string()),
                ("created_by".to_string(), "flight".to_string()),
            ],
        }
    }

    /// Create runtime component configuration for a specific platform
    pub fn create_runtime_component_config(platform: &str) -> ComponentInfo {
        let execution_mode = match platform {
            "dreamcast" | "psp" => "single-threaded",
            "vita" => "limited-threading",
            _ => "multi-threaded",
        };

        let wasm_engine = match platform {
            "dreamcast" | "psp" | "vita" => "uwrc",
            _ => "wasmtime",
        };

        let (memory_total, memory_used) = match platform {
            "dreamcast" => (16 * 1024 * 1024, 8 * 1024 * 1024),   // 16MB total, 8MB for runtime
            "psp" => (32 * 1024 * 1024, 16 * 1024 * 1024),        // 32MB total, 16MB for runtime
            "vita" => (512 * 1024 * 1024, 256 * 1024 * 1024),     // 512MB total, 256MB for runtime
            _ => (2048 * 1024 * 1024, 512 * 1024 * 1024),          // 2GB total, 512MB for runtime
        };

        let memory_usage = MemoryUsageSnapshot {
            timestamp: SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_secs(),
            session_id: format!("{}-runtime-session", platform),
            platform: platform.to_string(),
            total: MemorySize {
                bytes: memory_total,
                human_readable: Self::format_memory_size(memory_total),
            },
            used: MemorySize {
                bytes: memory_used,
                human_readable: Self::format_memory_size(memory_used),
            },
            available: MemorySize {
                bytes: memory_total - memory_used,
                human_readable: Self::format_memory_size(memory_total - memory_used),
            },
            fragmentation_ratio: 0.10,
        };

        ComponentInfo {
            id: format!("flight-runtime-{}", platform),
            name: format!("Flight Runtime - {}", platform.to_uppercase()),
            version: "1.0.0".to_string(),
            state: ComponentState::Running,
            world: "flight:runtime-world".to_string(),
            platform: platform.to_string(),
            session_id: Some(format!("{}-runtime-session", platform)),
            created_at: SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_secs(),
            last_activity: SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_secs(),
            memory_usage,
            metadata: vec![
                ("component_type".to_string(), "runtime".to_string()),
                ("execution_mode".to_string(), execution_mode.to_string()),
                ("wasm_engine".to_string(), wasm_engine.to_string()),
                ("cpp_standard".to_string(), "c++17".to_string()),
                ("created_by".to_string(), "flight".to_string()),
            ],
        }
    }

    /// Create C++17 binding configuration for a platform
    pub fn create_cpp17_binding_config(platform: &str) -> Cpp17BindingConfig {
        match platform {
            "dreamcast" => Cpp17BindingConfig {
                target_triple: "sh4-unknown-elf".to_string(),
                toolchain: "kos-gcc".to_string(),
                cpp_standard: "c++17".to_string(),
                compile_flags: vec![
                    "-std=c++17".to_string(),
                    "-Os".to_string(), // Size optimization
                    "-fno-exceptions".to_string(),
                    "-fno-rtti".to_string(),
                    "-ml".to_string(), // Little endian
                    "-m4-single-only".to_string(),
                ],
                link_flags: vec![
                    "-T$(KOS_BASE)/utils/ldscripts/shlelf.xc".to_string(),
                    "-nostartfiles".to_string(),
                    "-nostdlib".to_string(),
                ],
                required_libs: vec![
                    "kos".to_string(),
                    "kosutils".to_string(),
                    "png".to_string(),
                    "jpeg".to_string(),
                    "m".to_string(),
                ],
                include_paths: vec![
                    "$(KOS_BASE)/include".to_string(),
                    "$(KOS_BASE)/kernel/arch/dreamcast/include".to_string(),
                    "$(KOS_BASE)/addons/include".to_string(),
                ],
            },
            "psp" => Cpp17BindingConfig {
                target_triple: "mips-allegrex-elf".to_string(),
                toolchain: "psp-gcc".to_string(),
                cpp_standard: "c++17".to_string(),
                compile_flags: vec![
                    "-std=c++17".to_string(),
                    "-O2".to_string(),
                    "-G0".to_string(),
                    "-Wall".to_string(),
                    "-fno-exceptions".to_string(),
                    "-fno-rtti".to_string(),
                ],
                link_flags: vec![
                    "-specs=$(PSPSDK)/lib/prxspecs".to_string(),
                    "-Wl,-q,-T$(PSPSDK)/lib/linkfile.prx".to_string(),
                ],
                required_libs: vec![
                    "pspgu".to_string(),
                    "pspgum".to_string(),
                    "pspaudio".to_string(),
                    "pspdebug".to_string(),
                    "pspdisplay".to_string(),
                    "pspge".to_string(),
                    "pspctrl".to_string(),
                    "pspsdk".to_string(),
                    "c".to_string(),
                    "pspnet".to_string(),
                    "pspnet_inet".to_string(),
                    "pspnet_apctl".to_string(),
                    "pspnet_resolver".to_string(),
                    "psputility".to_string(),
                    "pspuser".to_string(),
                    "pspkernel".to_string(),
                ],
                include_paths: vec![
                    "$(PSPSDK)/include".to_string(),
                    "$(PSPDEV)/include".to_string(),
                ],
            },
            "vita" => Cpp17BindingConfig {
                target_triple: "arm-vita-eabi".to_string(),
                toolchain: "vita-gcc".to_string(),
                cpp_standard: "c++17".to_string(),
                compile_flags: vec![
                    "-std=c++17".to_string(),
                    "-O2".to_string(),
                    "-fno-exceptions".to_string(),
                    "-fno-rtti".to_string(),
                    "-Wl,-q".to_string(),
                ],
                link_flags: vec![
                    "-Wl,-q".to_string(),
                    "--specs=psp2.specs".to_string(),
                ],
                required_libs: vec![
                    "vitaGL".to_string(),
                    "vitashark".to_string(),
                    "SceLibKernel_stub".to_string(),
                    "SceAppMgr_stub".to_string(),
                    "SceCtrl_stub".to_string(),
                    "SceDisplay_stub".to_string(),
                    "SceGxm_stub".to_string(),
                    "SceSysmodule_stub".to_string(),
                    "SceAudio_stub".to_string(),
                    "m".to_string(),
                ],
                include_paths: vec![
                    "$(VITASDK)/arm-vita-eabi/include".to_string(),
                    "$(VITASDK)/include".to_string(),
                ],
            },
            _ => Cpp17BindingConfig {
                target_triple: "x86_64-unknown-linux-gnu".to_string(),
                toolchain: "gcc".to_string(),
                cpp_standard: "c++17".to_string(),
                compile_flags: vec![
                    "-std=c++17".to_string(),
                    "-O2".to_string(),
                    "-Wall".to_string(),
                    "-Wextra".to_string(),
                ],
                link_flags: vec![
                    "-pthread".to_string(),
                ],
                required_libs: vec![
                    "pthread".to_string(),
                    "m".to_string(),
                ],
                include_paths: vec![
                    "/usr/include".to_string(),
                    "/usr/local/include".to_string(),
                ],
            },
        }
    }

    /// Create execution context for a component on a specific platform
    pub fn create_execution_context(component_id: &str, platform: &str) -> ExecutionContext {
        let (available_memory, cpu_time_ms, stack_depth) = match platform {
            "dreamcast" => (8 * 1024 * 1024, 16, 16),      // 8MB, 16ms, 16 stack depth
            "psp" => (16 * 1024 * 1024, 33, 32),           // 16MB, 33ms, 32 stack depth
            "vita" => (256 * 1024 * 1024, 16, 64),         // 256MB, 16ms, 64 stack depth
            _ => (512 * 1024 * 1024, 10, 128),             // 512MB, 10ms, 128 stack depth
        };

        let execution_mode = match platform {
            "dreamcast" | "psp" => ExecutionMode::SingleThreaded,
            "vita" => ExecutionMode::AsyncExecution,
            _ => ExecutionMode::MultiThreaded,
        };

        ExecutionContext {
            component: component_id.to_string(),
            stack_depth,
            available_memory: MemorySize {
                bytes: available_memory,
                human_readable: Self::format_memory_size(available_memory),
            },
            cpu_time_ms,
            priority: ExecutionPriority::Normal,
            execution_mode,
        }
    }

    /// Format memory size in human readable format
    fn format_memory_size(bytes: u64) -> String {
        if bytes >= 1024 * 1024 * 1024 {
            format!("{}GB", bytes / (1024 * 1024 * 1024))
        } else if bytes >= 1024 * 1024 {
            format!("{}MB", bytes / (1024 * 1024))
        } else if bytes >= 1024 {
            format!("{}KB", bytes / 1024)
        } else {
            format!("{}B", bytes)
        }
    }
}

/// C++17 binding configuration for platform-specific compilation
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Cpp17BindingConfig {
    pub target_triple: String,
    pub toolchain: String,
    pub cpp_standard: String,
    pub compile_flags: Vec<String>,
    pub link_flags: Vec<String>,
    pub required_libs: Vec<String>,
    pub include_paths: Vec<String>,
}

/// Component utilities for validation and testing
pub struct ComponentUtils;

impl ComponentUtils {
    /// Check if component is in active state
    pub fn is_active(state: &ComponentState) -> bool {
        matches!(state, ComponentState::Running | ComponentState::Instantiated)
    }

    /// Check if component can be started
    pub fn can_start(state: &ComponentState) -> bool {
        matches!(state, ComponentState::Instantiated | ComponentState::Suspended)
    }

    /// Check if component can be suspended
    pub fn can_suspend(state: &ComponentState) -> bool {
        matches!(state, ComponentState::Running)
    }

    /// Check if component can be terminated
    pub fn can_terminate(state: &ComponentState) -> bool {
        !matches!(state, ComponentState::Terminated | ComponentState::Terminating)
    }

    /// Get memory usage percentage
    pub fn get_memory_usage_percentage(info: &ComponentInfo) -> f64 {
        let total = info.memory_usage.total.bytes as f64;
        let used = info.memory_usage.used.bytes as f64;
        if total > 0.0 {
            (used / total) * 100.0
        } else {
            0.0
        }
    }

    /// Get component type from metadata
    pub fn get_component_type(info: &ComponentInfo) -> String {
        for (key, value) in &info.metadata {
            if key == "component_type" {
                return value.clone();
            }
        }
        "unknown".to_string()
    }

    /// Check if component is platform-specific
    pub fn is_platform_specific(info: &ComponentInfo) -> bool {
        for (key, value) in &info.metadata {
            if key == "platform_specific" {
                return value == "true";
            }
        }
        false
    }
}

/// Platform compatibility matrix for component migration
pub mod platform_matrix {
    use super::*;

    #[derive(Debug, Clone, Serialize, Deserialize)]
    pub struct CompatibilityResult {
        pub compatible: bool,
        pub issues: Vec<CompatibilityIssue>,
        pub migration_path: Option<String>,
    }

    #[derive(Debug, Clone, Serialize, Deserialize)]
    pub struct CompatibilityIssue {
        pub severity: String,
        pub description: String,
        pub resolution: Option<String>,
    }

    /// Check compatibility between two platforms for component migration
    pub fn check_compatibility(from_platform: &str, to_platform: &str) -> FlightResult<CompatibilityResult> {
        let mut issues = Vec::new();
        let mut compatible = true;

        // Memory compatibility check
        let from_memory = get_platform_memory_limit(from_platform);
        let to_memory = get_platform_memory_limit(to_platform);

        if from_memory > to_memory {
            compatible = false;
            issues.push(CompatibilityIssue {
                severity: "error".to_string(),
                description: format!(
                    "Target platform has insufficient memory: {} -> {}",
                    format_memory(from_memory),
                    format_memory(to_memory)
                ),
                resolution: Some("Optimize memory usage or choose larger target platform".to_string()),
            });
        }

        // Threading model compatibility
        let from_threading = get_platform_threading_model(from_platform);
        let to_threading = get_platform_threading_model(to_platform);

        if from_threading == "multi-threaded" && to_threading == "single-threaded" {
            issues.push(CompatibilityIssue {
                severity: "warning".to_string(),
                description: "Migration from multi-threaded to single-threaded platform".to_string(),
                resolution: Some("Refactor code to remove threading dependencies".to_string()),
            });
        }

        // Graphics API compatibility
        let from_graphics = get_platform_graphics_api(from_platform);
        let to_graphics = get_platform_graphics_api(to_platform);

        if from_graphics != to_graphics {
            issues.push(CompatibilityIssue {
                severity: "warning".to_string(),
                description: format!(
                    "Different graphics APIs: {} -> {}",
                    from_graphics, to_graphics
                ),
                resolution: Some("Implement graphics abstraction layer".to_string()),
            });
        }

        let migration_path = if compatible {
            Some(format!("{} -> {}", from_platform, to_platform))
        } else {
            None
        };

        FlightResult::Ok(CompatibilityResult {
            compatible,
            issues,
            migration_path,
        })
    }

    fn get_platform_memory_limit(platform: &str) -> u64 {
        match platform {
            "dreamcast" => 16 * 1024 * 1024,        // 16MB
            "psp" => 32 * 1024 * 1024,              // 32MB
            "vita" => 512 * 1024 * 1024,            // 512MB
            "v6r-small" => 512 * 1024 * 1024,       // 512MB
            "v6r-medium" => 1024 * 1024 * 1024,     // 1GB
            "v6r-large" => 2048 * 1024 * 1024,      // 2GB
            _ => 1024 * 1024 * 1024,                 // 1GB default
        }
    }

    fn get_platform_threading_model(platform: &str) -> &'static str {
        match platform {
            "dreamcast" | "psp" => "single-threaded",
            "vita" => "limited-threading",
            _ => "multi-threaded",
        }
    }

    fn get_platform_graphics_api(platform: &str) -> &'static str {
        match platform {
            "dreamcast" => "PowerVR2",
            "psp" => "GU",
            "vita" => "OpenGL ES",
            _ => "Multiple",
        }
    }

    fn format_memory(bytes: u64) -> String {
        if bytes >= 1024 * 1024 * 1024 {
            format!("{}GB", bytes / (1024 * 1024 * 1024))
        } else if bytes >= 1024 * 1024 {
            format!("{}MB", bytes / (1024 * 1024))
        } else {
            format!("{}KB", bytes / 1024)
        }
    }
}

/// Component validation tests
pub mod validation {
    use super::*;

    /// Validate component lifecycle transitions
    pub fn validate_component_lifecycle() -> FlightResult<()> {
        // Test Dreamcast HAL component
        let dreamcast_hal = FlightCoreComponentPatterns::create_hal_component_config("dreamcast");
        assert_eq!(dreamcast_hal.platform, "dreamcast");
        assert_eq!(dreamcast_hal.memory_usage.total.bytes, 16 * 1024 * 1024);

        // Test PSP runtime component
        let psp_runtime = FlightCoreComponentPatterns::create_runtime_component_config("psp");
        let metadata_map: HashMap<String, String> = psp_runtime.metadata.into_iter().collect();
        assert_eq!(metadata_map.get("execution_mode"), Some(&"single-threaded".to_string()));
        assert_eq!(metadata_map.get("wasm_engine"), Some(&"uwrc".to_string()));

        // Test component state utilities
        assert!(ComponentUtils::is_active(&ComponentState::Running));
        assert!(ComponentUtils::can_start(&ComponentState::Instantiated));
        assert!(ComponentUtils::can_suspend(&ComponentState::Running));
        assert!(ComponentUtils::can_terminate(&ComponentState::Running));

        FlightResult::Ok(())
    }

    /// Validate platform-specific configurations
    pub fn validate_platform_configurations() -> FlightResult<()> {
        let platforms = ["dreamcast", "psp", "vita"];
        
        for platform in &platforms {
            let hal_config = FlightCoreComponentPatterns::create_hal_component_config(platform);
            let runtime_config = FlightCoreComponentPatterns::create_runtime_component_config(platform);
            let cpp_config = FlightCoreComponentPatterns::create_cpp17_binding_config(platform);
            let exec_context = FlightCoreComponentPatterns::create_execution_context("test", platform);

            // Verify platform consistency
            assert_eq!(hal_config.platform, *platform);
            assert_eq!(runtime_config.platform, *platform);
            
            // Verify memory constraints are respected
            assert!(hal_config.memory_usage.total.bytes > 0);
            assert!(runtime_config.memory_usage.total.bytes > 0);
            assert!(exec_context.available_memory.bytes > 0);

            // Verify platform-specific settings
            match *platform {
                "dreamcast" => {
                    assert_eq!(cpp_config.target_triple, "sh4-unknown-elf");
                    assert_eq!(exec_context.execution_mode, ExecutionMode::SingleThreaded);
                    assert!(hal_config.memory_usage.total.bytes <= 16 * 1024 * 1024);
                },
                "psp" => {
                    assert_eq!(cpp_config.target_triple, "mips-allegrex-elf");
                    assert_eq!(exec_context.execution_mode, ExecutionMode::SingleThreaded);
                    assert!(hal_config.memory_usage.total.bytes <= 32 * 1024 * 1024);
                },
                "vita" => {
                    assert_eq!(cpp_config.target_triple, "arm-vita-eabi");
                    assert_eq!(exec_context.execution_mode, ExecutionMode::AsyncExecution);
                    assert!(hal_config.memory_usage.total.bytes <= 512 * 1024 * 1024);
                },
                _ => {}
            }
        }

        FlightResult::Ok(())
    }

    /// Validate platform compatibility matrix
    pub fn validate_platform_compatibility() -> FlightResult<()> {
        // Test compatible migration (Dreamcast -> PSP)
        let compat_result = platform_matrix::check_compatibility("dreamcast", "psp")?;
        assert!(compat_result.compatible);

        // Test incompatible migration (V6R Large -> Dreamcast)
        let incompat_result = platform_matrix::check_compatibility("v6r-large", "dreamcast")?;
        assert!(!incompat_result.compatible);
        assert!(!incompat_result.issues.is_empty());

        // Test same platform (should be compatible)
        let same_result = platform_matrix::check_compatibility("vita", "vita")?;
        assert!(same_result.compatible);

        FlightResult::Ok(())
    }

    /// Run all component validation tests
    pub fn run_all_validations() -> FlightResult<()> {
        validate_component_lifecycle()?;
        validate_platform_configurations()?;
        validate_platform_compatibility()?;
        
        println!("✅ All component model validations passed!");
        FlightResult::Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_component_lifecycle() {
        validation::validate_component_lifecycle().unwrap();
    }

    #[test]
    fn test_platform_configurations() {
        validation::validate_platform_configurations().unwrap();
    }

    #[test]
    fn test_platform_compatibility() {
        validation::validate_platform_compatibility().unwrap();
    }

    #[test]
    fn test_memory_usage_percentage() {
        let component = ComponentInfo {
            id: "test".to_string(),
            name: "test".to_string(),
            version: "1.0.0".to_string(),
            state: ComponentState::Running,
            world: "test-world".to_string(),
            platform: "test-platform".to_string(),
            session_id: None,
            created_at: 0,
            last_activity: 0,
            memory_usage: MemoryUsageSnapshot {
                timestamp: 0,
                session_id: "test".to_string(),
                platform: "test".to_string(),
                total: MemorySize { bytes: 1000, human_readable: "1000B".to_string() },
                used: MemorySize { bytes: 250, human_readable: "250B".to_string() },
                available: MemorySize { bytes: 750, human_readable: "750B".to_string() },
                fragmentation_ratio: 0.1,
            },
            metadata: vec![],
        };

        let percentage = ComponentUtils::get_memory_usage_percentage(&component);
        assert_eq!(percentage, 25.0);
    }

    #[test]
    fn test_component_state_checks() {
        assert!(ComponentUtils::is_active(&ComponentState::Running));
        assert!(ComponentUtils::is_active(&ComponentState::Instantiated));
        assert!(!ComponentUtils::is_active(&ComponentState::Terminated));

        assert!(ComponentUtils::can_start(&ComponentState::Instantiated));
        assert!(ComponentUtils::can_start(&ComponentState::Suspended));
        assert!(!ComponentUtils::can_start(&ComponentState::Running));

        assert!(ComponentUtils::can_suspend(&ComponentState::Running));
        assert!(!ComponentUtils::can_suspend(&ComponentState::Instantiated));

        assert!(ComponentUtils::can_terminate(&ComponentState::Running));
        assert!(!ComponentUtils::can_terminate(&ComponentState::Terminated));
    }
}
