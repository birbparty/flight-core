/**
 * Flight Session Utilities
 * 
 * Utility functions for session management, validation, and state transitions.
 */

import {
  SessionInfo,
  SessionState,
  SessionType,
  SessionHealth,
  SessionConfig,
  ResourceLimits,
  SessionResources,
  SessionEvent,
  SessionEventType,
  SessionStateTransition,
  SessionFilterCriteria,
  MemorySize,
  FlightResult,
  FlightError
} from '../types';

/**
 * Session Manager - Core session management utilities
 */
export class SessionManager {
  
  /**
   * Validates session state transitions
   */
  static validateStateTransition(from: SessionState, to: SessionState): boolean {
    const validTransitions: Record<SessionState, SessionState[]> = {
      [SessionState.Initializing]: [SessionState.Active, SessionState.Error, SessionState.Terminated],
      [SessionState.Active]: [SessionState.Suspended, SessionState.Terminating, SessionState.Error],
      [SessionState.Suspended]: [SessionState.Active, SessionState.Terminating, SessionState.Error],
      [SessionState.Terminating]: [SessionState.Terminated, SessionState.Error],
      [SessionState.Terminated]: [], // Terminal state
      [SessionState.Error]: [SessionState.Active, SessionState.Terminating, SessionState.Terminated],
    };

    return from === to || validTransitions[from]?.includes(to) || false;
  }

  /**
   * Gets valid next states for current state
   */
  static getValidNextStates(current: SessionState): SessionState[] {
    const transitions: Record<SessionState, SessionState[]> = {
      [SessionState.Initializing]: [SessionState.Active, SessionState.Error, SessionState.Terminated],
      [SessionState.Active]: [SessionState.Suspended, SessionState.Terminating, SessionState.Error],
      [SessionState.Suspended]: [SessionState.Active, SessionState.Terminating, SessionState.Error],
      [SessionState.Terminating]: [SessionState.Terminated, SessionState.Error],
      [SessionState.Terminated]: [],
      [SessionState.Error]: [SessionState.Active, SessionState.Terminating, SessionState.Terminated],
    };

    return transitions[current] || [];
  }

  /**
   * Generates a unique session ID
   */
  static generateSessionId(sessionType: SessionType, platform: string): string {
    const timestamp = Date.now();
    const random = Math.random().toString(36).substring(2, 8);
    return `${sessionType}-${platform}-${timestamp}-${random}`;
  }

  /**
   * Validates session ID format
   */
  static validateSessionId(sessionId: string): boolean {
    if (!sessionId || sessionId.length === 0) return false;
    if (sessionId.length > 255) return false;
    
    // Check for valid characters (alphanumeric, hyphens, underscores)
    return /^[a-zA-Z0-9_-]+$/.test(sessionId);
  }

  /**
   * Creates default session configuration
   */
  static createDefaultConfig(platform: string): SessionConfig {
    return {
      environment: [],
      customConfig: [
        ['created_by', 'flight-session-manager'],
        ['platform', platform],
        ['version', '1.0.0']
      ]
    };
  }

  /**
   * Creates platform-specific resource limits
   */
  static createPlatformResourceLimits(platform: string): ResourceLimits {
    const limits: ResourceLimits = {
      customLimits: []
    };

    // Platform-specific defaults
    if (platform.startsWith('dreamcast')) {
      limits.maxMemory = { bytes: 16n * 1024n * 1024n, humanReadable: '16MB' };
      limits.maxCpuPercent = 95.0;
      limits.maxConnections = 1;
      limits.timeoutSeconds = 3600; // 1 hour
    } else if (platform.startsWith('psp')) {
      limits.maxMemory = { bytes: 32n * 1024n * 1024n, humanReadable: '32MB' };
      limits.maxCpuPercent = 90.0;
      limits.maxConnections = 4;
      limits.timeoutSeconds = 7200; // 2 hours
    } else if (platform.startsWith('cloud') || platform.startsWith('desktop')) {
      limits.maxMemory = { bytes: 1024n * 1024n * 1024n, humanReadable: '1GB' };
      limits.maxCpuPercent = 80.0;
      limits.maxConnections = 100;
      limits.timeoutSeconds = 28800; // 8 hours
    }

    return limits;
  }

  /**
   * Filters sessions based on criteria
   */
  static filterSessions(sessions: SessionInfo[], criteria: SessionFilterCriteria): SessionInfo[] {
    return sessions.filter(session => {
      if (criteria.userId && session.userId !== criteria.userId) return false;
      if (criteria.sessionType && session.sessionType !== criteria.sessionType) return false;
      if (criteria.platform && session.platform !== criteria.platform) return false;
      if (criteria.state && session.state !== criteria.state) return false;
      if (criteria.createdAfter && session.createdAt <= criteria.createdAfter) return false;
      if (criteria.createdBefore && session.createdAt >= criteria.createdBefore) return false;
      if (criteria.expiresAfter && (!session.expiresAt || session.expiresAt <= criteria.expiresAfter)) return false;
      if (criteria.expiresBefore && (!session.expiresAt || session.expiresAt >= criteria.expiresBefore)) return false;
      
      return true;
    });
  }

  /**
   * Calculates session health based on resources and limits
   */
  static calculateSessionHealth(resources: SessionResources, limits?: ResourceLimits): SessionHealth {
    if (!limits) return SessionHealth.Unknown;

    let healthScore = 100;
    let warningCount = 0;
    let criticalCount = 0;

    // Check memory usage
    if (limits.maxMemory) {
      const memoryUsagePercent = Number(resources.memory.used.bytes) / Number(limits.maxMemory.bytes) * 100;
      if (memoryUsagePercent > 90) {
        criticalCount++;
        healthScore -= 30;
      } else if (memoryUsagePercent > 75) {
        warningCount++;
        healthScore -= 15;
      }
    }

    // Check CPU usage
    if (limits.maxCpuPercent) {
      if (resources.cpuUsage > 90) {
        criticalCount++;
        healthScore -= 25;
      } else if (resources.cpuUsage > 75) {
        warningCount++;
        healthScore -= 10;
      }
    }

    // Check network usage
    if (limits.maxNetworkBps) {
      const networkUsagePercent = resources.networkUsage / limits.maxNetworkBps * 100;
      if (networkUsagePercent > 90) {
        criticalCount++;
        healthScore -= 20;
      } else if (networkUsagePercent > 75) {
        warningCount++;
        healthScore -= 10;
      }
    }

    // Check memory fragmentation
    if (resources.memory.fragmentationRatio > 0.8) {
      warningCount++;
      healthScore -= 15;
    }

    // Determine final health status
    if (criticalCount > 0 || healthScore <= 30) {
      return SessionHealth.Critical;
    } else if (warningCount > 1 || healthScore <= 60) {
      return SessionHealth.Degraded;
    } else if (warningCount > 0 || healthScore <= 85) {
      return SessionHealth.Warning;
    } else {
      return SessionHealth.Healthy;
    }
  }

  /**
   * Converts metadata array to object
   */
  static metadataToObject(metadata: Array<[string, string]>): Record<string, string> {
    const obj: Record<string, string> = {};
    for (const [key, value] of metadata) {
      obj[key] = value;
    }
    return obj;
  }

  /**
   * Converts metadata object to array
   */
  static objectToMetadata(obj: Record<string, string>): Array<[string, string]> {
    return Object.entries(obj);
  }

  /**
   * Merges session metadata
   */
  static mergeMetadata(
    existing: Array<[string, string]>, 
    updates: Array<[string, string]>
  ): Array<[string, string]> {
    const merged = new Map(existing);
    for (const [key, value] of updates) {
      merged.set(key, value);
    }
    return Array.from(merged.entries());
  }

  /**
   * Checks if session is expired
   */
  static isSessionExpired(session: SessionInfo): boolean {
    if (!session.expiresAt) return false;
    return Date.now() > session.expiresAt * 1000; // Convert to milliseconds
  }

  /**
   * Calculates session duration in seconds
   */
  static getSessionDuration(session: SessionInfo): number {
    const endTime = session.state === SessionState.Terminated 
      ? session.lastActivity 
      : Math.floor(Date.now() / 1000);
    return endTime - session.createdAt;
  }

  /**
   * Creates a session event
   */
  static createSessionEvent(
    sessionId: string,
    eventType: SessionEventType,
    message: string,
    data: Array<[string, string]> = []
  ): SessionEvent {
    return {
      id: `event-${Date.now()}-${Math.random().toString(36).substring(2, 8)}`,
      sessionId,
      eventType,
      timestamp: Math.floor(Date.now() / 1000),
      message,
      data
    };
  }
}

/**
 * Session validation utilities
 */
export class SessionValidator {
  
  /**
   * Validates session configuration
   */
  static validateSessionConfig(config: SessionConfig): string[] {
    const errors: string[] = [];

    // Validate environment variables
    const envKeys = new Set<string>();
    for (const [key, value] of config.environment) {
      if (!key || key.length === 0) {
        errors.push('Environment variable key cannot be empty');
      }
      if (envKeys.has(key)) {
        errors.push(`Duplicate environment variable: ${key}`);
      }
      envKeys.add(key);
      
      if (!/^[a-zA-Z_][a-zA-Z0-9_]*$/.test(key)) {
        errors.push(`Invalid environment variable name: ${key}`);
      }
      if (value.length > 4096) {
        errors.push(`Environment variable value too long: ${key} (max 4096 characters)`);
      }
    }

    if (config.environment.length > 100) {
      errors.push('Too many environment variables (max 100)');
    }

    // Validate working directory
    if (config.workingDirectory !== undefined) {
      if (config.workingDirectory.length === 0) {
        errors.push('Working directory cannot be empty');
      }
      if (config.workingDirectory.length > 1000) {
        errors.push('Working directory path too long (max 1000 characters)');
      }
      if (config.workingDirectory.includes('\0')) {
        errors.push('Working directory path contains null bytes');
      }
    }

    // Validate custom config
    const configKeys = new Set<string>();
    for (const [key, value] of config.customConfig) {
      if (configKeys.has(key)) {
        errors.push(`Duplicate custom config key: ${key}`);
      }
      configKeys.add(key);
      
      if (key.length > 100) {
        errors.push(`Custom config key too long: ${key} (max 100 characters)`);
      }
      if (value.length > 2000) {
        errors.push(`Custom config value too long for key '${key}' (max 2000 characters)`);
      }
    }

    if (config.customConfig.length > 50) {
      errors.push('Too many custom config entries (max 50)');
    }

    // Validate resource limits if present
    if (config.resourceLimits) {
      errors.push(...SessionValidator.validateResourceLimits(config.resourceLimits));
    }

    return errors;
  }

  /**
   * Validates resource limits
   */
  static validateResourceLimits(limits: ResourceLimits): string[] {
    const errors: string[] = [];

    if (limits.maxMemory && limits.maxMemory.bytes === 0n) {
      errors.push('Memory limit cannot be zero');
    }

    if (limits.maxCpuPercent !== undefined) {
      if (limits.maxCpuPercent <= 0 || limits.maxCpuPercent > 100) {
        errors.push('CPU limit must be between 0.1 and 100.0 percent');
      }
    }

    if (limits.maxNetworkBps !== undefined && limits.maxNetworkBps === 0) {
      errors.push('Network limit cannot be zero');
    }

    if (limits.maxStorage && limits.maxStorage.bytes === 0n) {
      errors.push('Storage limit cannot be zero');
    }

    if (limits.timeoutSeconds !== undefined) {
      if (limits.timeoutSeconds === 0) {
        errors.push('Timeout cannot be zero');
      }
      if (limits.timeoutSeconds > 30 * 24 * 60 * 60) {
        errors.push('Timeout cannot exceed 30 days');
      }
    }

    return errors;
  }
}

/**
 * Session format utilities
 */
export class SessionFormatter {
  
  /**
   * Formats session duration as human-readable string
   */
  static formatDuration(seconds: number): string {
    if (seconds < 60) return `${seconds}s`;
    if (seconds < 3600) return `${Math.floor(seconds / 60)}m ${seconds % 60}s`;
    if (seconds < 86400) {
      const hours = Math.floor(seconds / 3600);
      const minutes = Math.floor((seconds % 3600) / 60);
      return `${hours}h ${minutes}m`;
    }
    const days = Math.floor(seconds / 86400);
    const hours = Math.floor((seconds % 86400) / 3600);
    return `${days}d ${hours}h`;
  }

  /**
   * Formats timestamp as ISO string
   */
  static formatTimestamp(timestamp: number): string {
    return new Date(timestamp * 1000).toISOString();
  }

  /**
   * Formats memory size as human-readable string
   */
  static formatMemorySize(size: MemorySize): string {
    return size.humanReadable;
  }

  /**
   * Formats resource usage percentage
   */
  static formatResourceUsage(used: number, max?: number): string {
    if (!max) return used.toString();
    const percentage = (used / max) * 100;
    return `${used} (${percentage.toFixed(1)}%)`;
  }
}
