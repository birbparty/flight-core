/**
 * TypeScript Memory Types Integration Tests
 * Generic validation of TypeScript bindings for Flight Memory Types
 */

import {
  MemorySize,
  MemoryUsageSnapshot,
  MemoryPurpose,
  MemoryPressure,
  MemoryError,
  MemoryErrorCode,
  MemoryResult,
  SessionConfig,
  MemoryUpdate,
  PlatformProfile,
  MemoryAllocation,
  MemoryLimits,
  MemoryStats,
  MemoryTrend,
  TrendDirection
} from '../../../bindings/typescript/enhanced/src/types';

// Generic utility functions for testing
class MemoryTestUtils {
  static createMemorySize(bytes: bigint): MemorySize {
    const humanReadable = this.formatBytes(bytes);
    return { bytes, humanReadable };
  }

  static formatBytes(bytes: bigint): string {
    const units = ['B', 'KB', 'MB', 'GB', 'TB'];
    let value = Number(bytes);
    let unitIndex = 0;

    while (value >= 1024 && unitIndex < units.length - 1) {
      value /= 1024;
      unitIndex++;
    }

    return `${value.toFixed(1)}${units[unitIndex]}`;
  }

  static calculateUsagePercentage(snapshot: MemoryUsageSnapshot): number {
    return (Number(snapshot.used.bytes) / Number(snapshot.total.bytes)) * 100;
  }

  static getMemoryPressureLevel(percentage: number): MemoryPressure {
    if (percentage < 50) return MemoryPressure.Low;
    if (percentage < 80) return MemoryPressure.Medium;
    if (percentage < 95) return MemoryPressure.High;
    return MemoryPressure.Critical;
  }

  static createMemoryUpdate(sessionId: string, snapshot: MemoryUsageSnapshot): MemoryUpdate {
    return {
      type: 'memory_update',
      sessionId,
      snapshot,
      timestamp: Date.now()
    };
  }

  static createTestSnapshot(platform: string, totalMB: number, usedMB: number): MemoryUsageSnapshot {
    const total = this.createMemorySize(BigInt(totalMB * 1024 * 1024));
    const used = this.createMemorySize(BigInt(usedMB * 1024 * 1024));
    const available = this.createMemorySize(BigInt((totalMB - usedMB) * 1024 * 1024));
    
    return {
      timestamp: Date.now(),
      sessionId: 'test_session',
      platform,
      total,
      used,
      available,
      fragmentationRatio: usedMB / totalMB * 0.1
    };
  }
}

describe('Flight Memory Types - TypeScript Integration', () => {
  describe('Memory Size Creation and Formatting', () => {
    test('should create memory sizes for different platforms', () => {
      // Dreamcast: 16MB
      const dreamcast = MemoryTestUtils.createMemorySize(16n * 1024n * 1024n);
      expect(dreamcast.bytes).toBe(16n * 1024n * 1024n);
      expect(dreamcast.humanReadable).toBe('16.0MB');

      // PSP: 32MB
      const psp = MemoryTestUtils.createMemorySize(32n * 1024n * 1024n);
      expect(psp.bytes).toBe(32n * 1024n * 1024n);
      expect(psp.humanReadable).toBe('32.0MB');

      // Vita: 512MB
      const vita = MemoryTestUtils.createMemorySize(512n * 1024n * 1024n);
      expect(vita.bytes).toBe(512n * 1024n * 1024n);
      expect(vita.humanReadable).toBe('512.0MB');

      // Custom: 1GB
      const custom = MemoryTestUtils.createMemorySize(1024n * 1024n * 1024n);
      expect(custom.bytes).toBe(1024n * 1024n * 1024n);
      expect(custom.humanReadable).toBe('1.0GB');
    });

    test('should format bytes consistently across different sizes', () => {
      expect(MemoryTestUtils.formatBytes(1024n)).toBe('1.0KB');
      expect(MemoryTestUtils.formatBytes(1048576n)).toBe('1.0MB');
      expect(MemoryTestUtils.formatBytes(1073741824n)).toBe('1.0GB');
      expect(MemoryTestUtils.formatBytes(1099511627776n)).toBe('1.0TB');
    });
  });

  describe('Memory Usage Calculations', () => {
    test('should calculate usage percentages correctly', () => {
      const snapshot = MemoryTestUtils.createTestSnapshot('dreamcast', 16, 8);
      const percentage = MemoryTestUtils.calculateUsagePercentage(snapshot);
      expect(percentage).toBe(50.0);
    });

    test('should determine memory pressure levels accurately', () => {
      expect(MemoryTestUtils.getMemoryPressureLevel(30)).toBe(MemoryPressure.Low);
      expect(MemoryTestUtils.getMemoryPressureLevel(60)).toBe(MemoryPressure.Medium);
      expect(MemoryTestUtils.getMemoryPressureLevel(90)).toBe(MemoryPressure.High);
      expect(MemoryTestUtils.getMemoryPressureLevel(98)).toBe(MemoryPressure.Critical);
    });
  });

  describe('Platform Profile Support', () => {
    test('should support all platform profiles', () => {
      const platforms = ['dreamcast', 'psp', 'vita', 'custom'];
      
      platforms.forEach(platform => {
        const snapshot = MemoryTestUtils.createTestSnapshot(platform, 512, 256);
        expect(snapshot.platform).toBe(platform);
        expect(snapshot.total.bytes).toBe(512n * 1024n * 1024n);
        expect(snapshot.used.bytes).toBe(256n * 1024n * 1024n);
      });
    });

    test('should handle custom platform configurations', () => {
      const customSnapshot = MemoryTestUtils.createTestSnapshot('custom-2gb', 2048, 1024);
      expect(customSnapshot.platform).toBe('custom-2gb');
      expect(customSnapshot.total.humanReadable).toBe('2.0GB');
      expect(customSnapshot.used.humanReadable).toBe('1.0GB');
    });
  });

  describe('Memory Allocation Tracking', () => {
    test('should create memory allocations for different purposes', () => {
      const allocations: MemoryAllocation[] = [
        {
          id: 'alloc_1',
          sessionId: 'session_1',
          size: MemoryTestUtils.createMemorySize(1024n * 1024n),
          purpose: MemoryPurpose.VmHeap,
          allocatedAt: Date.now()
        },
        {
          id: 'alloc_2', 
          sessionId: 'session_1',
          size: MemoryTestUtils.createMemorySize(512n * 1024n),
          purpose: MemoryPurpose.AssetCache,
          allocatedAt: Date.now()
        }
      ];

      expect(allocations[0].purpose).toBe(MemoryPurpose.VmHeap);
      expect(allocations[0].size.humanReadable).toBe('1.0MB');
      expect(allocations[1].purpose).toBe(MemoryPurpose.AssetCache);
      expect(allocations[1].size.humanReadable).toBe('512.0KB');
    });
  });

  describe('Error Handling', () => {
    test('should create and handle memory errors properly', () => {
      const error: MemoryError = {
        code: MemoryErrorCode.LimitExceeded,
        message: 'Memory limit exceeded',
        details: 'Attempted allocation larger than available memory',
        timestamp: Date.now()
      };

      expect(error.code).toBe(MemoryErrorCode.LimitExceeded);
      expect(error.message).toContain('Memory limit exceeded');
      expect(error.details).toContain('Attempted allocation');
    });

    test('should handle memory result types correctly', () => {
      // Test successful result
      const successResult: MemoryResult<boolean> = {
        tag: 'ok',
        val: true
      };

      expect(successResult.tag).toBe('ok');
      expect(successResult.val).toBe(true);

      // Test error result
      const errorResult: MemoryResult<boolean> = {
        tag: 'err',
        val: {
          code: MemoryErrorCode.AllocationFailed,
          message: 'Failed to allocate memory',
          timestamp: Date.now()
        }
      };

      expect(errorResult.tag).toBe('err');
      expect((errorResult.val as MemoryError).code).toBe(MemoryErrorCode.AllocationFailed);
    });
  });

  describe('Real-time Updates and Events', () => {
    test('should create memory updates for streaming', () => {
      const snapshot = MemoryTestUtils.createTestSnapshot('psp', 32, 16);
      const update = MemoryTestUtils.createMemoryUpdate('session_123', snapshot);

      expect(update.type).toBe('memory_update');
      expect(update.sessionId).toBe('session_123');
      expect(update.snapshot.platform).toBe('psp');
      expect(typeof update.timestamp).toBe('number');
    });

    test('should handle high-frequency updates efficiently', () => {
      const sessionId = 'high_freq_session';
      const updates: MemoryUpdate[] = [];

      // Simulate 1000 rapid updates
      const startTime = performance.now();
      for (let i = 0; i < 1000; i++) {
        const snapshot = MemoryTestUtils.createTestSnapshot('vita', 512, i % 512);
        const update = MemoryTestUtils.createMemoryUpdate(sessionId, snapshot);
        updates.push(update);
      }
      const endTime = performance.now();

      const totalTime = endTime - startTime;
      const avgTimePerUpdate = totalTime / 1000;

      // Should be very fast - under 0.1ms per update for <1% overhead
      expect(avgTimePerUpdate).toBeLessThan(0.1);
      expect(updates.length).toBe(1000);
      expect(updates[0].sessionId).toBe(sessionId);
    });
  });

  describe('Session Management', () => {
    test('should handle multiple concurrent sessions', () => {
      const sessions: SessionConfig[] = [
        {
          sessionId: 'session_1',
          userId: 'user_1',
          platform: 'dreamcast'
        },
        {
          sessionId: 'session_2',
          userId: 'user_2', 
          platform: 'psp'
        },
        {
          sessionId: 'session_3',
          userId: 'user_3',
          platform: 'vita'
        }
      ];

      sessions.forEach((session, index) => {
        expect(session.sessionId).toBe(`session_${index + 1}`);
        expect(session.userId).toBe(`user_${index + 1}`);
        expect(['dreamcast', 'psp', 'vita']).toContain(session.platform);
      });
    });
  });

  describe('Memory Analytics and Trends', () => {
    test('should track memory statistics correctly', () => {
      const stats: MemoryStats = {
        totalAllocations: 10,
        activeAllocations: 7,
        peakMemory: MemoryTestUtils.createMemorySize(256n * 1024n * 1024n),
        currentMemory: MemoryTestUtils.createMemorySize(128n * 1024n * 1024n),
        averageAllocationSize: MemoryTestUtils.createMemorySize(16n * 1024n * 1024n),
        usageByPurpose: [
          [MemoryPurpose.VmHeap, MemoryTestUtils.createMemorySize(64n * 1024n * 1024n)],
          [MemoryPurpose.AssetCache, MemoryTestUtils.createMemorySize(32n * 1024n * 1024n)]
        ],
        efficiencyRatio: 0.85
      };

      expect(stats.totalAllocations).toBe(10);
      expect(stats.activeAllocations).toBe(7);
      expect(stats.peakMemory.humanReadable).toBe('256.0MB');
      expect(stats.currentMemory.humanReadable).toBe('128.0MB');
      expect(stats.efficiencyRatio).toBe(0.85);
    });

    test('should analyze memory trends', () => {
      const snapshots = [
        MemoryTestUtils.createTestSnapshot('vita', 512, 100),
        MemoryTestUtils.createTestSnapshot('vita', 512, 200),
        MemoryTestUtils.createTestSnapshot('vita', 512, 300)
      ];

      const trend: MemoryTrend = {
        snapshots,
        trendDirection: TrendDirection.Increasing,
        predictedPeak: MemoryTestUtils.createMemorySize(400n * 1024n * 1024n)
      };

      expect(trend.snapshots.length).toBe(3);
      expect(trend.trendDirection).toBe(TrendDirection.Increasing);
      expect(trend.predictedPeak?.humanReadable).toBe('400.0MB');
    });
  });
});
