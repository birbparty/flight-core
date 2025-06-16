/**
 * Flight-Core Platform Types for TypeScript
 * 
 * Universal platform abstraction enabling development from Dreamcast (16MB)
 * to V6R cloud environments (2GB+) with full type safety and performance
 */

// Local type definitions for memory and error types
export interface MemorySize {
  /** Raw byte count */
  bytes: number;
  /** Human-readable format: "16MB", "512KB", "2GB", etc. */
  humanReadable: string;
}

export interface FlightError {
  /** Error identifier */
  id: string;
  /** Error message */
  message: string;
  /** Error context */
  context?: Record<string, any>;
}

export type FlightResult<T> = 
  | { ok: true; value: T }
  | { ok: false; error: FlightError };

// Platform Capability Levels
export enum PlatformCapability {
  /** Minimal capabilities (Dreamcast-class) */
  Minimal = 'minimal',
  /** Basic capabilities (PSP-class) */
  Basic = 'basic',
  /** Standard capabilities (Vita-class) */
  Standard = 'standard',
  /** Enhanced capabilities (V6R small) */
  Enhanced = 'enhanced',
  /** Full capabilities (V6R medium+) */
  Full = 'full',
  /** Unlimited capabilities (V6R large+) */
  Unlimited = 'unlimited',
}

// Platform Categories
export enum PlatformCategory {
  /** Retro gaming platforms */
  RetroGaming = 'retro-gaming',
  /** Modern gaming platforms */
  ModernGaming = 'modern-gaming',
  /** Mobile platforms */
  Mobile = 'mobile',
  /** Desktop platforms */
  Desktop = 'desktop',
  /** Cloud/VM platforms */
  Cloud = 'cloud',
  /** Development platforms */
  Development = 'development',
  /** Testing platforms */
  Testing = 'testing',
}

// CPU Architectures
export enum CpuArchitecture {
  /** SuperH (Dreamcast) */
  SH4 = 'sh4',
  /** MIPS (PSP) */
  MIPS = 'mips',
  /** ARM (modern mobile/embedded) */
  ARM = 'arm',
  /** ARM64 (modern mobile/cloud) */
  ARM64 = 'arm64',
  /** x86 (legacy desktop) */
  X86 = 'x86',
  /** x86-64 (modern desktop/cloud) */
  X86_64 = 'x86-64',
  /** WebAssembly (universal) */
  WASM = 'wasm',
}

// Instruction Sets
export enum InstructionSet {
  /** Basic instruction set */
  Base = 'base',
  /** SIMD extensions */
  SIMD = 'simd',
  /** Vector processing */
  Vector = 'vector',
  /** Digital signal processing */
  DSP = 'dsp',
  /** Cryptography extensions */
  Crypto = 'crypto',
}

// Graphics APIs
export enum GraphicsApi {
  /** Software rendering */
  Software = 'software',
  /** OpenGL ES (mobile/embedded) */
  OpenGLES = 'opengl-es',
  /** OpenGL (desktop) */
  OpenGL = 'opengl',
  /** Vulkan (modern) */
  Vulkan = 'vulkan',
  /** DirectX (Windows) */
  DirectX = 'directx',
  /** Metal (Apple platforms) */
  Metal = 'metal',
  /** WebGL (web) */
  WebGL = 'webgl',
  /** Custom platform API */
  Custom = 'custom',
}

// Shader Support Levels
export enum ShaderSupportLevel {
  /** No shader support */
  None = 'none',
  /** Basic vertex/fragment shaders */
  Basic = 'basic',
  /** Advanced shading */
  Advanced = 'advanced',
  /** Compute shaders */
  Compute = 'compute',
}

// Storage Types
export enum StorageType {
  /** RAM-based storage */
  RAM = 'ram',
  /** Flash memory */
  Flash = 'flash',
  /** Hard disk drive */
  HDD = 'hdd',
  /** Solid state drive */
  SSD = 'ssd',
  /** Optical disc */
  Optical = 'optical',
  /** Network storage */
  Network = 'network',
  /** Cloud storage */
  Cloud = 'cloud',
}

// Network Interfaces
export enum NetworkInterface {
  /** No network interface */
  None = 'none',
  /** Ethernet */
  Ethernet = 'ethernet',
  /** WiFi */
  WiFi = 'wifi',
  /** Cellular */
  Cellular = 'cellular',
  /** Bluetooth */
  Bluetooth = 'bluetooth',
  /** USB networking */
  USB = 'usb',
}

// Network Latency Classes
export enum NetworkLatencyClass {
  /** No network */
  None = 'none',
  /** High latency (>500ms) */
  High = 'high',
  /** Medium latency (100-500ms) */
  Medium = 'medium',
  /** Low latency (<100ms) */
  Low = 'low',
  /** Ultra-low latency (<10ms) */
  UltraLow = 'ultra-low',
}

// Platform Features
export enum PlatformFeature {
  /** Multitasking support */
  Multitasking = 'multitasking',
  /** Real-time capabilities */
  RealTime = 'real-time',
  /** File system access */
  FileSystem = 'file-system',
  /** Memory mapped files */
  MemoryMappedFiles = 'memory-mapped-files',
  /** Threading support */
  Threading = 'threading',
  /** Atomic operations */
  Atomics = 'atomics',
  /** Exception handling */
  Exceptions = 'exceptions',
  /** Dynamic linking */
  DynamicLinking = 'dynamic-linking',
  /** JIT compilation */
  JITCompilation = 'jit-compilation',
  /** Garbage collection */
  GarbageCollection = 'garbage-collection',
  /** Audio playback */
  AudioPlayback = 'audio-playback',
  /** Audio recording */
  AudioRecording = 'audio-recording',
  /** Video playback */
  VideoPlayback = 'video-playback',
  /** Camera access */
  Camera = 'camera',
  /** Sensors */
  Sensors = 'sensors',
  /** GPS/location */
  Location = 'location',
  /** Push notifications */
  PushNotifications = 'push-notifications',
  /** Background processing */
  BackgroundProcessing = 'background-processing',
}

// V6R VM Size Configurations
export enum V6RVmSize {
  /** 512MB development VMs */
  Small = 'small',
  /** 1GB standard VMs */
  Medium = 'medium',
  /** 2GB+ intensive VMs */
  Large = 'large',
  /** Custom VM configuration */
  Custom = 'custom',
}

// Type Definitions

export interface Resolution {
  width: number;
  height: number;
}

export interface CpuCacheInfo {
  /** L1 cache size */
  l1Size: MemorySize;
  /** L2 cache size */
  l2Size: MemorySize;
  /** L3 cache size (if available) */
  l3Size?: MemorySize;
}

export interface PlatformMemoryInfo {
  /** Total available memory */
  totalMemory: MemorySize;
  /** Memory available for applications */
  availableMemory: MemorySize;
  /** Memory reserved for system */
  systemReserved: MemorySize;
  /** Stack memory limit */
  stackLimit: MemorySize;
  /** Heap memory limit */
  heapLimit: MemorySize;
  /** Whether virtual memory is available */
  hasVirtualMemory: boolean;
  /** Memory management unit present */
  hasMMU: boolean;
  /** DMA capabilities */
  hasDMA: boolean;
}

export interface PlatformCpuInfo {
  /** CPU architecture */
  architecture: CpuArchitecture;
  /** Core count */
  coreCount: number;
  /** Approximate clock speed (MHz) */
  clockSpeedMhz: number;
  /** Instruction sets supported */
  instructionSets: InstructionSet[];
  /** Cache sizes */
  cacheInfo: CpuCacheInfo;
  /** Floating point support */
  hasFPU: boolean;
  /** SIMD support */
  hasSIMD: boolean;
}

export interface PlatformGraphicsInfo {
  /** Graphics API support */
  apis: GraphicsApi[];
  /** Maximum texture size */
  maxTextureSize: number;
  /** Video memory */
  videoMemory: MemorySize;
  /** Hardware acceleration */
  hasHardwareAccel: boolean;
  /** Shader support */
  shaderSupport: ShaderSupportLevel;
  /** Color depth */
  colorDepth: number;
  /** Maximum resolution */
  maxResolution: Resolution;
}

export interface PlatformStorageInfo {
  /** Storage types available */
  storageTypes: StorageType[];
  /** Read speed (MB/s) */
  readSpeedMbps: number;
  /** Write speed (MB/s) */
  writeSpeedMbps: number;
  /** Total storage capacity */
  totalCapacity: MemorySize;
  /** Available storage */
  availableCapacity: MemorySize;
  /** Supports random access */
  hasRandomAccess: boolean;
}

export interface PlatformNetworkInfo {
  /** Network interfaces available */
  interfaces: NetworkInterface[];
  /** Maximum bandwidth (Mbps) */
  maxBandwidthMbps: number;
  /** Latency characteristics */
  latencyClass: NetworkLatencyClass;
  /** Supports TCP/IP */
  hasTcpIp: boolean;
  /** Supports UDP */
  hasUDP: boolean;
  /** Supports WebSockets */
  hasWebSockets: boolean;
  /** Supports TLS/SSL */
  hasTLS: boolean;
}

export interface PlatformBuildConfig {
  /** Target triple */
  targetTriple: string;
  /** Compiler toolchain */
  toolchain: string;
  /** Compilation flags */
  compileFlags: string[];
  /** Linker flags */
  linkFlags: string[];
  /** Required libraries */
  requiredLibs: string[];
  /** Asset processing pipeline */
  assetPipeline: string;
  /** Packaging format */
  packageFormat: string;
  /** Deployment method */
  deploymentMethod: string;
}

export interface V6RPlatformExtensions {
  /** VM size configuration */
  vmSize: V6RVmSize;
  /** Container runtime */
  containerRuntime: string;
  /** Orchestration platform */
  orchestration: string;
  /** Scaling capabilities */
  autoScaling: boolean;
  /** Load balancing */
  loadBalancing: boolean;
  /** Service mesh integration */
  serviceMesh: boolean;
  /** Monitoring integration */
  monitoring: boolean;
}

export interface PlatformInfo {
  /** Platform identifier */
  id: string;
  /** Human-readable platform name */
  name: string;
  /** Platform category */
  category: PlatformCategory;
  /** Capability level */
  capability: PlatformCapability;
  /** Memory constraints */
  memory: PlatformMemoryInfo;
  /** CPU characteristics */
  cpu: PlatformCpuInfo;
  /** Graphics capabilities */
  graphics: PlatformGraphicsInfo;
  /** Storage characteristics */
  storage: PlatformStorageInfo;
  /** Network capabilities */
  network: PlatformNetworkInfo;
  /** Supported features */
  features: PlatformFeature[];
  /** Build configuration */
  buildConfig: PlatformBuildConfig;
  /** V6R-specific extensions (optional) */
  v6rExtensions?: V6RPlatformExtensions;
}

// Platform Detection Types

export enum PlatformAdaptationStrategy {
  /** Use full feature implementation */
  FullImplementation = 'full-implementation',
  /** Use reduced feature set */
  ReducedImplementation = 'reduced-implementation',
  /** Use alternative implementation */
  AlternativeImplementation = 'alternative-implementation',
  /** Feature not available */
  NotAvailable = 'not-available',
  /** Defer to runtime detection */
  RuntimeDetection = 'runtime-detection',
}

export enum MemoryAllocationStrategy {
  /** Conservative allocation (Dreamcast) */
  Conservative = 'conservative',
  /** Balanced allocation (PSP) */
  Balanced = 'balanced',
  /** Aggressive allocation (modern) */
  Aggressive = 'aggressive',
  /** Dynamic allocation (cloud) */
  Dynamic = 'dynamic',
}

export enum GraphicsRenderingStrategy {
  /** Software rendering */
  Software = 'software',
  /** Hardware-accelerated */
  Hardware = 'hardware',
  /** Hybrid approach */
  Hybrid = 'hybrid',
  /** Cloud rendering */
  Cloud = 'cloud',
}

export enum AssetLoadingStrategy {
  /** Load everything upfront */
  Preload = 'preload',
  /** Load on demand */
  OnDemand = 'on-demand',
  /** Streaming loading */
  Streaming = 'streaming',
  /** Progressive loading */
  Progressive = 'progressive',
}

export enum ThreadingStrategy {
  /** Single-threaded */
  SingleThreaded = 'single-threaded',
  /** Multi-threaded */
  MultiThreaded = 'multi-threaded',
  /** Actor model */
  ActorModel = 'actor-model',
  /** Async/await */
  AsyncAwait = 'async-await',
}

export interface PlatformConfig {
  /** Memory allocation strategy */
  memoryStrategy: MemoryAllocationStrategy;
  /** Rendering strategy */
  graphicsStrategy: GraphicsRenderingStrategy;
  /** Asset loading strategy */
  assetStrategy: AssetLoadingStrategy;
  /** Threading strategy */
  threadingStrategy: ThreadingStrategy;
}

// Platform Matrix Types

export enum CompatibilityIssueCategory {
  /** Memory constraint violation */
  MemoryConstraint = 'memory-constraint',
  /** Missing platform feature */
  MissingFeature = 'missing-feature',
  /** Performance degradation */
  Performance = 'performance',
  /** API compatibility */
  ApiCompatibility = 'api-compatibility',
  /** Build system incompatibility */
  BuildSystem = 'build-system',
}

export enum CompatibilityIssueSeverity {
  /** Informational only */
  Info = 'info',
  /** Minor issue - workaround available */
  Minor = 'minor',
  /** Major issue - significant impact */
  Major = 'major',
  /** Blocking issue - prevents deployment */
  Blocking = 'blocking',
}

export interface CompatibilityIssue {
  /** Issue category */
  category: CompatibilityIssueCategory;
  /** Issue severity */
  severity: CompatibilityIssueSeverity;
  /** Issue description */
  description: string;
  /** Suggested resolution */
  resolution?: string;
}

export interface CompatibilityReport {
  /** Overall compatibility status */
  compatible: boolean;
  /** Compatibility issues found */
  issues: CompatibilityIssue[];
  /** Recommended adaptations */
  adaptations: string[];
  /** Performance impact estimate */
  performanceImpact: number;
}

// Platform Interfaces (matching WIT interface definitions)

export interface PlatformDetection {
  /** Detect current platform */
  detectPlatform(): Promise<FlightResult<PlatformInfo>>;
  
  /** Check if feature is supported */
  hasFeature(feature: PlatformFeature): boolean;
  
  /** Get platform capability level */
  getCapabilityLevel(): PlatformCapability;
  
  /** Adapt feature for platform */
  adaptFeature(
    feature: PlatformFeature,
    requestedLevel: PlatformCapability
  ): PlatformAdaptationStrategy;
  
  /** Get platform-specific configuration */
  getPlatformConfig(): Promise<FlightResult<PlatformConfig>>;
}

export interface PlatformMatrix {
  /** Get all supported platforms */
  getSupportedPlatforms(): PlatformInfo[];
  
  /** Filter platforms by capability */
  filterByCapability(minCapability: PlatformCapability): PlatformInfo[];
  
  /** Get platforms by category */
  getPlatformsByCategory(category: PlatformCategory): PlatformInfo[];
  
  /** Check platform compatibility */
  checkCompatibility(
    sourcePlatform: string,
    targetPlatform: string
  ): Promise<FlightResult<CompatibilityReport>>;
}

// All types are already exported above through interface declarations
