/** @module Interface flight:memory/memory-analytics@1.0.0 **/
/**
 * Calculate memory statistics
 * 
 * Provides detailed memory usage analytics
 */
export function calculateMemoryStats(sessionId: string): MemoryStats;
/**
 * Get memory usage trends over time
 * 
 * V6R uses for capacity planning and scaling decisions
 */
export function getMemoryTrends(sessionId: string, timeWindow: bigint): MemoryTrend;
/**
 * Generate memory usage report
 * 
 * Comprehensive report for analysis and debugging
 */
export function generateMemoryReport(sessionId: string): string;
export type MemorySize = import('./flight-memory-memory-types.js').MemorySize;
export type MemoryPurpose = import('./flight-memory-memory-types.js').MemoryPurpose;
export type MemoryUsageSnapshot = import('./flight-memory-memory-types.js').MemoryUsageSnapshot;
export type MemoryError = import('./flight-memory-memory-types.js').MemoryError;
/**
 * Comprehensive memory statistics
 */
export interface MemoryStats {
  /**
   * Total allocations made
   */
  totalAllocations: bigint,
  /**
   * Active allocations count
   */
  activeAllocations: bigint,
  /**
   * Peak memory usage recorded
   */
  peakMemory: MemorySize,
  /**
   * Current memory usage
   */
  currentMemory: MemorySize,
  /**
   * Average allocation size
   */
  averageAllocationSize: MemorySize,
  /**
   * Memory usage by purpose
   */
  usageByPurpose: Array<[MemoryPurpose, MemorySize]>,
  /**
   * Memory efficiency ratio (0.0-1.0)
   */
  efficiencyRatio: number,
}
/**
 * Trend direction enumeration
 * # Variants
 * 
 * ## `"increasing"`
 * 
 * ## `"decreasing"`
 * 
 * ## `"stable"`
 * 
 * ## `"volatile"`
 */
export type TrendDirection = 'increasing' | 'decreasing' | 'stable' | 'volatile';
/**
 * Memory usage trend data
 */
export interface MemoryTrend {
  /**
   * Data points over time
   */
  snapshots: Array<MemoryUsageSnapshot>,
  /**
   * Trend direction (increasing/decreasing/stable)
   */
  trendDirection: TrendDirection,
  /**
   * Predicted future usage
   */
  predictedPeak?: MemorySize,
}
