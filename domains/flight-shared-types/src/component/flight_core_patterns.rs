//! Flight-Core Component Model Integration Patterns
//!
//! Provides specialized component configurations and patterns for Flight-Core
//! hardware abstraction layer (HAL) and runtime components across multiple platforms.

use serde::{Deserialize, Serialize};
use std::collections::HashMap;

/// Flight-Core component patterns and configurations
pub struct FlightCoreComponentPatterns;

impl FlightCoreComponentPatterns {
    /// Create Flight-Core HAL component configuration
    /// 
    /// Generates platform-specific HAL component configuration with appropriate
    /// memory constraints and capabilities for retro gaming platforms.
    pub fn create_hal_component_config(platform: &str) -> ComponentInfo {
        let memory_usage = Self::get_hal_memory_usage(platform);
        let metadata = Self::get_hal_metadata(platform);

        ComponentInfo {
            id: format!("flight-hal-{}", platform),
            name: format!("Flight-Core HAL - {}", platform.to_uppercase()),
            version: "1.0.0".to_string(),
            state: ComponentState::Instantiated,
            world: "flight:hal-world".to_string(),
            platform: platform.to_string(),
            session_id: Some("flight-system".to_string()),
            created_at: chrono::Utc::now().timestamp() as u64,
            last_activity: chrono::Utc::now().timestamp() as u64,
            memory_usage,
            metadata,
        }
    }

    /// Create Flight-Core runtime component configuration
    /// 
    /// Generates runtime component configuration optimized for UWRC WebAssembly
    /// execution with platform-appropriate threading and memory models.
    pub fn create_runtime_component_config(platform: &str) -> ComponentInfo {
        let execution_mode = match platform {
            "dreamcast" | "psp" => "single-threaded",
            _ => "multi-threaded",
        };

        let memory_usage = Self::get_runtime_memory_usage(platform);
        let metadata = vec![
            ("component_type".to_string(), "runtime".to_string()),
            ("execution_mode".to_string(), execution_mode.to_string()),
            ("wasm_engine".to_string(), "uwrc".to_string()),
            ("cpp_standard".to_string(), "C++17".to_string()),
            ("threading_model".to_string(), Self::get_threading_model(platform)),
        ];

        ComponentInfo {
            id: format!("flight-runtime-{}", platform),
            name: format!("Flight-Core UWRC Runtime - {}", platform.to_uppercase()),
            version: "1.0.0".to_string(),
            state: ComponentState::Running,
            world: "flight:runtime-world".to_string(),
            platform: platform.to_string(),
            session_id: Some("flight-system".to_string()),
            created_at: chrono::Utc::now().timestamp() as u64,
            last_activity: chrono::Utc::now().timestamp() as u64,
            memory_usage,
            metadata,
        }
    }

    /// Create execution context for Flight-Core components
    /// 
    /// Provides platform-appropriate execution context with memory and CPU
    /// constraints suitable for retro gaming platforms.
    pub fn create_execution_context(
        component_id: &str,
        platform: &str,
    ) -> ExecutionContext {
        let (available_memory, cpu_time_ms, stack_depth) = match platform {
            "dreamcast" => (
                MemorySize {
                    bytes: 4 * 1024 * 1024, // 4MB available for components
                    human_readable: "4MB".to_string(),
                },
                500, // 500ms CPU time slice
                32,  // Conservative stack depth
            ),
            "psp" => (
                MemorySize {
                    bytes: 8 * 1024 * 1024, // 8MB available for components
                    human_readable: "8MB".to_string(),
                },
                1000, // 1s CPU time slice
                64,   // Moderate stack depth
            ),
            "vita" => (
                MemorySize {
                    bytes: 32 * 1024 * 1024, // 32MB available for components
                    human_readable: "32MB".to_string(),
                },
                2000, // 2s CPU time slice
                128,  // Higher stack depth
            ),
            _ => (
                MemorySize {
                    bytes: 64 * 1024 * 1024, // 64MB for modern platforms
                    human_readable: "64MB".to_string(),
                },
                4000, // 4s CPU time slice
                256,  // Full stack depth
            ),
        };

        let execution_mode = match platform {
            "dreamcast" | "psp" => ExecutionMode::SingleThreaded,
            _ => ExecutionMode::MultiThreaded,
        };

        ExecutionContext {
            component: component_id.to_string(),
            stack_depth,
            available_memory,
            cpu_time_ms,
            priority: ExecutionPriority::Normal,
            execution_mode,
        }
    }

    /// Create component dependencies for Flight-Core modules
    /// 
    /// Establishes dependency relationships between HAL, runtime, and system components.
    pub fn create_flight_dependencies() -> Vec<ComponentDependency> {
        vec![
            ComponentDependency {
                dependent: "flight-runtime".to_string(),
                dependency: "flight-hal".to_string(),
                dependency_type: DependencyType::ComponentDependency,
                optional: false,
                resolved: true,
            },
            ComponentDependency {
                dependent: "flight-runtime".to_string(),
                dependency: "flight-memory".to_string(),
                dependency_type: DependencyType::ComponentDependency,
                optional: false,
                resolved: true,
            },
            ComponentDependency {
                dependent: "flight-hal".to_string(),
                dependency: "platform-capabilities".to_string(),
                dependency_type: DependencyType::PlatformCapability,
                optional: false,
                resolved: true,
            },
            ComponentDependency {
                dependent: "flight-runtime".to_string(),
                dependency: "wasm-linear-memory".to_string(),
                dependency_type: DependencyType::MemoryRequirement,
                optional: false,
                resolved: true,
            },
        ]
    }

    /// Create C++17 binding configuration for Flight-Core components
    /// 
    /// Generates binding configuration for C++17 integration with Flight-Core components.
    pub fn create_cpp17_binding_config(platform: &str) -> CppBindingConfig {
        let compile_flags = match platform {
            "dreamcast" => vec![
                "-std=c++17".to_string(),
                "-Os".to_string(), // Optimize for size
                "-fno-exceptions".to_string(),
                "-fno-rtti".to_string(),
                "-mcpu=sh4".to_string(),
            ],
            "psp" => vec![
                "-std=c++17".to_string(),
                "-O2".to_string(),
                "-fno-exceptions".to_string(),
                "-G0".to_string(),
                "-mgp32".to_string(),
                "-mlong32".to_string(),
            ],
            _ => vec![
                "-std=c++17".to_string(),
                "-O3".to_string(),
                "-march=native".to_string(),
            ],
        };

        let required_libs = match platform {
            "dreamcast" => vec![
                "kallistios".to_string(),
                "newlib".to_string(),
            ],
            "psp" => vec![
                "pspsdk".to_string(),
                "psplibc".to_string(),
            ],
            _ => vec![
                "stdc++".to_string(),
                "pthread".to_string(),
            ],
        };

        CppBindingConfig {
            target_triple: Self::get_target_triple(platform),
            toolchain: Self::get_toolchain(platform),
            compile_flags,
            link_flags: Self::get_link_flags(platform),
            required_libs,
            include_paths: Self::get_include_paths(platform),
        }
    }

    // Private helper methods

    fn get_hal_memory_usage(platform: &str) -> MemoryUsageSnapshot {
        match platform {
            "dreamcast" => MemoryUsageSnapshot {
                timestamp: chrono::Utc::now().timestamp() as u64,
                session_id: "flight-hal".to_string(),
                platform: platform.to_string(),
                total: MemorySize {
                    bytes: 16 * 1024 * 1024,
                    human_readable: "16MB".to_string(),
                },
                used: MemorySize {
                    bytes: 2 * 1024 * 1024,
                    human_readable: "2MB".to_string(),
                },
                available: MemorySize {
                    bytes: 14 * 1024 * 1024,
                    human_readable: "14MB".to_string(),
                },
                fragmentation_ratio: 0.05,
            },
            "psp" => MemoryUsageSnapshot {
                timestamp: chrono::Utc::now().timestamp() as u64,
                session_id: "flight-hal".to_string(),
                platform: platform.to_string(),
                total: MemorySize {
                    bytes: 32 * 1024 * 1024,
                    human_readable: "32MB".to_string(),
                },
                used: MemorySize {
                    bytes: 4 * 1024 * 1024,
                    human_readable: "4MB".to_string(),
                },
                available: MemorySize {
                    bytes: 28 * 1024 * 1024,
                    human_readable: "28MB".to_string(),
                },
                fragmentation_ratio: 0.03,
            },
            "vita" => MemoryUsageSnapshot {
                timestamp: chrono::Utc::now().timestamp() as u64,
                session_id: "flight-hal".to_string(),
                platform: platform.to_string(),
                total: MemorySize {
                    bytes: 512 * 1024 * 1024,
                    human_readable: "512MB".to_string(),
                },
                used: MemorySize {
                    bytes: 32 * 1024 * 1024,
                    human_readable: "32MB".to_string(),
                },
                available: MemorySize {
                    bytes: 480 * 1024 * 1024,
                    human_readable: "480MB".to_string(),
                },
                fragmentation_ratio: 0.02,
            },
            _ => MemoryUsageSnapshot {
                timestamp: chrono::Utc::now().timestamp() as u64,
                session_id: "flight-hal".to_string(),
                platform: platform.to_string(),
                total: MemorySize {
                    bytes: 1024 * 1024 * 1024,
                    human_readable: "1GB".to_string(),
                },
                used: MemorySize {
                    bytes: 64 * 1024 * 1024,
                    human_readable: "64MB".to_string(),
                },
                available: MemorySize {
                    bytes: 960 * 1024 * 1024,
                    human_readable: "960MB".to_string(),
                },
                fragmentation_ratio: 0.01,
            },
        }
    }

    fn get_runtime_memory_usage(platform: &str) -> MemoryUsageSnapshot {
        match platform {
            "dreamcast" => MemoryUsageSnapshot {
                timestamp: chrono::Utc::now().timestamp() as u64,
                session_id: "flight-runtime".to_string(),
                platform: platform.to_string(),
                total: MemorySize {
                    bytes: 8 * 1024 * 1024,
                    human_readable: "8MB".to_string(),
                },
                used: MemorySize {
                    bytes: 3 * 1024 * 1024,
                    human_readable: "3MB".to_string(),
                },
                available: MemorySize {
                    bytes: 5 * 1024 * 1024,
                    human_readable: "5MB".to_string(),
                },
                fragmentation_ratio: 0.10,
            },
            "psp" => MemoryUsageSnapshot {
                timestamp: chrono::Utc::now().timestamp() as u64,
                session_id: "flight-runtime".to_string(),
                platform: platform.to_string(),
                total: MemorySize {
                    bytes: 16 * 1024 * 1024,
                    human_readable: "16MB".to_string(),
                },
                used: MemorySize {
                    bytes: 6 * 1024 * 1024,
                    human_readable: "6MB".to_string(),
                },
                available: MemorySize {
                    bytes: 10 * 1024 * 1024,
                    human_readable: "10MB".to_string(),
                },
                fragmentation_ratio: 0.08,
            },
            _ => MemoryUsageSnapshot {
                timestamp: chrono::Utc::now().timestamp() as u64,
                session_id: "flight-runtime".to_string(),
                platform: platform.to_string(),
                total: MemorySize {
                    bytes: 128 * 1024 * 1024,
                    human_readable: "128MB".to_string(),
                },
                used: MemorySize {
                    bytes: 32 * 1024 * 1024,
                    human_readable: "32MB".to_string(),
                },
                available: MemorySize {
                    bytes: 96 * 1024 * 1024,
                    human_readable: "96MB".to_string(),
                },
                fragmentation_ratio: 0.05,
            },
        }
    }

    fn get_hal_metadata(platform: &str) -> Vec<(String, String)> {
        let mut metadata = vec![
            ("component_type".to_string(), "hal".to_string()),
            ("subsystem".to_string(), "hardware-abstraction".to_string()),
            ("cpp_standard".to_string(), "C++17".to_string()),
            ("platform_specific".to_string(), "true".to_string()),
        ];

        match platform {
            "dreamcast" => {
                metadata.extend(vec![
                    ("cpu_arch".to_string(), "sh4".to_string()),
                    ("memory_constraint".to_string(), "16MB".to_string()),
                    ("graphics_api".to_string(), "PowerVR2".to_string()),
                    ("sound_system".to_string(), "AICA".to_string()),
                ]);
            }
            "psp" => {
                metadata.extend(vec![
                    ("cpu_arch".to_string(), "mips".to_string()),
                    ("memory_constraint".to_string(), "32MB".to_string()),
                    ("graphics_api".to_string(), "GU".to_string()),
                    ("media_engine".to_string(), "available".to_string()),
                ]);
            }
            "vita" => {
                metadata.extend(vec![
                    ("cpu_arch".to_string(), "arm".to_string()),
                    ("memory_constraint".to_string(), "512MB".to_string()),
                    ("graphics_api".to_string(), "OpenGL_ES".to_string()),
                    ("touch_support".to_string(), "dual".to_string()),
                ]);
            }
            _ => {
                metadata.extend(vec![
                    ("cpu_arch".to_string(), "modern".to_string()),
                    ("memory_constraint".to_string(), "flexible".to_string()),
                    ("graphics_api".to_string(), "multiple".to_string()),
                ]);
            }
        }

        metadata
    }

    fn get_threading_model(platform: &str) -> String {
        match platform {
            "dreamcast" | "psp" => "cooperative".to_string(),
            "vita" => "preemptive_limited".to_string(),
            _ => "preemptive_full".to_string(),
        }
    }

    fn get_target_triple(platform: &str) -> String {
        match platform {
            "dreamcast" => "sh4-unknown-elf".to_string(),
            "psp" => "mipsel-sony-psp".to_string(),
            "vita" => "arm-vita-eabihf".to_string(),
            _ => "x86_64-unknown-linux-gnu".to_string(),
        }
    }

    fn get_toolchain(platform: &str) -> String {
        match platform {
            "dreamcast" => "kos-gcc".to_string(),
            "psp" => "psp-gcc".to_string(),
            "vita" => "vita-gcc".to_string(),
            _ => "gcc".to_string(),
        }
    }

    fn get_link_flags(platform: &str) -> Vec<String> {
        match platform {
            "dreamcast" => vec![
                "-Wl,-Ttext=0x8c010000".to_string(),
                "-Wl,--gc-sections".to_string(),
            ],
            "psp" => vec![
                "-specs=psplibcpsp".to_string(),
                "-Wl,-q,-T$(PSPSDK)/lib/linkfile.prx".to_string(),
            ],
            _ => vec![
                "-Wl,--gc-sections".to_string(),
                "-pthread".to_string(),
            ],
        }
    }

    fn get_include_paths(platform: &str) -> Vec<String> {
        match platform {
            "dreamcast" => vec![
                "$(KOS_BASE)/include".to_string(),
                "$(KOS_BASE)/kernel/arch/dreamcast/include".to_string(),
            ],
            "psp" => vec![
                "$(PSPSDK)/include".to_string(),
                "$(PSPDEV)/psp/include".to_string(),
            ],
            _ => vec![
                "/usr/include".to_string(),
                "/usr/local/include".to_string(),
            ],
        }
    }
}

// Type definitions that would normally come from generated bindings

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ComponentInfo {
    pub id: String,
    pub name: String,
    pub version: String,
    pub state: ComponentState,
    pub world: String,
    pub platform: String,
    pub session_id: Option<String>,
    pub created_at: u64,
    pub last_activity: u64,
    pub memory_usage: MemoryUsageSnapshot,
    pub metadata: Vec<(String, String)>,
}

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

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct MemorySize {
    pub bytes: u64,
    pub human_readable: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ExecutionContext {
    pub component: String,
    pub stack_depth: u32,
    pub available_memory: MemorySize,
    pub cpu_time_ms: u64,
    pub priority: ExecutionPriority,
    pub execution_mode: ExecutionMode,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum ExecutionPriority {
    Low,
    Normal,
    High,
    Critical,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum ExecutionMode {
    SingleThreaded,
    MultiThreaded,
    AsyncExecution,
    RealTime,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ComponentDependency {
    pub dependent: String,
    pub dependency: String,
    pub dependency_type: DependencyType,
    pub optional: bool,
    pub resolved: bool,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum DependencyType {
    InterfaceImport,
    ResourceAccess,
    PlatformCapability,
    MemoryRequirement,
    ComponentDependency,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct CppBindingConfig {
    pub target_triple: String,
    pub toolchain: String,
    pub compile_flags: Vec<String>,
    pub link_flags: Vec<String>,
    pub required_libs: Vec<String>,
    pub include_paths: Vec<String>,
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_dreamcast_hal_component() {
        let hal = FlightCoreComponentPatterns::create_hal_component_config("dreamcast");
        
        assert_eq!(hal.platform, "dreamcast");
        assert_eq!(hal.world, "flight:hal-world");
        assert_eq!(hal.memory_usage.total.bytes, 16 * 1024 * 1024);
        
        // Verify C++17 metadata
        let cpp_standard = hal.metadata.iter()
            .find(|(k, _)| k == "cpp_standard")
            .map(|(_, v)| v.as_str());
        assert_eq!(cpp_standard, Some("C++17"));
        
        // Verify platform-specific metadata
        let cpu_arch = hal.metadata.iter()
            .find(|(k, _)| k == "cpu_arch")
            .map(|(_, v)| v.as_str());
        assert_eq!(cpu_arch, Some("sh4"));
    }

    #[test]
    fn test_psp_runtime_component() {
        let runtime = FlightCoreComponentPatterns::create_runtime_component_config("psp");
        
        assert_eq!(runtime.platform, "psp");
        assert_eq!(runtime.world, "flight:runtime-world");
        assert!(runtime.name.contains("UWRC Runtime"));
        
        // Verify execution mode
        let execution_mode = runtime.metadata.iter()
            .find(|(k, _)| k == "execution_mode")
            .map(|(_, v)| v.as_str());
        assert_eq!(execution_mode, Some("single-threaded"));
    }

    #[test]
    fn test_component_dependencies() {
        let deps = FlightCoreComponentPatterns::create_flight_dependencies();
        
        assert!(!deps.is_empty());
        assert!(deps.iter().any(|d| d.dependent == "flight-runtime" && d.dependency == "flight-hal"));
        assert!(deps.iter().all(|d| d.resolved));
        
        // Verify WASM memory dependency
        assert!(deps.iter().any(|d| 
            d.dependent == "flight-runtime" && 
            d.dependency == "wasm-linear-memory" &&
            matches!(d.dependency_type, DependencyType::MemoryRequirement)
        ));
    }

    #[test]
    fn test_execution_context_scaling() {
        let dreamcast_ctx = FlightCoreComponentPatterns::create_execution_context("test", "dreamcast");
        let vita_ctx = FlightCoreComponentPatterns::create_execution_context("test", "vita");
        
        // Dreamcast should have more constrained resources
        assert!(dreamcast_ctx.available_memory.bytes < vita_ctx.available_memory.bytes);
        assert!(dreamcast_ctx.cpu_time_ms < vita_ctx.cpu_time_ms);
        assert!(dreamcast_ctx.stack_depth < vita_ctx.stack_depth);
        
        // Dreamcast should be single-threaded
        assert!(matches!(dreamcast_ctx.execution_mode, ExecutionMode::SingleThreaded));
    }

    #[test]
    fn test_cpp17_binding_config() {
        let dreamcast_config = FlightCoreComponentPatterns::create_cpp17_binding_config("dreamcast");
        
        assert_eq!(dreamcast_config.target_triple, "sh4-unknown-elf");
        assert_eq!(dreamcast_config.toolchain, "kos-gcc");
        assert!(dreamcast_config.compile_flags.contains(&"-std=c++17".to_string()));
        assert!(dreamcast_config.compile_flags.contains(&"-Os".to_string())); // Size optimization
        assert!(dreamcast_config.required_libs.contains(&"kallistios".to_string()));
    }
}
