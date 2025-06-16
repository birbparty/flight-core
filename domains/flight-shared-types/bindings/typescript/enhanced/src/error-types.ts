// Enhanced Flight Error Types - Generic Implementation
// Production-ready types with enhanced type safety and utilities

export interface ErrorContext {
  readonly source: string;
  readonly operation: string;
  readonly sessionId?: string;
  readonly userId?: string;
  readonly platform?: string;
  readonly serviceId?: string;
  readonly metadata: ReadonlyArray<readonly [string, string]>;
}

export interface FlightError {
  readonly id: string;
  readonly severity: ErrorSeverity;
  readonly category: ErrorCategory;
  readonly message: string;
  readonly details?: string;
  readonly context: ErrorContext;
  readonly timestamp: number;
  readonly cause?: string;
}

export enum ErrorSeverity {
  Info = "info",
  Warning = "warning",
  Error = "error",
  Critical = "critical",
  Fatal = "fatal"
}

export enum ErrorCategory {
  Memory = "memory",
  Platform = "platform",
  Network = "network",
  Validation = "validation",
  Security = "security",
  Component = "component",
  ServiceIntegration = "service-integration",
  FlightSystem = "flight-system",
  Application = "application",
  Unknown = "unknown"
}

export type FlightResult<T> = 
  | { readonly tag: "ok"; readonly val: T }
  | { readonly tag: "err"; readonly val: FlightError };

export interface ErrorCollection {
  readonly errors: ReadonlyArray<FlightError>;
  readonly severityCounts: ReadonlyArray<readonly [ErrorSeverity, number]>;
  readonly categoryCounts: ReadonlyArray<readonly [ErrorCategory, number]>;
  readonly hasBlockingErrors: boolean;
  readonly totalCount: number;
}

export enum ServiceErrorCode {
  ResourceAllocationFailed = "resource-allocation-failed",
  SessionLimitExceeded = "session-limit-exceeded",
  AuthFailed = "auth-failed",
  RateLimitExceeded = "rate-limit-exceeded",
  ServiceUnavailable = "service-unavailable",
  InvalidConfig = "invalid-config",
  ResourceLimitExceeded = "resource-limit-exceeded",
  ConnectionFailed = "connection-failed",
  RequestTimeout = "request-timeout",
  InvalidRequest = "invalid-request",
  PermissionDenied = "permission-denied",
  ResourceNotFound = "resource-not-found"
}

export enum PlatformErrorCode {
  InsufficientPlatformMemory = "insufficient-platform-memory",
  FeatureNotSupported = "feature-not-supported",
  HardwareConstraintViolation = "hardware-constraint-violation",
  PlatformInitFailed = "platform-init-failed",
  ComponentLoadFailed = "component-load-failed",
  CompatibilityError = "compatibility-error",
  PlatformResourceExhausted = "platform-resource-exhausted"
}

export enum NetworkErrorCode {
  ConnectionTimeout = "connection-timeout",
  ConnectionRefused = "connection-refused",
  DnsResolutionFailed = "dns-resolution-failed",
  TlsError = "tls-error",
  HttpError = "http-error",
  WebsocketError = "websocket-error",
  RequestTimeout = "request-timeout",
  NetworkUnreachable = "network-unreachable",
  ProtocolError = "protocol-error",
  BandwidthLimitExceeded = "bandwidth-limit-exceeded"
}

export interface ValidationErrorDetails {
  readonly field: string;
  readonly rule: string;
  readonly expected: string;
  readonly actual: string;
  readonly context?: string;
}

export interface ErrorRecoverySuggestion {
  readonly actionType: RecoveryActionType;
  readonly description: string;
  readonly canAutomate: boolean;
  readonly priority: number;
}

export enum RecoveryActionType {
  Retry = "retry",
  ReduceResources = "reduce-resources",
  UpdateConfig = "update-config",
  ContactSupport = "contact-support",
  TryAlternative = "try-alternative",
  WaitRetry = "wait-retry",
  UpgradeResources = "upgrade-resources",
  CheckStatus = "check-status"
}

// Analytics types
export interface ErrorAnalyticsSummary {
  readonly totalErrors: number;
  readonly bySeverity: ReadonlyArray<readonly [ErrorSeverity, number]>;
  readonly byCategory: ReadonlyArray<readonly [ErrorCategory, number]>;
  readonly topErrors: ReadonlyArray<readonly [string, number]>;
  readonly errorRate: number;
  readonly timeWindow: number;
  readonly analyzedAt: number;
}

export enum SystemHealthStatus {
  Healthy = "healthy",
  Degraded = "degraded",
  Critical = "critical",
  Failing = "failing"
}

export interface ErrorTrendAnalysis {
  readonly trend: TrendDirection;
  readonly confidence: number;
  readonly predictedRate: number;
  readonly recommendations: ReadonlyArray<string>;
}

export enum TrendDirection {
  Increasing = "increasing",
  Decreasing = "decreasing",
  Stable = "stable",
  Volatile = "volatile"
}

// Event system types for real-time error handling
export interface ErrorEventData {
  readonly sessionId: string;
  readonly timestamp: number;
}

export interface ErrorOccurredEventData extends ErrorEventData {
  readonly error: FlightError;
}

export interface ErrorResolvedEventData extends ErrorEventData {
  readonly errorId: string;
  readonly resolutionMethod: string;
}

export interface SystemHealthEventData extends ErrorEventData {
  readonly healthStatus: SystemHealthStatus;
  readonly analytics: ErrorAnalyticsSummary;
}

export type ErrorEventHandler<T extends ErrorEventData = ErrorEventData> = (data: T) => void;

// Utility types for service integration
export interface ServiceIntegrationConfig {
  readonly serviceId: string;
  readonly serviceName: string;
  readonly errorCodeMapping: ReadonlyMap<string, ServiceErrorCode>;
  readonly retryStrategies: ReadonlyMap<ServiceErrorCode, RetryStrategy>;
}

export interface RetryStrategy {
  readonly maxAttempts: number;
  readonly initialDelayMs: number;
  readonly backoffMultiplier: number;
  readonly maxDelayMs: number;
  readonly retryableErrors: ReadonlyArray<ServiceErrorCode>;
}

// Configuration types
export interface ErrorHandlingConfig {
  readonly enableAnalytics: boolean;
  readonly analyticsWindowMs: number;
  readonly maxErrorsInCollection: number;
  readonly enableRecoverySuggestions: boolean;
  readonly logLevel: ErrorSeverity;
}

// API integration types
export interface ErrorApiResponse {
  readonly success: false;
  readonly error: {
    readonly id: string;
    readonly code: string;
    readonly message: string;
    readonly severity: ErrorSeverity;
    readonly timestamp: number;
  };
  readonly metadata?: {
    readonly sessionId?: string;
    readonly userId?: string;
    readonly requestId?: string;
    readonly [key: string]: string | undefined;
  };
}

export interface SuccessApiResponse<T> {
  readonly success: true;
  readonly data: T;
  readonly metadata?: {
    readonly requestId?: string;
    readonly [key: string]: string | undefined;
  };
}

export type ApiResponse<T> = SuccessApiResponse<T> | ErrorApiResponse;
