/** @module Interface flight:memory/memory-operations@1.0.0 **/
/**
 * Create a new memory allocation record
 * 
 * V6R calls this when allocating VM memory
 * Flight-Core calls this for component memory tracking
 */
export function createAllocation(sessionId: string, size: MemorySize, purpose: MemoryPurpose): MemoryAllocation;
/**
 * Free a memory allocation
 * 
 * Marks allocation as freed and updates tracking
 */
export function freeAllocation(allocationId: string): boolean;
/**
 * Get current memory usage snapshot
 * 
 * V6R uses for real-time UI updates and WebSocket streaming
 * Flight-Core uses for platform memory monitoring
 */
export function getMemorySnapshot(sessionId: string): MemoryUsageSnapshot;
/**
 * Get platform memory profile
 * 
 * Returns memory constraints and capabilities for platform
 */
export function getPlatformProfile(platform: string): PlatformProfile;
/**
 * Set memory limits for session
 * 
 * V6R uses for VM resource enforcement
 * Flight-Core uses for component constraint setting
 */
export function setMemoryLimits(sessionId: string, limits: MemoryLimits): boolean;
/**
 * Get memory limits for session
 */
export function getMemoryLimits(sessionId: string): MemoryLimits;
/**
 * Check current memory pressure level
 * 
 * Enables adaptive behavior based on memory availability
 */
export function getMemoryPressure(sessionId: string): MemoryPressure;
/**
 * Validate memory allocation request
 * 
 * Checks if allocation would succeed without actually allocating
 * Critical for V6R VM sizing decisions
 */
export function validateAllocationRequest(sessionId: string, size: MemorySize, purpose: MemoryPurpose): boolean;
/**
 * List all active allocations for session
 * 
 * V6R uses for memory usage breakdowns
 * Flight-Core uses for debugging and monitoring
 */
export function listAllocations(sessionId: string): Array<MemoryAllocation>;
export type MemorySize = import('./flight-memory-memory-types.js').MemorySize;
export type MemoryAllocation = import('./flight-memory-memory-types.js').MemoryAllocation;
export type MemoryPurpose = import('./flight-memory-memory-types.js').MemoryPurpose;
export type MemoryUsageSnapshot = import('./flight-memory-memory-types.js').MemoryUsageSnapshot;
export type MemoryError = import('./flight-memory-memory-types.js').MemoryError;
export type MemoryLimits = import('./flight-memory-memory-types.js').MemoryLimits;
export type MemoryPressure = import('./flight-memory-memory-types.js').MemoryPressure;
export type PlatformProfile = import('./flight-memory-memory-types.js').PlatformProfile;
