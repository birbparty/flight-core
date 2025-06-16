// Package memorytypes provides Go bindings for Flight Memory Types
// Core memory management types for Flight-Core and V6R integration
package memorytypes

import (
	"encoding/json"
	"fmt"
	"time"
)

// MemorySize represents memory allocation sizes with human-readable formatting
// Used throughout both ecosystems for consistent memory measurement
type MemorySize struct {
	// Raw byte count - allows precise calculations
	Bytes uint64 `json:"bytes"`
	// Human-readable format: "16MB", "512KB", "2GB", etc.
	// Critical for V6R UI components and logging
	HumanReadable string `json:"human_readable"`
}

// NewMemorySize creates a new MemorySize with human-readable formatting
func NewMemorySize(bytes uint64) MemorySize {
	return MemorySize{
		Bytes:         bytes,
		HumanReadable: formatBytes(bytes),
	}
}

// MemoryUsageSnapshot represents memory usage snapshot for real-time monitoring
// V6R uses this for WebSocket memory updates and UI display
// Flight-Core uses this for platform memory adaptation
type MemoryUsageSnapshot struct {
	// Timestamp when snapshot was taken (Unix timestamp)
	Timestamp uint64 `json:"timestamp"`
	// Session identifier (V6R session or Flight-Core component instance)
	SessionID string `json:"session_id"`
	// Platform identifier for memory constraints
	Platform string `json:"platform"`
	// Total available memory for this context
	Total MemorySize `json:"total"`
	// Currently used memory
	Used MemorySize `json:"used"`
	// Available memory (calculated: total - used)
	Available MemorySize `json:"available"`
	// Memory fragmentation ratio (0.0-1.0)
	// Critical for Flight-Core constrained platforms
	FragmentationRatio float32 `json:"fragmentation_ratio"`
}

// UsagePercentage calculates the memory usage percentage
func (m *MemoryUsageSnapshot) UsagePercentage() float64 {
	if m.Total.Bytes == 0 {
		return 0.0
	}
	return float64(m.Used.Bytes) / float64(m.Total.Bytes) * 100.0
}

// PlatformProfile represents platform-specific memory profiles
// Enables both Flight-Core and V6R to adapt behavior based on memory constraints
type PlatformProfile struct {
	Type       PlatformType `json:"type"`
	MemorySize MemorySize   `json:"memory_size"`
}

// PlatformType enumeration for different platform types
type PlatformType int

const (
	// Dreamcast: 16MB baseline (PlayStation 1 dropped for C++17)
	PlatformTypeDreamcast PlatformType = iota
	// PlayStation Portable: 32-64MB depending on model
	PlatformTypePSP
	// Sony PlayStation Vita: 512MB
	PlatformTypeVita
	// V6R Small VM: 512MB for lightweight development
	PlatformTypeV6RSmall
	// V6R Medium VM: 1GB for standard development
	PlatformTypeV6RMedium
	// V6R Large VM: 2GB+ for intensive development
	PlatformTypeV6RLarge
	// Custom platform with specified memory
	PlatformTypeCustom
)

func (pt PlatformType) String() string {
	switch pt {
	case PlatformTypeDreamcast:
		return "dreamcast"
	case PlatformTypePSP:
		return "psp"
	case PlatformTypeVita:
		return "vita"
	case PlatformTypeV6RSmall:
		return "v6r-small"
	case PlatformTypeV6RMedium:
		return "v6r-medium"
	case PlatformTypeV6RLarge:
		return "v6r-large"
	case PlatformTypeCustom:
		return "custom"
	default:
		return "unknown"
	}
}

// MemoryAllocation represents memory allocation tracking record
// V6R uses this for VM resource accounting
// Flight-Core uses this for component memory management
type MemoryAllocation struct {
	// Unique allocation identifier
	ID string `json:"id"`
	// Session this allocation belongs to
	SessionID string `json:"session_id"`
	// Size of the allocation
	Size MemorySize `json:"size"`
	// Purpose/category of this allocation
	Purpose MemoryPurpose `json:"purpose"`
	// When allocation was created (Unix timestamp)
	AllocatedAt uint64 `json:"allocated_at"`
	// When allocation was freed (nil if still active)
	FreedAt *uint64 `json:"freed_at,omitempty"`
}

// IsActive returns true if the allocation is still active (not freed)
func (ma *MemoryAllocation) IsActive() bool {
	return ma.FreedAt == nil
}

// Duration returns how long the allocation has been active
func (ma *MemoryAllocation) Duration() time.Duration {
	now := uint64(time.Now().Unix())
	var endTime uint64
	if ma.FreedAt != nil {
		endTime = *ma.FreedAt
	} else {
		endTime = now
	}
	return time.Duration(endTime-ma.AllocatedAt) * time.Second
}

// MemoryPurpose represents memory allocation purposes for categorization
// Enables both systems to track memory usage by category
type MemoryPurpose int

const (
	// VM heap memory (V6R primary use case)
	MemoryPurposeVMHeap MemoryPurpose = iota
	// Component stack memory (Flight-Core components)
	MemoryPurposeComponentStack
	// Asset cache (textures, audio, etc.)
	MemoryPurposeAssetCache
	// JIT compiled code cache
	MemoryPurposeJITCodeCache
	// System reserved memory
	MemoryPurposeSystemReserved
	// WebAssembly linear memory
	MemoryPurposeWASMLinear
	// Network buffers
	MemoryPurposeNetworkBuffers
	// Temporary/scratch memory
	MemoryPurposeTemporary
)

func (mp MemoryPurpose) String() string {
	switch mp {
	case MemoryPurposeVMHeap:
		return "vm-heap"
	case MemoryPurposeComponentStack:
		return "component-stack"
	case MemoryPurposeAssetCache:
		return "asset-cache"
	case MemoryPurposeJITCodeCache:
		return "jit-code-cache"
	case MemoryPurposeSystemReserved:
		return "system-reserved"
	case MemoryPurposeWASMLinear:
		return "wasm-linear"
	case MemoryPurposeNetworkBuffers:
		return "network-buffers"
	case MemoryPurposeTemporary:
		return "temporary"
	default:
		return "unknown"
	}
}

// MemoryPressure represents memory pressure levels for adaptive behavior
// Flight-Core uses for platform adaptation
// V6R uses for VM scaling decisions
type MemoryPressure int

const (
	// Plenty of memory available
	MemoryPressureLow MemoryPressure = iota
	// Memory usage getting high but manageable
	MemoryPressureMedium
	// Memory critically low - aggressive cleanup needed
	MemoryPressureHigh
	// Out of memory - emergency measures required
	MemoryPressureCritical
)

func (mp MemoryPressure) String() string {
	switch mp {
	case MemoryPressureLow:
		return "low"
	case MemoryPressureMedium:
		return "medium"
	case MemoryPressureHigh:
		return "high"
	case MemoryPressureCritical:
		return "critical"
	default:
		return "unknown"
	}
}

// MemoryLimits represents memory limits configuration
// V6R uses for VM resource enforcement
// Flight-Core uses for platform constraint adaptation
type MemoryLimits struct {
	// Maximum memory for VM/component heap
	HeapMax MemorySize `json:"heap_max"`
	// Maximum memory for stack
	StackMax MemorySize `json:"stack_max"`
	// Maximum memory for caching
	CacheMax MemorySize `json:"cache_max"`
	// Soft limit before warnings
	SoftLimit MemorySize `json:"soft_limit"`
	// Hard limit before allocation failures
	HardLimit MemorySize `json:"hard_limit"`
}

// MemoryErrorCode represents memory error codes
type MemoryErrorCode int

const (
	// Insufficient memory available
	MemoryErrorInsufficientMemory MemoryErrorCode = iota
	// Memory limit exceeded
	MemoryErrorLimitExceeded
	// Invalid memory size requested
	MemoryErrorInvalidSize
	// Memory allocation failed
	MemoryErrorAllocationFailed
	// Memory already freed
	MemoryErrorAlreadyFreed
	// Invalid allocation ID
	MemoryErrorInvalidAllocation
	// Platform not supported
	MemoryErrorUnsupportedPlatform
	// Memory fragmentation too high
	MemoryErrorFragmentationError
)

func (mec MemoryErrorCode) String() string {
	switch mec {
	case MemoryErrorInsufficientMemory:
		return "insufficient-memory"
	case MemoryErrorLimitExceeded:
		return "limit-exceeded"
	case MemoryErrorInvalidSize:
		return "invalid-size"
	case MemoryErrorAllocationFailed:
		return "allocation-failed"
	case MemoryErrorAlreadyFreed:
		return "already-freed"
	case MemoryErrorInvalidAllocation:
		return "invalid-allocation"
	case MemoryErrorUnsupportedPlatform:
		return "unsupported-platform"
	case MemoryErrorFragmentationError:
		return "fragmentation-error"
	default:
		return "unknown"
	}
}

// MemoryError represents memory-specific error types
// Comprehensive error handling for memory operations
type MemoryError struct {
	// Error code for programmatic handling
	Code MemoryErrorCode `json:"code"`
	// Human-readable error message
	Message string `json:"message"`
	// Additional context/details
	Details *string `json:"details,omitempty"`
	// When error occurred
	Timestamp uint64 `json:"timestamp"`
}

func (me *MemoryError) Error() string {
	if me.Details != nil {
		return fmt.Sprintf("%s: %s (%s)", me.Code.String(), me.Message, *me.Details)
	}
	return fmt.Sprintf("%s: %s", me.Code.String(), me.Message)
}

// NewMemoryError creates a new memory error
func NewMemoryError(code MemoryErrorCode, message string, details *string) *MemoryError {
	return &MemoryError{
		Code:      code,
		Message:   message,
		Details:   details,
		Timestamp: uint64(time.Now().Unix()),
	}
}

// MemoryStats represents comprehensive memory statistics
type MemoryStats struct {
	// Total allocations made
	TotalAllocations uint64 `json:"total_allocations"`
	// Active allocations count
	ActiveAllocations uint64 `json:"active_allocations"`
	// Peak memory usage recorded
	PeakMemory MemorySize `json:"peak_memory"`
	// Current memory usage
	CurrentMemory MemorySize `json:"current_memory"`
	// Average allocation size
	AverageAllocationSize MemorySize `json:"average_allocation_size"`
	// Memory usage by purpose
	UsageByPurpose map[MemoryPurpose]MemorySize `json:"usage_by_purpose"`
	// Memory efficiency ratio (0.0-1.0)
	EfficiencyRatio float32 `json:"efficiency_ratio"`
}

// TrendDirection represents trend direction enumeration
type TrendDirection int

const (
	TrendDirectionIncreasing TrendDirection = iota
	TrendDirectionDecreasing
	TrendDirectionStable
	TrendDirectionVolatile
)

func (td TrendDirection) String() string {
	switch td {
	case TrendDirectionIncreasing:
		return "increasing"
	case TrendDirectionDecreasing:
		return "decreasing"
	case TrendDirectionStable:
		return "stable"
	case TrendDirectionVolatile:
		return "volatile"
	default:
		return "unknown"
	}
}

// MemoryTrend represents memory usage trend data
type MemoryTrend struct {
	// Data points over time
	Snapshots []MemoryUsageSnapshot `json:"snapshots"`
	// Trend direction (increasing/decreasing/stable)
	TrendDirection TrendDirection `json:"trend_direction"`
	// Predicted future usage
	PredictedPeak *MemorySize `json:"predicted_peak,omitempty"`
}

// FlightResult represents a result that can be either success or error
type FlightResult[T any] struct {
	Success bool         `json:"success"`
	Value   *T           `json:"value,omitempty"`
	Error   *MemoryError `json:"error,omitempty"`
}

// NewSuccessResult creates a successful result
func NewSuccessResult[T any](value T) FlightResult[T] {
	return FlightResult[T]{
		Success: true,
		Value:   &value,
	}
}

// NewErrorResult creates an error result
func NewErrorResult[T any](err *MemoryError) FlightResult[T] {
	return FlightResult[T]{
		Success: false,
		Error:   err,
	}
}

// Memory operations manager
type MemoryManager struct {
	allocations map[string]*MemoryAllocation
	limits      map[string]*MemoryLimits
	profiles    map[string]*PlatformProfile
}

// NewMemoryManager creates a new memory manager
func NewMemoryManager() *MemoryManager {
	return &MemoryManager{
		allocations: make(map[string]*MemoryAllocation),
		limits:      make(map[string]*MemoryLimits),
		profiles:    make(map[string]*PlatformProfile),
	}
}

// CreateAllocation creates a new memory allocation record
// V6R calls this when allocating VM memory
// Flight-Core calls this for component memory tracking
func (mm *MemoryManager) CreateAllocation(sessionID string, size MemorySize, purpose MemoryPurpose) FlightResult[MemoryAllocation] {
	// Validate session limits
	if limits, exists := mm.limits[sessionID]; exists {
		if err := mm.validateAllocation(sessionID, size, limits); err != nil {
			return NewErrorResult[MemoryAllocation](err)
		}
	}

	allocationID := fmt.Sprintf("alloc-%s-%d", sessionID, time.Now().UnixNano())
	allocation := &MemoryAllocation{
		ID:          allocationID,
		SessionID:   sessionID,
		Size:        size,
		Purpose:     purpose,
		AllocatedAt: uint64(time.Now().Unix()),
	}

	mm.allocations[allocationID] = allocation
	return NewSuccessResult(*allocation)
}

// FreeAllocation frees a memory allocation
// Marks allocation as freed and updates tracking
func (mm *MemoryManager) FreeAllocation(allocationID string) FlightResult[bool] {
	allocation, exists := mm.allocations[allocationID]
	if !exists {
		return NewErrorResult[bool](NewMemoryError(
			MemoryErrorInvalidAllocation,
			"Allocation not found",
			&allocationID,
		))
	}

	if allocation.FreedAt != nil {
		return NewErrorResult[bool](NewMemoryError(
			MemoryErrorAlreadyFreed,
			"Allocation already freed",
			&allocationID,
		))
	}

	now := uint64(time.Now().Unix())
	allocation.FreedAt = &now
	return NewSuccessResult(true)
}

// GetMemorySnapshot gets current memory usage snapshot
// V6R uses for real-time UI updates and WebSocket streaming
// Flight-Core uses for platform memory monitoring
func (mm *MemoryManager) GetMemorySnapshot(sessionID string) FlightResult[MemoryUsageSnapshot] {
	var totalAllocated uint64
	var activeAllocations int

	for _, allocation := range mm.allocations {
		if allocation.SessionID == sessionID && allocation.IsActive() {
			totalAllocated += allocation.Size.Bytes
			activeAllocations++
		}
	}

	// Get platform profile for total memory
	profile, exists := mm.profiles[sessionID]
	var totalMemory MemorySize
	if exists {
		totalMemory = profile.MemorySize
	} else {
		// Default to 1GB if no profile set
		totalMemory = NewMemorySize(1024 * 1024 * 1024)
	}

	used := NewMemorySize(totalAllocated)
	available := NewMemorySize(totalMemory.Bytes - totalAllocated)

	// Calculate fragmentation ratio (simplified)
	fragmentationRatio := float32(0.05) // 5% default fragmentation
	if activeAllocations > 100 {
		fragmentationRatio = float32(activeAllocations) / 1000.0
		if fragmentationRatio > 0.3 {
			fragmentationRatio = 0.3 // Cap at 30%
		}
	}

	snapshot := MemoryUsageSnapshot{
		Timestamp:          uint64(time.Now().Unix()),
		SessionID:          sessionID,
		Platform:           mm.getPlatformName(sessionID),
		Total:              totalMemory,
		Used:               used,
		Available:          available,
		FragmentationRatio: fragmentationRatio,
	}

	return NewSuccessResult(snapshot)
}

// GetPlatformProfile returns memory constraints and capabilities for platform
func (mm *MemoryManager) GetPlatformProfile(platform string) FlightResult[PlatformProfile] {
	// Return predefined profiles for known platforms
	switch platform {
	case "dreamcast":
		return NewSuccessResult(PlatformProfile{
			Type:       PlatformTypeDreamcast,
			MemorySize: NewMemorySize(16 * 1024 * 1024), // 16MB
		})
	case "psp":
		return NewSuccessResult(PlatformProfile{
			Type:       PlatformTypePSP,
			MemorySize: NewMemorySize(64 * 1024 * 1024), // 64MB
		})
	case "vita":
		return NewSuccessResult(PlatformProfile{
			Type:       PlatformTypeVita,
			MemorySize: NewMemorySize(512 * 1024 * 1024), // 512MB
		})
	case "v6r-small":
		return NewSuccessResult(PlatformProfile{
			Type:       PlatformTypeV6RSmall,
			MemorySize: NewMemorySize(512 * 1024 * 1024), // 512MB
		})
	case "v6r-medium":
		return NewSuccessResult(PlatformProfile{
			Type:       PlatformTypeV6RMedium,
			MemorySize: NewMemorySize(1024 * 1024 * 1024), // 1GB
		})
	case "v6r-large":
		return NewSuccessResult(PlatformProfile{
			Type:       PlatformTypeV6RLarge,
			MemorySize: NewMemorySize(2048 * 1024 * 1024), // 2GB
		})
	default:
		return NewErrorResult[PlatformProfile](NewMemoryError(
			MemoryErrorUnsupportedPlatform,
			"Platform not supported",
			&platform,
		))
	}
}

// SetMemoryLimits sets memory limits for session
// V6R uses for VM resource enforcement
// Flight-Core uses for component constraint setting
func (mm *MemoryManager) SetMemoryLimits(sessionID string, limits MemoryLimits) FlightResult[bool] {
	mm.limits[sessionID] = &limits
	return NewSuccessResult(true)
}

// GetMemoryLimits gets memory limits for session
func (mm *MemoryManager) GetMemoryLimits(sessionID string) FlightResult[MemoryLimits] {
	limits, exists := mm.limits[sessionID]
	if !exists {
		return NewErrorResult[MemoryLimits](NewMemoryError(
			MemoryErrorInvalidAllocation,
			"No limits set for session",
			&sessionID,
		))
	}
	return NewSuccessResult(*limits)
}

// GetMemoryPressure checks current memory pressure level
// Enables adaptive behavior based on memory availability
func (mm *MemoryManager) GetMemoryPressure(sessionID string) FlightResult[MemoryPressure] {
	snapshot := mm.GetMemorySnapshot(sessionID)
	if !snapshot.Success {
		return NewErrorResult[MemoryPressure](snapshot.Error)
	}

	usagePercent := snapshot.Value.UsagePercentage()

	var pressure MemoryPressure
	switch {
	case usagePercent < 50:
		pressure = MemoryPressureLow
	case usagePercent < 75:
		pressure = MemoryPressureMedium
	case usagePercent < 90:
		pressure = MemoryPressureHigh
	default:
		pressure = MemoryPressureCritical
	}

	return NewSuccessResult(pressure)
}

// ListAllocations lists all active allocations for session
// V6R uses for memory usage breakdowns
// Flight-Core uses for debugging and monitoring
func (mm *MemoryManager) ListAllocations(sessionID string) FlightResult[[]MemoryAllocation] {
	var result []MemoryAllocation

	for _, allocation := range mm.allocations {
		if allocation.SessionID == sessionID {
			result = append(result, *allocation)
		}
	}

	return NewSuccessResult(result)
}

// Helper functions
func (mm *MemoryManager) validateAllocation(sessionID string, size MemorySize, limits *MemoryLimits) *MemoryError {
	// Check hard limit
	if size.Bytes > limits.HardLimit.Bytes {
		return NewMemoryError(
			MemoryErrorLimitExceeded,
			"Allocation exceeds hard limit",
			nil,
		)
	}

	// Check current usage + new allocation
	snapshot := mm.GetMemorySnapshot(sessionID)
	if snapshot.Success {
		newTotal := snapshot.Value.Used.Bytes + size.Bytes
		if newTotal > limits.HardLimit.Bytes {
			return NewMemoryError(
				MemoryErrorInsufficientMemory,
				"Not enough memory available",
				nil,
			)
		}
	}

	return nil
}

func (mm *MemoryManager) getPlatformName(sessionID string) string {
	profile, exists := mm.profiles[sessionID]
	if exists {
		return profile.Type.String()
	}
	return "unknown"
}

func formatBytes(bytes uint64) string {
	const unit = 1024
	if bytes < unit {
		return fmt.Sprintf("%dB", bytes)
	}
	div, exp := int64(unit), 0
	for n := bytes / unit; n >= unit; n /= unit {
		div *= unit
		exp++
	}
	return fmt.Sprintf("%.1f%cB", float64(bytes)/float64(div), "KMGTPE"[exp])
}

// MarshalJSON implements custom JSON marshaling for MemoryPurpose
func (mp MemoryPurpose) MarshalJSON() ([]byte, error) {
	return json.Marshal(mp.String())
}

// MarshalJSON implements custom JSON marshaling for MemoryPressure
func (mpr MemoryPressure) MarshalJSON() ([]byte, error) {
	return json.Marshal(mpr.String())
}

// MarshalJSON implements custom JSON marshaling for PlatformType
func (pt PlatformType) MarshalJSON() ([]byte, error) {
	return json.Marshal(pt.String())
}
