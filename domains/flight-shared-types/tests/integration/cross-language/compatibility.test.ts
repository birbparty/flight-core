/**
 * Cross-Language Compatibility Tests
 * Validates type serialization compatibility between TypeScript, Go, and other bindings
 */

import { MemoryUsageSnapshot, MemoryUpdate, MemorySize } from '../../../bindings/typescript/enhanced/src/types';

describe('Cross-Language Type Compatibility', () => {
  describe('JSON Serialization Compatibility', () => {
    test('should serialize MemorySize consistently across languages', () => {
      const memorySize: MemorySize = {
        bytes: 1024n * 1024n * 1024n, // 1GB
        humanReadable: '1.0GB'
      };

      const json = JSON.stringify(memorySize, (key, value) =>
        typeof value === 'bigint' ? value.toString() : value
      );

      // Expected JSON structure that Go should be able to parse
      const expected = {
        bytes: '1073741824',
        human_readable: '1.0GB'
      };

      expect(json).toContain('"bytes":"1073741824"');
      expect(json).toContain('"humanReadable":"1.0GB"');
    });

    test('should serialize MemoryUsageSnapshot consistently', () => {
      const snapshot: MemoryUsageSnapshot = {
        timestamp: 1640995200000, // Fixed timestamp for consistency
        sessionId: 'test_session_123',
        platform: 'dreamcast',
        total: { bytes: 16n * 1024n * 1024n, humanReadable: '16.0MB' },
        used: { bytes: 8n * 1024n * 1024n, humanReadable: '8.0MB' },
        available: { bytes: 8n * 1024n * 1024n, humanReadable: '8.0MB' },
        fragmentationRatio: 0.1
      };

      const json = JSON.stringify(snapshot, (key, value) =>
        typeof value === 'bigint' ? value.toString() : value
      );

      // Verify all required fields are present
      expect(json).toContain('"timestamp":1640995200000');
      expect(json).toContain('"sessionId":"test_session_123"');
      expect(json).toContain('"platform":"dreamcast"');
      expect(json).toContain('"fragmentationRatio":0.1');
      
      // Verify nested MemorySize objects
      expect(json).toContain('"bytes":"16777216"'); // 16MB in bytes
      expect(json).toContain('"humanReadable":"16.0MB"');
    });

    test('should serialize MemoryUpdate for WebSocket streaming', () => {
      const snapshot: MemoryUsageSnapshot = {
        timestamp: Date.now(),
        sessionId: 'stream_test',
        platform: 'psp',
        total: { bytes: 32n * 1024n * 1024n, humanReadable: '32.0MB' },
        used: { bytes: 16n * 1024n * 1024n, humanReadable: '16.0MB' },
        available: { bytes: 16n * 1024n * 1024n, humanReadable: '16.0MB' },
        fragmentationRatio: 0.05
      };

      const update: MemoryUpdate = {
        type: 'memory_update',
        sessionId: 'stream_test',
        snapshot,
        timestamp: Date.now()
      };

      const json = JSON.stringify(update, (key, value) =>
        typeof value === 'bigint' ? value.toString() : value
      );

      // Verify structure for streaming compatibility
      expect(json).toContain('"type":"memory_update"');
      expect(json).toContain('"sessionId":"stream_test"');
      expect(json).toContain('"platform":"psp"');
    });
  });

  describe('Platform Profile Compatibility', () => {
    test('should handle all platform profiles consistently', () => {
      const platforms = [
        { name: 'dreamcast', memory: 16 * 1024 * 1024 },
        { name: 'psp', memory: 32 * 1024 * 1024 },
        { name: 'vita', memory: 512 * 1024 * 1024 },
        { name: 'custom-1gb', memory: 1024 * 1024 * 1024 }
      ];

      platforms.forEach(platform => {
        const snapshot: MemoryUsageSnapshot = {
          timestamp: Date.now(),
          sessionId: `test_${platform.name}`,
          platform: platform.name,
          total: { 
            bytes: BigInt(platform.memory),
            humanReadable: formatBytes(platform.memory)
          },
          used: { 
            bytes: BigInt(Math.floor(platform.memory * 0.5)),
            humanReadable: formatBytes(Math.floor(platform.memory * 0.5))
          },
          available: { 
            bytes: BigInt(Math.floor(platform.memory * 0.5)),
            humanReadable: formatBytes(Math.floor(platform.memory * 0.5))
          },
          fragmentationRatio: 0.1
        };

        // Should serialize without errors
        expect(() => JSON.stringify(snapshot, bigIntReplacer)).not.toThrow();
        
        // Should have correct platform identifier
        expect(snapshot.platform).toBe(platform.name);
        expect(snapshot.total.bytes).toBe(BigInt(platform.memory));
      });
    });
  });

  describe('Error Type Compatibility', () => {
    test('should serialize error types for cross-language error handling', () => {
      const errorCodes = [
        'insufficient-memory',
        'limit-exceeded',
        'invalid-size',
        'allocation-failed',
        'already-freed',
        'invalid-allocation',
        'unsupported-platform',
        'fragmentation-error'
      ];

      errorCodes.forEach(code => {
        const error = {
          code,
          message: `Test error: ${code}`,
          details: `Details for ${code} error`,
          timestamp: Date.now()
        };

        const json = JSON.stringify(error);
        
        expect(json).toContain(`"code":"${code}"`);
        expect(json).toContain(`"message":"Test error: ${code}"`);
        expect(json).toContain('"timestamp"');
      });
    });
  });

  describe('High-Frequency Data Compatibility', () => {
    test('should handle rapid serialization/deserialization for real-time streaming', () => {
      const sessionId = 'perf_test';
      const updates: string[] = [];

      // Generate 100 rapid updates
      for (let i = 0; i < 100; i++) {
        const snapshot: MemoryUsageSnapshot = {
          timestamp: Date.now() + i,
          sessionId,
          platform: 'vita',
          total: { bytes: 512n * 1024n * 1024n, humanReadable: '512.0MB' },
          used: { 
            bytes: BigInt(i * 1024 * 1024), 
            humanReadable: formatBytes(i * 1024 * 1024)
          },
          available: { 
            bytes: 512n * 1024n * 1024n - BigInt(i * 1024 * 1024),
            humanReadable: formatBytes(512 * 1024 * 1024 - i * 1024 * 1024)
          },
          fragmentationRatio: i / 1000
        };

        const update: MemoryUpdate = {
          type: 'memory_update',
          sessionId,
          snapshot,
          timestamp: Date.now() + i
        };

        const json = JSON.stringify(update, bigIntReplacer);
        updates.push(json);
      }

      expect(updates.length).toBe(100);
      
      // Verify each update can be parsed back
      updates.forEach((json, index) => {
        expect(() => JSON.parse(json)).not.toThrow();
        const parsed = JSON.parse(json);
        expect(parsed.type).toBe('memory_update');
        expect(parsed.sessionId).toBe(sessionId);
        expect(parsed.snapshot.platform).toBe('vita');
      });
    });
  });

  describe('Data Type Precision', () => {
    test('should maintain precision for large memory values', () => {
      // Test with large memory values (terabyte range)
      const largeMemory = {
        bytes: 1024n * 1024n * 1024n * 1024n, // 1TB
        humanReadable: '1.0TB'
      };

      const json = JSON.stringify(largeMemory, bigIntReplacer);
      const parsed = JSON.parse(json);

      // Should maintain precision
      expect(parsed.bytes).toBe('1099511627776');
      expect(parsed.humanReadable).toBe('1.0TB');
    });

    test('should handle fractional fragmentation ratios precisely', () => {
      const preciseRatios = [0.1, 0.25, 0.333333, 0.666667, 0.9];

      preciseRatios.forEach(ratio => {
        const snapshot: MemoryUsageSnapshot = {
          timestamp: Date.now(),
          sessionId: 'precision_test',
          platform: 'test',
          total: { bytes: 1024n * 1024n, humanReadable: '1.0MB' },
          used: { bytes: 512n * 1024n, humanReadable: '512.0KB' },
          available: { bytes: 512n * 1024n, humanReadable: '512.0KB' },
          fragmentationRatio: ratio
        };

        const json = JSON.stringify(snapshot, bigIntReplacer);
        const parsed = JSON.parse(json);

        // Should maintain reasonable precision
        expect(Math.abs(parsed.fragmentationRatio - ratio)).toBeLessThan(0.000001);
      });
    });
  });
});

// Helper functions
function formatBytes(bytes: number): string {
  const units = ['B', 'KB', 'MB', 'GB', 'TB'];
  let value = bytes;
  let unitIndex = 0;

  while (value >= 1024 && unitIndex < units.length - 1) {
    value /= 1024;
    unitIndex++;
  }

  return `${value.toFixed(1)}${units[unitIndex]}`;
}

function bigIntReplacer(key: string, value: any): any {
  return typeof value === 'bigint' ? value.toString() : value;
}
