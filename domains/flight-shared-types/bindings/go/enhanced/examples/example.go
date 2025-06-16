// Go Enhanced Bindings - Usage Example
package main

import (
	"fmt"
	"time"

	memory "github.com/flight/shared-types/go/enhanced"
)

func main() {
	// Basic memory utilities
	utils := memory.MemoryUtils{}

	memorySize := memory.NewMemorySize(1024 * 1024) // 1MB
	fmt.Println("Memory size:", memorySize.HumanReadable)

	// Create a sample snapshot
	snapshot := memory.MemoryUsageSnapshot{
		Timestamp:          uint64(time.Now().Unix()),
		SessionID:          "example",
		Platform:           "v6r-medium",
		Total:              memory.NewMemorySize(1024 * 1024 * 1024), // 1GB
		Used:               memory.NewMemorySize(512 * 1024 * 1024),  // 512MB
		Available:          memory.NewMemorySize(512 * 1024 * 1024),  // 512MB
		FragmentationRatio: 0.1,
	}

	// Calculate usage percentage
	percentage := utils.CalculateUsagePercentage(snapshot)
	fmt.Printf("Memory usage: %.1f%%\n", percentage)

	// Generate summary
	summary := utils.GenerateMemorySummary(snapshot)
	fmt.Println("Summary:", summary)

	// Event system
	emitter := memory.NewMemoryEventEmitter()

	unsubscribe := emitter.Subscribe("session-1", func(data interface{}) {
		if update, ok := data.(memory.MemoryUpdate); ok {
			fmt.Println("Memory update received:", update.Snapshot.Used.HumanReadable)
		}
	})
	defer unsubscribe()

	// Session configuration
	config := memory.SessionConfig{
		SessionID: "user-session-1",
		UserID:    "user-123",
		Platform:  "v6r-medium",
	}

	if err := utils.ValidateSessionConfig(config); err != nil {
		fmt.Println("Config error:", err)
	} else {
		fmt.Println("Config valid!")
	}

	// Create and emit a test update
	update := utils.CreateMemoryUpdate("session-1", snapshot)
	emitter.Emit(update)

	// Platform compatibility check
	requiredMemory := memory.NewMemorySize(256 * 1024 * 1024) // 256MB
	compatible := utils.IsPlatformCompatible("dreamcast", requiredMemory)
	fmt.Printf("256MB compatible with Dreamcast: %v\n", compatible)
}
