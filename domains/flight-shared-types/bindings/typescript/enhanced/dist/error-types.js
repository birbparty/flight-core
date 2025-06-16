// Enhanced Flight Error Types - Generic Implementation
// Production-ready types with enhanced type safety and utilities
export var ErrorSeverity;
(function (ErrorSeverity) {
    ErrorSeverity["Info"] = "info";
    ErrorSeverity["Warning"] = "warning";
    ErrorSeverity["Error"] = "error";
    ErrorSeverity["Critical"] = "critical";
    ErrorSeverity["Fatal"] = "fatal";
})(ErrorSeverity || (ErrorSeverity = {}));
export var ErrorCategory;
(function (ErrorCategory) {
    ErrorCategory["Memory"] = "memory";
    ErrorCategory["Platform"] = "platform";
    ErrorCategory["Network"] = "network";
    ErrorCategory["Validation"] = "validation";
    ErrorCategory["Security"] = "security";
    ErrorCategory["Component"] = "component";
    ErrorCategory["ServiceIntegration"] = "service-integration";
    ErrorCategory["FlightSystem"] = "flight-system";
    ErrorCategory["Application"] = "application";
    ErrorCategory["Unknown"] = "unknown";
})(ErrorCategory || (ErrorCategory = {}));
export var ServiceErrorCode;
(function (ServiceErrorCode) {
    ServiceErrorCode["ResourceAllocationFailed"] = "resource-allocation-failed";
    ServiceErrorCode["SessionLimitExceeded"] = "session-limit-exceeded";
    ServiceErrorCode["AuthFailed"] = "auth-failed";
    ServiceErrorCode["RateLimitExceeded"] = "rate-limit-exceeded";
    ServiceErrorCode["ServiceUnavailable"] = "service-unavailable";
    ServiceErrorCode["InvalidConfig"] = "invalid-config";
    ServiceErrorCode["ResourceLimitExceeded"] = "resource-limit-exceeded";
    ServiceErrorCode["ConnectionFailed"] = "connection-failed";
    ServiceErrorCode["RequestTimeout"] = "request-timeout";
    ServiceErrorCode["InvalidRequest"] = "invalid-request";
    ServiceErrorCode["PermissionDenied"] = "permission-denied";
    ServiceErrorCode["ResourceNotFound"] = "resource-not-found";
})(ServiceErrorCode || (ServiceErrorCode = {}));
export var PlatformErrorCode;
(function (PlatformErrorCode) {
    PlatformErrorCode["InsufficientPlatformMemory"] = "insufficient-platform-memory";
    PlatformErrorCode["FeatureNotSupported"] = "feature-not-supported";
    PlatformErrorCode["HardwareConstraintViolation"] = "hardware-constraint-violation";
    PlatformErrorCode["PlatformInitFailed"] = "platform-init-failed";
    PlatformErrorCode["ComponentLoadFailed"] = "component-load-failed";
    PlatformErrorCode["CompatibilityError"] = "compatibility-error";
    PlatformErrorCode["PlatformResourceExhausted"] = "platform-resource-exhausted";
})(PlatformErrorCode || (PlatformErrorCode = {}));
export var NetworkErrorCode;
(function (NetworkErrorCode) {
    NetworkErrorCode["ConnectionTimeout"] = "connection-timeout";
    NetworkErrorCode["ConnectionRefused"] = "connection-refused";
    NetworkErrorCode["DnsResolutionFailed"] = "dns-resolution-failed";
    NetworkErrorCode["TlsError"] = "tls-error";
    NetworkErrorCode["HttpError"] = "http-error";
    NetworkErrorCode["WebsocketError"] = "websocket-error";
    NetworkErrorCode["RequestTimeout"] = "request-timeout";
    NetworkErrorCode["NetworkUnreachable"] = "network-unreachable";
    NetworkErrorCode["ProtocolError"] = "protocol-error";
    NetworkErrorCode["BandwidthLimitExceeded"] = "bandwidth-limit-exceeded";
})(NetworkErrorCode || (NetworkErrorCode = {}));
export var RecoveryActionType;
(function (RecoveryActionType) {
    RecoveryActionType["Retry"] = "retry";
    RecoveryActionType["ReduceResources"] = "reduce-resources";
    RecoveryActionType["UpdateConfig"] = "update-config";
    RecoveryActionType["ContactSupport"] = "contact-support";
    RecoveryActionType["TryAlternative"] = "try-alternative";
    RecoveryActionType["WaitRetry"] = "wait-retry";
    RecoveryActionType["UpgradeResources"] = "upgrade-resources";
    RecoveryActionType["CheckStatus"] = "check-status";
})(RecoveryActionType || (RecoveryActionType = {}));
export var SystemHealthStatus;
(function (SystemHealthStatus) {
    SystemHealthStatus["Healthy"] = "healthy";
    SystemHealthStatus["Degraded"] = "degraded";
    SystemHealthStatus["Critical"] = "critical";
    SystemHealthStatus["Failing"] = "failing";
})(SystemHealthStatus || (SystemHealthStatus = {}));
export var TrendDirection;
(function (TrendDirection) {
    TrendDirection["Increasing"] = "increasing";
    TrendDirection["Decreasing"] = "decreasing";
    TrendDirection["Stable"] = "stable";
    TrendDirection["Volatile"] = "volatile";
})(TrendDirection || (TrendDirection = {}));
//# sourceMappingURL=error-types.js.map