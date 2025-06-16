// Go Integration Tests for Flight Memory Types
// Generic validation of Go bindings for Flight Memory Types

package integration

import (
	"encoding/json"
	"fmt"
	"testing"
	"time"

	memory "../../../bindings/go/enhanced"
)

// Generic utility functions for testing
type MemoryTestUtils struct{}

func (u MemoryTestUtils) CreateMemorySize(bytes uint64) memory.MemorySize {
	return memory.MemorySize{
		Bytes:         bytes,
		HumanReadable: u.FormatBytes(bytes),
	}
}

func (u MemoryTestUtils) FormatBytes(bytes uint64) string {
	units := []string{"B", "KB", "MB", "GB", "TB"}
	value := float64(bytes)
	unitIndex := 0

	for value >= 1024 && unitIndex < len(units)-1 {
		value /= 1024
		unitIndex++
	}

	return fmt.Sprintf("%.1f%s", value, units[unitIndex])
}

func (u MemoryTestUtils) CalculateUsagePercentage(snapshot memory.MemoryUsageSnapshot) float64 {
	return (float64(snapshot.Used.Bytes) / float64(snapshot.Total.Bytes)) * 100
}

func (u MemoryTestUtils) GetMemoryPressureLevel(percentage float64) memory.MemoryPressure {
	if percentage < 50 {
		return memory.MemoryPressureLow
	}
	if percentage < 80 {
		return memory.MemoryPressureMedium
	}
	if percentage < 95 {
		return memory.MemoryPressureHigh
	}
	return memory.MemoryPressureCritical
}

func (u MemoryTestUtils) CreateMemoryUpdate(sessionID string, snapshot memory.MemoryUsageSnapshot) memory.MemoryUpdate {
	return memory.MemoryUpdate{
		Type:      "memory_update",
		SessionID: sessionID,
		Snapshot:  snapshot,
		Timestamp: uint64(time.Now().Unix()),
	}
}

func (u MemoryTestUtils) CreateTestSnapshot(platform string, totalMB, usedMB uint64) memory.MemoryUsageSnapshot {
	total := u.CreateMemorySize(totalMB * 1024 * 1024)
	used := u.CreateMemorySize(usedMB * 1024 * 1024)
	available := u.CreateMemorySize((totalMB - usedMB) * 1024 * 1024)

	return memory.MemoryUsageSnapshot{
		Timestamp:          uint64(time.Now().Unix()),
		SessionID:          "test_session",
		Platform:           platform,
		Total:              total,
		Used:               used,
		Available:          available,
		FragmentationRatio: float32(usedMB) / float32(totalMB) * 0.1,
	}
}

func TestFlightMemoryTypes_GoIntegration(t *testing.T) {
	utils := MemoryTestUtils{}

	t.Run("Memory Size Creation and Formatting", func(t *testing.T) {
		// Dreamcast: 16MB
		dreamcast := utils.CreateMemorySize(16 * 1024 * 1024)
		if dreamcast.Bytes != uint64(16*1024*1024) {
			t.Errorf("Expected Dreamcast bytes %d, got %d", uint64(16*1024*1024), dreamcast.Bytes)
		}
		if dreamcast.HumanReadable != "16.0MB" {
			t.Errorf("Expected Dreamcast human readable '16.0MB', got '%s'", dreamcast.HumanReadable)
		}

		// PSP: 32MB
		psp := utils.CreateMemorySize(32 * 1024 * 1024)
		if psp.Bytes != uint64(32*1024*1024) {
			t.Errorf("Expected PSP bytes %d, got %d", uint64(32*1024*1024), psp.Bytes)
		}
		if psp.HumanReadable != "32.0MB" {
			t.Errorf("Expected PSP human readable '32.0MB', got '%s'", psp.HumanReadable)
		}

		// Vita: 512MB
		vita := utils.CreateMemorySize(512 * 1024 * 1024)
		if vita.Bytes != uint64(512*1024*1024) {
			t.Errorf("Expected Vita bytes %d, got %d", uint64(512*1024*1024), vita.Bytes)
		}
		if vita.HumanReadable != "512.0MB" {
			t.Errorf("Expected Vita human readable '512.0MB', got '%s'", vita.HumanReadable)
		}

		// Custom: 1GB
		custom := utils.CreateMemorySize(1024 * 1024 * 1024)
		if custom.Bytes != uint64(1024*1024*1024) {
			t.Errorf("Expected Custom bytes %d, got %d", uint64(1024*1024*1024), custom.Bytes)
		}
		if custom.HumanReadable != "1.0GB" {
			t.Errorf("Expected Custom human readable '1.0GB', got '%s'", custom.HumanReadable)
		}
	})

	t.Run("Byte Formatting Consistency", func(t *testing.T) {
		if utils.FormatBytes(1024) != "1.0KB" {
			t.Errorf("Expected '1.0KB', got '%s'", utils.FormatBytes(1024))
		}
		if utils.FormatBytes(1048576) != "1.0MB" {
			t.Errorf("Expected '1.0MB', got '%s'", utils.FormatBytes(1048576))
		}
		if utils.FormatBytes(1073741824) != "1.0GB" {
			t.Errorf("Expected '1.0GB', got '%s'", utils.FormatBytes(1073741824))
		}
		if utils.FormatBytes(1099511627776) != "1.0TB" {
			t.Errorf("Expected '1.0TB', got '%s'", utils.FormatBytes(1099511627776))
		}
	})

	t.Run("Memory Usage Calculations", func(t *testing.T) {
		snapshot := utils.CreateTestSnapshot("dreamcast", 16, 8)
		percentage := utils.CalculateUsagePercentage(snapshot)
		if percentage != 50.0 {
			t.Errorf("Expected usage percentage 50.0, got %f", percentage)
		}
	})

	t.Run("Platform Profile Support", func(t *testing.T) {
		platforms := []string{"dreamcast", "psp", "vita", "custom"}

		for _, platform := range platforms {
			snapshot := utils.CreateTestSnapshot(platform, 512, 256)
			if snapshot.Platform != platform {
				t.Errorf("Expected platform %s, got %s", platform, snapshot.Platform)
			}
			if snapshot.Total.Bytes != uint64(512*1024*1024) {
				t.Errorf("Expected total bytes %d, got %d", uint64(512*1024*1024), snapshot.Total.Bytes)
			}
			if snapshot.Used.Bytes != uint64(256*1024*1024) {
				t.Errorf("Expected used bytes %d, got %d", uint64(256*1024*1024), snapshot.Used.Bytes)
			}
		}
	})

	t.Run("JSON Serialization", func(t *testing.T) {
		snapshot := utils.CreateTestSnapshot("vita", 512, 256)
		update := utils.CreateMemoryUpdate("json_test", snapshot)

		// Test JSON marshaling
		jsonData, err := json.Marshal(update)
		if err != nil {
			t.Errorf("JSON marshaling failed: %v", err)
		}

		// Test JSON unmarshaling
		var unmarshaled memory.MemoryUpdate
		err = json.Unmarshal(jsonData, &unmarshaled)
		if err != nil {
			t.Errorf("JSON unmarshaling failed: %v", err)
		}

		if update.Type != unmarshaled.Type {
			t.Errorf("Expected type %s, got %s", update.Type, unmarshaled.Type)
		}
		if update.SessionID != unmarshaled.SessionID {
			t.Errorf("Expected session ID %s, got %s", update.SessionID, unmarshaled.SessionID)
		}
	})
}

// Benchmark tests for performance validation
func BenchmarkMemoryOperations(b *testing.B) {
	utils := MemoryTestUtils{}

	b.Run("MemorySize Creation", func(b *testing.B) {
		for i := 0; i < b.N; i++ {
			_ = utils.CreateMemorySize(uint64(i * 1024 * 1024))
		}
	})

	b.Run("Usage Percentage Calculation", func(b *testing.B) {
		snapshot := utils.CreateTestSnapshot("test", 1024, 512)
		b.ResetTimer()

		for i := 0; i < b.N; i++ {
			_ = utils.CalculateUsagePercentage(snapshot)
		}
	})

	b.Run("Memory Update Creation", func(b *testing.B) {
		snapshot := utils.CreateTestSnapshot("test", 512, 256)
		b.ResetTimer()

		for i := 0; i < b.N; i++ {
			_ = utils.CreateMemoryUpdate("bench_test", snapshot)
		}
	})

	b.Run("Bytes Formatting", func(b *testing.B) {
		for i := 0; i < b.N; i++ {
			_ = utils.FormatBytes(uint64(i * 1024 * 1024))
		}
	})

	b.Run("High Frequency Updates", func(b *testing.B) {
		sessionID := "high_freq_bench"
		b.ResetTimer()

		for i := 0; i < b.N; i++ {
			snapshot := utils.CreateTestSnapshot("vita", 512, uint64(i%512))
			_ = utils.CreateMemoryUpdate(sessionID, snapshot)
		}
	})

	b.Run("JSON Marshaling", func(b *testing.B) {
		snapshot := utils.CreateTestSnapshot("json_bench", 1024, 512)
		update := utils.CreateMemoryUpdate("json_test", snapshot)
		b.ResetTimer()

		for i := 0; i < b.N; i++ {
			_, _ = json.Marshal(update)
		}
	})
}
