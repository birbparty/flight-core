/**
 * Flight Session Types - TypeScript Definitions
 * 
 * Universal session management for Flight-Core multi-platform development.
 * Vendor-neutral session tracking from Dreamcast (16MB) to modern cloud 
 * environments with extensible metadata for third-party integrations.
 */

// Import types from enhanced bindings (relative paths for now)
export interface MemorySize {
  readonly bytes: bigint;
  readonly humanReadable: string;
}

export interface MemoryUsageSnapshot {
  readonly timestamp: number;
  readonly sessionId: string;
  readonly platform: string;
  readonly total: MemorySize;
  readonly used: MemorySize;
  readonly available: MemorySize;
  readonly fragmentationRatio: number;
}

export interface FlightError {
  readonly id: string;
  readonly severity: string;
  readonly category: string;
  readonly message: string;
  readonly details?: string;
  readonly context: {
    readonly source: string;
    readonly operation: string;
    readonly sessionId?: string;
    readonly userId?: string;
    readonly platform?: string;
    readonly serviceId?: string;
    readonly metadata: ReadonlyArray<readonly [string, string]>;
  };
  readonly timestamp: number;
  readonly cause?: string;
}

export type FlightResult<T> = 
  | { readonly tag: "ok"; readonly val: T }
  | { readonly tag: "err"; readonly val: FlightError };

// Session lifecycle states
export enum SessionState {
  Initializing = 'initializing',
  Active = 'active',
  Suspended = 'suspended',
  Terminating = 'terminating',
  Terminated = 'terminated',
  Error = 'error',
}

// Session types for different use cases
export enum SessionType {
  Component = 'component',
  User = 'user',
  Development = 'development',
  System = 'system',
  Testing = 'testing',
  Custom = 'custom',
}

// Session health status
export enum SessionHealth {
  Healthy = 'healthy',
  Warning = 'warning',
  Degraded = 'degraded',
  Critical = 'critical',
  Unknown = 'unknown',
}

// Session event types
export enum SessionEventType {
  Created = 'created',
  Started = 'started',
  Suspended = 'suspended',
  Resumed = 'resumed',
  Terminated = 'terminated',
  ErrorOccurred = 'error-occurred',
  ResourceLimitExceeded = 'resource-limit-exceeded',
  HealthChanged = 'health-changed',
  Custom = 'custom',
}

// Core session information
export interface SessionInfo {
  /** Unique session identifier */
  id: string;
  /** Session type */
  sessionType: SessionType;
  /** Current state */
  state: SessionState;
  /** Platform where session runs */
  platform: string;
  /** User identifier (for user sessions) */
  userId?: string;
  /** Parent session (for nested sessions) */
  parentSessionId?: string;
  /** Session creation time (Unix timestamp) */
  createdAt: number;
  /** Last activity time (Unix timestamp) */
  lastActivity: number;
  /** Session expiry time (Unix timestamp) */
  expiresAt?: number;
  /** Extensible session metadata for third-party integrations */
  metadata: Array<[string, string]>;
}

// Session resource usage tracking
export interface SessionResources {
  /** Memory usage snapshot */
  memory: MemoryUsageSnapshot;
  /** CPU usage percentage (0.0-100.0) */
  cpuUsage: number;
  /** Network bandwidth usage (bytes/sec) */
  networkUsage: number;
  /** Storage usage */
  storageUsage: MemorySize;
  /** Active connections count */
  connectionCount: number;
  /** Custom resource metrics (extensible) */
  customMetrics: Array<[string, number]>;
}

// Generic resource limits configuration
export interface ResourceLimits {
  /** Maximum memory usage */
  maxMemory?: MemorySize;
  /** Maximum CPU usage percentage */
  maxCpuPercent?: number;
  /** Maximum network bandwidth (bytes/sec) */
  maxNetworkBps?: number;
  /** Maximum storage usage */
  maxStorage?: MemorySize;
  /** Maximum connection count */
  maxConnections?: number;
  /** Session timeout (seconds) */
  timeoutSeconds?: number;
  /** Custom resource limits (extensible) */
  customLimits: Array<[string, number]>;
}

// Session configuration
export interface SessionConfig {
  /** Resource limits for this session */
  resourceLimits?: ResourceLimits;
  /** Environment variables */
  environment: Array<[string, string]>;
  /** Working directory */
  workingDirectory?: string;
  /** Session-specific configuration (extensible) */
  customConfig: Array<[string, string]>;
}

// Session event record
export interface SessionEvent {
  /** Event identifier */
  id: string;
  /** Session this event relates to */
  sessionId: string;
  /** Type of event */
  eventType: SessionEventType;
  /** When event occurred (Unix timestamp) */
  timestamp: number;
  /** Event message */
  message: string;
  /** Additional event data */
  data: Array<[string, string]>;
}

// Session statistics summary
export interface SessionStats {
  /** Total active sessions */
  activeSessions: number;
  /** Total sessions created today */
  sessionsCreatedToday: number;
  /** Total sessions terminated today */
  sessionsTerminatedToday: number;
  /** Average session duration (seconds) */
  averageDuration: number;
  /** Sessions by type breakdown */
  sessionsByType: Array<[SessionType, number]>;
  /** Sessions by platform breakdown */
  sessionsByPlatform: Array<[string, number]>;
  /** Overall system health */
  systemHealth: SessionHealth;
}

// Resource usage aggregation
export interface ResourceUsageAggregate {
  /** Total memory usage across all sessions */
  totalMemoryUsage: MemorySize;
  /** Average CPU usage across sessions */
  averageCpuUsage: number;
  /** Total network bandwidth usage */
  totalNetworkUsage: number;
  /** Total storage usage */
  totalStorageUsage: MemorySize;
  /** Total active connections */
  totalConnections: number;
  /** Resource usage by platform */
  usageByPlatform: Array<[string, SessionResources]>;
}

// Session operations interface
export interface SessionOperations {
  /** Create new session */
  createSession(
    sessionType: SessionType,
    platform: string,
    userId?: string,
    config?: SessionConfig
  ): Promise<FlightResult<SessionInfo>>;

  /** Get session information */
  getSession(sessionId: string): Promise<FlightResult<SessionInfo>>;

  /** Update session state */
  updateSessionState(
    sessionId: string,
    newState: SessionState
  ): Promise<FlightResult<boolean>>;

  /** Terminate session */
  terminateSession(sessionId: string): Promise<FlightResult<boolean>>;

  /** Get session resources */
  getSessionResources(sessionId: string): Promise<FlightResult<SessionResources>>;

  /** List sessions */
  listSessions(
    userId?: string,
    sessionType?: SessionType,
    platform?: string
  ): Promise<FlightResult<SessionInfo[]>>;

  /** Extend session expiry */
  extendSession(
    sessionId: string,
    additionalSeconds: number
  ): Promise<FlightResult<boolean>>;

  /** Update session metadata */
  updateSessionMetadata(
    sessionId: string,
    metadata: Array<[string, string]>
  ): Promise<FlightResult<boolean>>;

  /** Set resource limits */
  setResourceLimits(
    sessionId: string,
    limits: ResourceLimits
  ): Promise<FlightResult<boolean>>;

  /** Get session health */
  getSessionHealth(sessionId: string): Promise<FlightResult<SessionHealth>>;

  /** Record session event */
  recordSessionEvent(
    sessionId: string,
    eventType: SessionEventType,
    message: string,
    data: Array<[string, string]>
  ): Promise<FlightResult<boolean>>;

  /** Get session events */
  getSessionEvents(
    sessionId: string,
    limit?: number
  ): Promise<FlightResult<SessionEvent[]>>;
}

// Session analytics interface
export interface SessionAnalytics {
  /** Get session statistics */
  getSessionStats(): Promise<FlightResult<SessionStats>>;

  /** Get resource usage aggregate */
  getResourceUsageAggregate(): Promise<FlightResult<ResourceUsageAggregate>>;

  /** Find sessions by criteria */
  findSessions(
    criteria: Array<[string, string]>
  ): Promise<FlightResult<SessionInfo[]>>;

  /** Get sessions by health status */
  getSessionsByHealth(
    health: SessionHealth
  ): Promise<FlightResult<SessionInfo[]>>;

  /** Calculate session efficiency */
  calculateSessionEfficiency(sessionId: string): Promise<FlightResult<number>>;

  /** Generate session report */
  generateSessionReport(
    timeWindowHours: number,
    includeEvents: boolean
  ): Promise<FlightResult<string>>;
}

// Utility type for session state transitions
export type SessionStateTransition = {
  from: SessionState;
  to: SessionState;
  valid: boolean;
};

// Utility type for session metadata entries
export type SessionMetadata = {
  [key: string]: string;
};

// Utility type for session filter criteria
export interface SessionFilterCriteria {
  userId?: string;
  sessionType?: SessionType;
  platform?: string;
  state?: SessionState;
  health?: SessionHealth;
  createdAfter?: number;
  createdBefore?: number;
  expiresAfter?: number;
  expiresBefore?: number;
}

// Export utilities and hooks
export * from './utils/session-utils';
export * from './react/session-hooks';
