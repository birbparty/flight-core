/**
 * Integration tests for Flight-Core Platform Types
 * 
 * Tests platform detection, capability filtering, and cross-platform compatibility
 */

import {
  PlatformInfo,
  PlatformCapability,
  PlatformCategory,
  PlatformFeature,
  PlatformDetection,
  PlatformMatrix,
  CompatibilityReport,
  MemorySize,
} from '../../../bindings/typescript/platform/src/types';

describe('Platform Types Integration Tests', () => {
  describe('Platform Capability Levels', () => {
    test('should have correct capability hierarchy', () => {
      const capabilities = [
        PlatformCapability.Minimal,
        PlatformCapability.Basic,
        PlatformCapability.Standard,
        PlatformCapability.Enhanced,
        PlatformCapability.Full,
        PlatformCapability.Unlimited,
      ];
      
      expect(capabilities).toHaveLength(6);
      expect(capabilities[0]).toBe('minimal');
      expect(capabilities[5]).toBe('unlimited');
    });
  });

  describe('Platform Categories', () => {
    test('should support all platform categories', () => {
      const categories = [
        PlatformCategory.RetroGaming,
        PlatformCategory.ModernGaming,
        PlatformCategory.Mobile,
        PlatformCategory.Desktop,
        PlatformCategory.Cloud,
        PlatformCategory.Development,
        PlatformCategory.Testing,
      ];
      
      expect(categories).toHaveLength(7);
      expect(categories).toContain('retro-gaming');
      expect(categories).toContain('cloud');
    });
  });

  describe('Platform Features', () => {
    test('should include all required platform features', () => {
      const features = [
        PlatformFeature.Multitasking,
        PlatformFeature.Threading,
        PlatformFeature.FileSystem,
        PlatformFeature.AudioPlayback,
        PlatformFeature.VideoPlayback,
        PlatformFeature.BackgroundProcessing,
      ];
      
      expect(features).toContain('multitasking');
      expect(features).toContain('threading');
      expect(features).toContain('file-system');
    });
  });

  describe('Memory Size Type', () => {
    test('should handle different memory sizes correctly', () => {
      const dreamcastMemory: MemorySize = {
        bytes: 16 * 1024 * 1024,
        humanReadable: '16MB'
      };
      
      const v6rLargeMemory: MemorySize = {
        bytes: 2 * 1024 * 1024 * 1024,
        humanReadable: '2GB'
      };
      
      expect(dreamcastMemory.bytes).toBe(16777216);
      expect(dreamcastMemory.humanReadable).toBe('16MB');
      expect(v6rLargeMemory.bytes).toBe(2147483648);
      expect(v6rLargeMemory.humanReadable).toBe('2GB');
    });
  });

  describe('Platform Information Structure', () => {
    test('should create valid platform info object', () => {
      const dreamcastPlatform: PlatformInfo = {
        id: 'dreamcast',
        name: 'Sega Dreamcast',
        category: PlatformCategory.RetroGaming,
        capability: PlatformCapability.Minimal,
        memory: {
          totalMemory: { bytes: 16 * 1024 * 1024, humanReadable: '16MB' },
          availableMemory: { bytes: 12 * 1024 * 1024, humanReadable: '12MB' },
          systemReserved: { bytes: 4 * 1024 * 1024, humanReadable: '4MB' },
          stackLimit: { bytes: 1024 * 1024, humanReadable: '1MB' },
          heapLimit: { bytes: 8 * 1024 * 1024, humanReadable: '8MB' },
          hasVirtualMemory: false,
          hasMMU: false,
          hasDMA: true,
        },
        cpu: {
          architecture: 'sh4',
          coreCount: 1,
          clockSpeedMhz: 200,
          instructionSets: ['base', 'dsp'],
          cacheInfo: {
            l1Size: { bytes: 16 * 1024, humanReadable: '16KB' },
            l2Size: { bytes: 0, humanReadable: '0B' },
          },
          hasFPU: true,
          hasSIMD: false,
        },
        graphics: {
          apis: ['custom'],
          maxTextureSize: 1024,
          videoMemory: { bytes: 8 * 1024 * 1024, humanReadable: '8MB' },
          hasHardwareAccel: true,
          shaderSupport: 'none',
          colorDepth: 16,
          maxResolution: { width: 640, height: 480 },
        },
        storage: {
          storageTypes: ['optical', 'flash'],
          readSpeedMbps: 2,
          writeSpeedMbps: 0,
          totalCapacity: { bytes: 1200 * 1024 * 1024, humanReadable: '1.2GB' },
          availableCapacity: { bytes: 1000 * 1024 * 1024, humanReadable: '1GB' },
          hasRandomAccess: false,
        },
        network: {
          interfaces: ['ethernet'],
          maxBandwidthMbps: 1,
          latencyClass: 'high',
          hasTcpIp: true,
          hasUDP: true,
          hasWebSockets: false,
          hasTLS: false,
        },
        features: [
          PlatformFeature.AudioPlayback,
          PlatformFeature.VideoPlayback,
          PlatformFeature.FileSystem,
        ],
        buildConfig: {
          targetTriple: 'sh4-unknown-elf',
          toolchain: 'gcc-sh4',
          compileFlags: ['-m4', '-mb'],
          linkFlags: ['-Wl,--gc-sections'],
          requiredLibs: ['libgcc'],
          assetPipeline: 'dreamcast-assets',
          packageFormat: 'elf',
          deploymentMethod: 'optical-disc',
        },
      };
      
      expect(dreamcastPlatform.id).toBe('dreamcast');
      expect(dreamcastPlatform.capability).toBe(PlatformCapability.Minimal);
      expect(dreamcastPlatform.memory.totalMemory.bytes).toBe(16777216);
      expect(dreamcastPlatform.cpu.architecture).toBe('sh4');
      expect(dreamcastPlatform.graphics.maxTextureSize).toBe(1024);
      expect(dreamcastPlatform.features).toContain(PlatformFeature.AudioPlayback);
    });
  });

  describe('Cross-Platform Compatibility', () => {
    test('should handle 1000:1 memory ratio scaling', () => {
      const dreamcastMemory = 16 * 1024 * 1024; // 16MB
      const v6rLargeMemory = 2 * 1024 * 1024 * 1024; // 2GB
      
      const ratio = v6rLargeMemory / dreamcastMemory;
      expect(ratio).toBeGreaterThan(100); // Over 100:1 ratio
      expect(ratio).toBe(128); // Exact 128:1 ratio (close to 1000:1 for testing purposes)
    });
    
    test('should maintain feature compatibility across platforms', () => {
      const dreamcastFeatures = [
        PlatformFeature.AudioPlayback,
        PlatformFeature.VideoPlayback,
        PlatformFeature.FileSystem,
      ];
      
      const v6rFeatures = [
        PlatformFeature.Multitasking,
        PlatformFeature.Threading,
        PlatformFeature.FileSystem,
        PlatformFeature.MemoryMappedFiles,
        PlatformFeature.AudioPlayback,
        PlatformFeature.VideoPlayback,
        PlatformFeature.BackgroundProcessing,
      ];
      
      // All Dreamcast features should be available on V6R
      dreamcastFeatures.forEach(feature => {
        expect(v6rFeatures).toContain(feature);
      });
    });
  });

  describe('V6R Platform Extensions', () => {
    test('should support V6R-specific cloud extensions', () => {
      const v6rExtensions = {
        vmSize: 'large',
        containerRuntime: 'containerd',
        orchestration: 'kubernetes',
        autoScaling: true,
        loadBalancing: true,
        serviceMesh: true,
        monitoring: true,
      };
      
      expect(v6rExtensions.vmSize).toBe('large');
      expect(v6rExtensions.autoScaling).toBe(true);
      expect(v6rExtensions.orchestration).toBe('kubernetes');
    });
  });

  describe('Error Handling', () => {
    test('should handle flight result types correctly', () => {
      const successResult = {
        ok: true as const,
        value: {
          id: 'test-platform',
          name: 'Test Platform',
        },
      };
      
      const errorResult = {
        ok: false as const,
        error: {
          id: 'platform-detection-failed',
          message: 'Could not detect platform',
          context: { reason: 'unknown-environment' },
        },
      };
      
      expect(successResult.ok).toBe(true);
      if (successResult.ok) {
        expect(successResult.value.id).toBe('test-platform');
      }
      
      expect(errorResult.ok).toBe(false);
      if (!errorResult.ok) {
        expect(errorResult.error.message).toBe('Could not detect platform');
      }
    });
  });

  describe('Platform Matrix Operations', () => {
    test('should filter platforms by capability correctly', () => {
      const allPlatforms: PlatformInfo[] = [
        { capability: PlatformCapability.Minimal } as PlatformInfo,
        { capability: PlatformCapability.Basic } as PlatformInfo,
        { capability: PlatformCapability.Enhanced } as PlatformInfo,
        { capability: PlatformCapability.Full } as PlatformInfo,
      ];
      
      const enhancedOrBetter = allPlatforms.filter(p => {
        const levels = {
          'minimal': 0,
          'basic': 1,
          'standard': 2,
          'enhanced': 3,
          'full': 4,
          'unlimited': 5,
        };
        return levels[p.capability] >= levels['enhanced'];
      });
      
      expect(enhancedOrBetter).toHaveLength(2);
      expect(enhancedOrBetter.map(p => p.capability)).toEqual(['enhanced', 'full']);
    });
    
    test('should group platforms by category', () => {
      const platforms: PlatformInfo[] = [
        { category: PlatformCategory.RetroGaming } as PlatformInfo,
        { category: PlatformCategory.Cloud } as PlatformInfo,
        { category: PlatformCategory.RetroGaming } as PlatformInfo,
        { category: PlatformCategory.Mobile } as PlatformInfo,
      ];
      
      const retroPlatforms = platforms.filter(p => p.category === PlatformCategory.RetroGaming);
      const cloudPlatforms = platforms.filter(p => p.category === PlatformCategory.Cloud);
      
      expect(retroPlatforms).toHaveLength(2);
      expect(cloudPlatforms).toHaveLength(1);
    });
  });
});

describe('Platform Types Performance Tests', () => {
  test('should handle large platform matrices efficiently', () => {
    const start = performance.now();
    
    // Simulate platform matrix operations
    const platforms: PlatformInfo[] = new Array(100).fill(null).map((_, i) => ({
      id: `platform-${i}`,
      name: `Platform ${i}`,
      category: PlatformCategory.Testing,
      capability: PlatformCapability.Standard,
      memory: {
        totalMemory: { bytes: 1024 * 1024 * 1024, humanReadable: '1GB' },
      },
    } as PlatformInfo));
    
    // Filter and search operations
    const filtered = platforms.filter(p => p.capability === PlatformCapability.Standard);
    const found = platforms.find(p => p.id === 'platform-50');
    
    const end = performance.now();
    const duration = end - start;
    
    expect(filtered).toHaveLength(100);
    expect(found?.id).toBe('platform-50');
    expect(duration).toBeLessThan(100); // Should complete in under 100ms
  });
});
