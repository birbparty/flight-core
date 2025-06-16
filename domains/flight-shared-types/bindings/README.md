# Flight Memory Types - Enhanced Bindings

This directory contains enhanced, production-ready bindings for Flight memory types that provide universal memory management capabilities for any application.

## Overview

The enhanced bindings offer significant improvements over the basic WIT-generated types:

- **Enhanced Type Safety**: Readonly interfaces, strict null checks, and comprehensive error handling
- **Utility Functions**: Memory formatting, calculations, platform compatibility checks
- **Real-time Features**: WebSocket integration, event emitters, and live monitoring
- **Framework Integration**: React hooks, Go monitoring utilities, context management
- **Cross-Platform**: Support for all Flight platforms (Dreamcast 16MB → V6R Large 2GB+)

## Available Languages

### TypeScript Enhanced (`typescript/enhanced/`)
- **Package**: `@flight/memory-enhanced`
- **Features**: React hooks, WebSocket utilities, event system
- **Use Cases**: Web applications, React Native, Node.js servers
- **Dependencies**: Optional React peer dependency

### Go Enhanced (`go/enhanced/`)
- **Module**: `github.com/flight-core/shared-types/go/enhanced`
- **Features**: Memory monitoring, event emitters, WebSocket ready
- **Use Cases**: Backend services, APIs, CLI applications
- **Dependencies**: Gorilla WebSocket (optional)

## Quick Start

### TypeScript

```typescript
import { 
  MemoryUtils, 
  useMemoryMonitor, 
  MemoryEventEmitter 
} from '@flight/memory-enhanced';

// Basic memory utilities
const memorySize = MemoryUtils.createMemorySize(BigInt(1024 * 1024)); // 1MB
const formatted = MemoryUtils.formatBytesForUI(BigInt(1048576)); // "1.0MB"

// React hook for memory monitoring
function MemoryDashboard({ sessionId }: { sessionId: string }) {
  const { 
    snapshot, 
    loading, 
    usagePercentage, 
    pressureColor 
  } = useMemoryMonitor(sessionId, {
    refreshInterval: 5000,
    enableRealtime: true
  });

  if (loading) return <div>Loading...</div>;

  return (
    <div>
      <h3>Memory Usage: {usagePercentage.toFixed(1)}%</h3>
      <div style={{ color: pressureColor }}>
        {snapshot?.used.humanReadable} / {snapshot?.total.humanReadable}
      </div>
    </div>
  );
}

// Event system for real-time updates
const emitter = new MemoryEventEmitter();
const unsubscribe = emitter.subscribe(sessionId, (update) => {
  console.log('Memory update:', update.snapshot);
});
```

### Go

```go
package main

import (
    "fmt"
    "time"
    
    memory "github.com/flight-core/shared-types/go/enhanced"
)

func main() {
    // Basic memory utilities
    utils := memory.MemoryUtils{}
    
    memorySize := memory.NewMemorySize(1024 * 1024) // 1MB
    fmt.Println("Memory size:", memorySize.HumanReadable)
    
    // Memory event emitter
    emitter := memory.NewMemoryEventEmitter()
    
    unsubscribe := emitter.Subscribe("session-1", func(data interface{}) {
        if update, ok := data.(memory.MemoryUpdate); ok {
            summary := utils.GenerateMemorySummary(update.Snapshot)
            fmt.Println("Memory update:", summary)
        }
    })
    defer unsubscribe()
    
    // Memory monitoring
    monitor := memory.NewMemoryMonitor(func(sessionID string) (memory.MemoryUsageSnapshot, error) {
        // Your implementation to fetch memory snapshot
        return memory.MemoryUsageSnapshot{}, nil
    })
    
    // Start monitoring every 5 seconds
    monitor.StartMonitoring("session-1", 5*time.Second)
    defer monitor.Stop()
}
```

## Key Features

### Memory Utilities

Both TypeScript and Go provide comprehensive utility functions:

```typescript
// TypeScript
MemoryUtils.formatBytesForUI(bytes)           // Format for display
MemoryUtils.parseMemorySize("512MB")          // Parse human-readable sizes  
MemoryUtils.calculateUsagePercentage(snapshot) // Calculate usage %
MemoryUtils.getMemoryPressureColor(pressure)  // Get UI colors
MemoryUtils.isPlatformCompatible(platform, size) // Check compatibility
```

```go
// Go
utils.FormatBytes(bytes)                    // Format for display
utils.ParseMemorySize("512MB")              // Parse human-readable sizes
utils.CalculateUsagePercentage(snapshot)    // Calculate usage %  
utils.GetMemoryPressureLevel(percentage)   // Determine pressure level
utils.IsPlatformCompatible(platform, size) // Check compatibility
```

### React Integration

The TypeScript package provides ready-to-use React hooks:

- `useMemoryMonitor()` - Monitor memory usage with automatic refresh
- `useMemoryWebSocket()` - WebSocket connection management
- `useSessionConfig()` - Session configuration validation
- `useMemoryStats()` - Memory analytics and statistics
- `useMemoryAllocations()` - List and track allocations

### Event System

Both packages include event emitters for real-time memory updates:

- Session-specific subscriptions
- Global event listeners  
- Automatic error handling
- Thread-safe operations (Go)
- Memory leak prevention

### Platform Support

Full support for all Flight memory platforms:

| Platform | Memory Limit | Description |
|----------|-------------|-------------|
| `dreamcast` | 16MB | Sega Dreamcast baseline |
| `psp` | 64MB | PlayStation Portable |
| `vita` | 512MB | PlayStation Vita |
| `v6r-small` | 512MB | Small VM instances |
| `v6r-medium` | 1GB | Medium VM instances |
| `v6r-large` | 2GB+ | Large VM instances |
| `custom` | Variable | Custom platform definitions |

## Integration Examples

### Web Application (TypeScript + React)

```typescript
import { useMemoryMonitor, useMemoryWebSocket } from '@flight/memory-enhanced';

function App() {
  const { snapshot, pressureColor } = useMemoryMonitor(sessionId);
  const { connected } = useMemoryWebSocket('ws://localhost:8080/memory', sessionId);
  
  return (
    <div>
      <div>WebSocket: {connected ? '🟢' : '🔴'}</div>
      <div style={{ color: pressureColor }}>
        Memory: {snapshot?.used.humanReadable} / {snapshot?.total.humanReadable}
      </div>
    </div>
  );
}
```

### Backend Service (Go)

```go
func startMemoryService() {
    monitor := memory.NewMemoryMonitor(fetchMemorySnapshot)
    
    // Monitor multiple sessions
    monitor.StartMonitoring("user-1", 5*time.Second)
    monitor.StartMonitoring("user-2", 5*time.Second)
    
    // Global memory pressure alerts
    monitor.SubscribeGlobal(func(data interface{}) {
        if update, ok := data.(memory.MemoryUpdate); ok {
            utils := memory.MemoryUtils{}
            percentage := utils.CalculateUsagePercentage(update.Snapshot)
            
            if percentage > 90 {
                sendAlert(update.SessionID, "High memory usage detected")
            }
        }
    })
}
```

## Architecture

The enhanced bindings are designed with these principles:

1. **Universal Compatibility**: No platform-specific branding, works everywhere
2. **Production Ready**: Comprehensive error handling, type safety, performance optimized  
3. **Framework Agnostic**: Core utilities work with any framework
4. **Real-time Capable**: Built-in event systems and monitoring
5. **Cross-Language Consistent**: Similar APIs across TypeScript and Go

## Migration from V6R-Specific Bindings

If you're currently using the V6R-specific bindings (`bindings/typescript/v6r-memory/` or `bindings/go/v6r-memory/`), migration is straightforward:

### TypeScript Migration

```typescript
// Old V6R-specific import
import { V6RMemoryUtils } from './v6r-memory';

// New generic import  
import { MemoryUtils } from '@flight/memory-enhanced';

// API remains the same, just remove V6R prefix
const size = MemoryUtils.createMemorySize(bytes); // Instead of V6RMemoryUtils
```

### Go Migration

```go
// Old V6R-specific types
v6rConfig := V6RMemoryConfig{...}

// New generic types
config := SessionConfig{...}

// Functionality is the same, just with generic naming
```

## Performance

The enhanced bindings are optimized for production use:

- **TypeScript**: Zero runtime overhead for type utilities
- **Go**: Concurrent-safe with minimal allocations
- **Memory Monitoring**: Configurable intervals, automatic cleanup
- **Event System**: Non-blocking event dispatch, error isolation

## License

MIT License - See LICENSE file for details.
