// Package errortypes provides Go bindings for Flight Error Types
// Universal error handling types for Flight-Core and external service integration
package errortypes

import (
	"encoding/json"
	"fmt"
	"time"
)

// ErrorSeverity represents core error severity levels
// Used for error prioritization and handling strategies across all integrating services
type ErrorSeverity int

const (
	// Informational - no action required
	ErrorSeverityInfo ErrorSeverity = iota
	// Warning - attention recommended
	ErrorSeverityWarning
	// Error - action required but system continues
	ErrorSeverityError
	// Critical - immediate action required
	ErrorSeverityCritical
	// Fatal - system cannot continue
	ErrorSeverityFatal
)

func (es ErrorSeverity) String() string {
	switch es {
	case ErrorSeverityInfo:
		return "info"
	case ErrorSeverityWarning:
		return "warning"
	case ErrorSeverityError:
		return "error"
	case ErrorSeverityCritical:
		return "critical"
	case ErrorSeverityFatal:
		return "fatal"
	default:
		return "unknown"
	}
}

// ErrorCategory represents error categories for systematic error handling
// Generic categories that any service can map their errors to
type ErrorCategory int

const (
	// Memory-related errors
	ErrorCategoryMemory ErrorCategory = iota
	// Platform-specific errors
	ErrorCategoryPlatform
	// Network and I/O errors
	ErrorCategoryNetwork
	// Validation and input errors
	ErrorCategoryValidation
	// Authentication and authorization
	ErrorCategorySecurity
	// Component Model errors
	ErrorCategoryComponent
	// External service integration errors
	ErrorCategoryServiceIntegration
	// Flight-Core system errors
	ErrorCategoryFlightSystem
	// Application-level errors
	ErrorCategoryApplication
	// Unknown/uncategorized errors
	ErrorCategoryUnknown
)

func (ec ErrorCategory) String() string {
	switch ec {
	case ErrorCategoryMemory:
		return "memory"
	case ErrorCategoryPlatform:
		return "platform"
	case ErrorCategoryNetwork:
		return "network"
	case ErrorCategoryValidation:
		return "validation"
	case ErrorCategorySecurity:
		return "security"
	case ErrorCategoryComponent:
		return "component"
	case ErrorCategoryServiceIntegration:
		return "service-integration"
	case ErrorCategoryFlightSystem:
		return "flight-system"
	case ErrorCategoryApplication:
		return "application"
	case ErrorCategoryUnknown:
		return "unknown"
	default:
		return "unknown"
	}
}

// ErrorContext represents structured error context for debugging and service integration
// Provides comprehensive context without performance impact
type ErrorContext struct {
	// Component or service that generated the error
	Source string `json:"source"`
	// Specific operation that failed
	Operation string `json:"operation"`
	// Session or request identifier
	SessionID *string `json:"session_id,omitempty"`
	// User identifier (for authenticated contexts)
	UserID *string `json:"user_id,omitempty"`
	// Platform identifier
	Platform *string `json:"platform,omitempty"`
	// Service-specific identifier (for external integrations)
	ServiceID *string `json:"service_id,omitempty"`
	// Additional key-value context pairs for extensibility
	Metadata []MetadataPair `json:"metadata"`
}

// MetadataPair represents key-value metadata
type MetadataPair struct {
	Key   string `json:"key"`
	Value string `json:"value"`
}

// FlightError represents core error type with rich context
// Universal error structure for all Flight systems and integrating services
type FlightError struct {
	// Unique error identifier
	ID string `json:"id"`
	// Error severity level
	Severity ErrorSeverity `json:"severity"`
	// Error category for handling
	Category ErrorCategory `json:"category"`
	// Human-readable error message
	Message string `json:"message"`
	// Technical details for debugging
	Details *string `json:"details,omitempty"`
	// Rich context information
	Context ErrorContext `json:"context"`
	// When the error occurred (Unix timestamp)
	Timestamp uint64 `json:"timestamp"`
	// Optional nested/causative error ID
	Cause *string `json:"cause,omitempty"`
}

// Error implements the error interface
func (fe *FlightError) Error() string {
	if fe.Details != nil {
		return fmt.Sprintf("[%s/%s] %s: %s", fe.Severity.String(), fe.Category.String(), fe.Message, *fe.Details)
	}
	return fmt.Sprintf("[%s/%s] %s", fe.Severity.String(), fe.Category.String(), fe.Message)
}

// FlightResult represents a result that can be either success or error
// Used throughout Flight systems for robust error management
type FlightResult[T any] struct {
	Success bool         `json:"success"`
	Value   *T           `json:"value,omitempty"`
	Error   *FlightError `json:"error,omitempty"`
}

// NewSuccessResult creates a successful result
func NewSuccessResult[T any](value T) FlightResult[T] {
	return FlightResult[T]{
		Success: true,
		Value:   &value,
	}
}

// NewErrorResult creates an error result
func NewErrorResult[T any](err *FlightError) FlightResult[T] {
	return FlightResult[T]{
		Success: false,
		Error:   err,
	}
}

// IsOk returns true if the result is successful
func (fr *FlightResult[T]) IsOk() bool {
	return fr.Success
}

// IsErr returns true if the result is an error
func (fr *FlightResult[T]) IsErr() bool {
	return !fr.Success
}

// Unwrap returns the value if successful, panics if error
func (fr *FlightResult[T]) Unwrap() T {
	if !fr.Success {
		panic("called Unwrap on error result")
	}
	return *fr.Value
}

// UnwrapOr returns the value if successful, or the default value if error
func (fr *FlightResult[T]) UnwrapOr(defaultValue T) T {
	if fr.Success {
		return *fr.Value
	}
	return defaultValue
}

// ErrorCollection represents error collection for batch operations
// Handles multiple errors from complex operations
type ErrorCollection struct {
	// All errors that occurred
	Errors []FlightError `json:"errors"`
	// Count of errors by severity
	SeverityCounts map[ErrorSeverity]uint32 `json:"severity_counts"`
	// Count of errors by category
	CategoryCounts map[ErrorCategory]uint32 `json:"category_counts"`
	// Whether any critical/fatal errors exist
	HasBlockingErrors bool `json:"has_blocking_errors"`
	// Total error count
	TotalCount uint32 `json:"total_count"`
}

// ServiceErrorCode represents generic service error codes for external integrations
// Common error patterns that external services can map to
type ServiceErrorCode int

const (
	// Resource allocation failed
	ServiceErrorResourceAllocationFailed ServiceErrorCode = iota
	// Session/connection limit exceeded
	ServiceErrorSessionLimitExceeded
	// Authentication failed
	ServiceErrorAuthFailed
	// API rate limit exceeded
	ServiceErrorRateLimitExceeded
	// Service temporarily unavailable
	ServiceErrorServiceUnavailable
	// Invalid configuration
	ServiceErrorInvalidConfig
	// Resource limit exceeded (memory, storage, etc.)
	ServiceErrorResourceLimitExceeded
	// Connection/communication failed
	ServiceErrorConnectionFailed
	// Request timeout
	ServiceErrorRequestTimeout
	// Invalid request format
	ServiceErrorInvalidRequest
	// Permission denied
	ServiceErrorPermissionDenied
	// Resource not found
	ServiceErrorResourceNotFound
)

func (sec ServiceErrorCode) String() string {
	switch sec {
	case ServiceErrorResourceAllocationFailed:
		return "resource-allocation-failed"
	case ServiceErrorSessionLimitExceeded:
		return "session-limit-exceeded"
	case ServiceErrorAuthFailed:
		return "auth-failed"
	case ServiceErrorRateLimitExceeded:
		return "rate-limit-exceeded"
	case ServiceErrorServiceUnavailable:
		return "service-unavailable"
	case ServiceErrorInvalidConfig:
		return "invalid-config"
	case ServiceErrorResourceLimitExceeded:
		return "resource-limit-exceeded"
	case ServiceErrorConnectionFailed:
		return "connection-failed"
	case ServiceErrorRequestTimeout:
		return "request-timeout"
	case ServiceErrorInvalidRequest:
		return "invalid-request"
	case ServiceErrorPermissionDenied:
		return "permission-denied"
	case ServiceErrorResourceNotFound:
		return "resource-not-found"
	default:
		return "unknown"
	}
}

// ValidationErrorDetails represents structured validation error information
type ValidationErrorDetails struct {
	// Field that failed validation
	Field string `json:"field"`
	// Validation rule that was violated
	Rule string `json:"rule"`
	// Expected value or format
	Expected string `json:"expected"`
	// Actual value received
	Actual string `json:"actual"`
	// Additional validation context
	Context *string `json:"context,omitempty"`
}

// RecoveryActionType represents recovery action types
type RecoveryActionType int

const (
	// Retry the operation
	RecoveryActionRetry RecoveryActionType = iota
	// Reduce resource usage
	RecoveryActionReduceResources
	// Update configuration
	RecoveryActionUpdateConfig
	// Contact support
	RecoveryActionContactSupport
	// Try alternative approach
	RecoveryActionTryAlternative
	// Wait and retry later
	RecoveryActionWaitRetry
	// Upgrade service plan/resources
	RecoveryActionUpgradeResources
	// Check system status
	RecoveryActionCheckStatus
)

func (rat RecoveryActionType) String() string {
	switch rat {
	case RecoveryActionRetry:
		return "retry"
	case RecoveryActionReduceResources:
		return "reduce-resources"
	case RecoveryActionUpdateConfig:
		return "update-config"
	case RecoveryActionContactSupport:
		return "contact-support"
	case RecoveryActionTryAlternative:
		return "try-alternative"
	case RecoveryActionWaitRetry:
		return "wait-retry"
	case RecoveryActionUpgradeResources:
		return "upgrade-resources"
	case RecoveryActionCheckStatus:
		return "check-status"
	default:
		return "unknown"
	}
}

// ErrorRecoverySuggestion represents actionable suggestions for error resolution
type ErrorRecoverySuggestion struct {
	// Type of recovery action
	ActionType RecoveryActionType `json:"action_type"`
	// Human-readable suggestion
	Description string `json:"description"`
	// Whether this action can be automated
	CanAutomate bool `json:"can_automate"`
	// Priority of this suggestion (higher = more important)
	Priority uint32 `json:"priority"`
}

// SystemHealthStatus represents system health status based on error patterns
type SystemHealthStatus int

const (
	// System operating normally
	SystemHealthHealthy SystemHealthStatus = iota
	// Some issues but system functioning
	SystemHealthDegraded
	// Serious issues affecting functionality
	SystemHealthCritical
	// System experiencing significant failures
	SystemHealthFailing
)

func (shs SystemHealthStatus) String() string {
	switch shs {
	case SystemHealthHealthy:
		return "healthy"
	case SystemHealthDegraded:
		return "degraded"
	case SystemHealthCritical:
		return "critical"
	case SystemHealthFailing:
		return "failing"
	default:
		return "unknown"
	}
}

// ErrorManager manages error operations
type ErrorManager struct {
	errorHistory []FlightError
}

// NewErrorManager creates a new error manager
func NewErrorManager() *ErrorManager {
	return &ErrorManager{
		errorHistory: make([]FlightError, 0),
	}
}

// CreateError creates a new error with context
// Primary error creation function for all Flight systems
func (em *ErrorManager) CreateError(severity ErrorSeverity, category ErrorCategory, message string, context ErrorContext) *FlightError {
	error := &FlightError{
		ID:        fmt.Sprintf("error-%d", time.Now().UnixNano()),
		Severity:  severity,
		Category:  category,
		Message:   message,
		Context:   context,
		Timestamp: uint64(time.Now().Unix()),
	}

	em.errorHistory = append(em.errorHistory, *error)
	return error
}

// CreateSimpleError creates error with simple context
// Convenience function for basic error creation
func (em *ErrorManager) CreateSimpleError(severity ErrorSeverity, category ErrorCategory, message, source, operation string) *FlightError {
	context := ErrorContext{
		Source:    source,
		Operation: operation,
		Metadata:  []MetadataPair{},
	}

	return em.CreateError(severity, category, message, context)
}

// CreateServiceError creates specialized function for external service errors
func (em *ErrorManager) CreateServiceError(serviceCode ServiceErrorCode, message, serviceID string, sessionID *string) *FlightError {
	context := ErrorContext{
		Source:    "service-integration",
		Operation: serviceCode.String(),
		ServiceID: &serviceID,
		SessionID: sessionID,
		Metadata: []MetadataPair{
			{Key: "service_error_code", Value: serviceCode.String()},
		},
	}

	return em.CreateError(ErrorSeverityError, ErrorCategoryServiceIntegration, message, context)
}

// EnrichError adds context to existing error
// Enriches error information as it propagates
func (em *ErrorManager) EnrichError(err *FlightError, additionalContext []MetadataPair) *FlightError {
	// Create a copy to avoid modifying the original
	enriched := *err
	enriched.Context.Metadata = append(enriched.Context.Metadata, additionalContext...)
	return &enriched
}

// IsRecoverable checks if error is recoverable
// Determines if operation can be retried
func (em *ErrorManager) IsRecoverable(err *FlightError) bool {
	switch err.Severity {
	case ErrorSeverityFatal, ErrorSeverityCritical:
		return false
	case ErrorSeverityError:
		// Check category for recoverability
		switch err.Category {
		case ErrorCategoryNetwork, ErrorCategoryServiceIntegration:
			return true
		default:
			return false
		}
	default:
		return true
	}
}

// GetRecoverySuggestions provides actionable recovery suggestions
func (em *ErrorManager) GetRecoverySuggestions(err *FlightError) []ErrorRecoverySuggestion {
	var suggestions []ErrorRecoverySuggestion

	switch err.Category {
	case ErrorCategoryNetwork:
		suggestions = append(suggestions, ErrorRecoverySuggestion{
			ActionType:  RecoveryActionRetry,
			Description: "Retry the network operation after a short delay",
			CanAutomate: true,
			Priority:    10,
		})
		suggestions = append(suggestions, ErrorRecoverySuggestion{
			ActionType:  RecoveryActionCheckStatus,
			Description: "Check network connectivity and service status",
			CanAutomate: false,
			Priority:    8,
		})

	case ErrorCategoryMemory:
		suggestions = append(suggestions, ErrorRecoverySuggestion{
			ActionType:  RecoveryActionReduceResources,
			Description: "Free up memory by closing unused components",
			CanAutomate: true,
			Priority:    9,
		})

	case ErrorCategoryServiceIntegration:
		suggestions = append(suggestions, ErrorRecoverySuggestion{
			ActionType:  RecoveryActionWaitRetry,
			Description: "Wait for service to recover and retry",
			CanAutomate: true,
			Priority:    7,
		})
	}

	return suggestions
}

// CollectErrors aggregates errors from batch operations
func (em *ErrorManager) CollectErrors(errors []FlightError) *ErrorCollection {
	collection := &ErrorCollection{
		Errors:            errors,
		SeverityCounts:    make(map[ErrorSeverity]uint32),
		CategoryCounts:    make(map[ErrorCategory]uint32),
		HasBlockingErrors: false,
		TotalCount:        uint32(len(errors)),
	}

	for _, err := range errors {
		collection.SeverityCounts[err.Severity]++
		collection.CategoryCounts[err.Category]++

		if err.Severity == ErrorSeverityCritical || err.Severity == ErrorSeverityFatal {
			collection.HasBlockingErrors = true
		}
	}

	return collection
}

// ErrorToJSON serializes error for API integration and storage
func (em *ErrorManager) ErrorToJSON(err *FlightError) (string, error) {
	data, jsonErr := json.Marshal(err)
	if jsonErr != nil {
		return "", jsonErr
	}
	return string(data), nil
}

// ErrorFromJSON deserializes error from API responses or storage
func (em *ErrorManager) ErrorFromJSON(jsonStr string) FlightResult[FlightError] {
	var err FlightError
	if jsonErr := json.Unmarshal([]byte(jsonStr), &err); jsonErr != nil {
		return NewErrorResult[FlightError](em.CreateSimpleError(
			ErrorSeverityError,
			ErrorCategoryValidation,
			"Failed to parse error JSON",
			"ErrorManager",
			"ErrorFromJSON",
		))
	}
	return NewSuccessResult(err)
}

// ValidateError ensures error meets structural requirements
func (em *ErrorManager) ValidateError(err *FlightError) FlightResult[bool] {
	if err.ID == "" {
		return NewErrorResult[bool](em.CreateSimpleError(
			ErrorSeverityError,
			ErrorCategoryValidation,
			"Error ID is required",
			"ErrorManager",
			"ValidateError",
		))
	}

	if err.Message == "" {
		return NewErrorResult[bool](em.CreateSimpleError(
			ErrorSeverityError,
			ErrorCategoryValidation,
			"Error message is required",
			"ErrorManager",
			"ValidateError",
		))
	}

	if err.Context.Source == "" {
		return NewErrorResult[bool](em.CreateSimpleError(
			ErrorSeverityError,
			ErrorCategoryValidation,
			"Error context source is required",
			"ErrorManager",
			"ValidateError",
		))
	}

	return NewSuccessResult(true)
}

// MarshalJSON implements custom JSON marshaling for ErrorSeverity
func (es ErrorSeverity) MarshalJSON() ([]byte, error) {
	return json.Marshal(es.String())
}

// MarshalJSON implements custom JSON marshaling for ErrorCategory
func (ec ErrorCategory) MarshalJSON() ([]byte, error) {
	return json.Marshal(ec.String())
}

// MarshalJSON implements custom JSON marshaling for ServiceErrorCode
func (sec ServiceErrorCode) MarshalJSON() ([]byte, error) {
	return json.Marshal(sec.String())
}
