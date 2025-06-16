/** @module Interface flight:memory/memory-types@1.0.0 **/
/**
 * Memory size representation with human-readable formatting
 * 
 * Used throughout both ecosystems for consistent memory measurement
 */
export interface MemorySize {
  /**
   * Raw byte count - allows precise calculations
   */
  bytes: bigint,
  /**
   * Human-readable format: "16MB", "512KB", "2GB", etc.
   * Critical for V6R UI components and logging
   */
  humanReadable: string,
}
/**
 * Memory usage snapshot for real-time monitoring
 * 
 * V6R uses this for WebSocket memory updates and UI display
 * Flight-Core uses this for platform memory adaptation
 */
export interface MemoryUsageSnapshot {
  /**
   * Timestamp when snapshot was taken (Unix timestamp)
   */
  timestamp: bigint,
  /**
   * Session identifier (V6R session or Flight-Core component instance)
   */
  sessionId: string,
  /**
   * Platform identifier for memory constraints
   */
  platform: string,
  /**
   * Total available memory for this context
   */
  total: MemorySize,
  /**
   * Currently used memory
   */
  used: MemorySize,
  /**
   * Available memory (calculated: total - used)
   */
  available: MemorySize,
  /**
   * Memory fragmentation ratio (0.0-1.0)
   * Critical for Flight-Core constrained platforms
   */
  fragmentationRatio: number,
}
/**
 * Platform-specific memory profiles
 * 
 * Enables both Flight-Core and V6R to adapt behavior based on memory constraints
 */
export type PlatformProfile = PlatformProfileDreamcast | PlatformProfilePsp | PlatformProfileVita | PlatformProfileV6rSmall | PlatformProfileV6rMedium | PlatformProfileV6rLarge | PlatformProfileCustom;
/**
 * Dreamcast: 16MB baseline (PlayStation 1 dropped for C++17)
 */
export interface PlatformProfileDreamcast {
  tag: 'dreamcast',
  val: MemorySize,
}
/**
 * PlayStation Portable: 32-64MB depending on model
 */
export interface PlatformProfilePsp {
  tag: 'psp',
  val: MemorySize,
}
/**
 * Sony PlayStation Vita: 512MB
 */
export interface PlatformProfileVita {
  tag: 'vita',
  val: MemorySize,
}
/**
 * V6R Small VM: 512MB for lightweight development
 */
export interface PlatformProfileV6rSmall {
  tag: 'v6r-small',
  val: MemorySize,
}
/**
 * V6R Medium VM: 1GB for standard development
 */
export interface PlatformProfileV6rMedium {
  tag: 'v6r-medium',
  val: MemorySize,
}
/**
 * V6R Large VM: 2GB+ for intensive development
 */
export interface PlatformProfileV6rLarge {
  tag: 'v6r-large',
  val: MemorySize,
}
/**
 * Custom platform with specified memory
 */
export interface PlatformProfileCustom {
  tag: 'custom',
  val: MemorySize,
}
/**
 * Memory allocation purposes for categorization
 * 
 * Enables both systems to track memory usage by category
 * # Variants
 * 
 * ## `"vm-heap"`
 * 
 * VM heap memory (V6R primary use case)
 * ## `"component-stack"`
 * 
 * Component stack memory (Flight-Core components)
 * ## `"asset-cache"`
 * 
 * Asset cache (textures, audio, etc.)
 * ## `"jit-code-cache"`
 * 
 * JIT compiled code cache
 * ## `"system-reserved"`
 * 
 * System reserved memory
 * ## `"wasm-linear"`
 * 
 * WebAssembly linear memory
 * ## `"network-buffers"`
 * 
 * Network buffers
 * ## `"temporary"`
 * 
 * Temporary/scratch memory
 */
export type MemoryPurpose = 'vm-heap' | 'component-stack' | 'asset-cache' | 'jit-code-cache' | 'system-reserved' | 'wasm-linear' | 'network-buffers' | 'temporary';
/**
 * Memory allocation tracking record
 * 
 * V6R uses this for VM resource accounting
 * Flight-Core uses this for component memory management
 */
export interface MemoryAllocation {
  /**
   * Unique allocation identifier
   */
  id: string,
  /**
   * Session this allocation belongs to
   */
  sessionId: string,
  /**
   * Size of the allocation
   */
  size: MemorySize,
  /**
   * Purpose/category of this allocation
   */
  purpose: MemoryPurpose,
  /**
   * When allocation was created (Unix timestamp)
   */
  allocatedAt: bigint,
  /**
   * When allocation was freed (None if still active)
   */
  freedAt?: bigint,
}
/**
 * Memory pressure levels for adaptive behavior
 * 
 * Flight-Core uses for platform adaptation
 * V6R uses for VM scaling decisions
 * # Variants
 * 
 * ## `"low"`
 * 
 * Plenty of memory available
 * ## `"medium"`
 * 
 * Memory usage getting high but manageable
 * ## `"high"`
 * 
 * Memory critically low - aggressive cleanup needed
 * ## `"critical"`
 * 
 * Out of memory - emergency measures required
 */
export type MemoryPressure = 'low' | 'medium' | 'high' | 'critical';
/**
 * Memory limits configuration
 * 
 * V6R uses for VM resource enforcement
 * Flight-Core uses for platform constraint adaptation
 */
export interface MemoryLimits {
  /**
   * Maximum memory for VM/component heap
   */
  heapMax: MemorySize,
  /**
   * Maximum memory for stack
   */
  stackMax: MemorySize,
  /**
   * Maximum memory for caching
   */
  cacheMax: MemorySize,
  /**
   * Soft limit before warnings
   */
  softLimit: MemorySize,
  /**
   * Hard limit before allocation failures
   */
  hardLimit: MemorySize,
}
/**
 * Memory error codes
 * # Variants
 * 
 * ## `"insufficient-memory"`
 * 
 * Insufficient memory available
 * ## `"limit-exceeded"`
 * 
 * Memory limit exceeded
 * ## `"invalid-size"`
 * 
 * Invalid memory size requested
 * ## `"allocation-failed"`
 * 
 * Memory allocation failed
 * ## `"already-freed"`
 * 
 * Memory already freed
 * ## `"invalid-allocation"`
 * 
 * Invalid allocation ID
 * ## `"unsupported-platform"`
 * 
 * Platform not supported
 * ## `"fragmentation-error"`
 * 
 * Memory fragmentation too high
 */
export type MemoryErrorCode = 'insufficient-memory' | 'limit-exceeded' | 'invalid-size' | 'allocation-failed' | 'already-freed' | 'invalid-allocation' | 'unsupported-platform' | 'fragmentation-error';
/**
 * Memory-specific error types
 * 
 * Comprehensive error handling for memory operations
 */
export interface MemoryError {
  /**
   * Error code for programmatic handling
   */
  code: MemoryErrorCode,
  /**
   * Human-readable error message
   */
  message: string,
  /**
   * Additional context/details
   */
  details?: string,
  /**
   * When error occurred
   */
  timestamp: bigint,
}
