// Flight Session Types - Go Implementation
// Universal session management for Flight-Core multi-platform development

package session

import (
	"encoding/json"
	"fmt"
	"time"
)

// Import types from other Flight packages (simplified for standalone)
type MemorySize struct {
	Bytes         uint64 `json:"bytes"`
	HumanReadable string `json:"human_readable"`
}

type MemoryUsageSnapshot struct {
	Timestamp          uint64     `json:"timestamp"`
	SessionID          string     `json:"session_id"`
	Platform           string     `json:"platform"`
	Total              MemorySize `json:"total"`
	Used               MemorySize `json:"used"`
	Available          MemorySize `json:"available"`
	FragmentationRatio float32    `json:"fragmentation_ratio"`
}

type FlightError struct {
	ID        string       `json:"id"`
	Severity  string       `json:"severity"`
	Category  string       `json:"category"`
	Message   string       `json:"message"`
	Details   *string      `json:"details,omitempty"`
	Context   ErrorContext `json:"context"`
	Timestamp uint64       `json:"timestamp"`
	Cause     *string      `json:"cause,omitempty"`
}

type ErrorContext struct {
	Source    string          `json:"source"`
	Operation string          `json:"operation"`
	SessionID *string         `json:"session_id,omitempty"`
	UserID    *string         `json:"user_id,omitempty"`
	Platform  *string         `json:"platform,omitempty"`
	ServiceID *string         `json:"service_id,omitempty"`
	Metadata  []MetadataEntry `json:"metadata"`
}

type MetadataEntry struct {
	Key   string `json:"key"`
	Value string `json:"value"`
}

func (e FlightError) Error() string {
	if e.Details != nil {
		return fmt.Sprintf("%s: %s (%s)", e.Category, e.Message, *e.Details)
	}
	return fmt.Sprintf("%s: %s", e.Category, e.Message)
}

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

// Session lifecycle states
type SessionState string

const (
	SessionStateInitializing SessionState = "initializing"
	SessionStateActive       SessionState = "active"
	SessionStateSuspended    SessionState = "suspended"
	SessionStateTerminating  SessionState = "terminating"
	SessionStateTerminated   SessionState = "terminated"
	SessionStateError        SessionState = "error"
)

// Session types for different use cases
type SessionType string

const (
	SessionTypeComponent   SessionType = "component"
	SessionTypeUser        SessionType = "user"
	SessionTypeDevelopment SessionType = "development"
	SessionTypeSystem      SessionType = "system"
	SessionTypeTesting     SessionType = "testing"
	SessionTypeCustom      SessionType = "custom"
)

// Session health status
type SessionHealth string

const (
	SessionHealthHealthy  SessionHealth = "healthy"
	SessionHealthWarning  SessionHealth = "warning"
	SessionHealthDegraded SessionHealth = "degraded"
	SessionHealthCritical SessionHealth = "critical"
	SessionHealthUnknown  SessionHealth = "unknown"
)

// Session event types
type SessionEventType string

const (
	SessionEventTypeCreated               SessionEventType = "created"
	SessionEventTypeStarted               SessionEventType = "started"
	SessionEventTypeSuspended             SessionEventType = "suspended"
	SessionEventTypeResumed               SessionEventType = "resumed"
	SessionEventTypeTerminated            SessionEventType = "terminated"
	SessionEventTypeErrorOccurred         SessionEventType = "error-occurred"
	SessionEventTypeResourceLimitExceeded SessionEventType = "resource-limit-exceeded"
	SessionEventTypeHealthChanged         SessionEventType = "health-changed"
	SessionEventTypeCustom                SessionEventType = "custom"
)

// Core session information
type SessionInfo struct {
	ID              string          `json:"id"`
	SessionType     SessionType     `json:"session_type"`
	State           SessionState    `json:"state"`
	Platform        string          `json:"platform"`
	UserID          *string         `json:"user_id,omitempty"`
	ParentSessionID *string         `json:"parent_session_id,omitempty"`
	CreatedAt       uint64          `json:"created_at"`
	LastActivity    uint64          `json:"last_activity"`
	ExpiresAt       *uint64         `json:"expires_at,omitempty"`
	Metadata        []MetadataEntry `json:"metadata"`
}

// Session resource usage tracking
type SessionResources struct {
	Memory          MemoryUsageSnapshot `json:"memory"`
	CPUUsage        float32             `json:"cpu_usage"`
	NetworkUsage    uint64              `json:"network_usage"`
	StorageUsage    MemorySize          `json:"storage_usage"`
	ConnectionCount uint32              `json:"connection_count"`
	CustomMetrics   []CustomMetric      `json:"custom_metrics"`
}

type CustomMetric struct {
	Name  string  `json:"name"`
	Value float32 `json:"value"`
}

// Generic resource limits configuration
type ResourceLimits struct {
	MaxMemory      *MemorySize    `json:"max_memory,omitempty"`
	MaxCPUPercent  *float32       `json:"max_cpu_percent,omitempty"`
	MaxNetworkBPS  *uint64        `json:"max_network_bps,omitempty"`
	MaxStorage     *MemorySize    `json:"max_storage,omitempty"`
	MaxConnections *uint32        `json:"max_connections,omitempty"`
	TimeoutSeconds *uint64        `json:"timeout_seconds,omitempty"`
	CustomLimits   []CustomMetric `json:"custom_limits"`
}

// Session configuration
type SessionConfig struct {
	ResourceLimits   *ResourceLimits `json:"resource_limits,omitempty"`
	Environment      []MetadataEntry `json:"environment"`
	WorkingDirectory *string         `json:"working_directory,omitempty"`
	CustomConfig     []MetadataEntry `json:"custom_config"`
}

// Session event record
type SessionEvent struct {
	ID        string           `json:"id"`
	SessionID string           `json:"session_id"`
	EventType SessionEventType `json:"event_type"`
	Timestamp uint64           `json:"timestamp"`
	Message   string           `json:"message"`
	Data      []MetadataEntry  `json:"data"`
}

// Session statistics summary
type SessionStats struct {
	ActiveSessions          uint32             `json:"active_sessions"`
	SessionsCreatedToday    uint32             `json:"sessions_created_today"`
	SessionsTerminatedToday uint32             `json:"sessions_terminated_today"`
	AverageDuration         float32            `json:"average_duration"`
	SessionsByType          []SessionTypeCount `json:"sessions_by_type"`
	SessionsByPlatform      []PlatformCount    `json:"sessions_by_platform"`
	SystemHealth            SessionHealth      `json:"system_health"`
}

type SessionTypeCount struct {
	Type  SessionType `json:"type"`
	Count uint32      `json:"count"`
}

type PlatformCount struct {
	Platform string `json:"platform"`
	Count    uint32 `json:"count"`
}

// Resource usage aggregation
type ResourceUsageAggregate struct {
	TotalMemoryUsage  MemorySize              `json:"total_memory_usage"`
	AverageCPUUsage   float32                 `json:"average_cpu_usage"`
	TotalNetworkUsage uint64                  `json:"total_network_usage"`
	TotalStorageUsage MemorySize              `json:"total_storage_usage"`
	TotalConnections  uint32                  `json:"total_connections"`
	UsageByPlatform   []PlatformResourceUsage `json:"usage_by_platform"`
}

type PlatformResourceUsage struct {
	Platform  string           `json:"platform"`
	Resources SessionResources `json:"resources"`
}

// Session filter criteria
type SessionFilterCriteria struct {
	UserID        *string        `json:"user_id,omitempty"`
	SessionType   *SessionType   `json:"session_type,omitempty"`
	Platform      *string        `json:"platform,omitempty"`
	State         *SessionState  `json:"state,omitempty"`
	Health        *SessionHealth `json:"health,omitempty"`
	CreatedAfter  *uint64        `json:"created_after,omitempty"`
	CreatedBefore *uint64        `json:"created_before,omitempty"`
	ExpiresAfter  *uint64        `json:"expires_after,omitempty"`
	ExpiresBefore *uint64        `json:"expires_before,omitempty"`
}

// Session operations interface
type SessionOperations interface {
	CreateSession(sessionType SessionType, platform string, userID *string, config *SessionConfig) FlightResult[SessionInfo]
	GetSession(sessionID string) FlightResult[SessionInfo]
	UpdateSessionState(sessionID string, newState SessionState) FlightResult[bool]
	TerminateSession(sessionID string) FlightResult[bool]
	GetSessionResources(sessionID string) FlightResult[SessionResources]
	ListSessions(userID *string, sessionType *SessionType, platform *string) FlightResult[[]SessionInfo]
	ExtendSession(sessionID string, additionalSeconds uint64) FlightResult[bool]
	UpdateSessionMetadata(sessionID string, metadata []MetadataEntry) FlightResult[bool]
	SetResourceLimits(sessionID string, limits ResourceLimits) FlightResult[bool]
	GetSessionHealth(sessionID string) FlightResult[SessionHealth]
	RecordSessionEvent(sessionID string, eventType SessionEventType, message string, data []MetadataEntry) FlightResult[bool]
	GetSessionEvents(sessionID string, limit *uint32) FlightResult[[]SessionEvent]
}

// Session analytics interface
type SessionAnalytics interface {
	GetSessionStats() FlightResult[SessionStats]
	GetResourceUsageAggregate() FlightResult[ResourceUsageAggregate]
	FindSessions(criteria []MetadataEntry) FlightResult[[]SessionInfo]
	GetSessionsByHealth(health SessionHealth) FlightResult[[]SessionInfo]
	CalculateSessionEfficiency(sessionID string) FlightResult[float32]
	GenerateSessionReport(timeWindowHours uint32, includeEvents bool) FlightResult[string]
}

// Helper functions for type creation
func NewSessionInfo(id string, sessionType SessionType, platform string) SessionInfo {
	now := uint64(time.Now().Unix())
	return SessionInfo{
		ID:           id,
		SessionType:  sessionType,
		State:        SessionStateInitializing,
		Platform:     platform,
		CreatedAt:    now,
		LastActivity: now,
		Metadata:     []MetadataEntry{},
	}
}

func NewSessionConfig() SessionConfig {
	return SessionConfig{
		Environment:  []MetadataEntry{},
		CustomConfig: []MetadataEntry{},
	}
}

func NewResourceLimits() ResourceLimits {
	return ResourceLimits{
		CustomLimits: []CustomMetric{},
	}
}

func NewSessionEvent(sessionID string, eventType SessionEventType, message string) SessionEvent {
	return SessionEvent{
		ID:        fmt.Sprintf("event-%d", time.Now().UnixNano()),
		SessionID: sessionID,
		EventType: eventType,
		Timestamp: uint64(time.Now().Unix()),
		Message:   message,
		Data:      []MetadataEntry{},
	}
}

func NewFlightResult[T any](value T) FlightResult[T] {
	return FlightResult[T]{Value: &value}
}

func NewFlightResultError[T any](err FlightError) FlightResult[T] {
	return FlightResult[T]{Error: &err}
}

func NewFlightError(category, message string) FlightError {
	return FlightError{
		ID:        fmt.Sprintf("error-%d", time.Now().UnixNano()),
		Severity:  "error",
		Category:  category,
		Message:   message,
		Context:   ErrorContext{Metadata: []MetadataEntry{}},
		Timestamp: uint64(time.Now().Unix()),
	}
}

func NewMemorySize(bytes uint64) MemorySize {
	return MemorySize{
		Bytes:         bytes,
		HumanReadable: FormatBytes(bytes),
	}
}

// Utility function to format bytes as human-readable string
func FormatBytes(bytes uint64) string {
	const unit = 1024
	if bytes < unit {
		return fmt.Sprintf("%d B", bytes)
	}
	div, exp := uint64(unit), 0
	for n := bytes / unit; n >= unit; n /= unit {
		div *= unit
		exp++
	}
	return fmt.Sprintf("%.1f %cB", float64(bytes)/float64(div), "KMGTPE"[exp])
}

// Session state transition validation
type SessionStateTransition struct {
	From  SessionState `json:"from"`
	To    SessionState `json:"to"`
	Valid bool         `json:"valid"`
}

// ValidateStateTransition checks if a state transition is valid
func ValidateStateTransition(from, to SessionState) bool {
	validTransitions := map[SessionState][]SessionState{
		SessionStateInitializing: {SessionStateActive, SessionStateError, SessionStateTerminated},
		SessionStateActive:       {SessionStateSuspended, SessionStateTerminating, SessionStateError},
		SessionStateSuspended:    {SessionStateActive, SessionStateTerminating, SessionStateError},
		SessionStateTerminating:  {SessionStateTerminated, SessionStateError},
		SessionStateTerminated:   {}, // Terminal state
		SessionStateError:        {SessionStateActive, SessionStateTerminating, SessionStateTerminated},
	}

	if from == to {
		return true
	}

	allowed, exists := validTransitions[from]
	if !exists {
		return false
	}

	for _, state := range allowed {
		if state == to {
			return true
		}
	}
	return false
}

// GetValidNextStates returns valid next states for current state
func GetValidNextStates(current SessionState) []SessionState {
	validTransitions := map[SessionState][]SessionState{
		SessionStateInitializing: {SessionStateActive, SessionStateError, SessionStateTerminated},
		SessionStateActive:       {SessionStateSuspended, SessionStateTerminating, SessionStateError},
		SessionStateSuspended:    {SessionStateActive, SessionStateTerminating, SessionStateError},
		SessionStateTerminating:  {SessionStateTerminated, SessionStateError},
		SessionStateTerminated:   {},
		SessionStateError:        {SessionStateActive, SessionStateTerminating, SessionStateTerminated},
	}

	if states, exists := validTransitions[current]; exists {
		return states
	}
	return []SessionState{}
}

// IsSessionExpired checks if session is expired
func IsSessionExpired(session SessionInfo) bool {
	if session.ExpiresAt == nil {
		return false
	}
	return uint64(time.Now().Unix()) > *session.ExpiresAt
}

// GetSessionDuration calculates session duration in seconds
func GetSessionDuration(session SessionInfo) uint64 {
	var endTime uint64
	if session.State == SessionStateTerminated {
		endTime = session.LastActivity
	} else {
		endTime = uint64(time.Now().Unix())
	}
	return endTime - session.CreatedAt
}

// MetadataToMap converts metadata array to map
func MetadataToMap(metadata []MetadataEntry) map[string]string {
	result := make(map[string]string)
	for _, entry := range metadata {
		result[entry.Key] = entry.Value
	}
	return result
}

// MapToMetadata converts map to metadata array
func MapToMetadata(m map[string]string) []MetadataEntry {
	var result []MetadataEntry
	for key, value := range m {
		result = append(result, MetadataEntry{Key: key, Value: value})
	}
	return result
}

// FilterSessions filters sessions based on criteria
func FilterSessions(sessions []SessionInfo, criteria SessionFilterCriteria) []SessionInfo {
	var filtered []SessionInfo

	for _, session := range sessions {
		if criteria.UserID != nil && (session.UserID == nil || *session.UserID != *criteria.UserID) {
			continue
		}
		if criteria.SessionType != nil && session.SessionType != *criteria.SessionType {
			continue
		}
		if criteria.Platform != nil && session.Platform != *criteria.Platform {
			continue
		}
		if criteria.State != nil && session.State != *criteria.State {
			continue
		}
		if criteria.CreatedAfter != nil && session.CreatedAt <= *criteria.CreatedAfter {
			continue
		}
		if criteria.CreatedBefore != nil && session.CreatedAt >= *criteria.CreatedBefore {
			continue
		}
		if criteria.ExpiresAfter != nil && (session.ExpiresAt == nil || *session.ExpiresAt <= *criteria.ExpiresAfter) {
			continue
		}
		if criteria.ExpiresBefore != nil && (session.ExpiresAt == nil || *session.ExpiresAt >= *criteria.ExpiresBefore) {
			continue
		}

		filtered = append(filtered, session)
	}

	return filtered
}

// CalculateSessionHealth calculates session health based on resources and limits
func CalculateSessionHealth(resources SessionResources, limits *ResourceLimits) SessionHealth {
	if limits == nil {
		return SessionHealthUnknown
	}

	healthScore := 100.0
	warningCount := 0
	criticalCount := 0

	// Check memory usage
	if limits.MaxMemory != nil {
		memoryUsagePercent := float64(resources.Memory.Used.Bytes) / float64(limits.MaxMemory.Bytes) * 100.0
		if memoryUsagePercent > 90.0 {
			criticalCount++
			healthScore -= 30.0
		} else if memoryUsagePercent > 75.0 {
			warningCount++
			healthScore -= 15.0
		}
	}

	// Check CPU usage
	if limits.MaxCPUPercent != nil {
		if resources.CPUUsage > 90.0 {
			criticalCount++
			healthScore -= 25.0
		} else if resources.CPUUsage > 75.0 {
			warningCount++
			healthScore -= 10.0
		}
	}

	// Check network usage
	if limits.MaxNetworkBPS != nil {
		networkUsagePercent := float64(resources.NetworkUsage) / float64(*limits.MaxNetworkBPS) * 100.0
		if networkUsagePercent > 90.0 {
			criticalCount++
			healthScore -= 20.0
		} else if networkUsagePercent > 75.0 {
			warningCount++
			healthScore -= 10.0
		}
	}

	// Check memory fragmentation
	if resources.Memory.FragmentationRatio > 0.8 {
		warningCount++
		healthScore -= 15.0
	}

	// Determine final health status
	if criticalCount > 0 || healthScore <= 30.0 {
		return SessionHealthCritical
	} else if warningCount > 1 || healthScore <= 60.0 {
		return SessionHealthDegraded
	} else if warningCount > 0 || healthScore <= 85.0 {
		return SessionHealthWarning
	} else {
		return SessionHealthHealthy
	}
}

// JSON marshaling for result types
func (r FlightResult[T]) MarshalJSON() ([]byte, error) {
	if r.Error != nil {
		return json.Marshal(map[string]interface{}{
			"tag": "err",
			"val": r.Error,
		})
	}
	return json.Marshal(map[string]interface{}{
		"tag": "ok",
		"val": r.Value,
	})
}
