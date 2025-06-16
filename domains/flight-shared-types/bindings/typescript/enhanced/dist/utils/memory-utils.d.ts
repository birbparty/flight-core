import { MemorySize, MemoryUsageSnapshot, MemoryPressure, MemoryUpdate, SessionConfig, PlatformProfile } from '../types';
export declare class MemoryUtils {
    /**
     * Format bytes for UI display with proper unit selection
     */
    static formatBytesForUI(bytes: bigint): string;
    /**
     * Create MemorySize from bytes with proper formatting
     */
    static createMemorySize(bytes: bigint): MemorySize;
    /**
     * Parse human-readable memory size to bytes
     */
    static parseMemorySize(humanReadable: string): bigint;
    /**
     * Calculate memory usage percentage
     */
    static calculateUsagePercentage(snapshot: MemoryUsageSnapshot): number;
    /**
     * Get memory pressure color for UI components
     */
    static getMemoryPressureColor(pressure: MemoryPressure): string;
    /**
     * Get memory pressure from percentage
     */
    static getMemoryPressureFromPercentage(percentage: number): MemoryPressure;
    /**
     * Create memory update message for real-time streaming
     */
    static createMemoryUpdate(sessionId: string, snapshot: MemoryUsageSnapshot): MemoryUpdate;
    /**
     * Validate session configuration
     */
    static validateSessionConfig(config: SessionConfig): boolean;
    /**
     * Get platform display name
     */
    static getPlatformDisplayName(platform: string): string;
    /**
     * Extract memory size from platform profile
     */
    static getMemorySizeFromProfile(profile: PlatformProfile): MemorySize;
    /**
     * Check if memory usage is approaching limits
     */
    static isApproachingLimit(current: MemorySize, limit: MemorySize, thresholdPercent?: number): boolean;
    /**
     * Calculate available memory
     */
    static calculateAvailableMemory(total: MemorySize, used: MemorySize): MemorySize;
    /**
     * Compare two memory sizes
     */
    static compareMemorySize(a: MemorySize, b: MemorySize): number;
    /**
     * Add two memory sizes
     */
    static addMemorySize(a: MemorySize, b: MemorySize): MemorySize;
    /**
     * Subtract two memory sizes
     */
    static subtractMemorySize(a: MemorySize, b: MemorySize): MemorySize;
    /**
     * Get memory efficiency ratio (0-1 scale)
     */
    static calculateEfficiency(used: MemorySize, allocated: MemorySize): number;
    /**
     * Check if platform supports specific memory size
     */
    static isPlatformCompatible(platform: string, requiredMemory: MemorySize): boolean;
    /**
     * Generate memory summary text
     */
    static generateMemorySummary(snapshot: MemoryUsageSnapshot): string;
}
/**
 * Memory Event Emitter for real-time updates
 * Generic implementation suitable for any application
 */
export declare class MemoryEventEmitter {
    private listeners;
    private globalListeners;
    /**
     * Subscribe to memory updates for a specific session
     */
    subscribe(sessionId: string, callback: (update: MemoryUpdate) => void): () => void;
    /**
     * Subscribe to all memory updates (global listener)
     */
    subscribeGlobal(callback: (update: MemoryUpdate) => void): () => void;
    /**
     * Emit memory update to subscribers
     */
    emit(update: MemoryUpdate): void;
    /**
     * Get subscriber count for session
     */
    getSubscriberCount(sessionId: string): number;
    /**
     * Get total subscriber count (including global)
     */
    getTotalSubscriberCount(): number;
    /**
     * Clear all listeners
     */
    clear(): void;
    /**
     * Get all active session IDs
     */
    getActiveSessions(): string[];
}
//# sourceMappingURL=memory-utils.d.ts.map