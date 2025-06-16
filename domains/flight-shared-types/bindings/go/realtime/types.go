// Package realtime provides Go bindings for Flight-Core and V6R real-time communication types
//
// This package implements the WIT interface for real-time communication,
// supporting WebSocket messaging, event streaming, and live updates across
// Flight-Core and V6R ecosystems.
package realtime

import (
	"time"

	componentTypes "github.com/flight/flight-shared-types/bindings/go/component"
	errorTypes "github.com/flight/flight-shared-types/bindings/go/error"
	memoryTypes "github.com/flight/flight-shared-types/bindings/go/memory"
	sessionTypes "github.com/flight/flight-shared-types/bindings/go/session"
)

// Connection Management Types

// ConnectionID represents a unique real-time connection identifier
type ConnectionID string

// ChannelID represents a unique channel identifier
type ChannelID string

// MessageID represents a unique message identifier
type MessageID string

// ConnectionState represents the state of a WebSocket connection
type ConnectionState string

const (
	ConnectionStateConnecting    ConnectionState = "connecting"
	ConnectionStateConnected     ConnectionState = "connected"
	ConnectionStateDisconnected  ConnectionState = "disconnected"
	ConnectionStateClosing       ConnectionState = "closing"
	ConnectionStateClosed        ConnectionState = "closed"
	ConnectionStateError         ConnectionState = "error"
	ConnectionStateAuthenticated ConnectionState = "authenticated"
)

// ClientType represents the type of client connecting
type ClientType string

const (
	ClientTypeWebBrowser        ClientType = "web-browser"
	ClientTypeFlightCoreNative  ClientType = "flight-native"
	ClientTypeV6RMobile         ClientType = "v6r-mobile"
	ClientTypeV6RCli            ClientType = "v6r-cli"
	ClientTypeV6RBridge         ClientType = "v6r-bridge"
	ClientTypeDevelopmentTool   ClientType = "development-tool"
	ClientTypeMonitoringService ClientType = "monitoring-service"
	ClientTypeApiClient         ClientType = "api-client"
	ClientTypeUnknown           ClientType = "unknown"
)

// ClientInfo contains information about the connecting client
type ClientInfo struct {
	ClientType   ClientType `json:"clientType"`
	Version      string     `json:"version"`
	Platform     string     `json:"platform"`
	Capabilities []string   `json:"capabilities"`
	UserAgent    *string    `json:"userAgent,omitempty"`
	IPAddress    *string    `json:"ipAddress,omitempty"`
}

// ConnectionInfo contains comprehensive connection information
type ConnectionInfo struct {
	ID           ConnectionID            `json:"id"`
	State        ConnectionState         `json:"state"`
	ConnectedAt  uint64                  `json:"connectedAt"`
	LastActivity uint64                  `json:"lastActivity"`
	UserID       *string                 `json:"userId,omitempty"`
	SessionID    *sessionTypes.SessionID `json:"sessionId,omitempty"`
	Platform     string                  `json:"platform"`
	ClientInfo   ClientInfo              `json:"clientInfo"`
	Metadata     map[string]string       `json:"metadata"`
}

// Messaging Types

// MessagePriority represents message priority levels
type MessagePriority string

const (
	MessagePriorityLow      MessagePriority = "low"
	MessagePriorityNormal   MessagePriority = "normal"
	MessagePriorityHigh     MessagePriority = "high"
	MessagePriorityCritical MessagePriority = "critical"
	MessagePriorityRealtime MessagePriority = "realtime"
)

// RetryConfig defines message retry configuration
type RetryConfig struct {
	MaxRetries         uint32  `json:"maxRetries"`
	RetryIntervalMs    uint64  `json:"retryIntervalMs"`
	BackoffMultiplier  float32 `json:"backoffMultiplier"`
	MaxRetryIntervalMs uint64  `json:"maxRetryIntervalMs"`
}

// MessageRouting contains message routing information
type MessageRouting struct {
	Source      string       `json:"source"`
	Route       []string     `json:"route"`
	ExpiresAt   *uint64      `json:"expiresAt,omitempty"`
	RequiresAck bool         `json:"requiresAck"`
	RetryConfig *RetryConfig `json:"retryConfig,omitempty"`
}

// RealtimeMessage represents a real-time message
type RealtimeMessage struct {
	ID          MessageID         `json:"id"`
	MessageType string            `json:"messageType"`
	ChannelID   ChannelID         `json:"channelId"`
	Payload     string            `json:"payload"`
	Timestamp   uint64            `json:"timestamp"`
	Sender      *string           `json:"sender,omitempty"`
	Target      *string           `json:"target,omitempty"`
	Priority    MessagePriority   `json:"priority"`
	Routing     MessageRouting    `json:"routing"`
	Metadata    map[string]string `json:"metadata"`
}

// Channel and Subscription Types

// ChannelType represents the type of real-time channel
type ChannelType string

const (
	ChannelTypePublicBroadcast  ChannelType = "public-broadcast"
	ChannelTypePrivateUser      ChannelType = "private-user"
	ChannelTypeSessionPrivate   ChannelType = "session-private"
	ChannelTypeSystemMonitoring ChannelType = "system-monitoring"
	ChannelTypeMemoryUpdates    ChannelType = "memory-updates"
	ChannelTypeComponentEvents  ChannelType = "component-events"
	ChannelTypeV6RVmManagement  ChannelType = "v6r-vm-management"
	ChannelTypeFlightHalEvents  ChannelType = "flight-hal-events"
	ChannelTypeDevelopment      ChannelType = "development"
)

// ChannelInfo defines a real-time channel
type ChannelInfo struct {
	ID                  ChannelID         `json:"id"`
	Name                string            `json:"name"`
	ChannelType         ChannelType       `json:"channelType"`
	RequiredPermissions []string          `json:"requiredPermissions"`
	MaxConnections      *uint32           `json:"maxConnections,omitempty"`
	CreatedAt           uint64            `json:"createdAt"`
	Metadata            map[string]string `json:"metadata"`
}

// FilterOperation represents subscription filter operations
type FilterOperation string

const (
	FilterOperationEquals      FilterOperation = "equals"
	FilterOperationNotEquals   FilterOperation = "not-equals"
	FilterOperationContains    FilterOperation = "contains"
	FilterOperationStartsWith  FilterOperation = "starts-with"
	FilterOperationEndsWith    FilterOperation = "ends-with"
	FilterOperationRegex       FilterOperation = "regex"
	FilterOperationGreaterThan FilterOperation = "greater-than"
	FilterOperationLessThan    FilterOperation = "less-than"
)

// SubscriptionFilter defines selective message delivery filters
type SubscriptionFilter struct {
	Field     string          `json:"field"`
	Operation FilterOperation `json:"operation"`
	Value     string          `json:"value"`
}

// Subscription represents a channel subscription
type Subscription struct {
	ID           string               `json:"id"`
	ConnectionID ConnectionID         `json:"connectionId"`
	ChannelID    ChannelID            `json:"channelId"`
	Filters      []SubscriptionFilter `json:"filters"`
	SubscribedAt uint64               `json:"subscribedAt"`
	LastActivity uint64               `json:"lastActivity"`
	Metadata     map[string]string    `json:"metadata"`
}

// Event System Types

// SessionEventType represents session lifecycle event types
type SessionEventType string

const (
	SessionEventTypeSessionCreated    SessionEventType = "session-created"
	SessionEventTypeSessionActivated  SessionEventType = "session-activated"
	SessionEventTypeSessionSuspended  SessionEventType = "session-suspended"
	SessionEventTypeSessionTerminated SessionEventType = "session-terminated"
	SessionEventTypeSessionExpired    SessionEventType = "session-expired"
	SessionEventTypeSessionError      SessionEventType = "session-error"
)

// SessionEvent represents session lifecycle events
type SessionEvent struct {
	EventType SessionEventType       `json:"eventType"`
	SessionID sessionTypes.SessionID `json:"sessionId"`
	UserID    *string                `json:"userId,omitempty"`
	Platform  string                 `json:"platform"`
	Timestamp uint64                 `json:"timestamp"`
	Metadata  map[string]string      `json:"metadata"`
}

// AuthEventType represents authentication event types
type AuthEventType string

const (
	AuthEventTypeUserAuthenticated AuthEventType = "user-authenticated"
	AuthEventTypeUserLoggedOut     AuthEventType = "user-logged-out"
	AuthEventTypeTokenRefreshed    AuthEventType = "token-refreshed"
	AuthEventTypeTokenExpired      AuthEventType = "token-expired"
	AuthEventTypeAuthFailed        AuthEventType = "auth-failed"
	AuthEventTypePermissionGranted AuthEventType = "permission-granted"
	AuthEventTypePermissionRevoked AuthEventType = "permission-revoked"
)

// AuthEvent represents authentication events
type AuthEvent struct {
	EventType AuthEventType     `json:"eventType"`
	UserID    string            `json:"userId"`
	Platform  string            `json:"platform"`
	Timestamp uint64            `json:"timestamp"`
	Metadata  map[string]string `json:"metadata"`
}

// SystemEventType represents system health event types
type SystemEventType string

const (
	SystemEventTypeSystemStartup      SystemEventType = "system-startup"
	SystemEventTypeSystemShutdown     SystemEventType = "system-shutdown"
	SystemEventTypeComponentLoaded    SystemEventType = "component-loaded"
	SystemEventTypeComponentUnloaded  SystemEventType = "component-unloaded"
	SystemEventTypeErrorOccurred      SystemEventType = "error-occurred"
	SystemEventTypePerformanceWarning SystemEventType = "performance-warning"
	SystemEventTypeMemoryPressure     SystemEventType = "memory-pressure"
	SystemEventTypeResourceExhausted  SystemEventType = "resource-exhausted"
)

// EventSeverity represents event severity levels
type EventSeverity string

const (
	EventSeverityInfo     EventSeverity = "info"
	EventSeverityWarning  EventSeverity = "warning"
	EventSeverityError    EventSeverity = "error"
	EventSeverityCritical EventSeverity = "critical"
	EventSeverityFatal    EventSeverity = "fatal"
)

// SystemEvent represents system health events
type SystemEvent struct {
	EventType SystemEventType   `json:"eventType"`
	Component string            `json:"component"`
	Platform  string            `json:"platform"`
	Severity  EventSeverity     `json:"severity"`
	Message   string            `json:"message"`
	Timestamp uint64            `json:"timestamp"`
	Metadata  map[string]string `json:"metadata"`
}

// V6REventType represents V6R specific event types
type V6REventType string

const (
	V6REventTypeVmCreated         V6REventType = "vm-created"
	V6REventTypeVmStarted         V6REventType = "vm-started"
	V6REventTypeVmStopped         V6REventType = "vm-stopped"
	V6REventTypeVmDeleted         V6REventType = "vm-deleted"
	V6REventTypeVmScaling         V6REventType = "vm-scaling"
	V6REventTypeQuotaExceeded     V6REventType = "quota-exceeded"
	V6REventTypeBillingEvent      V6REventType = "billing-event"
	V6REventTypeTeamMemberAdded   V6REventType = "team-member-added"
	V6REventTypeTeamMemberRemoved V6REventType = "team-member-removed"
)

// V6REvent represents V6R specific events
type V6REvent struct {
	EventType    V6REventType      `json:"eventType"`
	Resource     string            `json:"resource"`
	Organization *string           `json:"organization,omitempty"`
	Timestamp    uint64            `json:"timestamp"`
	Metadata     map[string]string `json:"metadata"`
}

// FlightEventType represents Flight-Core specific event types
type FlightEventType string

const (
	FlightEventTypePlatformDetected     FlightEventType = "platform-detected"
	FlightEventTypeHalInitialized       FlightEventType = "hal-initialized"
	FlightEventTypeComponentLoaded      FlightEventType = "component-loaded"
	FlightEventTypeMemoryPoolCreated    FlightEventType = "memory-pool-created"
	FlightEventTypeRuntimeStarted       FlightEventType = "runtime-started"
	FlightEventTypePerformanceMilestone FlightEventType = "performance-milestone"
)

// FlightEvent represents Flight-Core specific events
type FlightEvent struct {
	EventType    FlightEventType   `json:"eventType"`
	Platform     string            `json:"platform"`
	HalSubsystem *string           `json:"halSubsystem,omitempty"`
	Timestamp    uint64            `json:"timestamp"`
	Metadata     map[string]string `json:"metadata"`
}

// CustomEvent represents custom application events
type CustomEvent struct {
	Name        string            `json:"name"`
	Data        string            `json:"data"`
	Application string            `json:"application"`
	Timestamp   uint64            `json:"timestamp"`
	Metadata    map[string]string `json:"metadata"`
}

// RealtimeEvent represents a discriminated union of all real-time events
type RealtimeEvent struct {
	Type string      `json:"type"`
	Data interface{} `json:"data"`
}

// NewMemoryUpdateEvent creates a memory update event
func NewMemoryUpdateEvent(snapshot memoryTypes.MemoryUsageSnapshot) *RealtimeEvent {
	return &RealtimeEvent{
		Type: "memory-update",
		Data: snapshot,
	}
}

// NewComponentUpdateEvent creates a component update event
func NewComponentUpdateEvent(component componentTypes.ComponentInfo) *RealtimeEvent {
	return &RealtimeEvent{
		Type: "component-update",
		Data: component,
	}
}

// NewSessionEvent creates a session event
func NewSessionEvent(event SessionEvent) *RealtimeEvent {
	return &RealtimeEvent{
		Type: "session-event",
		Data: event,
	}
}

// NewAuthEvent creates an authentication event
func NewAuthEvent(event AuthEvent) *RealtimeEvent {
	return &RealtimeEvent{
		Type: "auth-event",
		Data: event,
	}
}

// NewSystemEvent creates a system event
func NewSystemEvent(event SystemEvent) *RealtimeEvent {
	return &RealtimeEvent{
		Type: "system-event",
		Data: event,
	}
}

// NewV6REvent creates a V6R specific event
func NewV6REvent(event V6REvent) *RealtimeEvent {
	return &RealtimeEvent{
		Type: "v6r-event",
		Data: event,
	}
}

// NewFlightEvent creates a Flight-Core specific event
func NewFlightEvent(event FlightEvent) *RealtimeEvent {
	return &RealtimeEvent{
		Type: "flight-event",
		Data: event,
	}
}

// NewCustomEvent creates a custom event
func NewCustomEvent(event CustomEvent) *RealtimeEvent {
	return &RealtimeEvent{
		Type: "custom-event",
		Data: event,
	}
}

// Analytics and Monitoring Types

// RealtimeMetrics contains real-time system metrics
type RealtimeMetrics struct {
	ActiveConnections   uint32  `json:"activeConnections"`
	MessagesSent        uint64  `json:"messagesSent"`
	MessagesReceived    uint64  `json:"messagesReceived"`
	MessagesPerSecond   float32 `json:"messagesPerSecond"`
	AvgLatencyMs        float32 `json:"avgLatencyMs"`
	ErrorRate           float32 `json:"errorRate"`
	MemoryUsage         uint64  `json:"memoryUsage"`
	ActiveSubscriptions uint32  `json:"activeSubscriptions"`
	CollectedAt         uint64  `json:"collectedAt"`
}

// ConnectionAnalytics contains connection analytics data
type ConnectionAnalytics struct {
	TotalConnections      uint32                `json:"totalConnections"`
	PeakConnections       uint32                `json:"peakConnections"`
	AvgConnectionDuration float32               `json:"avgConnectionDuration"`
	ConnectionsByType     map[ClientType]uint32 `json:"connectionsByType"`
	ConnectionsByPlatform map[string]uint32     `json:"connectionsByPlatform"`
	ErrorRate             float32               `json:"errorRate"`
}

// MessageAnalytics contains message analytics data
type MessageAnalytics struct {
	TotalMessages       uint64                     `json:"totalMessages"`
	MessagesByPriority  map[MessagePriority]uint64 `json:"messagesByPriority"`
	AvgMessageSize      uint32                     `json:"avgMessageSize"`
	DeliverySuccessRate float32                    `json:"deliverySuccessRate"`
	AvgLatencyMs        float32                    `json:"avgLatencyMs"`
	MessagesByEventType map[string]uint64          `json:"messagesByEventType"`
}

// PlatformPerformance contains platform-specific performance metrics
type PlatformPerformance struct {
	Platform          string             `json:"platform"`
	ActiveConnections uint32             `json:"activeConnections"`
	MessageThroughput float32            `json:"messageThroughput"`
	AvgResponseTimeMs float32            `json:"avgResponseTimeMs"`
	ErrorRate         float32            `json:"errorRate"`
	MemoryUsageBytes  uint64             `json:"memoryUsageBytes"`
	PlatformMetrics   map[string]float32 `json:"platformMetrics"`
}

// API Interface Types

// RealtimeConnectionAPI defines the connection management interface
type RealtimeConnectionAPI interface {
	EstablishConnection(clientInfo ClientInfo, authToken *string, platform string) (*errorTypes.FlightResult[ConnectionInfo], error)
	CloseConnection(connectionID ConnectionID, reason string) (*errorTypes.FlightResult[bool], error)
	GetConnection(connectionID ConnectionID) (*errorTypes.FlightResult[ConnectionInfo], error)
	ListConnections(userID *string, platform *string) (*errorTypes.FlightResult[[]ConnectionInfo], error)
	UpdateConnectionState(connectionID ConnectionID, newState ConnectionState) (*errorTypes.FlightResult[bool], error)
	AuthenticateConnection(connectionID ConnectionID, authToken string) (*errorTypes.FlightResult[bool], error)
}

// RealtimeMessagingAPI defines the messaging interface
type RealtimeMessagingAPI interface {
	SendMessage(connectionID ConnectionID, message RealtimeMessage) (*errorTypes.FlightResult[bool], error)
	BroadcastMessage(channelID ChannelID, message RealtimeMessage) (*errorTypes.FlightResult[uint32], error)
	SendUserEvent(userID string, event RealtimeEvent, platform *string) (*errorTypes.FlightResult[bool], error)
	BroadcastEvent(channelID ChannelID, event RealtimeEvent) (*errorTypes.FlightResult[uint32], error)
	SendPriorityMessage(connectionID ConnectionID, message RealtimeMessage, priority MessagePriority) (*errorTypes.FlightResult[bool], error)
	GetMessageHistory(channelID ChannelID, limit uint32, before *uint64) (*errorTypes.FlightResult[[]RealtimeMessage], error)
}

// RealtimeSubscriptionAPI defines the subscription management interface
type RealtimeSubscriptionAPI interface {
	SubscribeToChannel(connectionID ConnectionID, channelID ChannelID, filters *[]SubscriptionFilter) (*errorTypes.FlightResult[Subscription], error)
	UnsubscribeFromChannel(connectionID ConnectionID, channelID ChannelID) (*errorTypes.FlightResult[bool], error)
	CreateChannel(channelInfo ChannelInfo) (*errorTypes.FlightResult[ChannelID], error)
	DeleteChannel(channelID ChannelID) (*errorTypes.FlightResult[bool], error)
	ListChannels(userID *string, platform *string) (*errorTypes.FlightResult[[]ChannelInfo], error)
	GetChannelSubscribers(channelID ChannelID) (*errorTypes.FlightResult[[]ConnectionID], error)
	UpdateSubscriptionFilters(connectionID ConnectionID, channelID ChannelID, filters []SubscriptionFilter) (*errorTypes.FlightResult[bool], error)
}

// RealtimeAnalyticsAPI defines the analytics interface
type RealtimeAnalyticsAPI interface {
	GetRealtimeMetrics() (*errorTypes.FlightResult[RealtimeMetrics], error)
	GetConnectionAnalytics(timeWindowHours uint32) (*errorTypes.FlightResult[ConnectionAnalytics], error)
	GetMessageAnalytics(channelID *ChannelID, timeWindowHours *uint32) (*errorTypes.FlightResult[MessageAnalytics], error)
	GenerateHealthReport() (*errorTypes.FlightResult[string], error)
	GetPlatformPerformance(platform string) (*errorTypes.FlightResult[PlatformPerformance], error)
}

// Platform-specific helper types

// FlightCorePlatformConfig contains Flight-Core platform configuration
type FlightCorePlatformConfig struct {
	Platform         string   `json:"platform"`
	MemoryConstraint string   `json:"memoryConstraint"`
	ConnectionMode   string   `json:"connectionMode"`
	Capabilities     []string `json:"capabilities"`
}

// V6RPlatformConfig contains V6R platform configuration
type V6RPlatformConfig struct {
	SubscriptionTier string     `json:"subscriptionTier"`
	ClientType       ClientType `json:"clientType"`
	Capabilities     []string   `json:"capabilities"`
	BillingEnabled   bool       `json:"billingEnabled"`
}

// Utility Functions

// IsConnected checks if a connection state represents a connected state
func IsConnected(state ConnectionState) bool {
	return state == ConnectionStateConnected || state == ConnectionStateAuthenticated
}

// IsDisconnected checks if a connection state represents a disconnected state
func IsDisconnected(state ConnectionState) bool {
	return state == ConnectionStateDisconnected ||
		state == ConnectionStateClosed ||
		state == ConnectionStateError
}

// IsPriorityMessage checks if a message has high priority
func IsPriorityMessage(priority MessagePriority) bool {
	return priority == MessagePriorityHigh ||
		priority == MessagePriorityCritical ||
		priority == MessagePriorityRealtime
}

// GetPlatformCapabilities returns capabilities for a given platform
func GetPlatformCapabilities(platform string) []string {
	switch platform {
	case "dreamcast":
		return []string{"basic-messaging", "memory-updates", "polling-mode"}
	case "psp":
		return []string{"basic-messaging", "memory-updates", "component-events", "polling-mode"}
	case "vita":
		return []string{"basic-messaging", "memory-updates", "component-events", "system-monitoring", "wifi-mode"}
	case "v6r-cloud":
		return []string{"vm-monitoring", "vm-management", "performance-metrics", "high-throughput"}
	default:
		return []string{"basic-messaging"}
	}
}

// GetMemoryConstraint returns the memory constraint for a platform
func GetMemoryConstraint(platform string) string {
	switch platform {
	case "dreamcast":
		return "16MB"
	case "psp":
		return "32MB"
	case "vita":
		return "512MB"
	default:
		return "unlimited"
	}
}

// GetConnectionMode returns the connection mode for a platform
func GetConnectionMode(platform string) string {
	switch platform {
	case "dreamcast", "psp":
		return "polling"
	case "vita":
		return "wifi"
	default:
		return "websocket"
	}
}

// CreateFlightCoreClientInfo creates client info for Flight-Core platforms
func CreateFlightCoreClientInfo(platform, version string) ClientInfo {
	if version == "" {
		version = "1.0.0"
	}

	userAgent := "Flight-Core/" + version + "/" + platform

	return ClientInfo{
		ClientType:   ClientTypeFlightCoreNative,
		Version:      version,
		Platform:     platform,
		Capabilities: GetPlatformCapabilities(platform),
		UserAgent:    &userAgent,
	}
}

// CreateV6RClientInfo creates client info for V6R platforms
func CreateV6RClientInfo(clientType ClientType, version, subscriptionTier string) ClientInfo {
	if version == "" {
		version = "2.0.0"
	}

	capabilities := []string{"basic-messaging"}

	if subscriptionTier == "team" || subscriptionTier == "enterprise" {
		capabilities = append(capabilities, "team-collaboration", "shared-channels")
	}

	if subscriptionTier == "enterprise" {
		capabilities = append(capabilities, "admin-controls", "audit-logs")
	}

	userAgent := "V6R-Client/" + version + "/" + subscriptionTier

	return ClientInfo{
		ClientType:   clientType,
		Version:      version,
		Platform:     "v6r-cloud",
		Capabilities: capabilities,
		UserAgent:    &userAgent,
	}
}

// CreateMessageRouting creates platform-appropriate message routing
func CreateMessageRouting(platform string, priority MessagePriority, requiresAck bool) MessageRouting {
	var retryConfig *RetryConfig

	if platform == "dreamcast" || platform == "psp" {
		retryConfig = &RetryConfig{
			MaxRetries:         1,
			RetryIntervalMs:    5000,
			BackoffMultiplier:  1.0,
			MaxRetryIntervalMs: 5000,
		}
	} else {
		retryConfig = &RetryConfig{
			MaxRetries:         3,
			RetryIntervalMs:    1000,
			BackoffMultiplier:  2.0,
			MaxRetryIntervalMs: 10000,
		}
	}

	requiresAckFinal := requiresAck || priority == MessagePriorityCritical || priority == MessagePriorityRealtime

	return MessageRouting{
		Source:      platform + "-system",
		Route:       []string{platform},
		RequiresAck: requiresAckFinal,
		RetryConfig: retryConfig,
	}
}

// CreateMemoryUpdatesChannel creates a memory updates channel for a platform
func CreateMemoryUpdatesChannel(platform string) ChannelInfo {
	var maxConnections *uint32
	if platform == "dreamcast" {
		maxConn := uint32(1)
		maxConnections = &maxConn
	}

	updateInterval := "1000"
	if platform == "dreamcast" {
		updateInterval = "5000"
	}

	return ChannelInfo{
		ID:                  ChannelID(platform + "-memory-updates"),
		Name:                platform + " Memory Updates",
		ChannelType:         ChannelTypeMemoryUpdates,
		RequiredPermissions: []string{"memory-access"},
		MaxConnections:      maxConnections,
		CreatedAt:           uint64(time.Now().Unix()),
		Metadata: map[string]string{
			"platform":       platform,
			"updateInterval": updateInterval,
		},
	}
}

// CreateComponentEventsChannel creates a component events channel for a platform
func CreateComponentEventsChannel(platform string) ChannelInfo {
	var maxConnections *uint32
	if platform == "dreamcast" {
		maxConn := uint32(1)
		maxConnections = &maxConn
	}

	return ChannelInfo{
		ID:                  ChannelID(platform + "-component-events"),
		Name:                platform + " Component Events",
		ChannelType:         ChannelTypeComponentEvents,
		RequiredPermissions: []string{"component-access"},
		MaxConnections:      maxConnections,
		CreatedAt:           uint64(time.Now().Unix()),
		Metadata: map[string]string{
			"platform":   platform,
			"eventTypes": "component-loaded,component-unloaded,state-changed",
		},
	}
}

// IsMemoryUpdateEvent checks if an event is a memory update event
func (e *RealtimeEvent) IsMemoryUpdateEvent() bool {
	return e.Type == "memory-update"
}

// IsComponentUpdateEvent checks if an event is a component update event
func (e *RealtimeEvent) IsComponentUpdateEvent() bool {
	return e.Type == "component-update"
}

// IsSessionEvent checks if an event is a session event
func (e *RealtimeEvent) IsSessionEvent() bool {
	return e.Type == "session-event"
}

// IsAuthEvent checks if an event is an authentication event
func (e *RealtimeEvent) IsAuthEvent() bool {
	return e.Type == "auth-event"
}

// IsSystemEvent checks if an event is a system event
func (e *RealtimeEvent) IsSystemEvent() bool {
	return e.Type == "system-event"
}

// IsV6REvent checks if an event is a V6R event
func (e *RealtimeEvent) IsV6REvent() bool {
	return e.Type == "v6r-event"
}

// IsFlightEvent checks if an event is a Flight-Core event
func (e *RealtimeEvent) IsFlightEvent() bool {
	return e.Type == "flight-event"
}

// IsCustomEvent checks if an event is a custom event
func (e *RealtimeEvent) IsCustomEvent() bool {
	return e.Type == "custom-event"
}

// GetMemoryUpdateData extracts memory update data from the event
func (e *RealtimeEvent) GetMemoryUpdateData() (*memoryTypes.MemoryUsageSnapshot, error) {
	if !e.IsMemoryUpdateEvent() {
		return nil, errorTypes.NewFlightError("INVALID_EVENT_TYPE", "Event is not a memory update event", nil)
	}

	data, ok := e.Data.(memoryTypes.MemoryUsageSnapshot)
	if !ok {
		return nil, errorTypes.NewFlightError("TYPE_ASSERTION_FAILED", "Failed to extract memory update data", nil)
	}

	return &data, nil
}

// GetComponentUpdateData extracts component update data from the event
func (e *RealtimeEvent) GetComponentUpdateData() (*componentTypes.ComponentInfo, error) {
	if !e.IsComponentUpdateEvent() {
		return nil, errorTypes.NewFlightError("INVALID_EVENT_TYPE", "Event is not a component update event", nil)
	}

	data, ok := e.Data.(componentTypes.ComponentInfo)
	if !ok {
		return nil, errorTypes.NewFlightError("TYPE_ASSERTION_FAILED", "Failed to extract component update data", nil)
	}

	return &data, nil
}

// Constants for common configurations
var (
	FlightCorePlatforms  = []string{"dreamcast", "psp", "vita"}
	V6RSubscriptionTiers = []string{"free", "individual", "team", "enterprise"}
	ConstrainedPlatforms = []string{"dreamcast", "psp"}
)

// Default message priorities
var DefaultMessagePriorities = map[string]MessagePriority{
	"SYSTEM":      MessagePriorityCritical,
	"USER_ACTION": MessagePriorityHigh,
	"UPDATE":      MessagePriorityNormal,
	"BACKGROUND":  MessagePriorityLow,
}

// Default retry configurations for different platforms
var DefaultRetryConfigs = map[string]RetryConfig{
	"DREAMCAST": {
		MaxRetries:         1,
		RetryIntervalMs:    5000,
		BackoffMultiplier:  1.0,
		MaxRetryIntervalMs: 5000,
	},
	"PSP": {
		MaxRetries:         2,
		RetryIntervalMs:    3000,
		BackoffMultiplier:  1.5,
		MaxRetryIntervalMs: 6000,
	},
	"STANDARD": {
		MaxRetries:         3,
		RetryIntervalMs:    1000,
		BackoffMultiplier:  2.0,
		MaxRetryIntervalMs: 10000,
	},
}
