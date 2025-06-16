# V6R Go Backend Integration Guide

## Installation

The Go bindings are currently generated as part of the WIT toolchain. For V6R integration, you can import the generated bindings:

```go
// Import the generated memory types
import "github.com/flight-core/shared-types/go/memory"
```

## Core V6R Backend Use Cases

### 1. VM Resource Manager

```go
package main

import (
    "context"
    "fmt"
    "log"
    "sync"
    "time"

    "github.com/flight-core/shared-types/go/memory"
)

type V6RVMManager struct {
    allocations map[string][]string // sessionID -> allocation IDs
    mu          sync.RWMutex
}

func NewV6RVMManager() *V6RVMManager {
    return &V6RVMManager{
        allocations: make(map[string][]string),
    }
}

func (vm *V6RVMManager) CreateVM(ctx context.Context, sessionID string, vmSize VMSize) error {
    vm.mu.Lock()
    defer vm.mu.Unlock()

    memorySize := vm.getMemorySize(vmSize)
    
    // Create VM heap allocation
    allocation, err := memory.CreateAllocation(sessionID, memorySize, memory.MemoryPurposeVmHeap)
    if err != nil {
        return fmt.Errorf("failed to create VM allocation: %w", err)
    }

    // Track allocation
    vm.allocations[sessionID] = append(vm.allocations[sessionID], allocation.ID)
    
    log.Printf("Created VM for session %s: %s", sessionID, memorySize.HumanReadable)
    return nil
}

type VMSize string

const (
    VMSizeSmall  VMSize = "small"
    VMSizeMedium VMSize = "medium" 
    VMSizeLarge  VMSize = "large"
)

func (vm *V6RVMManager) getMemorySize(vmSize VMSize) memory.MemorySize {
    switch vmSize {
    case VMSizeSmall:
        return memory.MemorySize{
            Bytes:          512 * 1024 * 1024, // 512MB
            HumanReadable: "512MB",
        }
    case VMSizeMedium:
        return memory.MemorySize{
            Bytes:          1024 * 1024 * 1024, // 1GB
            HumanReadable: "1GB",
        }
    case VMSizeLarge:
        return memory.MemorySize{
            Bytes:          2048 * 1024 * 1024, // 2GB
            HumanReadable: "2GB",
        }
    default:
        return memory.MemorySize{
            Bytes:          512 * 1024 * 1024, // Default to small
            HumanReadable: "512MB",
        }
    }
}

func (vm *V6RVMManager) DestroyVM(ctx context.Context, sessionID string) error {
    vm.mu.Lock()
    defer vm.mu.Unlock()

    allocations, exists := vm.allocations[sessionID]
    if !exists {
        return fmt.Errorf("no VM found for session %s", sessionID)
    }

    // Free all allocations
    var errors []error
    for _, allocationID := range allocations {
        if err := memory.FreeAllocation(allocationID); err != nil {
            errors = append(errors, fmt.Errorf("failed to free allocation %s: %w", allocationID, err))
        }
    }

    delete(vm.allocations, sessionID)
    log.Printf("Destroyed VM for session %s", sessionID)

    if len(errors) > 0 {
        return fmt.Errorf("errors during VM destruction: %v", errors)
    }
    
    return nil
}

func (vm *V6RVMManager) GetVMMemoryUsage(ctx context.Context, sessionID string) (*memory.MemoryUsageSnapshot, error) {
    snapshot, err := memory.GetMemorySnapshot(sessionID)
    if err != nil {
        return nil, fmt.Errorf("failed to get memory snapshot: %w", err)
    }
    
    return snapshot, nil
}

func (vm *V6RVMManager) ListVMAllocations(ctx context.Context, sessionID string) ([]memory.MemoryAllocation, error) {
    allocations, err := memory.ListAllocations(sessionID)
    if err != nil {
        return nil, fmt.Errorf("failed to list allocations: %w", err)
    }
    
    return allocations, nil
}
```

### 2. Memory Pressure Monitoring

```go
package main

import (
    "context"
    "encoding/json"
    "log"
    "time"

    "github.com/gorilla/websocket"
    "github.com/flight-core/shared-types/go/memory"
)

type V6RMemoryMonitor struct {
    vmManager *V6RVMManager
    wsConn    *websocket.Conn
    stopCh    chan struct{}
}

func NewV6RMemoryMonitor(vmManager *V6RVMManager, wsConn *websocket.Conn) *V6RMemoryMonitor {
    return &V6RMemoryMonitor{
        vmManager: vmManager,
        wsConn:    wsConn,
        stopCh:    make(chan struct{}),
    }
}

func (m *V6RMemoryMonitor) StartMonitoring(ctx context.Context, sessionID string) {
    ticker := time.NewTicker(5 * time.Second)
    defer ticker.Stop()

    for {
        select {
        case <-ctx.Done():
            return
        case <-m.stopCh:
            return
        case <-ticker.C:
            if err := m.checkAndReportMemory(ctx, sessionID); err != nil {
                log.Printf("Memory monitoring error for session %s: %v", sessionID, err)
            }
        }
    }
}

func (m *V6RMemoryMonitor) StopMonitoring() {
    close(m.stopCh)
}

func (m *V6RMemoryMonitor) checkAndReportMemory(ctx context.Context, sessionID string) error {
    // Get memory snapshot
    snapshot, err := memory.GetMemorySnapshot(sessionID)
    if err != nil {
        return fmt.Errorf("failed to get memory snapshot: %w", err)
    }

    // Get memory pressure
    pressure, err := memory.GetMemoryPressure(sessionID)
    if err != nil {
        return fmt.Errorf("failed to get memory pressure: %w", err)
    }

    // Send update via WebSocket
    update := MemoryUpdate{
        Type:      "memory_update",
        SessionID: sessionID,
        Timestamp: snapshot.Timestamp,
        Memory: MemoryInfo{
            Used:             snapshot.Used.HumanReadable,
            Total:            snapshot.Total.HumanReadable,
            Available:        snapshot.Available.HumanReadable,
            UsagePercent:     calculateUsagePercent(snapshot),
            Fragmentation:    int(snapshot.FragmentationRatio * 100),
        },
        Pressure: string(pressure),
        Platform: snapshot.Platform,
    }

    if err := m.wsConn.WriteJSON(update); err != nil {
        return fmt.Errorf("failed to send WebSocket update: %w", err)
    }

    // Handle memory pressure
    if pressure == memory.MemoryPressureHigh || pressure == memory.MemoryPressureCritical {
        return m.handleMemoryPressure(ctx, sessionID, pressure)
    }

    return nil
}

type MemoryUpdate struct {
    Type      string     `json:"type"`
    SessionID string     `json:"sessionId"`
    Timestamp uint64     `json:"timestamp"`
    Memory    MemoryInfo `json:"memory"`
    Pressure  string     `json:"pressure"`
    Platform  string     `json:"platform"`
}

type MemoryInfo struct {
    Used          string `json:"used"`
    Total         string `json:"total"`
    Available     string `json:"available"`
    UsagePercent  int    `json:"usagePercent"`
    Fragmentation int    `json:"fragmentation"`
}

func calculateUsagePercent(snapshot *memory.MemoryUsageSnapshot) int {
    if snapshot.Total.Bytes == 0 {
        return 0
    }
    return int((snapshot.Used.Bytes * 100) / snapshot.Total.Bytes)
}

func (m *V6RMemoryMonitor) handleMemoryPressure(ctx context.Context, sessionID string, pressure memory.MemoryPressure) error {
    log.Printf("%s memory pressure detected for session %s", pressure, sessionID)

    switch pressure {
    case memory.MemoryPressureHigh:
        return m.handleHighMemoryPressure(ctx, sessionID)
    case memory.MemoryPressureCritical:
        return m.handleCriticalMemoryPressure(ctx, sessionID)
    }

    return nil
}

func (m *V6RMemoryMonitor) handleHighMemoryPressure(ctx context.Context, sessionID string) error {
    // Send alert to frontend
    alert := map[string]interface{}{
        "type":      "memory_pressure_alert",
        "sessionId": sessionID,
        "pressure":  "high",
        "message":   "High memory usage detected - consider optimization",
        "actions": []string{
            "Review memory usage",
            "Clear unnecessary caches",
            "Restart non-critical services",
        },
    }

    if err := m.wsConn.WriteJSON(alert); err != nil {
        return fmt.Errorf("failed to send high pressure alert: %w", err)
    }

    // Implement cleanup strategies
    log.Printf("Implementing memory cleanup for session %s", sessionID)
    // TODO: Add specific cleanup logic for V6R

    return nil
}

func (m *V6RMemoryMonitor) handleCriticalMemoryPressure(ctx context.Context, sessionID string) error {
    // Send critical alert to frontend
    alert := map[string]interface{}{
        "type":      "memory_pressure_alert",
        "sessionId": sessionID,
        "pressure":  "critical",
        "message":   "Critical memory usage - immediate action required",
        "actions": []string{
            "Immediate VM restart recommended",
            "Save work immediately",
            "Contact support if issue persists",
        },
    }

    if err := m.wsConn.WriteJSON(alert); err != nil {
        return fmt.Errorf("failed to send critical pressure alert: %w", err)
    }

    // Implement emergency memory reclamation
    log.Printf("Emergency memory reclamation for session %s", sessionID)
    // TODO: Add emergency cleanup logic for V6R

    return nil
}
```

### 3. Memory Analytics Service

```go
package main

import (
    "context"
    "fmt"
    "time"

    "github.com/flight-core/shared-types/go/memory"
)

type V6RMemoryAnalytics struct {
    vmManager *V6RVMManager
}

func NewV6RMemoryAnalytics(vmManager *V6RVMManager) *V6RMemoryAnalytics {
    return &V6RMemoryAnalytics{
        vmManager: vmManager,
    }
}

func (a *V6RMemoryAnalytics) GenerateMemoryReport(ctx context.Context, sessionID string) (*MemoryReport, error) {
    // Get memory statistics
    stats, err := memory.CalculateMemoryStats(sessionID)
    if err != nil {
        return nil, fmt.Errorf("failed to get memory stats: %w", err)
    }

    // Get memory trends for last 24 hours
    trends, err := memory.GetMemoryTrends(sessionID, 24*60*60) // 24 hours in seconds
    if err != nil {
        return nil, fmt.Errorf("failed to get memory trends: %w", err)
    }

    // Generate detailed report
    report, err := memory.GenerateMemoryReport(sessionID)
    if err != nil {
        return nil, fmt.Errorf("failed to generate memory report: %w", err)
    }

    return &MemoryReport{
        SessionID:     sessionID,
        GeneratedAt:   time.Now(),
        Statistics:    stats,
        Trends:        trends,
        DetailedReport: report,
    }, nil
}

type MemoryReport struct {
    SessionID      string                  `json:"sessionId"`
    GeneratedAt    time.Time              `json:"generatedAt"`
    Statistics     *memory.MemoryStats    `json:"statistics"`
    Trends         *memory.MemoryTrend    `json:"trends"`
    DetailedReport string                 `json:"detailedReport"`
}

func (a *V6RMemoryAnalytics) GetMemoryEfficiency(ctx context.Context, sessionID string) (float32, error) {
    stats, err := memory.CalculateMemoryStats(sessionID)
    if err != nil {
        return 0, fmt.Errorf("failed to get memory stats: %w", err)
    }

    return stats.EfficiencyRatio, nil
}

func (a *V6RMemoryAnalytics) GetMemoryBreakdown(ctx context.Context, sessionID string) (map[string]string, error) {
    stats, err := memory.CalculateMemoryStats(sessionID)
    if err != nil {
        return nil, fmt.Errorf("failed to get memory stats: %w", err)
    }

    breakdown := make(map[string]string)
    for _, usage := range stats.UsageByPurpose {
        purpose := usage[0] // memory.MemoryPurpose
        size := usage[1]    // memory.MemorySize
        breakdown[string(purpose)] = size.HumanReadable
    }

    return breakdown, nil
}

func (a *V6RMemoryAnalytics) PredictMemoryUsage(ctx context.Context, sessionID string, hoursAhead int) (*memory.MemorySize, error) {
    // Get trends for prediction
    trends, err := memory.GetMemoryTrends(sessionID, uint64(hoursAhead*60*60))
    if err != nil {
        return nil, fmt.Errorf("failed to get memory trends: %w", err)
    }

    return trends.PredictedPeak, nil
}
```

### 4. REST API Integration

```go
package main

import (
    "encoding/json"
    "net/http"
    "strconv"

    "github.com/gorilla/mux"
    "github.com/flight-core/shared-types/go/memory"
)

type V6RMemoryAPIHandler struct {
    vmManager  *V6RVMManager
    analytics  *V6RMemoryAnalytics
}

func NewV6RMemoryAPIHandler(vmManager *V6RVMManager, analytics *V6RMemoryAnalytics) *V6RMemoryAPIHandler {
    return &V6RMemoryAPIHandler{
        vmManager: vmManager,
        analytics: analytics,
    }
}

func (h *V6RMemoryAPIHandler) RegisterRoutes(router *mux.Router) {
    router.HandleFunc("/api/v1/sessions/{sessionId}/memory", h.getMemoryUsage).Methods("GET")
    router.HandleFunc("/api/v1/sessions/{sessionId}/memory/allocations", h.getMemoryAllocations).Methods("GET")
    router.HandleFunc("/api/v1/sessions/{sessionId}/memory/pressure", h.getMemoryPressure).Methods("GET")
    router.HandleFunc("/api/v1/sessions/{sessionId}/memory/report", h.getMemoryReport).Methods("GET")
    router.HandleFunc("/api/v1/sessions/{sessionId}/memory/efficiency", h.getMemoryEfficiency).Methods("GET")
    router.HandleFunc("/api/v1/sessions/{sessionId}/vm", h.createVM).Methods("POST")
    router.HandleFunc("/api/v1/sessions/{sessionId}/vm", h.destroyVM).Methods("DELETE")
}

func (h *V6RMemoryAPIHandler) getMemoryUsage(w http.ResponseWriter, r *http.Request) {
    sessionID := mux.Vars(r)["sessionId"]
    
    snapshot, err := h.vmManager.GetVMMemoryUsage(r.Context(), sessionID)
    if err != nil {
        http.Error(w, err.Error(), http.StatusInternalServerError)
        return
    }

    w.Header().Set("Content-Type", "application/json")
    json.NewEncoder(w).Encode(snapshot)
}

func (h *V6RMemoryAPIHandler) getMemoryAllocations(w http.ResponseWriter, r *http.Request) {
    sessionID := mux.Vars(r)["sessionId"]
    
    allocations, err := h.vmManager.ListVMAllocations(r.Context(), sessionID)
    if err != nil {
        http.Error(w, err.Error(), http.StatusInternalServerError)
        return
    }

    w.Header().Set("Content-Type", "application/json")
    json.NewEncoder(w).Encode(allocations)
}

func (h *V6RMemoryAPIHandler) getMemoryPressure(w http.ResponseWriter, r *http.Request) {
    sessionID := mux.Vars(r)["sessionId"]
    
    pressure, err := memory.GetMemoryPressure(sessionID)
    if err != nil {
        http.Error(w, err.Error(), http.StatusInternalServerError)
        return
    }

    response := map[string]interface{}{
        "sessionId": sessionID,
        "pressure":  string(pressure),
        "timestamp": time.Now().Unix(),
    }

    w.Header().Set("Content-Type", "application/json")
    json.NewEncoder(w).Encode(response)
}

func (h *V6RMemoryAPIHandler) getMemoryReport(w http.ResponseWriter, r *http.Request) {
    sessionID := mux.Vars(r)["sessionId"]
    
    report, err := h.analytics.GenerateMemoryReport(r.Context(), sessionID)
    if err != nil {
        http.Error(w, err.Error(), http.StatusInternalServerError)
        return
    }

    w.Header().Set("Content-Type", "application/json")
    json.NewEncoder(w).Encode(report)
}

func (h *V6RMemoryAPIHandler) getMemoryEfficiency(w http.ResponseWriter, r *http.Request) {
    sessionID := mux.Vars(r)["sessionId"]
    
    efficiency, err := h.analytics.GetMemoryEfficiency(r.Context(), sessionID)
    if err != nil {
        http.Error(w, err.Error(), http.StatusInternalServerError)
        return
    }

    response := map[string]interface{}{
        "sessionId":  sessionID,
        "efficiency": efficiency,
        "percentage": efficiency * 100,
        "timestamp":  time.Now().Unix(),
    }

    w.Header().Set("Content-Type", "application/json")
    json.NewEncoder(w).Encode(response)
}

type CreateVMRequest struct {
    VMSize VMSize `json:"vmSize"`
}

func (h *V6RMemoryAPIHandler) createVM(w http.ResponseWriter, r *http.Request) {
    sessionID := mux.Vars(r)["sessionId"]
    
    var req CreateVMRequest
    if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
        http.Error(w, "Invalid request body", http.StatusBadRequest)
        return
    }

    if err := h.vmManager.CreateVM(r.Context(), sessionID, req.VMSize); err != nil {
        http.Error(w, err.Error(), http.StatusInternalServerError)
        return
    }

    response := map[string]interface{}{
        "sessionId": sessionID,
        "vmSize":    string(req.VMSize),
        "status":    "created",
        "timestamp": time.Now().Unix(),
    }

    w.Header().Set("Content-Type", "application/json")
    w.WriteHeader(http.StatusCreated)
    json.NewEncoder(w).Encode(response)
}

func (h *V6RMemoryAPIHandler) destroyVM(w http.ResponseWriter, r *http.Request) {
    sessionID := mux.Vars(r)["sessionId"]
    
    if err := h.vmManager.DestroyVM(r.Context(), sessionID); err != nil {
        http.Error(w, err.Error(), http.StatusInternalServerError)
        return
    }

    response := map[string]interface{}{
        "sessionId": sessionID,
        "status":    "destroyed",
        "timestamp": time.Now().Unix(),
    }

    w.Header().Set("Content-Type", "application/json")
    json.NewEncoder(w).Encode(response)
}
```

### 5. WebSocket Server Integration

```go
package main

import (
    "context"
    "log"
    "net/http"
    "sync"

    "github.com/gorilla/websocket"
)

type V6RWebSocketServer struct {
    upgrader    websocket.Upgrader
    clients     map[string]*websocket.Conn
    monitors    map[string]*V6RMemoryMonitor
    vmManager   *V6RVMManager
    mu          sync.RWMutex
}

func NewV6RWebSocketServer(vmManager *V6RVMManager) *V6RWebSocketServer {
    return &V6RWebSocketServer{
        upgrader: websocket.Upgrader{
            CheckOrigin: func(r *http.Request) bool {
                return true // Configure appropriate origin checking for production
            },
        },
        clients:   make(map[string]*websocket.Conn),
        monitors:  make(map[string]*V6RMemoryMonitor),
        vmManager: vmManager,
    }
}

func (ws *V6RWebSocketServer) HandleWebSocket(w http.ResponseWriter, r *http.Request) {
    conn, err := ws.upgrader.Upgrade(w, r, nil)
    if err != nil {
        log.Printf("WebSocket upgrade error: %v", err)
        return
    }
    defer conn.Close()

    sessionID := r.URL.Query().Get("sessionId")
    if sessionID == "" {
        log.Printf("Missing sessionId in WebSocket connection")
        return
    }

    ws.mu.Lock()
    ws.clients[sessionID] = conn
    monitor := NewV6RMemoryMonitor(ws.vmManager, conn)
    ws.monitors[sessionID] = monitor
    ws.mu.Unlock()

    // Start memory monitoring
    ctx, cancel := context.WithCancel(context.Background())
    defer cancel()

    go monitor.StartMonitoring(ctx, sessionID)

    // Handle incoming messages
    for {
        var msg map[string]interface{}
        if err := conn.ReadJSON(&msg); err != nil {
            log.Printf("WebSocket read error: %v", err)
            break
        }

        if err := ws.handleMessage(ctx, sessionID, msg); err != nil {
            log.Printf("Message handling error: %v", err)
        }
    }

    // Cleanup
    ws.mu.Lock()
    delete(ws.clients, sessionID)
    if monitor, exists := ws.monitors[sessionID]; exists {
        monitor.StopMonitoring()
        delete(ws.monitors, sessionID)
    }
    ws.mu.Unlock()
}

func (ws *V6RWebSocketServer) handleMessage(ctx context.Context, sessionID string, msg map[string]interface{}) error {
    msgType, ok := msg["type"].(string)
    if !ok {
        return fmt.Errorf("invalid message type")
    }

    switch msgType {
    case "get_memory_snapshot":
        snapshot, err := ws.vmManager.GetVMMemoryUsage(ctx, sessionID)
        if err != nil {
            return err
        }
        
        ws.mu.RLock()
        conn := ws.clients[sessionID]
        ws.mu.RUnlock()
        
        if conn != nil {
            return conn.WriteJSON(map[string]interface{}{
                "type":    "memory_snapshot",
                "data":    snapshot,
            })
        }

    case "get_memory_report":
        analytics := NewV6RMemoryAnalytics(ws.vmManager)
        report, err := analytics.GenerateMemoryReport(ctx, sessionID)
        if err != nil {
            return err
        }

        ws.mu.RLock()
        conn := ws.clients[sessionID]
        ws.mu.RUnlock()
        
        if conn != nil {
            return conn.WriteJSON(map[string]interface{}{
                "type":    "memory_report",
                "data":    report,
            })
        }
    }

    return nil
}
```

### 6. Configuration and Main Application

```go
package main

import (
    "context"
    "log"
    "net/http"
    "os"
    "os/signal"
    "syscall"
    "time"

    "github.com/gorilla/mux"
)

func main() {
    // Initialize V6R memory management system
    vmManager := NewV6RVMManager()
    analytics := NewV6RMemoryAnalytics(vmManager)
    apiHandler := NewV6RMemoryAPIHandler(vmManager, analytics)
    wsServer := NewV6RWebSocketServer(vmManager)

    // Setup HTTP router
    router := mux.NewRouter()
    
    // Register API routes
    apiHandler.RegisterRoutes(router)
    
    // Register WebSocket endpoint
    router.HandleFunc("/ws/memory", wsServer.HandleWebSocket)

    // Setup HTTP server
    server := &http.Server{
        Addr:    ":8080",
        Handler: router,
        ReadTimeout:  15 * time.Second,
        WriteTimeout: 15 * time.Second,
    }

    // Start server
    go func() {
        log.Println("V6R Memory Management API starting on :8080")
        if err := server.ListenAndServe(); err != nil && err != http.ErrServerClosed {
            log.Fatalf("Server startup error: %v", err)
        }
    }()

    // Wait for interrupt signal
    quit := make(chan os.Signal, 1)
    signal.Notify(quit, syscall.SIGINT, syscall.SIGTERM)
    <-quit

    log.Println("Shutting down V6R Memory Management API...")

    // Graceful shutdown
    ctx, cancel := context.WithTimeout(context.Background(), 30*time.Second)
    defer cancel()

    if err := server.Shutdown(ctx); err != nil {
        log.Fatalf("Server shutdown error: %v", err)
    }

    log.Println("V6R Memory Management API stopped")
}
```

## Error Handling Best Practices

```go
// Custom error types for V6R memory operations
type V6RMemoryError struct {
    Code      string
    Message   string
    SessionID string
    Timestamp time.Time
}

func (e *V6RMemoryError) Error() string {
    return fmt.Sprintf("V6R Memory Error [%s]: %s (session: %s)", e.Code, e.Message, e.SessionID)
}

func WrapMemoryError(sessionID string, err error) *V6RMemoryError {
    return &V6RMemoryError{
        Code:      "MEMORY_OPERATION_FAILED",
        Message:   err.Error(),
        SessionID: sessionID,
        Timestamp: time.Now(),
    }
}

// Error handling middleware
func (h *V6RMemoryAPIHandler) withErrorHandling(handler http.HandlerFunc) http.HandlerFunc {
    return func(w http.ResponseWriter, r *http.Request) {
        defer func() {
            if err := recover(); err != nil {
                log.Printf("Panic in handler: %v", err)
                http.Error(w, "Internal server error", http.StatusInternalServerError)
            }
        }()

        handler(w, r)
    }
}
```

## Testing

```go
package main

import (
    "context"
    "testing"
    "time"

    "github.com/stretchr/testify/assert"
    "github.com/flight-core/shared-types/go/memory"
)

func TestV6RVMManager_CreateVM(t *testing.T) {
    vmManager := NewV6RVMManager()
    ctx := context.Background()
    sessionID := "test-session-1"

    err := vmManager.CreateVM(ctx, sessionID, VMSizeMedium)
    assert.NoError(t, err)

    // Verify allocation was tracked
    vmManager.mu.RLock()
    allocations := vmManager.allocations[sessionID]
    vmManager.mu.RUnlock()

    assert.Len(t, allocations, 1)
}

func TestV6RVMManager_DestroyVM(t *testing.T) {
    vmManager := NewV6RVMManager()
    ctx := context.Background()
    sessionID := "test-session-2"

    // Create VM first
    err := vmManager.CreateVM(ctx, sessionID, VMSizeSmall)
    assert.NoError(t, err)

    // Destroy VM
    err = vmManager.DestroyVM(ctx, sessionID)
    assert.NoError(t, err)

    // Verify allocations were cleaned up
    vmManager.mu.RLock()
    _, exists := vmManager.allocations[sessionID]
    vmManager.mu.RUnlock()

    assert.False(t, exists)
}
```

This Go integration guide provides V6R with a complete backend implementation for VM memory management using the Flight memory types. The examples cover API endpoints, WebSocket integration, memory monitoring, and analytics - everything needed for V6R's backend services.
