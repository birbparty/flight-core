//! Flight-Core Platform Registry
//! 
//! Comprehensive platform definitions for all Flight-Core targets

use std::collections::HashMap;

/// Memory size helper for platform definitions
#[derive(Debug, Clone, PartialEq)]
pub struct MemorySize {
    pub bytes: u64,
    pub human_readable: String,
}

impl MemorySize {
    pub fn new(bytes: u64) -> Self {
        let human_readable = if bytes >= 1024 * 1024 * 1024 {
            format!("{}GB", bytes / (1024 * 1024 * 1024))
        } else if bytes >= 1024 * 1024 {
            format!("{}MB", bytes / (1024 * 1024))
        } else if bytes >= 1024 {
            format!("{}KB", bytes / 1024)
        } else {
            format!("{}B", bytes)
        };
        
        Self { bytes, human_readable }
    }
}

/// Platform capability levels
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PlatformCapability {
    Minimal,
    Basic,
    Standard,
    Enhanced,
    Full,
    Unlimited,
}

/// Platform categories
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PlatformCategory {
    RetroGaming,
    ModernGaming,
    Mobile,
    Desktop,
    Cloud,
    Development,
    Testing,
}

/// CPU architectures
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CpuArchitecture {
    Sh4,
    Mips,
    Arm,
    Arm64,
    X86,
    X86_64,
    Wasm,
}

/// Instruction sets
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum InstructionSet {
    Base,
    Simd,
    Vector,
    Dsp,
    Crypto,
}

/// Graphics APIs
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum GraphicsApi {
    Software,
    OpenglEs,
    Opengl,
    Vulkan,
    DirectX,
    Metal,
    WebGL,
    Custom,
}

/// Shader support levels
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ShaderSupportLevel {
    None,
    Basic,
    Advanced,
    Compute,
}

/// Storage types
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum StorageType {
    Ram,
    Flash,
    Hdd,
    Ssd,
    Optical,
    Network,
    Cloud,
}

/// Network interfaces
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum NetworkInterface {
    None,
    Ethernet,
    Wifi,
    Cellular,
    Bluetooth,
    Usb,
}

/// Network latency classes
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum NetworkLatencyClass {
    None,
    High,
    Medium,
    Low,
    UltraLow,
}

/// Platform features
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum PlatformFeature {
    Multitasking,
    RealTime,
    FileSystem,
    MemoryMappedFiles,
    Threading,
    Atomics,
    Exceptions,
    DynamicLinking,
    JitCompilation,
    GarbageCollection,
    AudioPlayback,
    AudioRecording,
    VideoPlayback,
    Camera,
    Sensors,
    Location,
    PushNotifications,
    BackgroundProcessing,
}

/// Display resolution
#[derive(Debug, Clone, PartialEq)]
pub struct Resolution {
    pub width: u32,
    pub height: u32,
}

/// CPU cache information
#[derive(Debug, Clone, PartialEq)]
pub struct CpuCacheInfo {
    pub l1_size: MemorySize,
    pub l2_size: MemorySize,
    pub l3_size: Option<MemorySize>,
}

/// Platform memory information
#[derive(Debug, Clone, PartialEq)]
pub struct PlatformMemoryInfo {
    pub total_memory: MemorySize,
    pub available_memory: MemorySize,
    pub system_reserved: MemorySize,
    pub stack_limit: MemorySize,
    pub heap_limit: MemorySize,
    pub has_virtual_memory: bool,
    pub has_mmu: bool,
    pub has_dma: bool,
}

/// Platform CPU information
#[derive(Debug, Clone, PartialEq)]
pub struct PlatformCpuInfo {
    pub architecture: CpuArchitecture,
    pub core_count: u32,
    pub clock_speed_mhz: u32,
    pub instruction_sets: Vec<InstructionSet>,
    pub cache_info: CpuCacheInfo,
    pub has_fpu: bool,
    pub has_simd: bool,
}

/// Platform graphics information
#[derive(Debug, Clone, PartialEq)]
pub struct PlatformGraphicsInfo {
    pub apis: Vec<GraphicsApi>,
    pub max_texture_size: u32,
    pub video_memory: MemorySize,
    pub has_hardware_accel: bool,
    pub shader_support: ShaderSupportLevel,
    pub color_depth: u32,
    pub max_resolution: Resolution,
}

/// Platform storage information
#[derive(Debug, Clone, PartialEq)]
pub struct PlatformStorageInfo {
    pub storage_types: Vec<StorageType>,
    pub read_speed_mbps: u32,
    pub write_speed_mbps: u32,
    pub total_capacity: MemorySize,
    pub available_capacity: MemorySize,
    pub has_random_access: bool,
}

/// Platform network information
#[derive(Debug, Clone, PartialEq)]
pub struct PlatformNetworkInfo {
    pub interfaces: Vec<NetworkInterface>,
    pub max_bandwidth_mbps: u32,
    pub latency_class: NetworkLatencyClass,
    pub has_tcp_ip: bool,
    pub has_udp: bool,
    pub has_websockets: bool,
    pub has_tls: bool,
}

/// Platform build configuration
#[derive(Debug, Clone, PartialEq)]
pub struct PlatformBuildConfig {
    pub target_triple: String,
    pub toolchain: String,
    pub compile_flags: Vec<String>,
    pub link_flags: Vec<String>,
    pub required_libs: Vec<String>,
    pub asset_pipeline: String,
    pub package_format: String,
    pub deployment_method: String,
}

/// Complete platform information
#[derive(Debug, Clone, PartialEq)]
pub struct PlatformInfo {
    pub id: String,
    pub name: String,
    pub category: PlatformCategory,
    pub capability: PlatformCapability,
    pub memory: PlatformMemoryInfo,
    pub cpu: PlatformCpuInfo,
    pub graphics: PlatformGraphicsInfo,
    pub storage: PlatformStorageInfo,
    pub network: PlatformNetworkInfo,
    pub features: Vec<PlatformFeature>,
    pub build_config: PlatformBuildConfig,
}

/// Flight-Core Platform Registry
pub struct FlightPlatformRegistry {
    platforms: HashMap<String, PlatformInfo>,
}

impl Default for FlightPlatformRegistry {
    fn default() -> Self {
        Self::new()
    }
}

impl FlightPlatformRegistry {
    /// Create new platform registry with all predefined platforms
    pub fn new() -> Self {
        let mut registry = Self {
            platforms: HashMap::new(),
        };
        
        // Register all platforms
        let platforms = vec![
            Self::dreamcast(),
            Self::psp(),
            Self::vita(),
            Self::v6r_small(),
            Self::v6r_medium(),
            Self::v6r_large(),
            Self::windows(),
            Self::macos(),
            Self::linux(),
            Self::android(),
            Self::ios(),
            Self::web(),
        ];
        
        for platform in platforms {
            registry.platforms.insert(platform.id.clone(), platform);
        }
        
        registry
    }
    
    /// Get all supported platforms
    pub fn get_all_platforms(&self) -> Vec<&PlatformInfo> {
        self.platforms.values().collect()
    }
    
    /// Get platform by ID
    pub fn get_platform(&self, id: &str) -> Option<&PlatformInfo> {
        self.platforms.get(id)
    }
    
    /// Check if platform supports feature
    pub fn has_feature(&self, platform_id: &str, feature: PlatformFeature) -> bool {
        self.get_platform(platform_id)
            .map(|p| p.features.contains(&feature))
            .unwrap_or(false)
    }
    
    /// Filter platforms by capability
    pub fn filter_by_capability(&self, min_capability: PlatformCapability) -> Vec<&PlatformInfo> {
        self.platforms
            .values()
            .filter(|p| Self::capability_level(p.capability) >= Self::capability_level(min_capability))
            .collect()
    }
    
    /// Get platforms by category
    pub fn get_platforms_by_category(&self, category: PlatformCategory) -> Vec<&PlatformInfo> {
        self.platforms
            .values()
            .filter(|p| p.category == category)
            .collect()
    }
    
    /// Convert capability to numeric level for comparison
    fn capability_level(capability: PlatformCapability) -> u8 {
        match capability {
            PlatformCapability::Minimal => 0,
            PlatformCapability::Basic => 1,
            PlatformCapability::Standard => 2,
            PlatformCapability::Enhanced => 3,
            PlatformCapability::Full => 4,
            PlatformCapability::Unlimited => 5,
        }
    }
    
    /// Dreamcast platform definition - minimal capability baseline
    pub fn dreamcast() -> PlatformInfo {
        PlatformInfo {
            id: "dreamcast".to_string(),
            name: "Sega Dreamcast".to_string(),
            category: PlatformCategory::RetroGaming,
            capability: PlatformCapability::Minimal,
            memory: PlatformMemoryInfo {
                total_memory: MemorySize::new(16 * 1024 * 1024), // 16MB
                available_memory: MemorySize::new(12 * 1024 * 1024), // 12MB
                system_reserved: MemorySize::new(4 * 1024 * 1024), // 4MB
                stack_limit: MemorySize::new(1024 * 1024), // 1MB
                heap_limit: MemorySize::new(8 * 1024 * 1024), // 8MB
                has_virtual_memory: false,
                has_mmu: false,
                has_dma: true,
            },
            cpu: PlatformCpuInfo {
                architecture: CpuArchitecture::Sh4,
                core_count: 1,
                clock_speed_mhz: 200,
                instruction_sets: vec![InstructionSet::Base, InstructionSet::Dsp],
                cache_info: CpuCacheInfo {
                    l1_size: MemorySize::new(16 * 1024), // 16KB
                    l2_size: MemorySize::new(0), // No L2
                    l3_size: None,
                },
                has_fpu: true,
                has_simd: false,
            },
            graphics: PlatformGraphicsInfo {
                apis: vec![GraphicsApi::Custom],
                max_texture_size: 1024,
                video_memory: MemorySize::new(8 * 1024 * 1024), // 8MB
                has_hardware_accel: true,
                shader_support: ShaderSupportLevel::None,
                color_depth: 16,
                max_resolution: Resolution { width: 640, height: 480 },
            },
            storage: PlatformStorageInfo {
                storage_types: vec![StorageType::Optical, StorageType::Flash],
                read_speed_mbps: 2,
                write_speed_mbps: 0, // Read-only optical
                total_capacity: MemorySize::new(1200 * 1024 * 1024), // 1.2GB
                available_capacity: MemorySize::new(1000 * 1024 * 1024), // 1GB
                has_random_access: false,
            },
            network: PlatformNetworkInfo {
                interfaces: vec![NetworkInterface::Ethernet],
                max_bandwidth_mbps: 1,
                latency_class: NetworkLatencyClass::High,
                has_tcp_ip: true,
                has_udp: true,
                has_websockets: false,
                has_tls: false,
            },
            features: vec![
                PlatformFeature::AudioPlayback,
                PlatformFeature::VideoPlayback,
                PlatformFeature::FileSystem,
            ],
            build_config: PlatformBuildConfig {
                target_triple: "sh4-unknown-elf".to_string(),
                toolchain: "gcc-sh4".to_string(),
                compile_flags: vec!["-m4".to_string(), "-mb".to_string()],
                link_flags: vec!["-Wl,--gc-sections".to_string()],
                required_libs: vec!["libgcc".to_string()],
                asset_pipeline: "dreamcast-assets".to_string(),
                package_format: "elf".to_string(),
                deployment_method: "optical-disc".to_string(),
            },
        }
    }
    
    /// PSP platform definition - basic capability
    pub fn psp() -> PlatformInfo {
        PlatformInfo {
            id: "psp".to_string(),
            name: "Sony PlayStation Portable".to_string(),
            category: PlatformCategory::RetroGaming,
            capability: PlatformCapability::Basic,
            memory: PlatformMemoryInfo {
                total_memory: MemorySize::new(64 * 1024 * 1024), // 64MB
                available_memory: MemorySize::new(48 * 1024 * 1024), // 48MB
                system_reserved: MemorySize::new(16 * 1024 * 1024), // 16MB
                stack_limit: MemorySize::new(4 * 1024 * 1024), // 4MB
                heap_limit: MemorySize::new(32 * 1024 * 1024), // 32MB
                has_virtual_memory: false,
                has_mmu: true,
                has_dma: true,
            },
            cpu: PlatformCpuInfo {
                architecture: CpuArchitecture::Mips,
                core_count: 1,
                clock_speed_mhz: 333,
                instruction_sets: vec![InstructionSet::Base, InstructionSet::Dsp],
                cache_info: CpuCacheInfo {
                    l1_size: MemorySize::new(32 * 1024), // 32KB
                    l2_size: MemorySize::new(0), // No L2
                    l3_size: None,
                },
                has_fpu: true,
                has_simd: false,
            },
            graphics: PlatformGraphicsInfo {
                apis: vec![GraphicsApi::Custom],
                max_texture_size: 512,
                video_memory: MemorySize::new(4 * 1024 * 1024), // 4MB
                has_hardware_accel: true,
                shader_support: ShaderSupportLevel::None,
                color_depth: 16,
                max_resolution: Resolution { width: 480, height: 272 },
            },
            storage: PlatformStorageInfo {
                storage_types: vec![StorageType::Flash, StorageType::Optical],
                read_speed_mbps: 5,
                write_speed_mbps: 2,
                total_capacity: MemorySize::new(1800 * 1024 * 1024), // 1.8GB UMD
                available_capacity: MemorySize::new(1600 * 1024 * 1024), // 1.6GB
                has_random_access: true,
            },
            network: PlatformNetworkInfo {
                interfaces: vec![NetworkInterface::Wifi],
                max_bandwidth_mbps: 11,
                latency_class: NetworkLatencyClass::Medium,
                has_tcp_ip: true,
                has_udp: true,
                has_websockets: false,
                has_tls: false,
            },
            features: vec![
                PlatformFeature::AudioPlayback,
                PlatformFeature::VideoPlayback,
                PlatformFeature::FileSystem,
                PlatformFeature::Threading,
            ],
            build_config: PlatformBuildConfig {
                target_triple: "mips-allegrex-elf".to_string(),
                toolchain: "gcc-psp".to_string(),
                compile_flags: vec!["-G0".to_string(), "-mno-abicalls".to_string()],
                link_flags: vec!["-specs=pspsdk.specs".to_string()],
                required_libs: vec!["pspsdk".to_string()],
                asset_pipeline: "psp-assets".to_string(),
                package_format: "pbp".to_string(),
                deployment_method: "memory-stick".to_string(),
            },
        }
    }
    
    /// PlayStation Vita platform definition - standard capability
    pub fn vita() -> PlatformInfo {
        PlatformInfo {
            id: "vita".to_string(),
            name: "Sony PlayStation Vita".to_string(),
            category: PlatformCategory::ModernGaming,
            capability: PlatformCapability::Standard,
            memory: PlatformMemoryInfo {
                total_memory: MemorySize::new(512 * 1024 * 1024), // 512MB
                available_memory: MemorySize::new(400 * 1024 * 1024), // 400MB
                system_reserved: MemorySize::new(112 * 1024 * 1024), // 112MB
                stack_limit: MemorySize::new(16 * 1024 * 1024), // 16MB
                heap_limit: MemorySize::new(256 * 1024 * 1024), // 256MB
                has_virtual_memory: true,
                has_mmu: true,
                has_dma: true,
            },
            cpu: PlatformCpuInfo {
                architecture: CpuArchitecture::Arm,
                core_count: 4,
                clock_speed_mhz: 444,
                instruction_sets: vec![InstructionSet::Base, InstructionSet::Simd],
                cache_info: CpuCacheInfo {
                    l1_size: MemorySize::new(32 * 1024), // 32KB
                    l2_size: MemorySize::new(512 * 1024), // 512KB
                    l3_size: None,
                },
                has_fpu: true,
                has_simd: true,
            },
            graphics: PlatformGraphicsInfo {
                apis: vec![GraphicsApi::OpenglEs],
                max_texture_size: 2048,
                video_memory: MemorySize::new(128 * 1024 * 1024), // 128MB
                has_hardware_accel: true,
                shader_support: ShaderSupportLevel::Basic,
                color_depth: 24,
                max_resolution: Resolution { width: 960, height: 544 },
            },
            storage: PlatformStorageInfo {
                storage_types: vec![StorageType::Flash],
                read_speed_mbps: 20,
                write_speed_mbps: 15,
                total_capacity: MemorySize::new(4 * 1024 * 1024 * 1024), // 4GB
                available_capacity: MemorySize::new(3 * 1024 * 1024 * 1024), // 3GB
                has_random_access: true,
            },
            network: PlatformNetworkInfo {
                interfaces: vec![NetworkInterface::Wifi, NetworkInterface::Cellular],
                max_bandwidth_mbps: 54,
                latency_class: NetworkLatencyClass::Medium,
                has_tcp_ip: true,
                has_udp: true,
                has_websockets: true,
                has_tls: true,
            },
            features: vec![
                PlatformFeature::Multitasking,
                PlatformFeature::Threading,
                PlatformFeature::FileSystem,
                PlatformFeature::AudioPlayback,
                PlatformFeature::AudioRecording,
                PlatformFeature::VideoPlayback,
                PlatformFeature::Camera,
                PlatformFeature::Sensors,
                PlatformFeature::Location,
            ],
            build_config: PlatformBuildConfig {
                target_triple: "arm-vita-eabi".to_string(),
                toolchain: "vitasdk".to_string(),
                compile_flags: vec!["-Wl,-q".to_string()],
                link_flags: vec!["-lvita2d".to_string()],
                required_libs: vec!["vitasdk".to_string()],
                asset_pipeline: "vita-assets".to_string(),
                package_format: "vpk".to_string(),
                deployment_method: "pkg-installer".to_string(),
            },
        }
    }
    
    /// V6R Small VM platform definition - enhanced capability
    pub fn v6r_small() -> PlatformInfo {
        PlatformInfo {
            id: "v6r-small".to_string(),
            name: "V6R Small Development VM".to_string(),
            category: PlatformCategory::Development,
            capability: PlatformCapability::Enhanced,
            memory: PlatformMemoryInfo {
                total_memory: MemorySize::new(512 * 1024 * 1024), // 512MB
                available_memory: MemorySize::new(400 * 1024 * 1024), // 400MB
                system_reserved: MemorySize::new(112 * 1024 * 1024), // 112MB
                stack_limit: MemorySize::new(32 * 1024 * 1024), // 32MB
                heap_limit: MemorySize::new(300 * 1024 * 1024), // 300MB
                has_virtual_memory: true,
                has_mmu: true,
                has_dma: true,
            },
            cpu: PlatformCpuInfo {
                architecture: CpuArchitecture::X86_64,
                core_count: 2,
                clock_speed_mhz: 2400,
                instruction_sets: vec![
                    InstructionSet::Base,
                    InstructionSet::Simd,
                    InstructionSet::Vector,
                ],
                cache_info: CpuCacheInfo {
                    l1_size: MemorySize::new(64 * 1024), // 64KB
                    l2_size: MemorySize::new(512 * 1024), // 512KB
                    l3_size: Some(MemorySize::new(4 * 1024 * 1024)), // 4MB
                },
                has_fpu: true,
                has_simd: true,
            },
            graphics: PlatformGraphicsInfo {
                apis: vec![GraphicsApi::Vulkan, GraphicsApi::Opengl],
                max_texture_size: 8192,
                video_memory: MemorySize::new(128 * 1024 * 1024), // 128MB
                has_hardware_accel: true,
                shader_support: ShaderSupportLevel::Compute,
                color_depth: 32,
                max_resolution: Resolution { width: 1920, height: 1080 },
            },
            storage: PlatformStorageInfo {
                storage_types: vec![StorageType::Ssd, StorageType::Network],
                read_speed_mbps: 500,
                write_speed_mbps: 300,
                total_capacity: MemorySize::new(20 * 1024 * 1024 * 1024), // 20GB
                available_capacity: MemorySize::new(15 * 1024 * 1024 * 1024), // 15GB
                has_random_access: true,
            },
            network: PlatformNetworkInfo {
                interfaces: vec![NetworkInterface::Ethernet, NetworkInterface::Wifi],
                max_bandwidth_mbps: 1000,
                latency_class: NetworkLatencyClass::Low,
                has_tcp_ip: true,
                has_udp: true,
                has_websockets: true,
                has_tls: true,
            },
            features: vec![
                PlatformFeature::Multitasking,
                PlatformFeature::Threading,
                PlatformFeature::FileSystem,
                PlatformFeature::MemoryMappedFiles,
                PlatformFeature::DynamicLinking,
                PlatformFeature::AudioPlayback,
                PlatformFeature::VideoPlayback,
                PlatformFeature::BackgroundProcessing,
            ],
            build_config: PlatformBuildConfig {
                target_triple: "x86_64-unknown-linux-gnu".to_string(),
                toolchain: "clang".to_string(),
                compile_flags: vec!["-O2".to_string(), "-march=native".to_string()],
                link_flags: vec!["-lpthread".to_string(), "-ldl".to_string()],
                required_libs: vec!["glibc".to_string(), "libstdc++".to_string()],
                asset_pipeline: "cloud-assets".to_string(),
                package_format: "container".to_string(),
                deployment_method: "kubernetes".to_string(),
            },
        }
    }
    
    /// V6R Medium VM platform definition - full capability
    pub fn v6r_medium() -> PlatformInfo {
        PlatformInfo {
            id: "v6r-medium".to_string(),
            name: "V6R Medium Standard VM".to_string(),
            category: PlatformCategory::Cloud,
            capability: PlatformCapability::Full,
            memory: PlatformMemoryInfo {
                total_memory: MemorySize::new(1024 * 1024 * 1024), // 1GB
                available_memory: MemorySize::new(800 * 1024 * 1024), // 800MB
                system_reserved: MemorySize::new(224 * 1024 * 1024), // 224MB
                stack_limit: MemorySize::new(64 * 1024 * 1024), // 64MB
                heap_limit: MemorySize::new(600 * 1024 * 1024), // 600MB
                has_virtual_memory: true,
                has_mmu: true,
                has_dma: true,
            },
            cpu: PlatformCpuInfo {
                architecture: CpuArchitecture::X86_64,
                core_count: 4,
                clock_speed_mhz: 2800,
                instruction_sets: vec![
                    InstructionSet::Base,
                    InstructionSet::Simd,
                    InstructionSet::Vector,
                    InstructionSet::Crypto,
                ],
                cache_info: CpuCacheInfo {
                    l1_size: MemorySize::new(64 * 1024), // 64KB
                    l2_size: MemorySize::new(512 * 1024), // 512KB
                    l3_size: Some(MemorySize::new(8 * 1024 * 1024)), // 8MB
                },
                has_fpu: true,
                has_simd: true,
            },
            graphics: PlatformGraphicsInfo {
                apis: vec![GraphicsApi::Vulkan, GraphicsApi::Opengl],
                max_texture_size: 16384,
                video_memory: MemorySize::new(256 * 1024 * 1024), // 256MB
                has_hardware_accel: true,
                shader_support: ShaderSupportLevel::Compute,
                color_depth: 32,
                max_resolution: Resolution { width: 4096, height: 2160 },
            },
            storage: PlatformStorageInfo {
                storage_types: vec![StorageType::Ssd, StorageType::Network, StorageType::Cloud],
                read_speed_mbps: 1000,
                write_speed_mbps: 600,
                total_capacity: MemorySize::new(50 * 1024 * 1024 * 1024), // 50GB
                available_capacity: MemorySize::new(40 * 1024 * 1024 * 1024), // 40GB
                has_random_access: true,
            },
            network: PlatformNetworkInfo {
                interfaces: vec![NetworkInterface::Ethernet, NetworkInterface::Wifi],
                max_bandwidth_mbps: 10000,
                latency_class: NetworkLatencyClass::UltraLow,
                has_tcp_ip: true,
                has_udp: true,
                has_websockets: true,
                has_tls: true,
            },
            features: vec![
                PlatformFeature::Multitasking,
                PlatformFeature::Threading,
                PlatformFeature::FileSystem,
                PlatformFeature::MemoryMappedFiles,
                PlatformFeature::DynamicLinking,
                PlatformFeature::JitCompilation,
                PlatformFeature::AudioPlayback,
                PlatformFeature::VideoPlayback,
                PlatformFeature::BackgroundProcessing,
                PlatformFeature::Atomics,
                PlatformFeature::Exceptions,
            ],
            build_config: PlatformBuildConfig {
                target_triple: "x86_64-unknown-linux-gnu".to_string(),
                toolchain: "clang".to_string(),
                compile_flags: vec!["-O3".to_string(), "-march=native".to_string()],
                link_flags: vec!["-lpthread".to_string(), "-ldl".to_string()],
                required_libs: vec!["glibc".to_string(), "libstdc++".to_string()],
                asset_pipeline: "cloud-assets".to_string(),
                package_format: "container".to_string(),
                deployment_method: "kubernetes".to_string(),
            },
        }
    }
    
    /// V6R Large VM platform definition - unlimited capability
    pub fn v6r_large() -> PlatformInfo {
        PlatformInfo {
            id: "v6r-large".to_string(),
            name: "V6R Large Intensive VM".to_string(),
            category: PlatformCategory::Cloud,
            capability: PlatformCapability::Unlimited,
            memory: PlatformMemoryInfo {
                total_memory: MemorySize::new(2 * 1024 * 1024 * 1024), // 2GB
                available_memory: MemorySize::new(1600 * 1024 * 1024), // 1.6GB
                system_reserved: MemorySize::new(400 * 1024 * 1024), // 400MB
                stack_limit: MemorySize::new(128 * 1024 * 1024), // 128MB
                heap_limit: MemorySize::new(1200 * 1024 * 1024), // 1.2GB
                has_virtual_memory: true,
                has_mmu: true,
                has_dma: true,
            },
            cpu: PlatformCpuInfo {
                architecture: CpuArchitecture::X86_64,
                core_count: 8,
                clock_speed_mhz: 3200,
                instruction_sets: vec![
                    InstructionSet::Base,
                    InstructionSet::Simd,
                    InstructionSet::Vector,
                    InstructionSet::Crypto,
                ],
                cache_info: CpuCacheInfo {
                    l1_size: MemorySize::new(64 * 1024), // 64KB
                    l2_size: MemorySize::new(512 * 1024), // 512KB
                    l3_size: Some(MemorySize::new(16 * 1024 * 1024)), // 16MB
                },
                has_fpu: true,
                has_simd: true,
            },
            graphics: PlatformGraphicsInfo {
                apis: vec![GraphicsApi::Vulkan, GraphicsApi::Opengl],
                max_texture_size: 32768,
                video_memory: MemorySize::new(512 * 1024 * 1024), // 512MB
                has_hardware_accel: true,
                shader_support: ShaderSupportLevel::Compute,
                color_depth: 32,
                max_resolution: Resolution { width: 7680, height: 4320 },
            },
            storage: PlatformStorageInfo {
                storage_types: vec![StorageType::Ssd, StorageType::Network, StorageType::Cloud],
                read_speed_mbps: 2000,
                write_speed_mbps: 1500,
                total_capacity: MemorySize::new(100 * 1024 * 1024 * 1024), // 100GB
                available_capacity: MemorySize::new(80 * 1024 * 1024 * 1024), // 80GB
                has_random_access: true,
            },
            network: PlatformNetworkInfo {
                interfaces: vec![NetworkInterface::Ethernet, NetworkInterface::Wifi],
                max_bandwidth_mbps: 25000,
                latency_class: NetworkLatencyClass::UltraLow,
                has_tcp_ip: true,
                has_udp: true,
                has_websockets: true,
                has_tls: true,
            },
            features: vec![
                PlatformFeature::Multitasking,
                PlatformFeature::Threading,
                PlatformFeature::FileSystem,
                PlatformFeature::MemoryMappedFiles,
                PlatformFeature::DynamicLinking,
                PlatformFeature::JitCompilation,
                PlatformFeature::GarbageCollection,
                PlatformFeature::AudioPlayback,
                PlatformFeature::VideoPlayback,
                PlatformFeature::BackgroundProcessing,
                PlatformFeature::Atomics,
                PlatformFeature::Exceptions,
            ],
            build_config: PlatformBuildConfig {
                target_triple: "x86_64-unknown-linux-gnu".to_string(),
                toolchain: "clang".to_string(),
                compile_flags: vec!["-O3".to_string(), "-march=native".to_string(), "-flto".to_string()],
                link_flags: vec!["-lpthread".to_string(), "-ldl".to_string()],
                required_libs: vec!["glibc".to_string(), "libstdc++".to_string()],
                asset_pipeline: "cloud-assets".to_string(),
                package_format: "container".to_string(),
                deployment_method: "kubernetes".to_string(),
            },
        }
    }
    
    /// Windows platform definition - full capability
    pub fn windows() -> PlatformInfo {
        PlatformInfo {
            id: "windows".to_string(),
            name: "Microsoft Windows".to_string(),
            category: PlatformCategory::Desktop,
            capability: PlatformCapability::Full,
            memory: PlatformMemoryInfo {
                total_memory: MemorySize::new(8 * 1024 * 1024 * 1024), // 8GB typical
                available_memory: MemorySize::new(6 * 1024 * 1024 * 1024), // 6GB
                system_reserved: MemorySize::new(2 * 1024 * 1024 * 1024), // 2GB
                stack_limit: MemorySize::new(1024 * 1024 * 1024), // 1GB
                heap_limit: MemorySize::new(4 * 1024 * 1024 * 1024), // 4GB
                has_virtual_memory: true,
                has_mmu: true,
                has_dma: true,
            },
            cpu: PlatformCpuInfo {
                architecture: CpuArchitecture::X86_64,
                core_count: 8,
                clock_speed_mhz: 3000,
                instruction_sets: vec![
                    InstructionSet::Base,
                    InstructionSet::Simd,
                    InstructionSet::Vector,
                    InstructionSet::Crypto,
                ],
                cache_info: CpuCacheInfo {
                    l1_size: MemorySize::new(64 * 1024), // 64KB
                    l2_size: MemorySize::new(512 * 1024), // 512KB
                    l3_size: Some(MemorySize::new(16 * 1024 * 1024)), // 16MB
                },
                has_fpu: true,
                has_simd: true,
            },
            graphics: PlatformGraphicsInfo {
                apis: vec![GraphicsApi::DirectX, GraphicsApi::Vulkan, GraphicsApi::Opengl],
                max_texture_size: 16384,
                video_memory: MemorySize::new(4 * 1024 * 1024 * 1024), // 4GB
                has_hardware_accel: true,
                shader_support: ShaderSupportLevel::Compute,
                color_depth: 32,
                max_resolution: Resolution { width: 3840, height: 2160 },
            },
            storage: PlatformStorageInfo {
                storage_types: vec![StorageType::Ssd, StorageType::Hdd, StorageType::Network],
                read_speed_mbps: 500,
                write_speed_mbps: 400,
                total_capacity: MemorySize::new(1024 * 1024 * 1024 * 1024), // 1TB
                available_capacity: MemorySize::new(800 * 1024 * 1024 * 1024), // 800GB
                has_random_access: true,
            },
            network: PlatformNetworkInfo {
                interfaces: vec![NetworkInterface::Ethernet, NetworkInterface::Wifi],
                max_bandwidth_mbps: 1000,
                latency_class: NetworkLatencyClass::Low,
                has_tcp_ip: true,
                has_udp: true,
                has_websockets: true,
                has_tls: true,
            },
            features: vec![
                PlatformFeature::Multitasking,
                PlatformFeature::Threading,
                PlatformFeature::FileSystem,
                PlatformFeature::MemoryMappedFiles,
                PlatformFeature::DynamicLinking,
                PlatformFeature::JitCompilation,
                PlatformFeature::GarbageCollection,
                PlatformFeature::AudioPlayback,
                PlatformFeature::AudioRecording,
                PlatformFeature::VideoPlayback,
                PlatformFeature::Camera,
                PlatformFeature::BackgroundProcessing,
                PlatformFeature::Atomics,
                PlatformFeature::Exceptions,
            ],
            build_config: PlatformBuildConfig {
                target_triple: "x86_64-pc-windows-msvc".to_string(),
                toolchain: "msvc".to_string(),
                compile_flags: vec!["/O2".to_string(), "/arch:AVX2".to_string()],
                link_flags: vec!["/LTCG".to_string()],
                required_libs: vec!["kernel32.lib".to_string(), "user32.lib".to_string()],
                asset_pipeline: "windows-assets".to_string(),
                package_format: "exe".to_string(),
                deployment_method: "installer".to_string(),
            },
        }
    }
    
    // Placeholder implementations for remaining platforms
    pub fn macos() -> PlatformInfo { Self::windows() }
    pub fn linux() -> PlatformInfo { Self::windows() }
    pub fn android() -> PlatformInfo { Self::vita() }
    pub fn ios() -> PlatformInfo { Self::vita() }
    pub fn web() -> PlatformInfo { Self::vita() }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_platform_registry() {
        let registry = FlightPlatformRegistry::new();
        let platforms = registry.get_all_platforms();
        assert!(!platforms.is_empty());
        
        // Verify Dreamcast is minimal capability
        let dreamcast = registry.get_platform("dreamcast").unwrap();
        assert_eq!(dreamcast.capability, PlatformCapability::Minimal);
        assert_eq!(dreamcast.memory.total_memory.bytes, 16 * 1024 * 1024);
        
        // Verify V6R has enhanced capabilities
        let v6r = registry.get_platform("v6r-small").unwrap();
        assert_eq!(v6r.capability, PlatformCapability::Enhanced);
        assert!(v6r.memory.total_memory.bytes > dreamcast.memory.total_memory.bytes);
    }
    
    #[test]
    fn test_feature_detection() {
        let registry = FlightPlatformRegistry::new();
        assert!(!registry.has_feature("dreamcast", PlatformFeature::Threading));
        assert!(registry.has_feature("v6r-small", PlatformFeature::Threading));
    }
    
    #[test]
    fn test_capability_filtering() {
        let registry = FlightPlatformRegistry::new();
        let enhanced_platforms = registry.filter_by_capability(PlatformCapability::Enhanced);
        assert!(enhanced_platforms.len() >= 1);
        
        for platform in enhanced_platforms {
            assert!(FlightPlatformRegistry::capability_level(platform.capability) >= 3);
        }
    }
    
    #[test]
    fn test_category_filtering() {
        let registry = FlightPlatformRegistry::new();
        let retro_platforms = registry.get_platforms_by_category(PlatformCategory::RetroGaming);
        assert!(retro_platforms.len() >= 2); // Dreamcast, PSP
        
        let cloud_platforms = registry.get_platforms_by_category(PlatformCategory::Cloud);
        assert!(cloud_platforms.len() >= 1); // V6R medium/large
    }
}
