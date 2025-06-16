// Generic Memory Utilities - Universal Flight Memory Operations
// Production-ready utilities for any application using Flight memory types

import {
  MemorySize,
  MemoryUsageSnapshot,
  MemoryPressure,
  MemoryUpdate,
  SessionConfig,
  PlatformProfile
} from '../types';

export class MemoryUtils {
  /**
   * Format bytes for UI display with proper unit selection
   */
  static formatBytesForUI(bytes: bigint): string {
    const units = ['B', 'KB', 'MB', 'GB', 'TB'];
    let size = Number(bytes);
    let unitIndex = 0;
    
    while (size >= 1024 && unitIndex < units.length - 1) {
      size /= 1024;
      unitIndex++;
    }
    
    return unitIndex === 0 
      ? `${size}${units[unitIndex]}`
      : `${size.toFixed(1)}${units[unitIndex]}`;
  }

  /**
   * Create MemorySize from bytes with proper formatting
   */
  static createMemorySize(bytes: bigint): MemorySize {
    return {
      bytes,
      humanReadable: this.formatBytesForUI(bytes)
    };
  }

  /**
   * Parse human-readable memory size to bytes
   */
  static parseMemorySize(humanReadable: string): bigint {
    const match = humanReadable.match(/^(\d+(?:\.\d+)?)\s*([KMGT]?B)$/i);
    if (!match) {
      throw new Error(`Invalid memory size format: ${humanReadable}`);
    }

    const [, numStr, unit] = match;
    const num = parseFloat(numStr);
    
    const multipliers: Record<string, number> = {
      'B': 1,
      'KB': 1024,
      'MB': 1024 * 1024,
      'GB': 1024 * 1024 * 1024,
      'TB': 1024 * 1024 * 1024 * 1024
    };

    const multiplier = multipliers[unit.toUpperCase()] || 1;
    return BigInt(Math.floor(num * multiplier));
  }

  /**
   * Calculate memory usage percentage
   */
  static calculateUsagePercentage(snapshot: MemoryUsageSnapshot): number {
    if (snapshot.total.bytes === 0n) {
      return 0;
    }
    return Number(snapshot.used.bytes) / Number(snapshot.total.bytes) * 100;
  }

  /**
   * Get memory pressure color for UI components
   */
  static getMemoryPressureColor(pressure: MemoryPressure): string {
    switch (pressure) {
      case MemoryPressure.Critical:
        return '#dc3545'; // Red
      case MemoryPressure.High:
        return '#fd7e14'; // Orange
      case MemoryPressure.Medium:
        return '#ffc107'; // Yellow
      case MemoryPressure.Low:
      default:
        return '#28a745'; // Green
    }
  }

  /**
   * Get memory pressure from percentage
   */
  static getMemoryPressureFromPercentage(percentage: number): MemoryPressure {
    if (percentage >= 95) return MemoryPressure.Critical;
    if (percentage >= 85) return MemoryPressure.High;
    if (percentage >= 70) return MemoryPressure.Medium;
    return MemoryPressure.Low;
  }

  /**
   * Create memory update message for real-time streaming
   */
  static createMemoryUpdate(
    sessionId: string,
    snapshot: MemoryUsageSnapshot
  ): MemoryUpdate {
    return {
      type: "memory_update",
      sessionId,
      snapshot,
      timestamp: Date.now()
    };
  }

  /**
   * Validate session configuration
   */
  static validateSessionConfig(config: SessionConfig): boolean {
    return (
      config.sessionId.length > 0 &&
      config.userId.length > 0 &&
      config.platform.length > 0
    );
  }

  /**
   * Get platform display name
   */
  static getPlatformDisplayName(platform: string): string {
    const platformNames: Record<string, string> = {
      'dreamcast': 'Sega Dreamcast',
      'psp': 'PlayStation Portable',
      'vita': 'PlayStation Vita',
      'v6r-small': 'Small VM (512MB)',
      'v6r-medium': 'Medium VM (1GB)',
      'v6r-large': 'Large VM (2GB+)',
      'custom': 'Custom Platform'
    };
    
    return platformNames[platform] || platform;
  }

  /**
   * Extract memory size from platform profile
   */
  static getMemorySizeFromProfile(profile: PlatformProfile): MemorySize {
    return profile.val;
  }

  /**
   * Check if memory usage is approaching limits
   */
  static isApproachingLimit(
    current: MemorySize,
    limit: MemorySize,
    thresholdPercent: number = 90
  ): boolean {
    const percentage = Number(current.bytes) / Number(limit.bytes) * 100;
    return percentage >= thresholdPercent;
  }

  /**
   * Calculate available memory
   */
  static calculateAvailableMemory(total: MemorySize, used: MemorySize): MemorySize {
    const availableBytes = total.bytes - used.bytes;
    return this.createMemorySize(availableBytes >= 0n ? availableBytes : 0n);
  }

  /**
   * Compare two memory sizes
   */
  static compareMemorySize(a: MemorySize, b: MemorySize): number {
    if (a.bytes < b.bytes) return -1;
    if (a.bytes > b.bytes) return 1;
    return 0;
  }

  /**
   * Add two memory sizes
   */
  static addMemorySize(a: MemorySize, b: MemorySize): MemorySize {
    return this.createMemorySize(a.bytes + b.bytes);
  }

  /**
   * Subtract two memory sizes
   */
  static subtractMemorySize(a: MemorySize, b: MemorySize): MemorySize {
    const result = a.bytes - b.bytes;
    return this.createMemorySize(result >= 0n ? result : 0n);
  }

  /**
   * Get memory efficiency ratio (0-1 scale)
   */
  static calculateEfficiency(
    used: MemorySize,
    allocated: MemorySize
  ): number {
    if (allocated.bytes === 0n) return 1;
    return Number(used.bytes) / Number(allocated.bytes);
  }

  /**
   * Check if platform supports specific memory size
   */
  static isPlatformCompatible(platform: string, requiredMemory: MemorySize): boolean {
    const platformLimits: Record<string, bigint> = {
      'dreamcast': BigInt(16 * 1024 * 1024), // 16MB
      'psp': BigInt(64 * 1024 * 1024), // 64MB
      'vita': BigInt(512 * 1024 * 1024), // 512MB
      'v6r-small': BigInt(512 * 1024 * 1024), // 512MB
      'v6r-medium': BigInt(1024 * 1024 * 1024), // 1GB
      'v6r-large': BigInt(2 * 1024 * 1024 * 1024), // 2GB
    };

    const limit = platformLimits[platform];
    return limit ? requiredMemory.bytes <= limit : true; // Default to true for custom platforms
  }

  /**
   * Generate memory summary text
   */
  static generateMemorySummary(snapshot: MemoryUsageSnapshot): string {
    const percentage = this.calculateUsagePercentage(snapshot);
    const pressure = this.getMemoryPressureFromPercentage(percentage);
    const platformName = this.getPlatformDisplayName(snapshot.platform);
    
    return `${platformName}: ${snapshot.used.humanReadable} / ${snapshot.total.humanReadable} (${percentage.toFixed(1)}%) - ${pressure.toUpperCase()} pressure`;
  }
}

/**
 * Memory Event Emitter for real-time updates
 * Generic implementation suitable for any application
 */
export class MemoryEventEmitter {
  private listeners: Map<string, Set<(update: MemoryUpdate) => void>> = new Map();
  private globalListeners: Set<(update: MemoryUpdate) => void> = new Set();

  /**
   * Subscribe to memory updates for a specific session
   */
  subscribe(
    sessionId: string, 
    callback: (update: MemoryUpdate) => void
  ): () => void {
    if (!this.listeners.has(sessionId)) {
      this.listeners.set(sessionId, new Set());
    }
    
    this.listeners.get(sessionId)!.add(callback);
    
    // Return unsubscribe function
    return () => {
      const callbacks = this.listeners.get(sessionId);
      if (callbacks) {
        callbacks.delete(callback);
        if (callbacks.size === 0) {
          this.listeners.delete(sessionId);
        }
      }
    };
  }

  /**
   * Subscribe to all memory updates (global listener)
   */
  subscribeGlobal(callback: (update: MemoryUpdate) => void): () => void {
    this.globalListeners.add(callback);
    
    return () => {
      this.globalListeners.delete(callback);
    };
  }

  /**
   * Emit memory update to subscribers
   */
  emit(update: MemoryUpdate): void {
    // Emit to session-specific listeners
    const callbacks = this.listeners.get(update.sessionId);
    if (callbacks) {
      callbacks.forEach(callback => {
        try {
          callback(update);
        } catch (error) {
          console.error('Error in memory update callback:', error);
        }
      });
    }

    // Emit to global listeners
    this.globalListeners.forEach(callback => {
      try {
        callback(update);
      } catch (error) {
        console.error('Error in global memory update callback:', error);
      }
    });
  }

  /**
   * Get subscriber count for session
   */
  getSubscriberCount(sessionId: string): number {
    return this.listeners.get(sessionId)?.size ?? 0;
  }

  /**
   * Get total subscriber count (including global)
   */
  getTotalSubscriberCount(): number {
    let total = this.globalListeners.size;
    this.listeners.forEach(callbacks => {
      total += callbacks.size;
    });
    return total;
  }

  /**
   * Clear all listeners
   */
  clear(): void {
    this.listeners.clear();
    this.globalListeners.clear();
  }

  /**
   * Get all active session IDs
   */
  getActiveSessions(): string[] {
    return Array.from(this.listeners.keys());
  }
}
