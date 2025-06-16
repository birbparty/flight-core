// Package platformtypes provides Go bindings for Flight Platform Types
package platformtypes

import (
	errortypes "github.com/flight/domains/flight-shared-types/bindings/go/error"
)

// PlatformConstraints represents platform resource constraints
type PlatformConstraints struct {
	MaxMemory      MemorySize `json:"max_memory"`
	MaxCPUThreads  uint32     `json:"max_cpu_threads"`
	MaxOpenFiles   uint32     `json:"max_open_files"`
	NetworkEnabled bool       `json:"network_enabled"`
}

// MemorySize represents memory size with human-readable format
type MemorySize struct {
	Bytes         uint64 `json:"bytes"`
	HumanReadable string `json:"human_readable"`
}

// PlatformInfo contains platform information and capabilities
type PlatformInfo struct {
	ID           string              `json:"id"`
	Name         string              `json:"name"`
	Architecture string              `json:"architecture"`
	MemoryTotal  MemorySize          `json:"memory_total"`
	Capabilities []string            `json:"capabilities"`
	Constraints  PlatformConstraints `json:"constraints"`
	Version      string              `json:"version"`
	Vendor       string              `json:"vendor"`
	Features     []string            `json:"features"`
}

// PlatformManager manages platform operations
type PlatformManager struct {
	platforms map[string]PlatformInfo
}

// NewPlatformManager creates a new platform manager
func NewPlatformManager() *PlatformManager {
	return &PlatformManager{
		platforms: make(map[string]PlatformInfo),
	}
}

// GetPlatformInfo retrieves platform information by ID
func (pm *PlatformManager) GetPlatformInfo(platformID string) (*PlatformInfo, error) {
	platform, exists := pm.platforms[platformID]
	if !exists {
		errorManager := errortypes.NewErrorManager()
		err := errorManager.CreateSimpleError(
			errortypes.ErrorSeverityError,
			errortypes.ErrorCategoryPlatform,
			"Platform not found: "+platformID,
			"platform",
			"GetPlatformInfo",
		)
		return nil, err
	}
	return &platform, nil
}

// RegisterPlatform registers a new platform
func (pm *PlatformManager) RegisterPlatform(info PlatformInfo) error {
	pm.platforms[info.ID] = info
	return nil
}

// ListPlatforms returns all registered platforms
func (pm *PlatformManager) ListPlatforms() []PlatformInfo {
	platforms := make([]PlatformInfo, 0, len(pm.platforms))
	for _, platform := range pm.platforms {
		platforms = append(platforms, platform)
	}
	return platforms
}

// GetDreamcastPlatform returns Dreamcast platform configuration
func GetDreamcastPlatform() PlatformInfo {
	return PlatformInfo{
		ID:           "dreamcast",
		Name:         "Sega Dreamcast",
		Architecture: "SH-4",
		MemoryTotal: MemorySize{
			Bytes:         16 * 1024 * 1024, // 16MB
			HumanReadable: "16MB",
		},
		Capabilities: []string{"basic-graphics", "audio"},
		Constraints: PlatformConstraints{
			MaxMemory: MemorySize{
				Bytes:         16 * 1024 * 1024,
				HumanReadable: "16MB",
			},
			MaxCPUThreads:  1,
			MaxOpenFiles:   32,
			NetworkEnabled: false,
		},
		Version:  "1.0",
		Vendor:   "Sega",
		Features: []string{"component-model"},
	}
}

// GetVitaPlatform returns PlayStation Vita platform configuration
func GetVitaPlatform() PlatformInfo {
	return PlatformInfo{
		ID:           "vita",
		Name:         "PlayStation Vita",
		Architecture: "ARM Cortex-A9",
		MemoryTotal: MemorySize{
			Bytes:         512 * 1024 * 1024, // 512MB
			HumanReadable: "512MB",
		},
		Capabilities: []string{"graphics", "audio", "network"},
		Constraints: PlatformConstraints{
			MaxMemory: MemorySize{
				Bytes:         512 * 1024 * 1024,
				HumanReadable: "512MB",
			},
			MaxCPUThreads:  4,
			MaxOpenFiles:   1024,
			NetworkEnabled: true,
		},
		Version:  "3.60",
		Vendor:   "Sony",
		Features: []string{"component-model", "real-time"},
	}
}

// ValidatePlatformConstraints validates if platform meets minimum requirements
func ValidatePlatformConstraints(info PlatformInfo, requiredMemory uint64, requiredThreads uint32) bool {
	return info.Constraints.MaxMemory.Bytes >= requiredMemory &&
		info.Constraints.MaxCPUThreads >= requiredThreads
}
