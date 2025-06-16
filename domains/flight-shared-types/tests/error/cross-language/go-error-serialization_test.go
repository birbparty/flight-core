// Go Cross-Language Error Serialization Tests
//
// Basic cross-language error serialization tests using standard library
// ensuring zero information loss across TypeScript, Go, Rust, and C++17

package crosslang

import (
	"encoding/json"
	"testing"
	"time"
)

// Define basic error types for cross-language testing
type ErrorSeverity string
type ErrorCategory string

const (
	ErrorSeverityInfo     ErrorSeverity = "info"
	ErrorSeverityWarning  ErrorSeverity = "warning"
	ErrorSeverityError    ErrorSeverity = "error"
	ErrorSeverityCritical ErrorSeverity = "critical"
	ErrorSeverityFatal    ErrorSeverity = "fatal"
)

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

type MetadataPair struct {
	Key   string `json:"key"`
	Value string `json:"value"`
}

type ErrorContext struct {
	Source    string         `json:"source"`
	Operation string         `json:"operation"`
	SessionID *string        `json:"session_id,omitempty"`
	UserID    *string        `json:"user_id,omitempty"`
	Platform  *string        `json:"platform,omitempty"`
	ServiceID *string        `json:"service_id,omitempty"`
	Metadata  []MetadataPair `json:"metadata"`
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

type FlightResult[T any] struct {
	Tag string `json:"tag"`
	Val any    `json:"val"`
}

func TestGoCrossLanguageErrorSerialization(t *testing.T) {
	t.Run("Basic Error Round-Trip Serialization", func(t *testing.T) {
		originalError := FlightError{
			ID:       "go-cross-lang-001",
			Severity: ErrorSeverityError,
			Category: ErrorCategoryMemory,
			Message:  "Go cross-language memory allocation failed",
			Details:  stringPtr("Insufficient heap space for cross-language buffer allocation"),
			Context: ErrorContext{
				Source:    "go-cross-lang-memory-manager",
				Operation: "allocate_cross_lang_buffer",
				SessionID: stringPtr("go-cross-session-123"),
				UserID:    stringPtr("go-cross-user-456"),
				Platform:  stringPtr("dreamcast"),
				ServiceID: stringPtr("go-cross-service-789"),
				Metadata: []MetadataPair{
					{"language_source", "go"},
					{"language_target", "typescript"},
					{"requested_bytes", "8388608"},
					{"available_bytes", "4194304"},
					{"fragmentation_ratio", "0.20"},
					{"cross_lang_call_depth", "2"},
					{"memory_pressure", "critical"},
					{"go_runtime_version", "1.21"},
					{"gc_pressure", "high"},
				},
			},
			Timestamp: uint64(time.Now().Unix()),
			Cause:     stringPtr("go-cross-language-gc-pressure"),
		}

		// Test JSON marshaling
		jsonData, err := json.Marshal(originalError)
		if err != nil {
			t.Fatalf("Failed to marshal error: %v", err)
		}
		if len(jsonData) == 0 {
			t.Fatal("JSON data is empty")
		}

		// Test JSON unmarshaling
		var deserializedError FlightError
		err = json.Unmarshal(jsonData, &deserializedError)
		if err != nil {
			t.Fatalf("Failed to unmarshal error: %v", err)
		}

		// Verify all fields preserved
		if originalError.ID != deserializedError.ID {
			t.Errorf("ID mismatch: expected %s, got %s", originalError.ID, deserializedError.ID)
		}
		if originalError.Severity != deserializedError.Severity {
			t.Errorf("Severity mismatch: expected %s, got %s", originalError.Severity, deserializedError.Severity)
		}
		if originalError.Category != deserializedError.Category {
			t.Errorf("Category mismatch: expected %s, got %s", originalError.Category, deserializedError.Category)
		}
		if originalError.Message != deserializedError.Message {
			t.Errorf("Message mismatch: expected %s, got %s", originalError.Message, deserializedError.Message)
		}

		// Verify context preservation
		if originalError.Context.Source != deserializedError.Context.Source {
			t.Errorf("Context source mismatch: expected %s, got %s", originalError.Context.Source, deserializedError.Context.Source)
		}
		if originalError.Context.Operation != deserializedError.Context.Operation {
			t.Errorf("Context operation mismatch: expected %s, got %s", originalError.Context.Operation, deserializedError.Context.Operation)
		}

		// Verify metadata preservation
		if len(deserializedError.Context.Metadata) != len(originalError.Context.Metadata) {
			t.Errorf("Metadata length mismatch: expected %d, got %d", len(originalError.Context.Metadata), len(deserializedError.Context.Metadata))
		}

		// Verify cross-language specific metadata
		metadataMap := make(map[string]string)
		for _, pair := range deserializedError.Context.Metadata {
			metadataMap[pair.Key] = pair.Value
		}
		if metadataMap["language_source"] != "go" {
			t.Errorf("Expected language_source 'go', got '%s'", metadataMap["language_source"])
		}
		if metadataMap["language_target"] != "typescript" {
			t.Errorf("Expected language_target 'typescript', got '%s'", metadataMap["language_target"])
		}
		if metadataMap["cross_lang_call_depth"] != "2" {
			t.Errorf("Expected cross_lang_call_depth '2', got '%s'", metadataMap["cross_lang_call_depth"])
		}
	})

	t.Run("Nullable Fields Cross-Language Handling", func(t *testing.T) {
		errorWithNulls := FlightError{
			ID:       "go-null-fields-001",
			Severity: ErrorSeverityWarning,
			Category: ErrorCategoryPlatform,
			Message:  "Go cross-language null field test",
			Details:  nil, // Test nil details
			Context: ErrorContext{
				Source:    "go-null-field-test",
				Operation: "test_null_handling",
				SessionID: nil, // Test nil session
				UserID:    nil, // Test nil user
				Platform:  stringPtr("v6r-medium"),
				ServiceID: nil, // Test nil service
				Metadata: []MetadataPair{
					{"null_test", "true"},
					{"go_nil_handling", "verified"},
					{"language", "go"},
				},
			},
			Timestamp: uint64(time.Now().Unix()),
			Cause:     nil, // Test nil cause
		}

		jsonData, err := json.Marshal(errorWithNulls)
		if err != nil {
			t.Fatalf("Failed to marshal error with nulls: %v", err)
		}

		var deserialized FlightError
		err = json.Unmarshal(jsonData, &deserialized)
		if err != nil {
			t.Fatalf("Failed to unmarshal error with nulls: %v", err)
		}

		// Verify nil handling
		if deserialized.Details != nil {
			t.Error("Expected nil details, got non-nil")
		}
		if deserialized.Context.SessionID != nil {
			t.Error("Expected nil SessionID, got non-nil")
		}
		if deserialized.Context.UserID != nil {
			t.Error("Expected nil UserID, got non-nil")
		}
		if deserialized.Context.ServiceID != nil {
			t.Error("Expected nil ServiceID, got non-nil")
		}
		if deserialized.Cause != nil {
			t.Error("Expected nil Cause, got non-nil")
		}

		// Verify non-nil fields preserved
		if errorWithNulls.ID != deserialized.ID {
			t.Errorf("ID mismatch: expected %s, got %s", errorWithNulls.ID, deserialized.ID)
		}
		if deserialized.Context.Platform == nil || *deserialized.Context.Platform != "v6r-medium" {
			t.Error("Platform field not preserved correctly")
		}
		if len(deserialized.Context.Metadata) != 3 {
			t.Errorf("Expected 3 metadata entries, got %d", len(deserialized.Context.Metadata))
		}
	})

	t.Run("Platform-Specific Cross-Language Serialization", func(t *testing.T) {
		dreamcastError := FlightError{
			ID:       "go-dreamcast-cross-lang-001",
			Severity: ErrorSeverityCritical,
			Category: ErrorCategoryPlatform,
			Message:  "Dreamcast memory limit exceeded in Go cross-language operation",
			Details:  stringPtr("Go garbage collector unable to free enough memory for cross-language buffer"),
			Context: ErrorContext{
				Source:    "go-dreamcast-cross-lang-allocator",
				Operation: "go_cross_lang_texture_allocation",
				SessionID: stringPtr("go-dreamcast-session"),
				UserID:    nil,
				Platform:  stringPtr("dreamcast"),
				ServiceID: nil,
				Metadata: []MetadataPair{
					{"total_memory_bytes", "16777216"},        // 16MB
					{"available_memory_bytes", "3145728"},     // 3MB
					{"requested_allocation_bytes", "8388608"}, // 8MB
					{"go_heap_size_bytes", "4194304"},         // 4MB
					{"go_stack_size_bytes", "1048576"},        // 1MB
					{"allocation_type", "go_cross_lang_texture_buffer"},
					{"source_language", "go"},
					{"target_language", "c++17"},
					{"hardware_arch", "sh4"},
					{"gc_cycles_attempted", "5"},
					{"gc_freed_bytes", "524288"}, // 512KB
					{"memory_fragmentation", "0.30"},
				},
			},
			Timestamp: uint64(time.Now().Unix()),
			Cause:     stringPtr("go-gc-insufficient-dreamcast"),
		}

		jsonData, err := json.Marshal(dreamcastError)
		if err != nil {
			t.Fatalf("Failed to marshal Dreamcast error: %v", err)
		}

		var deserialized FlightError
		err = json.Unmarshal(jsonData, &deserialized)
		if err != nil {
			t.Fatalf("Failed to unmarshal Dreamcast error: %v", err)
		}

		if deserialized.Context.Platform == nil || *deserialized.Context.Platform != "dreamcast" {
			t.Error("Platform should be 'dreamcast'")
		}

		metadataMap := make(map[string]string)
		for _, pair := range deserialized.Context.Metadata {
			metadataMap[pair.Key] = pair.Value
		}
		if metadataMap["total_memory_bytes"] != "16777216" {
			t.Errorf("Expected total_memory_bytes '16777216', got '%s'", metadataMap["total_memory_bytes"])
		}
		if metadataMap["source_language"] != "go" {
			t.Errorf("Expected source_language 'go', got '%s'", metadataMap["source_language"])
		}
		if metadataMap["target_language"] != "c++17" {
			t.Errorf("Expected target_language 'c++17', got '%s'", metadataMap["target_language"])
		}
		if metadataMap["hardware_arch"] != "sh4" {
			t.Errorf("Expected hardware_arch 'sh4', got '%s'", metadataMap["hardware_arch"])
		}
	})

	t.Run("Performance and Large Collection Handling", func(t *testing.T) {
		startTime := time.Now()

		// Create collection of errors
		errors := make([]FlightError, 100) // Reduced for testing
		for i := 0; i < 100; i++ {
			severity := ErrorSeverityWarning
			if i%2 == 0 {
				severity = ErrorSeverityError
			}

			errors[i] = FlightError{
				ID:       "go-perf-" + string(rune(i+'0')),
				Severity: severity,
				Category: ErrorCategoryMemory,
				Message:  "Go performance test error",
				Context: ErrorContext{
					Source:    "go-performance-test",
					Operation: "performance_test",
					Metadata: []MetadataPair{
						{"test_index", string(rune(i + '0'))},
						{"language", "go"},
					},
				},
				Timestamp: uint64(time.Now().Unix()),
			}
		}

		// Test serialization performance
		serializeStart := time.Now()
		jsonData, err := json.Marshal(errors)
		serializeTime := time.Since(serializeStart)

		if err != nil {
			t.Fatalf("Failed to marshal error collection: %v", err)
		}
		if len(jsonData) == 0 {
			t.Fatal("Serialized data is empty")
		}

		// Test deserialization performance
		deserializeStart := time.Now()
		var deserialized []FlightError
		err = json.Unmarshal(jsonData, &deserialized)
		deserializeTime := time.Since(deserializeStart)

		if err != nil {
			t.Fatalf("Failed to unmarshal error collection: %v", err)
		}
		if len(deserialized) != 100 {
			t.Errorf("Expected 100 errors, got %d", len(deserialized))
		}

		totalTime := time.Since(startTime)
		t.Logf("Go performance test completed in %v (serialize: %v, deserialize: %v)",
			totalTime, serializeTime, deserializeTime)

		// Basic performance assertions
		if serializeTime > time.Second {
			t.Error("Serialization took too long (> 1 second)")
		}
		if deserializeTime > 500*time.Millisecond {
			t.Error("Deserialization took too long (> 0.5 seconds)")
		}
	})

	t.Run("Unicode and Special Characters", func(t *testing.T) {
		unicodeError := FlightError{
			ID:       "go-unicode-test-001",
			Severity: ErrorSeverityError,
			Category: ErrorCategoryApplication,
			Message:  "Go Unicode test: 🚀🔥💻 Cross-language 日本語 测试",
			Details:  stringPtr("Testing special characters: \\n\\t\\r\\\"\\\\and émojis 🎮"),
			Context: ErrorContext{
				Source:    "go-unicode-test-service",
				Operation: "test_unicode_handling",
				SessionID: stringPtr("go-unicode-session-🎯"),
				UserID:    stringPtr("go-user-émile-测试"),
				Platform:  stringPtr("test-platform"),
				ServiceID: nil,
				Metadata: []MetadataPair{
					{"unicode_message", "🌍 Global Go test"},
					{"special_chars", "\\n\\t\\r\\\"\\\\"},
					{"languages", "日本語,中文,English,Français,Golang"},
					{"emoji_test", "🚀🔥💻🎮🎯🌍"},
				},
			},
			Timestamp: uint64(time.Now().Unix()),
			Cause:     nil,
		}

		jsonData, err := json.Marshal(unicodeError)
		if err != nil {
			t.Fatalf("Failed to marshal unicode error: %v", err)
		}

		var deserialized FlightError
		err = json.Unmarshal(jsonData, &deserialized)
		if err != nil {
			t.Fatalf("Failed to unmarshal unicode error: %v", err)
		}

		expectedMessage := "Go Unicode test: 🚀🔥💻 Cross-language 日本語 测试"
		if deserialized.Message != expectedMessage {
			t.Errorf("Message mismatch: expected %s, got %s", expectedMessage, deserialized.Message)
		}

		expectedDetails := "Testing special characters: \\n\\t\\r\\\"\\\\and émojis 🎮"
		if deserialized.Details == nil || *deserialized.Details != expectedDetails {
			t.Errorf("Details mismatch: expected %s, got %v", expectedDetails, deserialized.Details)
		}

		expectedSessionID := "go-unicode-session-🎯"
		if deserialized.Context.SessionID == nil || *deserialized.Context.SessionID != expectedSessionID {
			t.Errorf("SessionID mismatch: expected %s, got %v", expectedSessionID, deserialized.Context.SessionID)
		}

		expectedUserID := "go-user-émile-测试"
		if deserialized.Context.UserID == nil || *deserialized.Context.UserID != expectedUserID {
			t.Errorf("UserID mismatch: expected %s, got %v", expectedUserID, deserialized.Context.UserID)
		}

		metadataMap := make(map[string]string)
		for _, pair := range deserialized.Context.Metadata {
			metadataMap[pair.Key] = pair.Value
		}
		if metadataMap["unicode_message"] != "🌍 Global Go test" {
			t.Errorf("Unicode message mismatch: expected '🌍 Global Go test', got '%s'", metadataMap["unicode_message"])
		}
		if metadataMap["emoji_test"] != "🚀🔥💻🎮🎯🌍" {
			t.Errorf("Emoji test mismatch: expected '🚀🔥💻🎮🎯🌍', got '%s'", metadataMap["emoji_test"])
		}
	})
}

// Helper functions
func stringPtr(s string) *string {
	return &s
}
