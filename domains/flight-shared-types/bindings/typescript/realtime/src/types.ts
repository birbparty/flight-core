/**
 * Real-time Communication Types for Flight-Core and V6R
 * 
 * TypeScript bindings for real-time communication WIT types
 * Supports WebSocket messaging, event streaming, and live updates
 */

// Local type definitions for development (would be imported from peer dependencies in production)
interface FlightResult<T> {
  success: boolean;
  data: T;
  error?: { message: string; code?: string };
}

type SessionId = string;
type AuthToken = string;

interface MemoryUsageSnapshot {
  timestamp: number;
  sessionId: string;
  platform: string;
  total: { bytes: number; humanReadable: string };
  used: { bytes: number; humanReadable: string };
  available: { bytes: number; humanReadable: string };
  fragmentationRatio: number;
}

interface ComponentInfo {
  id: string;
  name: string;
  version: string;
  state: string;
  platform: string;
  memoryUsage: { bytes: number; humanReadable: string };
  createdAt: number;
  metadata: Record<string, any>;
}

// Connection Management Types
export type ConnectionId = string;
export type ChannelId = string;
export type MessageId = string;

export enum ConnectionState {
  Connecting = 'connecting',
  Connected = 'connected',
  Disconnected = 'disconnected',
  Closing = 'closing',
  Closed = 'closed',
  Error = 'error',
  Authenticated = 'authenticated',
}

export interface ConnectionInfo {
  id: ConnectionId;
  state: ConnectionState;
  connectedAt: number;
  lastActivity: number;
  userId?: string;
  sessionId?: SessionId;
  platform: string;
  clientInfo: ClientInfo;
  metadata: Record<string, string>;
}

export enum ClientType {
  WebBrowser = 'web-browser',
  FlightCoreNative = 'flight-native',
  V6RMobile = 'v6r-mobile',
  V6RCli = 'v6r-cli',
  V6RBridge = 'v6r-bridge',
  DevelopmentTool = 'development-tool',
  MonitoringService = 'monitoring-service',
  ApiClient = 'api-client',
  Unknown = 'unknown',
}

export interface ClientInfo {
  clientType: ClientType;
  version: string;
  platform: string;
  capabilities: string[];
  userAgent?: string;
  ipAddress?: string;
}

// Messaging Types
export enum MessagePriority {
  Low = 'low',
  Normal = 'normal',
  High = 'high',
  Critical = 'critical',
  Realtime = 'realtime',
}

export interface RetryConfig {
  maxRetries: number;
  retryIntervalMs: number;
  backoffMultiplier: number;
  maxRetryIntervalMs: number;
}

export interface MessageRouting {
  source: string;
  route: string[];
  expiresAt?: number;
  requiresAck: boolean;
  retryConfig?: RetryConfig;
}

export interface RealtimeMessage {
  id: MessageId;
  messageType: string;
  channelId: ChannelId;
  payload: string;
  timestamp: number;
  sender?: string;
  target?: string;
  priority: MessagePriority;
  routing: MessageRouting;
  metadata: Record<string, string>;
}

// Channel and Subscription Types
export enum ChannelType {
  PublicBroadcast = 'public-broadcast',
  PrivateUser = 'private-user',
  SessionPrivate = 'session-private',
  SystemMonitoring = 'system-monitoring',
  MemoryUpdates = 'memory-updates',
  ComponentEvents = 'component-events',
  V6RVmManagement = 'v6r-vm-management',
  FlightHalEvents = 'flight-hal-events',
  Development = 'development',
}

export interface ChannelInfo {
  id: ChannelId;
  name: string;
  channelType: ChannelType;
  requiredPermissions: string[];
  maxConnections?: number;
  createdAt: number;
  metadata: Record<string, string>;
}

export enum FilterOperation {
  Equals = 'equals',
  NotEquals = 'not-equals',
  Contains = 'contains',
  StartsWith = 'starts-with',
  EndsWith = 'ends-with',
  Regex = 'regex',
  GreaterThan = 'greater-than',
  LessThan = 'less-than',
}

export interface SubscriptionFilter {
  field: string;
  operation: FilterOperation;
  value: string;
}

export interface Subscription {
  id: string;
  connectionId: ConnectionId;
  channelId: ChannelId;
  filters: SubscriptionFilter[];
  subscribedAt: number;
  lastActivity: number;
  metadata: Record<string, string>;
}

// Event System Types
export enum SessionEventType {
  SessionCreated = 'session-created',
  SessionActivated = 'session-activated',
  SessionSuspended = 'session-suspended',
  SessionTerminated = 'session-terminated',
  SessionExpired = 'session-expired',
  SessionError = 'session-error',
}

export interface SessionEvent {
  eventType: SessionEventType;
  sessionId: SessionId;
  userId?: string;
  platform: string;
  timestamp: number;
  metadata: Record<string, string>;
}

export enum AuthEventType {
  UserAuthenticated = 'user-authenticated',
  UserLoggedOut = 'user-logged-out',
  TokenRefreshed = 'token-refreshed',
  TokenExpired = 'token-expired',
  AuthFailed = 'auth-failed',
  PermissionGranted = 'permission-granted',
  PermissionRevoked = 'permission-revoked',
}

export interface AuthEvent {
  eventType: AuthEventType;
  userId: string;
  platform: string;
  timestamp: number;
  metadata: Record<string, string>;
}

export enum SystemEventType {
  SystemStartup = 'system-startup',
  SystemShutdown = 'system-shutdown',
  ComponentLoaded = 'component-loaded',
  ComponentUnloaded = 'component-unloaded',
  ErrorOccurred = 'error-occurred',
  PerformanceWarning = 'performance-warning',
  MemoryPressure = 'memory-pressure',
  ResourceExhausted = 'resource-exhausted',
}

export enum EventSeverity {
  Info = 'info',
  Warning = 'warning',
  Error = 'error',
  Critical = 'critical',
  Fatal = 'fatal',
}

export interface SystemEvent {
  eventType: SystemEventType;
  component: string;
  platform: string;
  severity: EventSeverity;
  message: string;
  timestamp: number;
  metadata: Record<string, string>;
}

export enum V6REventType {
  VmCreated = 'vm-created',
  VmStarted = 'vm-started',
  VmStopped = 'vm-stopped',
  VmDeleted = 'vm-deleted',
  VmScaling = 'vm-scaling',
  QuotaExceeded = 'quota-exceeded',
  BillingEvent = 'billing-event',
  TeamMemberAdded = 'team-member-added',
  TeamMemberRemoved = 'team-member-removed',
}

export interface V6REvent {
  eventType: V6REventType;
  resource: string;
  organization?: string;
  timestamp: number;
  metadata: Record<string, string>;
}

export enum FlightEventType {
  PlatformDetected = 'platform-detected',
  HalInitialized = 'hal-initialized',
  ComponentLoaded = 'component-loaded',
  MemoryPoolCreated = 'memory-pool-created',
  RuntimeStarted = 'runtime-started',
  PerformanceMilestone = 'performance-milestone',
}

export interface FlightEvent {
  eventType: FlightEventType;
  platform: string;
  halSubsystem?: string;
  timestamp: number;
  metadata: Record<string, string>;
}

export interface CustomEvent {
  name: string;
  data: string;
  application: string;
  timestamp: number;
  metadata: Record<string, string>;
}

export type RealtimeEvent =
  | { type: 'memory-update'; data: MemoryUsageSnapshot }
  | { type: 'component-update'; data: ComponentInfo }
  | { type: 'session-event'; data: SessionEvent }
  | { type: 'auth-event'; data: AuthEvent }
  | { type: 'system-event'; data: SystemEvent }
  | { type: 'v6r-event'; data: V6REvent }
  | { type: 'flight-event'; data: FlightEvent }
  | { type: 'custom-event'; data: CustomEvent };

// Analytics and Monitoring Types
export interface RealtimeMetrics {
  activeConnections: number;
  messagesSent: number;
  messagesReceived: number;
  messagesPerSecond: number;
  avgLatencyMs: number;
  errorRate: number;
  memoryUsage: number;
  activeSubscriptions: number;
  collectedAt: number;
}

export interface ConnectionAnalytics {
  totalConnections: number;
  peakConnections: number;
  avgConnectionDuration: number;
  connectionsByType: Record<ClientType, number>;
  connectionsByPlatform: Record<string, number>;
  errorRate: number;
}

export interface MessageAnalytics {
  totalMessages: number;
  messagesByPriority: Record<MessagePriority, number>;
  avgMessageSize: number;
  deliverySuccessRate: number;
  avgLatencyMs: number;
  messagesByEventType: Record<string, number>;
}

export interface PlatformPerformance {
  platform: string;
  activeConnections: number;
  messageThroughput: number;
  avgResponseTimeMs: number;
  errorRate: number;
  memoryUsageBytes: number;
  platformMetrics: Record<string, number>;
}

// API Interface Types
export interface RealtimeConnectionAPI {
  establishConnection(
    clientInfo: ClientInfo,
    platform: string,
    authToken?: string
  ): Promise<FlightResult<ConnectionInfo>>;

  closeConnection(
    connectionId: ConnectionId,
    reason: string
  ): Promise<FlightResult<boolean>>;

  getConnection(
    connectionId: ConnectionId
  ): Promise<FlightResult<ConnectionInfo>>;

  listConnections(
    userId?: string,
    platform?: string
  ): Promise<FlightResult<ConnectionInfo[]>>;

  updateConnectionState(
    connectionId: ConnectionId,
    newState: ConnectionState
  ): Promise<FlightResult<boolean>>;

  authenticateConnection(
    connectionId: ConnectionId,
    authToken: string
  ): Promise<FlightResult<boolean>>;
}

export interface RealtimeMessagingAPI {
  sendMessage(
    connectionId: ConnectionId,
    message: RealtimeMessage
  ): Promise<FlightResult<boolean>>;

  broadcastMessage(
    channelId: ChannelId,
    message: RealtimeMessage
  ): Promise<FlightResult<number>>;

  sendUserEvent(
    userId: string,
    event: RealtimeEvent,
    platform?: string
  ): Promise<FlightResult<boolean>>;

  broadcastEvent(
    channelId: ChannelId,
    event: RealtimeEvent
  ): Promise<FlightResult<number>>;

  sendPriorityMessage(
    connectionId: ConnectionId,
    message: RealtimeMessage,
    priority: MessagePriority
  ): Promise<FlightResult<boolean>>;

  getMessageHistory(
    channelId: ChannelId,
    limit: number,
    before?: number
  ): Promise<FlightResult<RealtimeMessage[]>>;
}

export interface RealtimeSubscriptionAPI {
  subscribeToChannel(
    connectionId: ConnectionId,
    channelId: ChannelId,
    filters?: SubscriptionFilter[]
  ): Promise<FlightResult<Subscription>>;

  unsubscribeFromChannel(
    connectionId: ConnectionId,
    channelId: ChannelId
  ): Promise<FlightResult<boolean>>;

  createChannel(
    channelInfo: ChannelInfo
  ): Promise<FlightResult<ChannelId>>;

  deleteChannel(
    channelId: ChannelId
  ): Promise<FlightResult<boolean>>;

  listChannels(
    userId?: string,
    platform?: string
  ): Promise<FlightResult<ChannelInfo[]>>;

  getChannelSubscribers(
    channelId: ChannelId
  ): Promise<FlightResult<ConnectionId[]>>;

  updateSubscriptionFilters(
    connectionId: ConnectionId,
    channelId: ChannelId,
    filters: SubscriptionFilter[]
  ): Promise<FlightResult<boolean>>;
}

export interface RealtimeAnalyticsAPI {
  getRealtimeMetrics(): Promise<FlightResult<RealtimeMetrics>>;

  getConnectionAnalytics(
    timeWindowHours: number
  ): Promise<FlightResult<ConnectionAnalytics>>;

  getMessageAnalytics(
    channelId?: ChannelId,
    timeWindowHours?: number
  ): Promise<FlightResult<MessageAnalytics>>;

  generateHealthReport(): Promise<FlightResult<string>>;

  getPlatformPerformance(
    platform: string
  ): Promise<FlightResult<PlatformPerformance>>;
}

// Event Emitter Interface for Real-time Events
export interface RealtimeEventEmitter {
  on(event: 'connection', listener: (connection: ConnectionInfo) => void): void;
  on(event: 'disconnection', listener: (connectionId: ConnectionId) => void): void;
  on(event: 'message', listener: (message: RealtimeMessage) => void): void;
  on(event: 'event', listener: (event: RealtimeEvent) => void): void;
  on(event: 'error', listener: (error: Error) => void): void;
  
  off(event: string, listener: Function): void;
  emit(event: string, ...args: any[]): boolean;
}

// Platform-specific helper types
export interface FlightCorePlatformConfig {
  platform: 'dreamcast' | 'psp' | 'vita' | string;
  memoryConstraint: string;
  connectionMode: 'polling' | 'wifi' | 'websocket';
  capabilities: string[];
}

export interface V6RPlatformConfig {
  subscriptionTier: 'free' | 'individual' | 'team' | 'enterprise';
  clientType: ClientType;
  capabilities: string[];
  billingEnabled: boolean;
}

// Utility type for creating typed event handlers
export type EventHandler<T extends RealtimeEvent> = (event: T) => void | Promise<void>;

// Type guards for real-time events
export const isMemoryUpdateEvent = (event: RealtimeEvent): event is { type: 'memory-update'; data: MemoryUsageSnapshot } =>
  event.type === 'memory-update';

export const isComponentUpdateEvent = (event: RealtimeEvent): event is { type: 'component-update'; data: ComponentInfo } =>
  event.type === 'component-update';

export const isSessionEvent = (event: RealtimeEvent): event is { type: 'session-event'; data: SessionEvent } =>
  event.type === 'session-event';

export const isAuthEvent = (event: RealtimeEvent): event is { type: 'auth-event'; data: AuthEvent } =>
  event.type === 'auth-event';

export const isSystemEvent = (event: RealtimeEvent): event is { type: 'system-event'; data: SystemEvent } =>
  event.type === 'system-event';

export const isV6REvent = (event: RealtimeEvent): event is { type: 'v6r-event'; data: V6REvent } =>
  event.type === 'v6r-event';

export const isFlightEvent = (event: RealtimeEvent): event is { type: 'flight-event'; data: FlightEvent } =>
  event.type === 'flight-event';

export const isCustomEvent = (event: RealtimeEvent): event is { type: 'custom-event'; data: CustomEvent } =>
  event.type === 'custom-event';
