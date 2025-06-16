// Generic Memory Utilities - Universal Flight Memory Operations
// Production-ready utilities for any Go application using Flight memory types

package memory

import (
	"context"
	"fmt"
	"log"
	"regexp"
	"strconv"
	"strings"
	"sync"
	"time"
)

// MemoryUtils provides utility functions for memory management
type MemoryUtils struct{}

// FormatBytes formats bytes for API responses and UI display
func FormatBytes(bytes uint64) string {
	units := []string{"B", "KB", "MB", "GB", "TB"}
	size := float64(bytes)
	unitIndex := 0

	for size >= 1024 && unitIndex < len(units)-1 {
		size /= 1024
		unitIndex++
	}

	if unitIndex == 0 {
		return fmt.Sprintf("%dB", bytes)
	}
	return fmt.Sprintf("%.1f%s", size, units[unitIndex])
}

// CreateMemorySize creates a MemorySize from bytes
func (MemoryUtils) CreateMemorySize(bytes uint64) MemorySize {
	return MemorySize{
		Bytes:         bytes,
		HumanReadable: FormatBytes(bytes),
	}
}

// CalculateUsagePercentage calculates memory usage percentage
func (MemoryUtils) CalculateUsagePercentage(snapshot MemoryUsageSnapshot) float64 {
	if snapshot.Total.Bytes == 0 {
		return 0
	}
	return float64(snapshot.Used.Bytes) / float64(snapshot.Total.Bytes) * 100
}

// GetMemoryPressureLevel determines memory pressure from percentage
func (MemoryUtils) GetMemoryPressureLevel(percentage float64) MemoryPressure {
	switch {
	case percentage >= 95:
		return MemoryPressureCritical
	case percentage >= 85:
		return MemoryPressureHigh
	case percentage >= 70:
		return MemoryPressureMedium
	default:
		return MemoryPressureLow
	}
}

// CreateMemoryUpdate creates a memory update message
func (MemoryUtils) CreateMemoryUpdate(sessionID string, snapshot MemoryUsageSnapshot) MemoryUpdate {
	return MemoryUpdate{
		Type:      "memory_update",
		SessionID: sessionID,
		Snapshot:  snapshot,
		Timestamp: uint64(time.Now().Unix()),
	}
}

// ValidateSessionConfig validates session configuration
func (MemoryUtils) ValidateSessionConfig(config SessionConfig) error {
	if config.SessionID == "" {
		return fmt.Errorf("session_id cannot be empty")
	}
	if config.UserID == "" {
		return fmt.Errorf("user_id cannot be empty")
	}
	if config.Platform == "" {
		return fmt.Errorf("platform cannot be empty")
	}
	return nil
}

// GetPlatformDisplayName returns human-readable platform name
func (MemoryUtils) GetPlatformDisplayName(platform string) string {
	platformNames := map[string]string{
		"dreamcast":  "Sega Dreamcast",
		"psp":        "PlayStation Portable",
		"vita":       "PlayStation Vita",
		"v6r-small":  "Small VM (512MB)",
		"v6r-medium": "Medium VM (1GB)",
		"v6r-large":  "Large VM (2GB+)",
		"custom":     "Custom Platform",
	}

	if name, exists := platformNames[platform]; exists {
		return name
	}
	return platform
}

// ParseMemorySize parses a human-readable memory size string
func (MemoryUtils) ParseMemorySize(humanReadable string) (uint64, error) {
	// Regex to match patterns like "512MB", "1GB", "16KB", etc.
	re := regexp.MustCompile(`^(\d+(?:\.\d+)?)\s*([KMGT]?B)$`)
	matches := re.FindStringSubmatch(strings.ToUpper(strings.TrimSpace(humanReadable)))

	if len(matches) != 3 {
		return 0, fmt.Errorf("invalid memory size format: %s", humanReadable)
	}

	// Parse numeric part
	numStr := matches[1]
	unit := matches[2]

	num, err := strconv.ParseFloat(numStr, 64)
	if err != nil {
		return 0, fmt.Errorf("invalid numeric value: %s", numStr)
	}

	// Define multipliers
	multipliers := map[string]uint64{
		"B":  1,
		"KB": 1024,
		"MB": 1024 * 1024,
		"GB": 1024 * 1024 * 1024,
		"TB": 1024 * 1024 * 1024 * 1024,
	}

	multiplier, exists := multipliers[unit]
	if !exists {
		return 0, fmt.Errorf("unknown unit: %s", unit)
	}

	return uint64(num * float64(multiplier)), nil
}

// IsApproachingLimit checks if memory usage is approaching limits
func (MemoryUtils) IsApproachingLimit(current, limit MemorySize, thresholdPercent float64) bool {
	if limit.Bytes == 0 {
		return false
	}
	percentage := float64(current.Bytes) / float64(limit.Bytes) * 100
	return percentage >= thresholdPercent
}

// CalculateAvailableMemory calculates available memory
func (MemoryUtils) CalculateAvailableMemory(total, used MemorySize) MemorySize {
	var availableBytes uint64
	if total.Bytes > used.Bytes {
		availableBytes = total.Bytes - used.Bytes
	} else {
		availableBytes = 0
	}
	return NewMemorySize(availableBytes)
}

// CompareMemorySize compares two memory sizes
func (MemoryUtils) CompareMemorySize(a, b MemorySize) int {
	if a.Bytes < b.Bytes {
		return -1
	}
	if a.Bytes > b.Bytes {
		return 1
	}
	return 0
}

// AddMemorySize adds two memory sizes
func (MemoryUtils) AddMemorySize(a, b MemorySize) MemorySize {
	return NewMemorySize(a.Bytes + b.Bytes)
}

// SubtractMemorySize subtracts two memory sizes
func (MemoryUtils) SubtractMemorySize(a, b MemorySize) MemorySize {
	if a.Bytes <= b.Bytes {
		return NewMemorySize(0)
	}
	return NewMemorySize(a.Bytes - b.Bytes)
}

// CalculateEfficiency calculates memory efficiency ratio
func (MemoryUtils) CalculateEfficiency(used, allocated MemorySize) float32 {
	if allocated.Bytes == 0 {
		return 1.0
	}
	return float32(used.Bytes) / float32(allocated.Bytes)
}

// IsPlatformCompatible checks if platform supports specific memory size
func (MemoryUtils) IsPlatformCompatible(platform string, requiredMemory MemorySize) bool {
	platformLimits := map[string]uint64{
		"dreamcast":  16 * 1024 * 1024,       // 16MB
		"psp":        64 * 1024 * 1024,       // 64MB
		"vita":       512 * 1024 * 1024,      // 512MB
		"v6r-small":  512 * 1024 * 1024,      // 512MB
		"v6r-medium": 1024 * 1024 * 1024,     // 1GB
		"v6r-large":  2 * 1024 * 1024 * 1024, // 2GB
	}

	limit, exists := platformLimits[platform]
	if !exists {
		return true // Default to true for custom platforms
	}
	return requiredMemory.Bytes <= limit
}

// GenerateMemorySummary creates a text summary of memory usage
func (MemoryUtils) GenerateMemorySummary(snapshot MemoryUsageSnapshot) string {
	utils := MemoryUtils{}
	percentage := utils.CalculateUsagePercentage(snapshot)
	pressure := utils.GetMemoryPressureLevel(percentage)
	platformName := utils.GetPlatformDisplayName(snapshot.Platform)

	return fmt.Sprintf("%s: %s / %s (%.1f%%) - %s pressure",
		platformName,
		snapshot.Used.HumanReadable,
		snapshot.Total.HumanReadable,
		percentage,
		strings.ToUpper(string(pressure)))
}

// MemoryEventEmitter for real-time updates
// Generic implementation suitable for any Go application
type MemoryEventEmitter struct {
	mu              sync.RWMutex
	listeners       map[string]map[string]MemoryEventHandler // sessionID -> listenerID -> handler
	globalListeners map[string]MemoryEventHandler            // listenerID -> handler
	nextListenerID  uint64
}

// NewMemoryEventEmitter creates a new event emitter
func NewMemoryEventEmitter() *MemoryEventEmitter {
	return &MemoryEventEmitter{
		listeners:       make(map[string]map[string]MemoryEventHandler),
		globalListeners: make(map[string]MemoryEventHandler),
		nextListenerID:  1,
	}
}

// Subscribe to memory updates for a specific session
func (e *MemoryEventEmitter) Subscribe(sessionID string, handler MemoryEventHandler) (unsubscribe func()) {
	e.mu.Lock()
	defer e.mu.Unlock()

	listenerID := fmt.Sprintf("listener_%d", e.nextListenerID)
	e.nextListenerID++

	if e.listeners[sessionID] == nil {
		e.listeners[sessionID] = make(map[string]MemoryEventHandler)
	}
	e.listeners[sessionID][listenerID] = handler

	return func() {
		e.mu.Lock()
		defer e.mu.Unlock()
		if sessionListeners, exists := e.listeners[sessionID]; exists {
			delete(sessionListeners, listenerID)
			if len(sessionListeners) == 0 {
				delete(e.listeners, sessionID)
			}
		}
	}
}

// SubscribeGlobal subscribes to all memory updates
func (e *MemoryEventEmitter) SubscribeGlobal(handler MemoryEventHandler) (unsubscribe func()) {
	e.mu.Lock()
	defer e.mu.Unlock()

	listenerID := fmt.Sprintf("global_listener_%d", e.nextListenerID)
	e.nextListenerID++

	e.globalListeners[listenerID] = handler

	return func() {
		e.mu.Lock()
		defer e.mu.Unlock()
		delete(e.globalListeners, listenerID)
	}
}

// Emit memory update to subscribers
func (e *MemoryEventEmitter) Emit(update MemoryUpdate) {
	e.mu.RLock()
	defer e.mu.RUnlock()

	// Emit to session-specific listeners
	if sessionListeners, exists := e.listeners[update.SessionID]; exists {
		for _, handler := range sessionListeners {
			go func(h MemoryEventHandler) {
				defer func() {
					if r := recover(); r != nil {
						log.Printf("Error in memory update handler: %v", r)
					}
				}()
				h(update)
			}(handler)
		}
	}

	// Emit to global listeners
	for _, handler := range e.globalListeners {
		go func(h MemoryEventHandler) {
			defer func() {
				if r := recover(); r != nil {
					log.Printf("Error in global memory update handler: %v", r)
				}
			}()
			h(update)
		}(handler)
	}
}

// GetSubscriberCount returns the number of subscribers for a session
func (e *MemoryEventEmitter) GetSubscriberCount(sessionID string) int {
	e.mu.RLock()
	defer e.mu.RUnlock()

	if sessionListeners, exists := e.listeners[sessionID]; exists {
		return len(sessionListeners)
	}
	return 0
}

// GetTotalSubscriberCount returns total subscriber count including global
func (e *MemoryEventEmitter) GetTotalSubscriberCount() int {
	e.mu.RLock()
	defer e.mu.RUnlock()

	total := len(e.globalListeners)
	for _, sessionListeners := range e.listeners {
		total += len(sessionListeners)
	}
	return total
}

// Clear removes all listeners
func (e *MemoryEventEmitter) Clear() {
	e.mu.Lock()
	defer e.mu.Unlock()

	e.listeners = make(map[string]map[string]MemoryEventHandler)
	e.globalListeners = make(map[string]MemoryEventHandler)
}

// GetActiveSessions returns all session IDs with active listeners
func (e *MemoryEventEmitter) GetActiveSessions() []string {
	e.mu.RLock()
	defer e.mu.RUnlock()

	sessions := make([]string, 0, len(e.listeners))
	for sessionID := range e.listeners {
		sessions = append(sessions, sessionID)
	}
	return sessions
}

// MemoryMonitor provides periodic memory monitoring capabilities
type MemoryMonitor struct {
	emitter      *MemoryEventEmitter
	sessions     map[string]*MonitoredSession
	mu           sync.RWMutex
	ctx          context.Context
	cancel       context.CancelFunc
	fetchHandler func(sessionID string) (MemoryUsageSnapshot, error)
}

type MonitoredSession struct {
	SessionID string
	Interval  time.Duration
	LastFetch time.Time
	ticker    *time.Ticker
	stopChan  chan struct{}
}

// NewMemoryMonitor creates a new memory monitor
func NewMemoryMonitor(fetchHandler func(sessionID string) (MemoryUsageSnapshot, error)) *MemoryMonitor {
	ctx, cancel := context.WithCancel(context.Background())
	return &MemoryMonitor{
		emitter:      NewMemoryEventEmitter(),
		sessions:     make(map[string]*MonitoredSession),
		ctx:          ctx,
		cancel:       cancel,
		fetchHandler: fetchHandler,
	}
}

// StartMonitoring starts monitoring a session
func (m *MemoryMonitor) StartMonitoring(sessionID string, interval time.Duration) error {
	m.mu.Lock()
	defer m.mu.Unlock()

	if _, exists := m.sessions[sessionID]; exists {
		return fmt.Errorf("session %s is already being monitored", sessionID)
	}

	session := &MonitoredSession{
		SessionID: sessionID,
		Interval:  interval,
		LastFetch: time.Now(),
		ticker:    time.NewTicker(interval),
		stopChan:  make(chan struct{}),
	}

	m.sessions[sessionID] = session

	// Start monitoring goroutine
	go m.monitorSession(session)

	return nil
}

// StopMonitoring stops monitoring a session
func (m *MemoryMonitor) StopMonitoring(sessionID string) {
	m.mu.Lock()
	defer m.mu.Unlock()

	if session, exists := m.sessions[sessionID]; exists {
		session.ticker.Stop()
		close(session.stopChan)
		delete(m.sessions, sessionID)
	}
}

// Subscribe to memory updates
func (m *MemoryMonitor) Subscribe(sessionID string, handler MemoryEventHandler) func() {
	return m.emitter.Subscribe(sessionID, handler)
}

// SubscribeGlobal to all memory updates
func (m *MemoryMonitor) SubscribeGlobal(handler MemoryEventHandler) func() {
	return m.emitter.SubscribeGlobal(handler)
}

// Stop stops all monitoring
func (m *MemoryMonitor) Stop() {
	m.cancel()

	m.mu.Lock()
	defer m.mu.Unlock()

	for sessionID := range m.sessions {
		m.StopMonitoring(sessionID)
	}
}

func (m *MemoryMonitor) monitorSession(session *MonitoredSession) {
	defer session.ticker.Stop()

	for {
		select {
		case <-m.ctx.Done():
			return
		case <-session.stopChan:
			return
		case <-session.ticker.C:
			if snapshot, err := m.fetchHandler(session.SessionID); err == nil {
				session.LastFetch = time.Now()
				utils := MemoryUtils{}
				update := utils.CreateMemoryUpdate(session.SessionID, snapshot)
				m.emitter.Emit(update)
			} else {
				log.Printf("Failed to fetch memory snapshot for session %s: %v", session.SessionID, err)
			}
		}
	}
}
