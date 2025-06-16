// Enhanced Flight Error Types - Generic Implementation
// Production-ready Go types with enhanced features and utilities

package memory

import (
	"fmt"
	"time"
)

// Core error types
type ErrorContext struct {
	Source    string         `json:"source"`
	Operation string         `json:"operation"`
	SessionID *string        `json:"session_id,omitempty"`
	UserID    *string        `json:"user_id,omitempty"`
	Platform  *string        `json:"platform,omitempty"`
	ServiceID *string        `json:"service_id,omitempty"`
	Metadata  []MetadataPair `json:"metadata"`
}

type MetadataPair struct {
	Key   string `json:"key"`
	Value string `json:"value"`
}

type FlightError struct {
	ID        string        `json:"id"`
	Severity  ErrorSeverity `json:"severity"`
	Category  ErrorCategory `json:"category"`
	Message   string        `json:"message"`
	Details   *string       `json:"details,omitempty"`
	Context   ErrorContext  `json:"context"`
	Timestamp uint64        `json:"timestamp"`
	Cause     *string       `json:"cause,omitempty"`
}

type ErrorSeverity string

const (
	ErrorSeverityInfo     ErrorSeverity = "info"
	ErrorSeverityWarning  ErrorSeverity = "warning"
	ErrorSeverityError    ErrorSeverity = "error"
	ErrorSeverityCritical ErrorSeverity = "critical"
	ErrorSeverityFatal    ErrorSeverity = "fatal"
)

type ErrorCategory string

const (
	ErrorCategoryMemory             ErrorCategory = "memory"
	ErrorCategoryPlatform           ErrorCategory = "platform"
	ErrorCategoryNetwork            ErrorCategory = "network"
	ErrorCategoryValidation         ErrorCategory = "validation"
	ErrorCategorySecurity           ErrorCategory = "security"
	ErrorCategoryComponent          ErrorCategory = "component"
	ErrorCategoryServiceIntegration ErrorCategory = "service-integration"
	ErrorCategoryFlightSystem       ErrorCategory = "flight-system"
	ErrorCategoryApplication        ErrorCategory = "application"
	ErrorCategoryUnknown            ErrorCategory = "unknown"
)

// Result type for exception-free error handling
type FlightResult[T any] struct {
	Value *T           `json:"value,omitempty"`
	Error *FlightError `json:"error,omitempty"`
}

func (r FlightResult[T]) IsOk() bool {
	return r.Error == nil
}

func (r FlightResult[T]) IsErr() bool {
	return r.Error != nil
}

func (r FlightResult[T]) Unwrap() (T, error) {
	if r.Error != nil {
		return *new(T), *r.Error
	}
	if r.Value == nil {
		return *new(T), fmt.Errorf("result has no value")
	}
	return *r.Value, nil
}

func (r FlightResult[T]) UnwrapOr(defaultValue T) T {
	if r.Error != nil || r.Value == nil {
		return defaultValue
	}
	return *r.Value
}

type ErrorCollection struct {
	Errors            []FlightError       `json:"errors"`
	SeverityCounts    []SeverityCountPair `json:"severity_counts"`
	CategoryCounts    []CategoryCountPair `json:"category_counts"`
	HasBlockingErrors bool                `json:"has_blocking_errors"`
	TotalCount        uint32              `json:"total_count"`
}

type SeverityCountPair struct {
	Severity ErrorSeverity `json:"severity"`
	Count    uint32        `json:"count"`
}

type CategoryCountPair struct {
	Category ErrorCategory `json:"category"`
	Count    uint32        `json:"count"`
}

// Error code enums
type ServiceErrorCode string

const (
	ServiceErrorCodeResourceAllocationFailed ServiceErrorCode = "resource-allocation-failed"
	ServiceErrorCodeSessionLimitExceeded     ServiceErrorCode = "session-limit-exceeded"
	ServiceErrorCodeAuthFailed               ServiceErrorCode = "auth-failed"
	ServiceErrorCodeRateLimitExceeded        ServiceErrorCode = "rate-limit-exceeded"
	ServiceErrorCodeServiceUnavailable       ServiceErrorCode = "service-unavailable"
	ServiceErrorCodeInvalidConfig            ServiceErrorCode = "invalid-config"
	ServiceErrorCodeResourceLimitExceeded    ServiceErrorCode = "resource-limit-exceeded"
	ServiceErrorCodeConnectionFailed         ServiceErrorCode = "connection-failed"
	ServiceErrorCodeRequestTimeout           ServiceErrorCode = "request-timeout"
	ServiceErrorCodeInvalidRequest           ServiceErrorCode = "invalid-request"
	ServiceErrorCodePermissionDenied         ServiceErrorCode = "permission-denied"
	ServiceErrorCodeResourceNotFound         ServiceErrorCode = "resource-not-found"
)

type PlatformErrorCode string

const (
	PlatformErrorCodeInsufficientPlatformMemory  PlatformErrorCode = "insufficient-platform-memory"
	PlatformErrorCodeFeatureNotSupported         PlatformErrorCode = "feature-not-supported"
	PlatformErrorCodeHardwareConstraintViolation PlatformErrorCode = "hardware-constraint-violation"
	PlatformErrorCodePlatformInitFailed          PlatformErrorCode = "platform-init-failed"
	PlatformErrorCodeComponentLoadFailed         PlatformErrorCode = "component-load-failed"
	PlatformErrorCodeCompatibilityError          PlatformErrorCode = "compatibility-error"
	PlatformErrorCodePlatformResourceExhausted   PlatformErrorCode = "platform-resource-exhausted"
)

type NetworkErrorCode string

const (
	NetworkErrorCodeConnectionTimeout      NetworkErrorCode = "connection-timeout"
	NetworkErrorCodeConnectionRefused      NetworkErrorCode = "connection-refused"
	NetworkErrorCodeDnsResolutionFailed    NetworkErrorCode = "dns-resolution-failed"
	NetworkErrorCodeTlsError               NetworkErrorCode = "tls-error"
	NetworkErrorCodeHttpError              NetworkErrorCode = "http-error"
	NetworkErrorCodeWebsocketError         NetworkErrorCode = "websocket-error"
	NetworkErrorCodeRequestTimeout         NetworkErrorCode = "request-timeout"
	NetworkErrorCodeNetworkUnreachable     NetworkErrorCode = "network-unreachable"
	NetworkErrorCodeProtocolError          NetworkErrorCode = "protocol-error"
	NetworkErrorCodeBandwidthLimitExceeded NetworkErrorCode = "bandwidth-limit-exceeded"
)

type ValidationErrorDetails struct {
	Field    string  `json:"field"`
	Rule     string  `json:"rule"`
	Expected string  `json:"expected"`
	Actual   string  `json:"actual"`
	Context  *string `json:"context,omitempty"`
}

type ErrorRecoverySuggestion struct {
	ActionType  RecoveryActionType `json:"action_type"`
	Description string             `json:"description"`
	CanAutomate bool               `json:"can_automate"`
	Priority    uint32             `json:"priority"`
}

type RecoveryActionType string

const (
	RecoveryActionTypeRetry            RecoveryActionType = "retry"
	RecoveryActionTypeReduceResources  RecoveryActionType = "reduce-resources"
	RecoveryActionTypeUpdateConfig     RecoveryActionType = "update-config"
	RecoveryActionTypeContactSupport   RecoveryActionType = "contact-support"
	RecoveryActionTypeTryAlternative   RecoveryActionType = "try-alternative"
	RecoveryActionTypeWaitRetry        RecoveryActionType = "wait-retry"
	RecoveryActionTypeUpgradeResources RecoveryActionType = "upgrade-resources"
	RecoveryActionTypeCheckStatus      RecoveryActionType = "check-status"
)

// Analytics types
type ErrorAnalyticsSummary struct {
	TotalErrors uint32              `json:"total_errors"`
	BySeverity  []SeverityCountPair `json:"by_severity"`
	ByCategory  []CategoryCountPair `json:"by_category"`
	TopErrors   []TopErrorPair      `json:"top_errors"`
	ErrorRate   float32             `json:"error_rate"`
	TimeWindow  uint64              `json:"time_window"`
	AnalyzedAt  uint64              `json:"analyzed_at"`
}

type TopErrorPair struct {
	Message string `json:"message"`
	Count   uint32 `json:"count"`
}

type SystemHealthStatus string

const (
	SystemHealthStatusHealthy  SystemHealthStatus = "healthy"
	SystemHealthStatusDegraded SystemHealthStatus = "degraded"
	SystemHealthStatusCritical SystemHealthStatus = "critical"
	SystemHealthStatusFailing  SystemHealthStatus = "failing"
)

type ErrorTrendAnalysis struct {
	Trend           TrendDirection `json:"trend"`
	Confidence      float32        `json:"confidence"`
	PredictedRate   float32        `json:"predicted_rate"`
	Recommendations []string       `json:"recommendations"`
}

// Event system types
type ErrorEventData struct {
	SessionID string `json:"session_id"`
	Timestamp uint64 `json:"timestamp"`
}

type ErrorOccurredEventData struct {
	ErrorEventData
	Error FlightError `json:"error"`
}

type ErrorResolvedEventData struct {
	ErrorEventData
	ErrorID          string `json:"error_id"`
	ResolutionMethod string `json:"resolution_method"`
}

type SystemHealthEventData struct {
	ErrorEventData
	HealthStatus SystemHealthStatus    `json:"health_status"`
	Analytics    ErrorAnalyticsSummary `json:"analytics"`
}

type ErrorEventHandler func(data interface{})

// Configuration types
type ServiceIntegrationConfig struct {
	ServiceID        string                             `json:"service_id"`
	ServiceName      string                             `json:"service_name"`
	ErrorCodeMapping map[string]ServiceErrorCode        `json:"error_code_mapping"`
	RetryStrategies  map[ServiceErrorCode]RetryStrategy `json:"retry_strategies"`
}

type RetryStrategy struct {
	MaxAttempts       int                `json:"max_attempts"`
	InitialDelayMs    int                `json:"initial_delay_ms"`
	BackoffMultiplier float32            `json:"backoff_multiplier"`
	MaxDelayMs        int                `json:"max_delay_ms"`
	RetryableErrors   []ServiceErrorCode `json:"retryable_errors"`
}

type ErrorHandlingConfig struct {
	EnableAnalytics           bool          `json:"enable_analytics"`
	AnalyticsWindowMs         int           `json:"analytics_window_ms"`
	MaxErrorsInCollection     int           `json:"max_errors_in_collection"`
	EnableRecoverySuggestions bool          `json:"enable_recovery_suggestions"`
	LogLevel                  ErrorSeverity `json:"log_level"`
}

// API integration types
type ErrorApiResponse struct {
	Success  bool                   `json:"success"`
	Error    ErrorApiInfo           `json:"error"`
	Metadata map[string]interface{} `json:"metadata,omitempty"`
}

type ErrorApiInfo struct {
	ID        string        `json:"id"`
	Code      string        `json:"code"`
	Message   string        `json:"message"`
	Severity  ErrorSeverity `json:"severity"`
	Timestamp uint64        `json:"timestamp"`
}

type SuccessApiResponse[T any] struct {
	Success  bool                   `json:"success"`
	Data     T                      `json:"data"`
	Metadata map[string]interface{} `json:"metadata,omitempty"`
}

// FlightError implements the error interface
func (e FlightError) Error() string {
	if e.Details != nil {
		return fmt.Sprintf("[%s] %s: %s (%s)", e.Severity, e.Category, e.Message, *e.Details)
	}
	return fmt.Sprintf("[%s] %s: %s", e.Severity, e.Category, e.Message)
}

// Helper functions for type creation
func NewFlightError(severity ErrorSeverity, category ErrorCategory, message string, context ErrorContext) FlightError {
	return FlightError{
		ID:        generateErrorID(),
		Severity:  severity,
		Category:  category,
		Message:   message,
		Context:   context,
		Timestamp: uint64(time.Now().Unix()),
	}
}

func NewFlightResult[T any](value T) FlightResult[T] {
	return FlightResult[T]{Value: &value}
}

func NewFlightResultError[T any](err FlightError) FlightResult[T] {
	return FlightResult[T]{Error: &err}
}

func NewErrorContext(source, operation string) ErrorContext {
	return ErrorContext{
		Source:    source,
		Operation: operation,
		Metadata:  make([]MetadataPair, 0),
	}
}

func (ctx *ErrorContext) AddMetadata(key, value string) {
	ctx.Metadata = append(ctx.Metadata, MetadataPair{Key: key, Value: value})
}

func (ctx *ErrorContext) SetSessionID(sessionID string) {
	ctx.SessionID = &sessionID
}

func (ctx *ErrorContext) SetUserID(userID string) {
	ctx.UserID = &userID
}

func (ctx *ErrorContext) SetPlatform(platform string) {
	ctx.Platform = &platform
}

func (ctx *ErrorContext) SetServiceID(serviceID string) {
	ctx.ServiceID = &serviceID
}

// Helper function to generate unique error IDs
func generateErrorID() string {
	timestamp := time.Now().UnixNano()
	return fmt.Sprintf("error_%d_%d", timestamp/1000000, timestamp%1000000)
}

// Utility functions for working with error codes
func GetServiceErrorSeverity(code ServiceErrorCode) ErrorSeverity {
	switch code {
	case ServiceErrorCodeAuthFailed, ServiceErrorCodePermissionDenied:
		return ErrorSeverityCritical
	case ServiceErrorCodeResourceAllocationFailed, ServiceErrorCodeSessionLimitExceeded, ServiceErrorCodeResourceLimitExceeded:
		return ErrorSeverityError
	case ServiceErrorCodeRateLimitExceeded, ServiceErrorCodeServiceUnavailable, ServiceErrorCodeRequestTimeout:
		return ErrorSeverityWarning
	default:
		return ErrorSeverityError
	}
}

func GetPlatformErrorSeverity(code PlatformErrorCode) ErrorSeverity {
	switch code {
	case PlatformErrorCodePlatformInitFailed:
		return ErrorSeverityFatal
	case PlatformErrorCodeInsufficientPlatformMemory, PlatformErrorCodeHardwareConstraintViolation:
		return ErrorSeverityCritical
	case PlatformErrorCodeComponentLoadFailed, PlatformErrorCodePlatformResourceExhausted:
		return ErrorSeverityError
	default:
		return ErrorSeverityWarning
	}
}

func GetNetworkErrorSeverity(code NetworkErrorCode) ErrorSeverity {
	switch code {
	case NetworkErrorCodeTlsError:
		return ErrorSeverityCritical
	case NetworkErrorCodeConnectionRefused, NetworkErrorCodeNetworkUnreachable:
		return ErrorSeverityError
	default:
		return ErrorSeverityWarning
	}
}

// Validation functions
func IsRecoverableError(error FlightError) bool {
	// Security errors typically not recoverable
	if error.Category == ErrorCategorySecurity {
		return false
	}

	// Fatal errors are not recoverable
	if error.Severity == ErrorSeverityFatal {
		return false
	}

	// Check by specific error codes
	errorCode := getErrorCodeFromMetadata(error.Context.Metadata)
	if errorCode != "" {
		if serviceCode := ServiceErrorCode(errorCode); isValidServiceErrorCode(serviceCode) {
			return isServiceErrorRecoverable(serviceCode)
		}
		if platformCode := PlatformErrorCode(errorCode); isValidPlatformErrorCode(platformCode) {
			return isPlatformErrorRecoverable(platformCode)
		}
		if networkCode := NetworkErrorCode(errorCode); isValidNetworkErrorCode(networkCode) {
			return isNetworkErrorRecoverable(networkCode)
		}
	}

	// Default: errors and warnings are typically recoverable
	return error.Severity == ErrorSeverityError || error.Severity == ErrorSeverityWarning
}

// Helper functions
func getErrorCodeFromMetadata(metadata []MetadataPair) string {
	for _, pair := range metadata {
		if pair.Key == "service_error_code" || pair.Key == "platform_error_code" || pair.Key == "network_error_code" {
			return pair.Value
		}
	}
	return ""
}

func isValidServiceErrorCode(code ServiceErrorCode) bool {
	validCodes := []ServiceErrorCode{
		ServiceErrorCodeResourceAllocationFailed, ServiceErrorCodeSessionLimitExceeded,
		ServiceErrorCodeAuthFailed, ServiceErrorCodeRateLimitExceeded,
		ServiceErrorCodeServiceUnavailable, ServiceErrorCodeInvalidConfig,
		ServiceErrorCodeResourceLimitExceeded, ServiceErrorCodeConnectionFailed,
		ServiceErrorCodeRequestTimeout, ServiceErrorCodeInvalidRequest,
		ServiceErrorCodePermissionDenied, ServiceErrorCodeResourceNotFound,
	}
	for _, validCode := range validCodes {
		if code == validCode {
			return true
		}
	}
	return false
}

func isValidPlatformErrorCode(code PlatformErrorCode) bool {
	validCodes := []PlatformErrorCode{
		PlatformErrorCodeInsufficientPlatformMemory, PlatformErrorCodeFeatureNotSupported,
		PlatformErrorCodeHardwareConstraintViolation, PlatformErrorCodePlatformInitFailed,
		PlatformErrorCodeComponentLoadFailed, PlatformErrorCodeCompatibilityError,
		PlatformErrorCodePlatformResourceExhausted,
	}
	for _, validCode := range validCodes {
		if code == validCode {
			return true
		}
	}
	return false
}

func isValidNetworkErrorCode(code NetworkErrorCode) bool {
	validCodes := []NetworkErrorCode{
		NetworkErrorCodeConnectionTimeout, NetworkErrorCodeConnectionRefused,
		NetworkErrorCodeDnsResolutionFailed, NetworkErrorCodeTlsError,
		NetworkErrorCodeHttpError, NetworkErrorCodeWebsocketError,
		NetworkErrorCodeRequestTimeout, NetworkErrorCodeNetworkUnreachable,
		NetworkErrorCodeProtocolError, NetworkErrorCodeBandwidthLimitExceeded,
	}
	for _, validCode := range validCodes {
		if code == validCode {
			return true
		}
	}
	return false
}

func isServiceErrorRecoverable(code ServiceErrorCode) bool {
	switch code {
	case ServiceErrorCodeAuthFailed, ServiceErrorCodePermissionDenied, ServiceErrorCodeInvalidRequest:
		return false
	case ServiceErrorCodeRateLimitExceeded, ServiceErrorCodeServiceUnavailable, ServiceErrorCodeRequestTimeout, ServiceErrorCodeConnectionFailed:
		return true
	default:
		return true
	}
}

func isPlatformErrorRecoverable(code PlatformErrorCode) bool {
	switch code {
	case PlatformErrorCodePlatformInitFailed, PlatformErrorCodeFeatureNotSupported, PlatformErrorCodeCompatibilityError:
		return false
	default:
		return true
	}
}

func isNetworkErrorRecoverable(code NetworkErrorCode) bool {
	switch code {
	case NetworkErrorCodeTlsError, NetworkErrorCodeProtocolError:
		return false
	default:
		return true
	}
}
