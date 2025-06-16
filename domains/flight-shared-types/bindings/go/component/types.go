// Package component provides Go bindings for Flight Component Model types
// Optimized for Component Model validation and Flight-Core integration
package component

import (
	"time"
)

// Component identification types
type ComponentID string
type InstanceID string
type WorldName string
type InterfaceName string

// Component lifecycle states
// Universal state machine for component management
type ComponentState int

const (
	// Component binary loaded but not instantiated
	ComponentStateLoaded ComponentState = iota
	// Component instantiating (calling constructors)
	ComponentStateInstantiating
	// Component fully instantiated and ready
	ComponentStateInstantiated
	// Component actively executing
	ComponentStateRunning
	// Component execution suspended
	ComponentStateSuspended
	// Component shutting down (calling destructors)
	ComponentStateTerminating
	// Component terminated and resources released
	ComponentStateTerminated
	// Component in error state
	ComponentStateError
)

func (cs ComponentState) String() string {
	switch cs {
	case ComponentStateLoaded:
		return "loaded"
	case ComponentStateInstantiating:
		return "instantiating"
	case ComponentStateInstantiated:
		return "instantiated"
	case ComponentStateRunning:
		return "running"
	case ComponentStateSuspended:
		return "suspended"
	case ComponentStateTerminating:
		return "terminating"
	case ComponentStateTerminated:
		return "terminated"
	case ComponentStateError:
		return "error"
	default:
		return "unknown"
	}
}

// MemorySize represents memory allocation sizes
type MemorySize struct {
	Bytes         uint64 `json:"bytes"`
	HumanReadable string `json:"human_readable"`
}

// MemoryUsageSnapshot represents memory usage at a point in time
type MemoryUsageSnapshot struct {
	Timestamp          uint64     `json:"timestamp"`
	SessionID          string     `json:"session_id"`
	Platform           string     `json:"platform"`
	Total              MemorySize `json:"total"`
	Used               MemorySize `json:"used"`
	Available          MemorySize `json:"available"`
	FragmentationRatio float64    `json:"fragmentation_ratio"`
	UsagePercentage    float64    `json:"usage_percentage"`
}

// ComponentInfo represents complete component description for tracking and management
type ComponentInfo struct {
	// Unique component identifier
	ID ComponentID `json:"id"`
	// Component name
	Name string `json:"name"`
	// Component version
	Version string `json:"version"`
	// Current lifecycle state
	State ComponentState `json:"state"`
	// World interface this component implements
	World WorldName `json:"world"`
	// Platform where component runs
	Platform string `json:"platform"`
	// Session this component belongs to
	SessionID *string `json:"session_id,omitempty"`
	// Component creation time (Unix timestamp)
	CreatedAt uint64 `json:"created_at"`
	// Last activity timestamp (Unix timestamp)
	LastActivity uint64 `json:"last_activity"`
	// Memory usage by this component
	MemoryUsage MemoryUsageSnapshot `json:"memory_usage"`
	// Component-specific metadata
	Metadata []MetadataPair `json:"metadata"`
}

// MetadataPair represents key-value metadata
type MetadataPair struct {
	Key   string `json:"key"`
	Value string `json:"value"`
}

// Component resource management
type ResourceHandle uint32
type ResourceType string

// ResourceInfo tracks resource ownership and prevents leaks
type ResourceInfo struct {
	// Resource handle identifier
	Handle ResourceHandle `json:"handle"`
	// Type of resource (memory, file, network, etc.)
	ResourceType ResourceType `json:"resource_type"`
	// Component that owns this resource
	Owner ComponentID `json:"owner"`
	// Reference count for shared resources
	RefCount uint32 `json:"ref_count"`
	// Resource creation time (Unix timestamp)
	CreatedAt uint64 `json:"created_at"`
	// Resource size/usage
	Size MemorySize `json:"size"`
	// Resource-specific metadata
	Metadata []MetadataPair `json:"metadata"`
}

// Interface connection state
type InterfaceState int

const (
	// Interface available for connection
	InterfaceStateAvailable InterfaceState = iota
	// Interface connected and operational
	InterfaceStateConnected
	// Interface temporarily disconnected
	InterfaceStateDisconnected
	// Interface deprecated but still functional
	InterfaceStateDeprecated
	// Interface no longer available
	InterfaceStateUnavailable
)

func (is InterfaceState) String() string {
	switch is {
	case InterfaceStateAvailable:
		return "available"
	case InterfaceStateConnected:
		return "connected"
	case InterfaceStateDisconnected:
		return "disconnected"
	case InterfaceStateDeprecated:
		return "deprecated"
	case InterfaceStateUnavailable:
		return "unavailable"
	default:
		return "unknown"
	}
}

// ComponentInterface represents type-safe component interface definition
type ComponentInterface struct {
	// Interface name
	Name InterfaceName `json:"name"`
	// Component providing this interface
	Provider ComponentID `json:"provider"`
	// Interface version
	Version string `json:"version"`
	// Interface methods available
	Methods []string `json:"methods"`
	// Interface state (connected, disconnected, etc.)
	State InterfaceState `json:"state"`
}

// Dependency types for components
type DependencyType int

const (
	// Required interface import
	DependencyTypeInterfaceImport DependencyType = iota
	// Required resource access
	DependencyTypeResourceAccess
	// Required platform capability
	DependencyTypePlatformCapability
	// Required memory allocation
	DependencyTypeMemoryRequirement
	// Required other component
	DependencyTypeComponentDependency
)

func (dt DependencyType) String() string {
	switch dt {
	case DependencyTypeInterfaceImport:
		return "interface-import"
	case DependencyTypeResourceAccess:
		return "resource-access"
	case DependencyTypePlatformCapability:
		return "platform-capability"
	case DependencyTypeMemoryRequirement:
		return "memory-requirement"
	case DependencyTypeComponentDependency:
		return "component-dependency"
	default:
		return "unknown"
	}
}

// ComponentDependency manages component dependencies and resolution
type ComponentDependency struct {
	// Component that has the dependency
	Dependent ComponentID `json:"dependent"`
	// Component or interface being depended on
	Dependency string `json:"dependency"`
	// Type of dependency
	DependencyType DependencyType `json:"dependency_type"`
	// Whether dependency is optional
	Optional bool `json:"optional"`
	// Dependency resolution status
	Resolved bool `json:"resolved"`
}

// Execution priority levels
type ExecutionPriority int

const (
	// Low priority background tasks
	ExecutionPriorityLow ExecutionPriority = iota
	// Normal priority execution
	ExecutionPriorityNormal
	// High priority interactive tasks
	ExecutionPriorityHigh
	// Critical priority system tasks
	ExecutionPriorityCritical
)

func (ep ExecutionPriority) String() string {
	switch ep {
	case ExecutionPriorityLow:
		return "low"
	case ExecutionPriorityNormal:
		return "normal"
	case ExecutionPriorityHigh:
		return "high"
	case ExecutionPriorityCritical:
		return "critical"
	default:
		return "unknown"
	}
}

// Platform execution modes
type ExecutionMode int

const (
	// Single-threaded execution (Dreamcast)
	ExecutionModeSingleThreaded ExecutionMode = iota
	// Multi-threaded execution
	ExecutionModeMultiThreaded
	// Async/await execution
	ExecutionModeAsyncExecution
	// Real-time execution
	ExecutionModeRealTime
)

func (em ExecutionMode) String() string {
	switch em {
	case ExecutionModeSingleThreaded:
		return "single-threaded"
	case ExecutionModeMultiThreaded:
		return "multi-threaded"
	case ExecutionModeAsyncExecution:
		return "async-execution"
	case ExecutionModeRealTime:
		return "real-time"
	default:
		return "unknown"
	}
}

// ExecutionContext represents runtime execution environment and constraints
type ExecutionContext struct {
	// Component being executed
	Component ComponentID `json:"component"`
	// Current execution stack depth
	StackDepth uint32 `json:"stack_depth"`
	// Available memory
	AvailableMemory MemorySize `json:"available_memory"`
	// CPU time allocated (milliseconds)
	CPUTimeMs uint64 `json:"cpu_time_ms"`
	// Execution priority
	Priority ExecutionPriority `json:"priority"`
	// Platform execution mode
	ExecutionMode ExecutionMode `json:"execution_mode"`
}

// V6RAutoScalingConfig represents auto-scaling configuration
type V6RAutoScalingConfig struct {
	// Minimum component instances
	MinInstances uint32 `json:"min_instances"`
	// Maximum component instances
	MaxInstances uint32 `json:"max_instances"`
	// CPU utilization trigger percentage
	CPUThreshold float32 `json:"cpu_threshold"`
	// Memory utilization trigger percentage
	MemoryThreshold float32 `json:"memory_threshold"`
	// Scaling enabled
	Enabled bool `json:"enabled"`
}

// V6RMonitoringConfig represents monitoring configuration
type V6RMonitoringConfig struct {
	// Metrics collection enabled
	MetricsEnabled bool `json:"metrics_enabled"`
	// Distributed tracing enabled
	TracingEnabled bool `json:"tracing_enabled"`
	// Log aggregation enabled
	LoggingEnabled bool `json:"logging_enabled"`
	// Health check configuration
	HealthChecks bool `json:"health_checks"`
}

// V6RComponentExtensions represents cloud-native component features for V6R environments
type V6RComponentExtensions struct {
	// Container runtime integration
	ContainerID *string `json:"container_id,omitempty"`
	// Kubernetes pod integration
	PodName *string `json:"pod_name,omitempty"`
	// Service mesh integration
	ServiceMeshEnabled bool `json:"service_mesh_enabled"`
	// Auto-scaling configuration
	AutoScaling V6RAutoScalingConfig `json:"auto_scaling"`
	// Load balancing configuration
	LoadBalancing bool `json:"load_balancing"`
	// Monitoring and observability
	Monitoring V6RMonitoringConfig `json:"monitoring"`
}

// FlightResult represents a result that can be either success or error
type FlightResult[T any] struct {
	Success bool         `json:"success"`
	Value   *T           `json:"value,omitempty"`
	Error   *FlightError `json:"error,omitempty"`
}

// FlightError represents error information
type FlightError struct {
	ID       string                 `json:"id"`
	Severity string                 `json:"severity"`
	Category string                 `json:"category"`
	Message  string                 `json:"message"`
	Details  *string                `json:"details,omitempty"`
	Context  map[string]interface{} `json:"context"`
}

// Component lifecycle management functions

// CreateComponentRequest represents component creation parameters
type CreateComponentRequest struct {
	Name      string  `json:"name"`
	World     string  `json:"world"`
	Platform  string  `json:"platform"`
	SessionID *string `json:"session_id,omitempty"`
}

// ComponentManager manages component lifecycle operations
type ComponentManager struct {
	components map[ComponentID]*ComponentInfo
	resources  map[ResourceHandle]*ResourceInfo
	interfaces map[InterfaceName]*ComponentInterface
}

// NewComponentManager creates a new component manager
func NewComponentManager() *ComponentManager {
	return &ComponentManager{
		components: make(map[ComponentID]*ComponentInfo),
		resources:  make(map[ResourceHandle]*ResourceInfo),
		interfaces: make(map[InterfaceName]*ComponentInterface),
	}
}

// CreateComponent creates a new component instance
func (cm *ComponentManager) CreateComponent(req CreateComponentRequest) FlightResult[ComponentID] {
	componentID := ComponentID(generateComponentID())

	now := uint64(time.Now().Unix())

	component := &ComponentInfo{
		ID:           componentID,
		Name:         req.Name,
		Version:      "1.0.0",
		State:        ComponentStateLoaded,
		World:        WorldName(req.World),
		Platform:     req.Platform,
		SessionID:    req.SessionID,
		CreatedAt:    now,
		LastActivity: now,
		MemoryUsage: MemoryUsageSnapshot{
			Timestamp:          now,
			SessionID:          getSessionID(req.SessionID),
			Platform:           req.Platform,
			Total:              MemorySize{Bytes: 0, HumanReadable: "0B"},
			Used:               MemorySize{Bytes: 0, HumanReadable: "0B"},
			Available:          MemorySize{Bytes: 0, HumanReadable: "0B"},
			FragmentationRatio: 0.0,
			UsagePercentage:    0.0,
		},
		Metadata: []MetadataPair{
			{Key: "created_by", Value: "go-component-manager"},
			{Key: "version", Value: "1.0.0"},
		},
	}

	cm.components[componentID] = component

	return FlightResult[ComponentID]{
		Success: true,
		Value:   &componentID,
	}
}

// GetComponent retrieves complete component information
func (cm *ComponentManager) GetComponent(id ComponentID) FlightResult[ComponentInfo] {
	component, exists := cm.components[id]
	if !exists {
		return FlightResult[ComponentInfo]{
			Success: false,
			Error: &FlightError{
				ID:       "component-not-found",
				Severity: "error",
				Category: "ComponentLifecycle",
				Message:  "Component not found",
				Details:  stringPtr("Component ID: " + string(id)),
				Context:  map[string]interface{}{"component_id": id},
			},
		}
	}

	return FlightResult[ComponentInfo]{
		Success: true,
		Value:   component,
	}
}

// StartComponent transitions component to running state
func (cm *ComponentManager) StartComponent(id ComponentID) FlightResult[bool] {
	component, exists := cm.components[id]
	if !exists {
		return cm.componentNotFoundError(id)
	}

	if component.State != ComponentStateInstantiated {
		return FlightResult[bool]{
			Success: false,
			Error: &FlightError{
				ID:       "invalid-state-transition",
				Severity: "error",
				Category: "ComponentLifecycle",
				Message:  "Cannot start component from current state",
				Details:  stringPtr("Current state: " + component.State.String()),
				Context:  map[string]interface{}{"component_id": id, "current_state": component.State.String()},
			},
		}
	}

	component.State = ComponentStateRunning
	component.LastActivity = uint64(time.Now().Unix())

	success := true
	return FlightResult[bool]{
		Success: true,
		Value:   &success,
	}
}

// ListComponents lists components with optional filtering
func (cm *ComponentManager) ListComponents(sessionID *string, stateFilter *ComponentState) FlightResult[[]ComponentInfo] {
	var result []ComponentInfo

	for _, component := range cm.components {
		// Filter by session ID if provided
		if sessionID != nil {
			if component.SessionID == nil || *component.SessionID != *sessionID {
				continue
			}
		}

		// Filter by state if provided
		if stateFilter != nil && component.State != *stateFilter {
			continue
		}

		result = append(result, *component)
	}

	return FlightResult[[]ComponentInfo]{
		Success: true,
		Value:   &result,
	}
}

// AllocateResource allocates a new resource and tracks ownership
func (cm *ComponentManager) AllocateResource(componentID ComponentID, resourceType ResourceType, size MemorySize) FlightResult[ResourceHandle] {
	_, exists := cm.components[componentID]
	if !exists {
		return FlightResult[ResourceHandle]{
			Success: false,
			Error: &FlightError{
				ID:       "component-not-found",
				Severity: "error",
				Category: "ResourceManagement",
				Message:  "Component not found for resource allocation",
				Context:  map[string]interface{}{"component_id": componentID},
			},
		}
	}

	handle := ResourceHandle(generateResourceHandle())
	resource := &ResourceInfo{
		Handle:       handle,
		ResourceType: resourceType,
		Owner:        componentID,
		RefCount:     1,
		CreatedAt:    uint64(time.Now().Unix()),
		Size:         size,
		Metadata: []MetadataPair{
			{Key: "allocated_by", Value: "go-component-manager"},
		},
	}

	cm.resources[handle] = resource

	return FlightResult[ResourceHandle]{
		Success: true,
		Value:   &handle,
	}
}

// Helper functions
func (cm *ComponentManager) componentNotFoundError(id ComponentID) FlightResult[bool] {
	return FlightResult[bool]{
		Success: false,
		Error: &FlightError{
			ID:       "component-not-found",
			Severity: "error",
			Category: "ComponentLifecycle",
			Message:  "Component not found",
			Details:  stringPtr("Component ID: " + string(id)),
			Context:  map[string]interface{}{"component_id": id},
		},
	}
}

func generateComponentID() string {
	return "comp-" + generateUUID()
}

func generateResourceHandle() uint32 {
	return uint32(time.Now().UnixNano() & 0xFFFFFFFF)
}

func generateUUID() string {
	// Simple UUID generation for demo purposes
	return "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx"
}

func getSessionID(sessionID *string) string {
	if sessionID != nil {
		return *sessionID
	}
	return "default-session"
}

func stringPtr(s string) *string {
	return &s
}
