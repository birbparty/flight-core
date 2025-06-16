# Flight Session Types Integration Guide

## Overview

Flight Session Types provide universal session management for Flight-Core multi-platform development. This vendor-neutral system enables consistent session tracking from Dreamcast (16MB) to modern cloud environments with extensible metadata for third-party integrations.

## Key Features

- **Universal Session Management**: Consistent session handling across all Flight-Core platforms
- **Vendor-Neutral Design**: No dependencies on private companies or proprietary services
- **Resource Monitoring**: Real-time tracking of memory, CPU, network, and storage usage
- **State Management**: Complete session lifecycle with validated state transitions
- **Platform Adaptation**: Automatic configuration based on platform constraints
- **Type Safety**: Full type safety across TypeScript, Go, Rust, and C++17
- **React Integration**: Ready-to-use React hooks for frontend development

## Architecture

### Core Components

1. **Session Types** (`wit/session.wit`)
   - WIT interface definitions for universal compatibility
   - Session lifecycle states and types
   - Resource usage and limits structures
   - Event tracking system

2. **Validation Layer** (`validation/session/`)
   - Rust-based validation for session operations
   - State transition validation
   - Resource limit enforcement
   - Configuration validation

3. **Language Bindings**
   - **TypeScript** (`bindings/typescript/session/`)
   - **Go** (`bindings/go/session/`)
   - **Rust** (via validation module)

4. **Utilities and Hooks**
   - Session management utilities
   - React hooks for frontend integration
   - Formatters and validators

## Quick Start

### TypeScript Integration

```typescript
import {
  SessionManager,
  SessionType,
  SessionState,
  useSession,
  useSessionResources
} from '@flight/session-types';

// Create a session
const sessionId = SessionManager.generateSessionId(
  SessionType.Development,
  'cloud'
);

// Use React hooks
function SessionMonitor({ sessionId }: { sessionId: string }) {
  const { session, loading, error } = useSession(sessionId, operations);
  const { resources, health } = useSessionResources(sessionId, operations);

  if (loading) return <div>Loading...</div>;
  if (error) return <div>Error: {error}</div>;

  return (
    <div>
      <h3>Session: {session?.id}</h3>
      <p>State: {session?.state}</p>
      <p>Health: {health}</p>
      <p>Memory: {resources?.memory.used.humanReadable}</p>
    </div>
  );
}
```

### Go Integration

```go
package main

import (
    "fmt"
    "github.com/flight-core/shared-types/session"
)

func main() {
    // Create session configuration
    config := session.NewSessionConfig()
    config.Environment = []session.MetadataEntry{
        {Key: "NODE_ENV", Value: "production"},
        {Key: "PORT", Value: "3000"},
    }
    
    // Create session info
    sessionInfo := session.NewSessionInfo(
        "my-session-123",
        session.SessionTypeDevelopment,
        "cloud"
    )
    
    // Validate state transition
    canTransition := session.ValidateStateTransition(
        session.SessionStateInitializing,
        session.SessionStateActive
    )
    
    fmt.Printf("Can transition: %v\n", canTransition)
    fmt.Printf("Session ID: %s\n", sessionInfo.ID)
}
```

## Session Lifecycle

### States

1. **Initializing** - Session being created
2. **Active** - Session operational
3. **Suspended** - Session temporarily paused
4. **Terminating** - Session shutting down
5. **Terminated** - Session ended
6. **Error** - Session in error state

### Valid Transitions

```
Initializing → Active, Error, Terminated
Active → Suspended, Terminating, Error
Suspended → Active, Terminating, Error  
Terminating → Terminated, Error
Error → Active, Terminating, Terminated
Terminated → (terminal state)
```

### State Validation

```typescript
// Validate transitions
const isValid = SessionManager.validateStateTransition(
  SessionState.Active,
  SessionState.Suspended
); // true

const nextStates = SessionManager.getValidNextStates(
  SessionState.Active
); // [Suspended, Terminating, Error]
```

## Resource Management

### Platform-Specific Limits

Different platforms have different resource constraints:

| Platform | Memory | CPU | Connections | Timeout |
|----------|--------|-----|-------------|---------|
| Dreamcast | 16MB | 95% | 1 | 1 hour |
| PSP | 32MB | 90% | 4 | 2 hours |
| Cloud | 1GB | 80% | 100 | 8 hours |

```typescript
// Auto-configure for platform
const limits = SessionManager.createPlatformResourceLimits('dreamcast');
console.log(limits.maxMemory?.humanReadable); // "16MB"
```

### Resource Monitoring

```typescript
// Monitor resource usage
const resources = await operations.getSessionResources(sessionId);
const health = SessionManager.calculateSessionHealth(resources, limits);

if (health === SessionHealth.Critical) {
  console.warn('Session experiencing critical resource issues');
}
```

## Session Configuration

### Environment Variables

```typescript
const config: SessionConfig = {
  environment: [
    ['NODE_ENV', 'development'],
    ['PORT', '3000'],
    ['DATABASE_URL', 'postgresql://...']
  ],
  workingDirectory: '/app',
  customConfig: [
    ['theme', 'dark'],
    ['language', 'en']
  ]
};

// Validate configuration
const errors = SessionValidator.validateSessionConfig(config);
if (errors.length > 0) {
  console.error('Configuration errors:', errors);
}
```

### Resource Limits

```typescript
const limits: ResourceLimits = {
  maxMemory: { bytes: 512n * 1024n * 1024n, humanReadable: '512MB' },
  maxCpuPercent: 80.0,
  maxNetworkBps: 1000000, // 1 MB/s
  maxConnections: 50,
  timeoutSeconds: 3600, // 1 hour
  customLimits: [
    ['gpu_memory', 256.0]
  ]
};
```

## Event Tracking

### Recording Events

```typescript
// Record session events
await operations.recordSessionEvent(
  sessionId,
  SessionEventType.HealthChanged,
  'Session health degraded due to high memory usage',
  [
    ['memory_usage', '85%'],
    ['threshold', '80%']
  ]
);

// Get event history
const events = await operations.getSessionEvents(sessionId, 10);
```

### Event Types

- `created` - Session was created
- `started` - Session became active  
- `suspended` - Session was suspended
- `resumed` - Session resumed from suspension
- `terminated` - Session ended
- `error-occurred` - Error happened
- `resource-limit-exceeded` - Resource limit hit
- `health-changed` - Health status changed
- `custom` - Custom application event

## Analytics and Monitoring

### Session Statistics

```typescript
const stats = await analytics.getSessionStats();
console.log(`Active sessions: ${stats.activeSessions}`);
console.log(`Average duration: ${stats.averageDuration}s`);
console.log(`System health: ${stats.systemHealth}`);
```

### Resource Aggregation

```typescript
const aggregate = await analytics.getResourceUsageAggregate();
console.log(`Total memory: ${aggregate.totalMemoryUsage.humanReadable}`);
console.log(`Average CPU: ${aggregate.averageCpuUsage}%`);
```

### Health Monitoring

```typescript
// Get sessions by health status
const criticalSessions = await analytics.getSessionsByHealth(
  SessionHealth.Critical
);

// Calculate efficiency
const efficiency = await analytics.calculateSessionEfficiency(sessionId);
console.log(`Session efficiency: ${efficiency * 100}%`);
```

## React Hooks Reference

### useSession

Monitor a single session's state and lifecycle:

```typescript
const {
  session,      // Current session info
  loading,      // Loading state
  error,        // Error message
  refresh,      // Manual refresh function
  updateState,  // Update session state
  terminate,    // Terminate session
  extend,       // Extend session timeout
  isExpired,    // Whether session expired
  duration      // Session duration in seconds
} = useSession(sessionId, operations);
```

### useSessionResources

Monitor session resource usage:

```typescript
const {
  resources,    // Current resource usage
  health,       // Session health status
  loading,      // Loading state
  error,        // Error message
  refresh       // Manual refresh function
} = useSessionResources(sessionId, operations, 5000); // 5s refresh
```

### useSessions

Manage multiple sessions:

```typescript
const {
  sessions,         // All sessions
  loading,          // Loading state
  error,            // Error message
  refresh,          // Refresh sessions
  createSession,    // Create new session
  activeSessions,   // Only active sessions
  expiredSessions   // Only expired sessions
} = useSessions(operations, filterCriteria, 10000); // 10s refresh
```

### useSessionAnalytics

Monitor system-wide session analytics:

```typescript
const {
  stats,      // Session statistics
  loading,    // Loading state
  error,      // Error message
  refresh     // Refresh stats
} = useSessionAnalytics(analytics, 30000); // 30s refresh
```

## Validation and Error Handling

### Configuration Validation

```typescript
// Validate session configuration
const errors = SessionValidator.validateSessionConfig(config);
errors.forEach(error => console.error(error));

// Validate resource limits
const limitErrors = SessionValidator.validateResourceLimits(limits);
limitErrors.forEach(error => console.error(error));
```

### Error Handling

All operations return `FlightResult<T>` for consistent error handling:

```typescript
const result = await operations.createSession(
  SessionType.Development,
  'cloud',
  'user123',
  config
);

if (result.tag === 'ok') {
  console.log('Session created:', result.val.id);
} else {
  console.error('Failed to create session:', result.val.message);
}
```

## Performance Considerations

### Efficient Session Management

1. **Batch Operations**: Use batch APIs when possible
2. **Polling Intervals**: Adjust refresh intervals based on need
3. **Resource Cleanup**: Always terminate unused sessions
4. **Memory Monitoring**: Monitor fragmentation on constrained platforms

### Platform Optimization

```typescript
// Auto-configure based on platform capabilities
const config = SessionManager.createDefaultConfig(platform);
const limits = SessionManager.createPlatformResourceLimits(platform);

// Dreamcast: Conservative settings
// Cloud: Aggressive resource usage
```

## Integration Examples

### Web Application

```typescript
// Session dashboard component
function SessionDashboard() {
  const { sessions } = useSessions(operations);
  const { stats } = useSessionAnalytics(analytics);

  return (
    <div>
      <h2>Session Dashboard</h2>
      <div>Total Active: {stats?.activeSessions}</div>
      <div>System Health: {stats?.systemHealth}</div>
      
      <div>
        {sessions.map(session => (
          <SessionCard key={session.id} session={session} />
        ))}
      </div>
    </div>
  );
}
```

### CLI Tool

```go
package main

import (
    "context"
    "fmt"
    "time"
    "github.com/flight-core/shared-types/session"
)

func monitorSessions(ctx context.Context) {
    ticker := time.NewTicker(5 * time.Second)
    defer ticker.Stop()

    for {
        select {
        case <-ctx.Done():
            return
        case <-ticker.C:
            // Monitor all active sessions
            sessions := getAllActiveSessions()
            for _, sess := range sessions {
                if session.IsSessionExpired(sess) {
                    fmt.Printf("Session %s expired\n", sess.ID)
                    terminateSession(sess.ID)
                }
            }
        }
    }
}
```

## Migration Guide

### From Legacy Session Systems

1. **Identify Current Sessions**: Map existing session data to Flight session structure
2. **Configure Resource Limits**: Set appropriate limits based on platform
3. **Update State Management**: Replace custom state logic with validated transitions
4. **Add Resource Monitoring**: Implement resource tracking
5. **Integrate Error Handling**: Use FlightResult for consistent error management

### Best Practices

1. **Always Validate**: Use validation functions before operations
2. **Monitor Resources**: Implement resource monitoring for production systems
3. **Handle Errors**: Always check FlightResult return values
4. **Clean Up**: Terminate sessions when no longer needed
5. **Platform Awareness**: Use platform-specific configurations

## Testing

Run the integration tests to verify implementation:

```bash
# TypeScript tests
cd bindings/typescript/session
npm test

# Go tests  
cd bindings/go/session
go test ./...

# Rust validation tests
cd validation/session
cargo test
```

## Support and Documentation

- **WIT Interface**: `wit/session.wit` - Core type definitions
- **TypeScript Docs**: `bindings/typescript/session/README.md`
- **Go Docs**: `bindings/go/session/README.md`  
- **Integration Tests**: `tests/integration/typescript/session-types.test.ts`
- **Examples**: See `docs/examples/` directory

## Version Compatibility

- **Flight Session Types**: v1.0.0
- **WIT Component Model**: Compatible
- **TypeScript**: 5.0+
- **Go**: 1.21+
- **React**: 16.8+ (for hooks)

---

**Status**: ✅ **READY FOR PRODUCTION**

Flight Session Types provide a comprehensive, vendor-neutral foundation for session management across all Flight-Core platforms and integrations.
