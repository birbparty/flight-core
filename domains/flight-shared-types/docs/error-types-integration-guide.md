# Flight Error Types Integration Guide

## Overview

The Flight Error Types system provides comprehensive, exception-free error handling across Flight-Core and external service integration. It supports type-safe error management from constrained platforms (Dreamcast 16MB) to modern cloud environments.

## Key Features

- **Exception-Free**: All errors as values, no exception throwing
- **Type Safety**: Complete type safety across TypeScript, Go, Rust, C++17
- **Rich Context**: Comprehensive error context for debugging
- **Service Integration**: Generic patterns for external service integration
- **Platform Support**: Handles errors across all Flight-Core platforms
- **Analytics**: Built-in error analytics and trend analysis

## Architecture

### Core Error Structure

```typescript
interface FlightError {
  id: string;              // Unique error identifier
  severity: ErrorSeverity; // Info, Warning, Error, Critical, Fatal
  category: ErrorCategory; // Memory, Platform, Network, etc.
  message: string;         // Human-readable message
  details?: string;        // Technical details
  context: ErrorContext;   // Rich context information
  timestamp: number;       // When error occurred
  cause?: string;          // Optional causative error ID
}
```

### Result Pattern

All operations that can fail return a `FlightResult<T>`:

```typescript
type FlightResult<T> = 
  | { tag: "ok"; val: T }
  | { tag: "err"; val: FlightError };

// Usage
const result: FlightResult<Data> = await fetchData();
if (result.tag === 'err') {
  handleError(result.val);
} else {
  processData(result.val);
}
```

## Integration Examples

### TypeScript Integration

```typescript
import { FlightErrorUtils, ErrorSeverity, ErrorCategory } from '@flight/error-types';

// Create service integration error
const error = FlightErrorUtils.createServiceError(
  ServiceErrorCode.ResourceAllocationFailed,
  'VM allocation failed',
  'my-service',
  sessionId,
  userId
);

// Check if recoverable
if (FlightErrorUtils.isRecoverable(error)) {
  const suggestions = FlightErrorUtils.getRecoverySuggestions(error);
  // Show recovery options to user
}

// Create API response
const apiResponse = FlightErrorUtils.createApiErrorResponse(error);
res.status(500).json(apiResponse);
```

### Go Integration

```go
import "github.com/flight-core/shared-types/go/enhanced"

// Create platform error
error := memory.NewFlightError(
    memory.ErrorSeverityError,
    memory.ErrorCategoryPlatform,
    "Insufficient memory",
    context,
)

// Check recoverability
if memory.IsRecoverableError(error) {
    // Attempt recovery
}

// JSON serialization
errorJSON, _ := json.Marshal(error)
```

### Rust Integration

```rust
use flight_error_validation::*;

// Create and handle errors through WIT interface
let error = create_platform_error(
    PlatformErrorCode::InsufficientPlatformMemory,
    "Memory allocation failed".to_string(),
    "dreamcast".to_string(),
    "texture_load".to_string(),
);

// Check recoverability
let recoverable = is_recoverable(error.clone());
```

## Error Categories

| Category | Description | Use Cases |
|----------|-------------|-----------|
| Memory | Memory-related errors | OutOfMemory, AllocationFailed |
| Platform | Platform-specific errors | Dreamcast constraints, feature support |
| Network | Network and I/O errors | Timeouts, connection failures |
| Validation | Input validation errors | Form validation, API input |
| Security | Authentication/authorization | Auth failures, permission denied |
| Component | Component Model errors | Loading, lifecycle issues |
| ServiceIntegration | External service errors | API failures, service unavailable |
| FlightSystem | Flight-Core system errors | Internal system failures |
| Application | Application-level errors | Business logic errors |
| Unknown | Uncategorized errors | Fallback category |

## Service Error Codes

Generic error codes that external services can map to:

- `resource-allocation-failed` - Resource allocation failed
- `session-limit-exceeded` - Session/connection limit exceeded  
- `auth-failed` - Authentication failed
- `rate-limit-exceeded` - API rate limit exceeded
- `service-unavailable` - Service temporarily unavailable
- `invalid-config` - Invalid configuration
- `resource-limit-exceeded` - Resource limit exceeded
- `connection-failed` - Connection/communication failed
- `request-timeout` - Request timeout
- `invalid-request` - Invalid request format
- `permission-denied` - Permission denied
- `resource-not-found` - Resource not found

## Platform Error Codes

Flight-Core specific error codes:

- `insufficient-platform-memory` - Insufficient platform memory
- `feature-not-supported` - Platform feature not supported
- `hardware-constraint-violation` - Hardware constraint violation
- `platform-init-failed` - Platform initialization failed
- `component-load-failed` - Component loading failed
- `compatibility-error` - Cross-platform compatibility issue
- `platform-resource-exhausted` - Platform resource exhausted

## Error Recovery

### Recovery Action Types

- `retry` - Retry the operation
- `reduce-resources` - Reduce resource usage
- `update-config` - Update configuration
- `contact-support` - Contact technical support
- `try-alternative` - Try alternative approach
- `wait-retry` - Wait and retry later
- `upgrade-resources` - Upgrade service plan/resources
- `check-status` - Check system status

### Example Recovery Logic

```typescript
const suggestions = FlightErrorUtils.getRecoverySuggestions(error);

for (const suggestion of suggestions) {
  switch (suggestion.actionType) {
    case RecoveryActionType.Retry:
      if (suggestion.canAutomate) {
        setTimeout(() => retryOperation(), 2000);
      }
      break;
    
    case RecoveryActionType.ReduceResources:
      await cleanupResources();
      break;
    
    case RecoveryActionType.ContactSupport:
      showSupportDialog(suggestion.description);
      break;
  }
}
```

## Error Analytics

### System Health Monitoring

```typescript
// Analyze error patterns
const analytics = await analyzeErrors(errorHistory, timeWindow);

// Assess system health
const healthStatus = await assessSystemHealth(analytics);

switch (healthStatus) {
  case SystemHealthStatus.Healthy:
    // System operating normally
    break;
  case SystemHealthStatus.Degraded:
    // Some issues but functioning
    alertOpsTeam(analytics);
    break;
  case SystemHealthStatus.Critical:
    // Serious issues
    escalateToOnCall(analytics);
    break;
  case SystemHealthStatus.Failing:
    // System experiencing significant failures
    triggerEmergencyResponse(analytics);
    break;
}
```

## Platform-Specific Considerations

### Dreamcast (16MB)

- Memory errors are critical due to severe constraints
- Recovery suggestions focus on resource reduction
- Error context should be minimal to save memory

### V6R Cloud Services

- Service integration errors are common
- Rate limiting and resource allocation errors expected
- Rich error context helpful for debugging distributed systems

### Cross-Platform Development

- Use generic error categories when possible
- Platform-specific error codes for platform-unique issues
- Consistent error handling patterns across platforms

## API Integration Patterns

### Error Response Format

```typescript
interface ErrorApiResponse {
  success: false;
  error: {
    id: string;
    code: string;
    message: string;
    severity: ErrorSeverity;
    timestamp: number;
  };
  metadata?: {
    sessionId?: string;
    userId?: string;
    requestId?: string;
  };
}
```

### Success Response Format

```typescript
interface SuccessApiResponse<T> {
  success: true;
  data: T;
  metadata?: {
    requestId?: string;
  };
}
```

## Best Practices

### Error Creation

1. Use appropriate severity levels
2. Provide descriptive error messages
3. Include relevant context information
4. Use service-specific error codes when available

### Error Handling

1. Always check `FlightResult` success/failure
2. Handle recoverable errors appropriately
3. Log errors with sufficient context
4. Provide user-friendly error messages

### Performance

1. Keep error context minimal on constrained platforms
2. Use error codes instead of long descriptions when possible
3. Avoid deep error chaining
4. Cache frequently used error patterns

### Testing

1. Test both success and error paths
2. Verify error recoverability logic
3. Test error serialization/deserialization
4. Validate error analytics accuracy

## Migration Guide

### From Traditional Exception Handling

```typescript
// Before: Exception-based
try {
  const data = await riskyOperation();
  return data;
} catch (error) {
  console.error(error);
  throw error;
}

// After: Result-based
const result = await riskyOperation();
if (result.tag === 'err') {
  console.error(FlightErrorUtils.getErrorSummary(result.val));
  return result; // Propagate error
}
return result; // Success
```

### From Basic Error Objects

```typescript
// Before: Basic error objects
return { error: "Something went wrong", code: 500 };

// After: Rich error context
return {
  tag: 'err',
  val: FlightErrorUtils.createServiceError(
    ServiceErrorCode.ServiceUnavailable,
    "Service temporarily unavailable",
    "my-service",
    sessionId,
    userId
  )
};
```

## Conclusion

The Flight Error Types system provides a robust, type-safe foundation for error handling across diverse platforms and services. By following the patterns and best practices outlined in this guide, external services can seamlessly integrate with Flight-Core while maintaining excellent error handling and user experience.
