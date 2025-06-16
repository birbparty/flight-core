// Enhanced Flight Memory Types - Generic Implementation
// Production-ready types with enhanced type safety and utilities

export interface MemorySize {
  readonly bytes: bigint;
  readonly humanReadable: string;
}

export interface MemoryUsageSnapshot {
  readonly timestamp: number;
  readonly sessionId: string;
  readonly platform: string;
  readonly total: MemorySize;
  readonly used: MemorySize;
  readonly available: MemorySize;
  readonly fragmentationRatio: number;
}

export type PlatformProfile = 
  | { readonly tag: "dreamcast"; readonly val: MemorySize }
  | { readonly tag: "psp"; readonly val: MemorySize }
  | { readonly tag: "vita"; readonly val: MemorySize }
  | { readonly tag: "v6r-small"; readonly val: MemorySize }
  | { readonly tag: "v6r-medium"; readonly val: MemorySize }
  | { readonly tag: "v6r-large"; readonly val: MemorySize }
  | { readonly tag: "custom"; readonly val: MemorySize };

export interface MemoryAllocation {
  readonly id: string;
  readonly sessionId: string;
  readonly size: MemorySize;
  readonly purpose: MemoryPurpose;
  readonly allocatedAt: number;
  readonly freedAt?: number;
}

export enum MemoryPurpose {
  VmHeap = "vm-heap",
  ComponentStack = "component-stack",
  AssetCache = "asset-cache",
  JitCodeCache = "jit-code-cache",
  SystemReserved = "system-reserved",
  WasmLinear = "wasm-linear",
  NetworkBuffers = "network-buffers",
  Temporary = "temporary"
}

export enum MemoryPressure {
  Low = "low",
  Medium = "medium", 
  High = "high",
  Critical = "critical"
}

export interface MemoryLimits {
  readonly heapMax: MemorySize;
  readonly stackMax: MemorySize;
  readonly cacheMax: MemorySize;
  readonly softLimit: MemorySize;
  readonly hardLimit: MemorySize;
}

export interface MemoryError {
  readonly code: MemoryErrorCode;
  readonly message: string;
  readonly details?: string;
  readonly timestamp: number;
}

export enum MemoryErrorCode {
  InsufficientMemory = "insufficient-memory",
  LimitExceeded = "limit-exceeded",
  InvalidSize = "invalid-size",
  AllocationFailed = "allocation-failed",
  AlreadyFreed = "already-freed",
  InvalidAllocation = "invalid-allocation",
  UnsupportedPlatform = "unsupported-platform",
  FragmentationError = "fragmentation-error"
}

export type MemoryResult<T> = 
  | { readonly tag: "ok"; readonly val: T }
  | { readonly tag: "err"; readonly val: MemoryError };

// Generic configuration types
export interface SessionConfig {
  readonly sessionId: string;
  readonly userId: string;
  readonly platform: string;
  readonly customLimits?: MemoryLimits;
}

export interface MemoryUpdate {
  readonly type: "memory_update";
  readonly sessionId: string;
  readonly snapshot: MemoryUsageSnapshot;
  readonly timestamp: number;
}

// Memory analytics types
export interface MemoryStats {
  readonly totalAllocations: number;
  readonly activeAllocations: number;
  readonly peakMemory: MemorySize;
  readonly currentMemory: MemorySize;
  readonly averageAllocationSize: MemorySize;
  readonly usageByPurpose: ReadonlyArray<readonly [MemoryPurpose, MemorySize]>;
  readonly efficiencyRatio: number;
}

export interface MemoryTrend {
  readonly snapshots: ReadonlyArray<MemoryUsageSnapshot>;
  readonly trendDirection: TrendDirection;
  readonly predictedPeak?: MemorySize;
}

export enum TrendDirection {
  Increasing = "increasing",
  Decreasing = "decreasing", 
  Stable = "stable",
  Volatile = "volatile"
}

// Event system types
export interface MemoryEventData {
  readonly sessionId: string;
  readonly timestamp: number;
}

export interface AllocationEventData extends MemoryEventData {
  readonly allocation: MemoryAllocation;
}

export interface PressureEventData extends MemoryEventData {
  readonly pressure: MemoryPressure;
  readonly snapshot: MemoryUsageSnapshot;
}

export interface LimitEventData extends MemoryEventData {
  readonly limitType: "soft" | "hard";
  readonly currentUsage: MemorySize;
  readonly limit: MemorySize;
}

export type MemoryEventHandler<T extends MemoryEventData = MemoryEventData> = (data: T) => void;
