/**
 * Session Types Integration Tests
 * 
 * Comprehensive testing for Flight Session Types across TypeScript bindings
 */

import {
  SessionInfo,
  SessionState,
  SessionType,
  SessionHealth,
  SessionConfig,
  ResourceLimits,
  SessionManager,
  SessionValidator,
  SessionFormatter,
  MemorySize
} from '../../../bindings/typescript/session/src';

describe('Session Types Integration Tests', () => {
  
  describe('SessionManager', () => {
    test('should validate state transitions correctly', () => {
      // Valid transitions
      expect(SessionManager.validateStateTransition(
        SessionState.Initializing, 
        SessionState.Active
      )).toBe(true);
      
      expect(SessionManager.validateStateTransition(
        SessionState.Active, 
        SessionState.Suspended
      )).toBe(true);
      
      expect(SessionManager.validateStateTransition(
        SessionState.Suspended, 
        SessionState.Active
      )).toBe(true);
      
      // Invalid transitions
      expect(SessionManager.validateStateTransition(
        SessionState.Active, 
        SessionState.Initializing
      )).toBe(false);
      
      expect(SessionManager.validateStateTransition(
        SessionState.Terminated, 
        SessionState.Active
      )).toBe(false);
    });

    test('should generate valid session IDs', () => {
      const sessionId = SessionManager.generateSessionId(
        SessionType.Development, 
        'cloud'
      );
      
      expect(sessionId).toMatch(/^development-cloud-\d+-[a-z0-9]+$/);
      expect(SessionManager.validateSessionId(sessionId)).toBe(true);
    });

    test('should create platform-specific resource limits', () => {
      const dreamcastLimits = SessionManager.createPlatformResourceLimits('dreamcast');
      expect(dreamcastLimits.maxMemory?.humanReadable).toBe('16MB');
      expect(dreamcastLimits.maxCpuPercent).toBe(95.0);
      
      const cloudLimits = SessionManager.createPlatformResourceLimits('cloud');
      expect(cloudLimits.maxMemory?.humanReadable).toBe('1GB');
      expect(cloudLimits.maxCpuPercent).toBe(80.0);
    });

    test('should calculate session health correctly', () => {
      const resources = {
        memory: {
          timestamp: Date.now(),
          sessionId: 'test-session',
          platform: 'test',
          total: { bytes: 1024n * 1024n * 1024n, humanReadable: '1GB' },
          used: { bytes: 900n * 1024n * 1024n, humanReadable: '900MB' },
          available: { bytes: 124n * 1024n * 1024n, humanReadable: '124MB' },
          fragmentationRatio: 0.9
        },
        cpuUsage: 95.0,
        networkUsage: 1000000,
        storageUsage: { bytes: 500n * 1024n * 1024n, humanReadable: '500MB' },
        connectionCount: 50,
        customMetrics: []
      };

      const limits: ResourceLimits = {
        maxMemory: { bytes: 1024n * 1024n * 1024n, humanReadable: '1GB' },
        maxCpuPercent: 100.0,
        maxNetworkBps: 10000000,
        customLimits: []
      };

      const health = SessionManager.calculateSessionHealth(resources, limits);
      expect(health).toBe(SessionHealth.Critical); // High memory + CPU + fragmentation
    });

    test('should filter sessions correctly', () => {
      const sessions: SessionInfo[] = [
        {
          id: 'session-1',
          sessionType: SessionType.Development,
          state: SessionState.Active,
          platform: 'cloud',
          userId: 'user1',
          createdAt: 1000,
          lastActivity: 1100,
          metadata: []
        },
        {
          id: 'session-2',
          sessionType: SessionType.User,
          state: SessionState.Suspended,
          platform: 'mobile',
          userId: 'user2',
          createdAt: 2000,
          lastActivity: 2100,
          metadata: []
        }
      ];

      const filtered = SessionManager.filterSessions(sessions, {
        sessionType: SessionType.Development,
        state: SessionState.Active
      });

      expect(filtered).toHaveLength(1);
      expect(filtered[0].id).toBe('session-1');
    });
  });

  describe('SessionValidator', () => {
    test('should validate session configuration', () => {
      const validConfig: SessionConfig = {
        environment: [
          ['NODE_ENV', 'development'],
          ['PORT', '3000']
        ],
        workingDirectory: '/app',
        customConfig: [
          ['theme', 'dark'],
          ['language', 'en']
        ]
      };

      const errors = SessionValidator.validateSessionConfig(validConfig);
      expect(errors).toHaveLength(0);
    });

    test('should detect invalid environment variables', () => {
      const invalidConfig: SessionConfig = {
        environment: [
          ['123INVALID', 'value'], // Invalid name
          ['VALID_NAME', 'x'.repeat(5000)] // Too long value
        ],
        customConfig: []
      };

      const errors = SessionValidator.validateSessionConfig(invalidConfig);
      expect(errors.length).toBeGreaterThan(0);
      expect(errors.some(e => e.includes('Invalid environment variable name'))).toBe(true);
      expect(errors.some(e => e.includes('too long'))).toBe(true);
    });

    test('should validate resource limits', () => {
      const invalidLimits: ResourceLimits = {
        maxCpuPercent: 150.0, // Invalid percentage
        timeoutSeconds: 0, // Invalid timeout
        customLimits: []
      };

      const errors = SessionValidator.validateResourceLimits(invalidLimits);
      expect(errors.length).toBeGreaterThan(0);
      expect(errors.some(e => e.includes('CPU limit must be between'))).toBe(true);
      expect(errors.some(e => e.includes('Timeout cannot be zero'))).toBe(true);
    });
  });

  describe('SessionFormatter', () => {
    test('should format durations correctly', () => {
      expect(SessionFormatter.formatDuration(30)).toBe('30s');
      expect(SessionFormatter.formatDuration(90)).toBe('1m 30s');
      expect(SessionFormatter.formatDuration(3661)).toBe('1h 1m');
      expect(SessionFormatter.formatDuration(90061)).toBe('1d 1h');
    });

    test('should format timestamps correctly', () => {
      const timestamp = 1640995200; // 2022-01-01T00:00:00Z
      const formatted = SessionFormatter.formatTimestamp(timestamp);
      expect(formatted).toBe('2022-01-01T00:00:00.000Z');
    });

    test('should format memory sizes correctly', () => {
      const size: MemorySize = {
        bytes: 1024n * 1024n,
        humanReadable: '1MB'
      };
      expect(SessionFormatter.formatMemorySize(size)).toBe('1MB');
    });

    test('should format resource usage with percentages', () => {
      expect(SessionFormatter.formatResourceUsage(800, 1000)).toBe('800 (80.0%)');
      expect(SessionFormatter.formatResourceUsage(500)).toBe('500');
    });
  });

  describe('Session Lifecycle', () => {
    test('should handle complete session lifecycle', () => {
      // Create session
      const sessionId = SessionManager.generateSessionId(
        SessionType.Development,
        'cloud'
      );
      
      // Validate initial state
      expect(SessionManager.validateStateTransition(
        SessionState.Initializing,
        SessionState.Active
      )).toBe(true);

      // Validate transition to suspended
      expect(SessionManager.validateStateTransition(
        SessionState.Active,
        SessionState.Suspended
      )).toBe(true);

      // Validate transition back to active
      expect(SessionManager.validateStateTransition(
        SessionState.Suspended,
        SessionState.Active
      )).toBe(true);

      // Validate termination
      expect(SessionManager.validateStateTransition(
        SessionState.Active,
        SessionState.Terminating
      )).toBe(true);

      expect(SessionManager.validateStateTransition(
        SessionState.Terminating,
        SessionState.Terminated
      )).toBe(true);
    });
  });

  describe('Metadata Handling', () => {
    test('should convert metadata between formats', () => {
      const metadata = [
        ['key1', 'value1'],
        ['key2', 'value2']
      ] as Array<[string, string]>;

      const obj = SessionManager.metadataToObject(metadata);
      expect(obj).toEqual({
        key1: 'value1',
        key2: 'value2'
      });

      const backToArray = SessionManager.objectToMetadata(obj);
      expect(backToArray).toEqual(expect.arrayContaining(metadata));
    });

    test('should merge metadata correctly', () => {
      const existing = [
        ['key1', 'value1'],
        ['key2', 'value2']
      ] as Array<[string, string]>;

      const updates = [
        ['key2', 'updated_value2'],
        ['key3', 'value3']
      ] as Array<[string, string]>;

      const merged = SessionManager.mergeMetadata(existing, updates);
      const mergedObj = SessionManager.metadataToObject(merged);

      expect(mergedObj).toEqual({
        key1: 'value1',
        key2: 'updated_value2',
        key3: 'value3'
      });
    });
  });

  describe('Session Expiry', () => {
    test('should detect expired sessions', () => {
      const expiredSession: SessionInfo = {
        id: 'expired-session',
        sessionType: SessionType.Testing,
        state: SessionState.Active,
        platform: 'test',
        createdAt: 1000,
        lastActivity: 1100,
        expiresAt: 500, // Already expired
        metadata: []
      };

      expect(SessionManager.isSessionExpired(expiredSession)).toBe(true);
    });

    test('should not detect non-expired sessions as expired', () => {
      const activeSession: SessionInfo = {
        id: 'active-session',
        sessionType: SessionType.Development,
        state: SessionState.Active,
        platform: 'cloud',
        createdAt: Date.now() / 1000 - 3600, // 1 hour ago
        lastActivity: Date.now() / 1000 - 60, // 1 minute ago
        expiresAt: (Date.now() / 1000) + 3600, // 1 hour from now
        metadata: []
      };

      expect(SessionManager.isSessionExpired(activeSession)).toBe(false);
    });
  });

  describe('Cross-Platform Compatibility', () => {
    test('should handle different platform configurations', () => {
      const platforms = ['dreamcast', 'psp', 'vita', 'cloud', 'mobile'];
      
      platforms.forEach(platform => {
        const limits = SessionManager.createPlatformResourceLimits(platform);
        const config = SessionManager.createDefaultConfig(platform);
        
        expect(limits).toBeDefined();
        expect(config).toBeDefined();
        expect(config.customConfig.some(([k, v]) => k === 'platform' && v === platform)).toBe(true);
      });
    });
  });
});

describe('Session Types Performance Tests', () => {
  test('should handle large numbers of sessions efficiently', () => {
    const sessions: SessionInfo[] = [];
    const startTime = Date.now();
    
    // Create 1000 sessions
    for (let i = 0; i < 1000; i++) {
      sessions.push({
        id: `session-${i}`,
        sessionType: i % 2 === 0 ? SessionType.Development : SessionType.User,
        state: SessionState.Active,
        platform: i % 3 === 0 ? 'cloud' : 'mobile',
        userId: `user-${i % 10}`,
        createdAt: startTime - (i * 1000),
        lastActivity: startTime - (i * 100),
        metadata: []
      });
    }
    
    const createTime = Date.now() - startTime;
    expect(createTime).toBeLessThan(100); // Should create 1000 sessions in <100ms
    
    // Filter sessions
    const filterStart = Date.now();
    const filtered = SessionManager.filterSessions(sessions, {
      sessionType: SessionType.Development,
      userId: 'user-5'
    });
    const filterTime = Date.now() - filterStart;
    
    expect(filterTime).toBeLessThan(10); // Should filter in <10ms
    expect(filtered.length).toBeGreaterThan(0);
  });
});
