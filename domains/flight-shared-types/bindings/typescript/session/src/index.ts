/**
 * Flight Session Types - Main Export
 * 
 * Universal session management for Flight-Core multi-platform development.
 * Vendor-neutral session tracking from Dreamcast (16MB) to modern cloud 
 * environments with extensible metadata for third-party integrations.
 */

// Export all types and interfaces
export * from './types';

// Export utilities
export * from './utils/session-utils';

// Export React hooks (optional, only if React is available)
export * from './react/session-hooks';

// Re-export key types for convenience
export type {
  SessionInfo,
  SessionState,
  SessionType,
  SessionHealth,
  SessionResources,
  ResourceLimits,
  SessionConfig,
  SessionEvent,
  SessionEventType,
  SessionStats,
  SessionOperations,
  SessionAnalytics,
  SessionFilterCriteria,
  MemorySize,
  MemoryUsageSnapshot,
  FlightResult,
  FlightError
} from './types';

// Re-export key utilities for convenience
export {
  SessionManager,
  SessionValidator,
  SessionFormatter
} from './utils/session-utils';

// Version information
export const VERSION = '1.0.0';

// Package information
export const PACKAGE_INFO = {
  name: '@flight/session-types',
  version: VERSION,
  description: 'Flight Session Types - Universal session management for Flight-Core',
  homepage: 'https://github.com/flight/flight-shared-types',
  license: 'MIT'
} as const;

// Default configurations for different platforms
export const DEFAULT_PLATFORM_CONFIGS = {
  dreamcast: {
    maxMemory: { bytes: 16n * 1024n * 1024n, humanReadable: '16MB' },
    maxCpuPercent: 95.0,
    maxConnections: 1,
    timeoutSeconds: 3600
  },
  psp: {
    maxMemory: { bytes: 32n * 1024n * 1024n, humanReadable: '32MB' },
    maxCpuPercent: 90.0,
    maxConnections: 4,
    timeoutSeconds: 7200
  },
  vita: {
    maxMemory: { bytes: 512n * 1024n * 1024n, humanReadable: '512MB' },
    maxCpuPercent: 85.0,
    maxConnections: 16,
    timeoutSeconds: 14400
  },
  cloud: {
    maxMemory: { bytes: 1024n * 1024n * 1024n, humanReadable: '1GB' },
    maxCpuPercent: 80.0,
    maxConnections: 100,
    timeoutSeconds: 28800
  }
} as const;
