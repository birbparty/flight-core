// Enhanced Flight Memory Types - Generic Implementation
// Production-ready Go types with enhanced features and utilities

package memory

import (
	"encoding/json"
	"fmt"
	"time"
)

// Core memory types
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

type PlatformProfile interface {
	platformProfile()
	GetMemorySize() MemorySize
	GetPlatformName() string
}

type Dreamcast struct {
	Size MemorySize `json:"size"`
}

func (Dreamcast) platformProfile()            {}
func (d Dreamcast) GetMemorySize() MemorySize { return d.Size }
func (Dreamcast) GetPlatformName() string     { return "dreamcast" }

type PSP struct {
	Size MemorySize `json:"size"`
}

func (PSP) platformProfile()            {}
func (p PSP) GetMemorySize() MemorySize { return p.Size }
func (PSP) GetPlatformName() string     { return "psp" }

type Vita struct {
	Size MemorySize `json:"size"`
}

func (Vita) platformProfile()            {}
func (v Vita) GetMemorySize() MemorySize { return v.Size }
func (Vita) GetPlatformName() string     { return "vita" }

type V6RSmall struct {
	Size MemorySize `json:"size"`
}

func (V6RSmall) platformProfile()            {}
func (v V6RSmall) GetMemorySize() MemorySize { return v.Size }
func (V6RSmall) GetPlatformName() string     { return "v6r-small" }

type V6RMedium struct {
	Size MemorySize `json:"size"`
}

func (V6RMedium) platformProfile()            {}
func (v V6RMedium) GetMemorySize() MemorySize { return v.Size }
func (V6RMedium) GetPlatformName() string     { return "v6r-medium" }

type V6RLarge struct {
	Size MemorySize `json:"size"`
}

func (V6RLarge) platformProfile()            {}
func (v V6RLarge) GetMemorySize() MemorySize { return v.Size }
func (V6RLarge) GetPlatformName() string     { return "v6r-large" }

type Custom struct {
	Size MemorySize `json:"size"`
	Name string     `json:"name"`
}

func (Custom) platformProfile()            {}
func (c Custom) GetMemorySize() MemorySize { return c.Size }
func (c Custom) GetPlatformName() string   { return c.Name }

type MemoryAllocation struct {
	ID          string        `json:"id"`
	SessionID   string        `json:"session_id"`
	Size        MemorySize    `json:"size"`
	Purpose     MemoryPurpose `json:"purpose"`
	AllocatedAt uint64        `json:"allocated_at"`
	FreedAt     *uint64       `json:"freed_at,omitempty"`
}

type MemoryPurpose string

const (
	MemoryPurposeVmHeap         MemoryPurpose = "vm-heap"
	MemoryPurposeComponentStack MemoryPurpose = "component-stack"
	MemoryPurposeAssetCache     MemoryPurpose = "asset-cache"
	MemoryPurposeJitCodeCache   MemoryPurpose = "jit-code-cache"
	MemoryPurposeSystemReserved MemoryPurpose = "system-reserved"
	MemoryPurposeWasmLinear     MemoryPurpose = "wasm-linear"
	MemoryPurposeNetworkBuffers MemoryPurpose = "network-buffers"
	MemoryPurposeTemporary      MemoryPurpose = "temporary"
)

type MemoryPressure string

const (
	MemoryPressureLow      MemoryPressure = "low"
	MemoryPressureMedium   MemoryPressure = "medium"
	MemoryPressureHigh     MemoryPressure = "high"
	MemoryPressureCritical MemoryPressure = "critical"
)

type MemoryLimits struct {
	HeapMax   MemorySize `json:"heap_max"`
	StackMax  MemorySize `json:"stack_max"`
	CacheMax  MemorySize `json:"cache_max"`
	SoftLimit MemorySize `json:"soft_limit"`
	HardLimit MemorySize `json:"hard_limit"`
}

type MemoryError struct {
	Code      MemoryErrorCode `json:"code"`
	Message   string          `json:"message"`
	Details   *string         `json:"details,omitempty"`
	Timestamp uint64          `json:"timestamp"`
}

func (e MemoryError) Error() string {
	if e.Details != nil {
		return fmt.Sprintf("%s: %s (%s)", e.Code, e.Message, *e.Details)
	}
	return fmt.Sprintf("%s: %s", e.Code, e.Message)
}

type MemoryErrorCode string

const (
	MemoryErrorCodeInsufficientMemory  MemoryErrorCode = "insufficient-memory"
	MemoryErrorCodeLimitExceeded       MemoryErrorCode = "limit-exceeded"
	MemoryErrorCodeInvalidSize         MemoryErrorCode = "invalid-size"
	MemoryErrorCodeAllocationFailed    MemoryErrorCode = "allocation-failed"
	MemoryErrorCodeAlreadyFreed        MemoryErrorCode = "already-freed"
	MemoryErrorCodeInvalidAllocation   MemoryErrorCode = "invalid-allocation"
	MemoryErrorCodeUnsupportedPlatform MemoryErrorCode = "unsupported-platform"
	MemoryErrorCodeFragmentationError  MemoryErrorCode = "fragmentation-error"
)

// Result type for exception-free error handling
type MemoryResult[T any] struct {
	Value *T           `json:"value,omitempty"`
	Error *MemoryError `json:"error,omitempty"`
}

func (r MemoryResult[T]) IsOk() bool {
	return r.Error == nil
}

func (r MemoryResult[T]) IsErr() bool {
	return r.Error != nil
}

func (r MemoryResult[T]) Unwrap() (T, error) {
	if r.Error != nil {
		return *new(T), *r.Error
	}
	if r.Value == nil {
		return *new(T), fmt.Errorf("result has no value")
	}
	return *r.Value, nil
}

func (r MemoryResult[T]) UnwrapOr(defaultValue T) T {
	if r.Error != nil || r.Value == nil {
		return defaultValue
	}
	return *r.Value
}

// Generic configuration types
type SessionConfig struct {
	SessionID    string        `json:"session_id"`
	UserID       string        `json:"user_id"`
	Platform     string        `json:"platform"`
	CustomLimits *MemoryLimits `json:"custom_limits,omitempty"`
}

type MemoryUpdate struct {
	Type      string              `json:"type"`
	SessionID string              `json:"session_id"`
	Snapshot  MemoryUsageSnapshot `json:"snapshot"`
	Timestamp uint64              `json:"timestamp"`
}

// Memory analytics types
type MemoryStats struct {
	TotalAllocations      uint64               `json:"total_allocations"`
	ActiveAllocations     uint64               `json:"active_allocations"`
	PeakMemory            MemorySize           `json:"peak_memory"`
	CurrentMemory         MemorySize           `json:"current_memory"`
	AverageAllocationSize MemorySize           `json:"average_allocation_size"`
	UsageByPurpose        []MemoryPurposeUsage `json:"usage_by_purpose"`
	EfficiencyRatio       float32              `json:"efficiency_ratio"`
}

type MemoryPurposeUsage struct {
	Purpose MemoryPurpose `json:"purpose"`
	Size    MemorySize    `json:"size"`
}

type MemoryTrend struct {
	Snapshots      []MemoryUsageSnapshot `json:"snapshots"`
	TrendDirection TrendDirection        `json:"trend_direction"`
	PredictedPeak  *MemorySize           `json:"predicted_peak,omitempty"`
}

type TrendDirection string

const (
	TrendDirectionIncreasing TrendDirection = "increasing"
	TrendDirectionDecreasing TrendDirection = "decreasing"
	TrendDirectionStable     TrendDirection = "stable"
	TrendDirectionVolatile   TrendDirection = "volatile"
)

// Event system types
type MemoryEventData struct {
	SessionID string `json:"session_id"`
	Timestamp uint64 `json:"timestamp"`
}

type AllocationEventData struct {
	MemoryEventData
	Allocation MemoryAllocation `json:"allocation"`
}

type PressureEventData struct {
	MemoryEventData
	Pressure MemoryPressure      `json:"pressure"`
	Snapshot MemoryUsageSnapshot `json:"snapshot"`
}

type LimitEventData struct {
	MemoryEventData
	LimitType    string     `json:"limit_type"` // "soft" or "hard"
	CurrentUsage MemorySize `json:"current_usage"`
	Limit        MemorySize `json:"limit"`
}

type MemoryEventHandler func(data interface{})

// Helper functions for type creation
func NewMemorySize(bytes uint64) MemorySize {
	return MemorySize{
		Bytes:         bytes,
		HumanReadable: FormatBytes(bytes),
	}
}

func NewMemoryResult[T any](value T) MemoryResult[T] {
	return MemoryResult[T]{Value: &value}
}

func NewMemoryResultError[T any](err MemoryError) MemoryResult[T] {
	return MemoryResult[T]{Error: &err}
}

func NewMemoryError(code MemoryErrorCode, message string) MemoryError {
	return MemoryError{
		Code:      code,
		Message:   message,
		Timestamp: uint64(time.Now().Unix()),
	}
}

func NewMemoryErrorWithDetails(code MemoryErrorCode, message, details string) MemoryError {
	return MemoryError{
		Code:      code,
		Message:   message,
		Details:   &details,
		Timestamp: uint64(time.Now().Unix()),
	}
}

// JSON marshaling for platform profiles
func (p Dreamcast) MarshalJSON() ([]byte, error) {
	return json.Marshal(map[string]interface{}{
		"tag": "dreamcast",
		"val": p.Size,
	})
}

func (p PSP) MarshalJSON() ([]byte, error) {
	return json.Marshal(map[string]interface{}{
		"tag": "psp",
		"val": p.Size,
	})
}

func (p Vita) MarshalJSON() ([]byte, error) {
	return json.Marshal(map[string]interface{}{
		"tag": "vita",
		"val": p.Size,
	})
}

func (p V6RSmall) MarshalJSON() ([]byte, error) {
	return json.Marshal(map[string]interface{}{
		"tag": "v6r-small",
		"val": p.Size,
	})
}

func (p V6RMedium) MarshalJSON() ([]byte, error) {
	return json.Marshal(map[string]interface{}{
		"tag": "v6r-medium",
		"val": p.Size,
	})
}

func (p V6RLarge) MarshalJSON() ([]byte, error) {
	return json.Marshal(map[string]interface{}{
		"tag": "v6r-large",
		"val": p.Size,
	})
}

func (p Custom) MarshalJSON() ([]byte, error) {
	return json.Marshal(map[string]interface{}{
		"tag": "custom",
		"val": p.Size,
	})
}
