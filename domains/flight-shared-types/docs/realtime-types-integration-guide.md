# Real-time Communication Types Integration Guide

## Overview

The Flight Real-time Communication Types provide comprehensive TypeScript and Go bindings for real-time messaging, event streaming, and live updates across Flight-Core and V6R platforms. This integration enables WebSocket communication, pub/sub messaging, and real-time analytics for applications running on constrained retro platforms (Dreamcast, PSP, PlayStation Vita) as well as modern cloud environments.

## Key Features

### 🔄 Real-time Communication
- **WebSocket Messaging**: Full-duplex communication with automatic fallbacks for constrained platforms
- **Event Streaming**: Real-time event propagation across all connected clients
- **Message Routing**: Intelligent routing based on platform capabilities and constraints
- **Connection Management**: Robust connection handling with automatic reconnection

### 🎮 Platform Optimization
- **Flight-Core Native**: Optimized for Dreamcast (16MB), PSP (32MB), and PlayStation Vita (512MB)
- **Polling Mode**: Efficient polling for platforms without WebSocket support
- **Memory-Aware**: Adaptive message batching based on available memory
- **Performance Tuning**: Platform-specific retry logic and timeouts

### ☁️ V6R Cloud Integration
- **VM Management**: Real-time monitoring of cloud VM instances
- **Team Collaboration**: Multi-user real-time features with role-based access
- **Analytics Dashboard**: Live performance metrics and usage analytics
- **Auto-scaling Events**: Real-time notifications for VM scaling operations

## Architecture

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Flight-Core   │    │   V6R Cloud     │    │  Web Browser    │
│   (Dreamcast,   │    │   (VM Mgmt)     │    │   (Dashboard)   │
│    PSP, Vita)   │    │                 │    │                 │
└─────────┬───────┘    └─────────┬───────┘    └─────────┬───────┘
          │                      │                      │
          │                      │                      │
     ┌────▼──────────────────────▼──────────────────────▼────┐
     │              Real-time Message Broker                │
     │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐    │
     │  │   Memory    │ │ Component   │ │ System      │    │
     │  │  Updates    │ │   Events    │ │ Monitoring  │    │
     │  └─────────────┘ └─────────────┘ └─────────────┘    │
     └───────────────────────────────────────────────────────┘
```

## Quick Start

### TypeScript Integration

```typescript
import {
  createFlightCoreClientInfo,
  createMemoryUpdatesChannel,
  getPlatformCapabilities,
  ConnectionState,
  MessagePriority,
  RealtimeEvent
} from '@flight/realtime-types';

// Create a Flight-Core client for Dreamcast
const clientInfo = createFlightCoreClientInfo('dreamcast', '1.0.0');
console.log('Platform capabilities:', clientInfo.capabilities);
// Output: ['basic-messaging', 'memory-updates', 'polling-mode']

// Create a memory updates channel optimized for Dreamcast
const memoryChannel = createMemoryUpdatesChannel('dreamcast');
console.log('Update interval:', memoryChannel.metadata.updateInterval);
// Output: '5000' (5 seconds for constrained platform)

// Handle real-time memory events
function handleMemoryUpdate(event: RealtimeEvent) {
  if (event.type === 'memory-update') {
    const snapshot = event.data;
    console.log(`Memory usage: ${snapshot.used.humanReadable}/${snapshot.total.humanReadable}`);
    
    if (snapshot.fragmentationRatio > 0.3) {
      console.warn('High fragmentation detected:', snapshot.fragmentationRatio);
    }
  }
}
```

### Go Integration

```go
package main

import (
    "fmt"
    realtime "github.com/flight-core/shared-types/go/realtime"
)

func main() {
    // Create client info for PSP platform
    client := realtime.ClientInfo{
        ClientType:   realtime.ClientTypeFlightCoreNative,
        Platform:     "psp",
        Version:      "1.0.0",
        Capabilities: []string{"basic-messaging", "memory-updates", "polling-mode"},
    }

    // Get platform-specific memory constraint
    memoryConstraint := realtime.GetMemoryConstraint("psp")
    fmt.Printf("PSP Memory Constraint: %s\n", memoryConstraint) // "32MB"

    // Create message routing for PSP with appropriate retry settings
    routing := realtime.CreateMessageRouting("psp", realtime.MessagePriorityNormal, false)
    fmt.Printf("Max retries: %d, Interval: %dms\n", 
        routing.RetryConfig.MaxRetries,     // 2
        routing.RetryConfig.RetryIntervalMs) // 3000
}
```

## Platform-Specific Configuration

### Dreamcast (16MB RAM)
- **Connection Mode**: Polling (5-second intervals)
- **Max Connections**: 1 per channel
- **Message Batching**: Aggressive batching to conserve memory
- **Retry Logic**: Single retry with 5-second delay

```typescript
const dreamcastConfig = {
  platform: 'dreamcast',
  memoryConstraint: '16MB',
  connectionMode: 'polling',
  updateInterval: 5000,
  maxRetries: 1,
  capabilities: ['basic-messaging', 'memory-updates']
};
```

### PSP (32MB RAM)
- **Connection Mode**: Polling (3-second intervals)
- **Max Connections**: Unlimited
- **Component Events**: Supported
- **Retry Logic**: 2 retries with exponential backoff

```typescript
const pspConfig = {
  platform: 'psp',
  memoryConstraint: '32MB',
  connectionMode: 'polling',
  updateInterval: 3000,
  maxRetries: 2,
  capabilities: ['basic-messaging', 'memory-updates', 'component-events']
};
```

### PlayStation Vita (512MB RAM)
- **Connection Mode**: WiFi with WebSocket fallback
- **Real-time Features**: Full feature set
- **System Monitoring**: Advanced metrics collection
- **Performance**: Near real-time updates

```typescript
const vitaConfig = {
  platform: 'vita',
  memoryConstraint: '512MB',
  connectionMode: 'wifi',
  updateInterval: 1000,
  maxRetries: 3,
  capabilities: ['basic-messaging', 'memory-updates', 'component-events', 'system-monitoring']
};
```

## Event Types and Data Structures

### Memory Update Events
```typescript
interface MemoryUpdateEvent {
  type: 'memory-update';
  data: {
    timestamp: number;
    sessionId: string;
    platform: string;
    total: { bytes: number; humanReadable: string };
    used: { bytes: number; humanReadable: string };
    available: { bytes: number; humanReadable: string };
    fragmentationRatio: number;
  };
}
```

### Component Events
```typescript
interface ComponentEvent {
  type: 'component-update';
  data: {
    id: string;
    name: string;
    version: string;
    state: 'active' | 'inactive' | 'error';
    platform: string;
    memoryUsage: { bytes: number; humanReadable: string };
    createdAt: number;
    metadata: Record<string, any>;
  };
}
```

### V6R VM Events
```typescript
interface V6REvent {
  type: 'v6r-event';
  data: {
    eventType: 'vm-created' | 'vm-started' | 'vm-stopped' | 'vm-deleted' | 'vm-scaling';
    resource: string;
    organization?: string;
    timestamp: number;
    metadata: Record<string, string>;
  };
}
```

## Integration Patterns

### Flight-Core Memory Monitoring
```typescript
import { useRealtimeEvents, isMemoryUpdateEvent } from '@flight/realtime-types';

function FlightCoreMemoryMonitor({ platform }: { platform: string }) {
  const { events, latestEvent } = useRealtimeEvents(connectionId, ['memory-update']);
  
  const handleMemoryPressure = useCallback((event: RealtimeEvent) => {
    if (isMemoryUpdateEvent(event)) {
      const fragmentation = event.data.fragmentationRatio;
      const usage = event.data.used.bytes / event.data.total.bytes;
      
      if (usage > 0.9 || fragmentation > 0.4) {
        // Trigger garbage collection or component cleanup
        triggerMemoryOptimization(platform);
      }
    }
  }, [platform]);
  
  useEffect(() => {
    if (latestEvent) {
      handleMemoryPressure(latestEvent);
    }
  }, [latestEvent, handleMemoryPressure]);
}
```

### V6R Dashboard Integration
```typescript
import { useRealtimeMetrics, usePlatformConfig } from '@flight/realtime-types';

function V6RDashboard() {
  const { metrics, platformPerformance } = useRealtimeMetrics(1000); // 1-second refresh
  const { v6rConfig } = usePlatformConfig('v6r-cloud');
  
  return (
    <div className="dashboard">
      <MetricsPanel metrics={metrics} />
      <VMStatusGrid platforms={platformPerformance} />
      {v6rConfig?.billingEnabled && (
        <BillingMonitor subscription={v6rConfig.subscriptionTier} />
      )}
    </div>
  );
}
```

## Performance Considerations

### Memory-Constrained Platforms
- **Message Queuing**: Implement bounded queues to prevent memory overflow
- **Event Batching**: Batch multiple events into single messages
- **Compression**: Use message compression for large payloads
- **Cleanup**: Aggressive cleanup of old events and connections

### High-Throughput Scenarios
- **Connection Pooling**: Reuse connections across multiple channels
- **Load Balancing**: Distribute connections across multiple broker instances
- **Caching**: Cache frequently accessed data to reduce message volume
- **Rate Limiting**: Implement client-side rate limiting to prevent overwhelm

## Security Best Practices

### Authentication & Authorization
```typescript
// Secure connection establishment
const connection = await establishConnection(
  clientInfo,
  platform,
  authToken // JWT or similar secure token
);

// Channel-level permissions
const secureChannel: ChannelInfo = {
  id: 'secure-admin-channel',
  channelType: ChannelType.SystemMonitoring,
  requiredPermissions: ['admin', 'system-monitoring'],
  // ... other properties
};
```

### Message Validation
```typescript
function validateRealtimeMessage(message: RealtimeMessage): boolean {
  // Validate message structure
  if (!message.id || !message.messageType || !message.timestamp) {
    return false;
  }
  
  // Validate payload size (especially important for constrained platforms)
  if (message.payload.length > getMaxPayloadSize(message.platform)) {
    return false;
  }
  
  // Validate message age (prevent replay attacks)
  const messageAge = Date.now() - message.timestamp;
  if (messageAge > MAX_MESSAGE_AGE_MS) {
    return false;
  }
  
  return true;
}
```

## Deployment Guide

### Development Environment
```bash
# Install the real-time types package
npm install @flight/realtime-types

# For Go projects
go mod tidy
```

### Production Deployment
1. **Message Broker Setup**: Deploy Redis or RabbitMQ for message routing
2. **WebSocket Gateway**: Set up WebSocket gateway with fallback support
3. **Load Balancing**: Configure load balancers for high availability
4. **Monitoring**: Set up metrics collection and alerting

### Docker Configuration
```dockerfile
FROM node:18-alpine

WORKDIR /app
COPY package*.json ./
RUN npm ci --only=production

COPY . .

# Environment-specific configuration
ENV REALTIME_BROKER_URL=redis://redis:6379
ENV WEBSOCKET_PORT=8080
ENV PLATFORM_MODE=production

EXPOSE 8080
CMD ["npm", "start"]
```

## Troubleshooting

### Common Issues

#### Connection Timeouts on Constrained Platforms
```typescript
// Increase timeout for Dreamcast/PSP
const connectionConfig = {
  timeout: platform === 'dreamcast' ? 30000 : 10000,
  retryInterval: platform === 'dreamcast' ? 5000 : 1000,
};
```

#### Memory Pressure Handling
```typescript
function handleMemoryPressure(platform: string) {
  if (platform === 'dreamcast' || platform === 'psp') {
    // Clear event history to free memory
    clearEventHistory();
    // Reduce message queue size
    setMaxQueueSize(10);
    // Force garbage collection if available
    if (global.gc) global.gc();
  }
}
```

#### Message Loss Prevention
```typescript
// Enable acknowledgments for critical messages
const criticalMessage: RealtimeMessage = {
  // ... message properties
  priority: MessagePriority.Critical,
  routing: {
    // ... routing config
    requiresAck: true,
    retryConfig: {
      maxRetries: 3,
      retryIntervalMs: 2000,
    }
  }
};
```

## Migration Guide

### From Legacy Polling Systems
1. **Identify Polling Endpoints**: Map existing polling endpoints to real-time channels
2. **Update Client Code**: Replace polling logic with event subscription
3. **Configure Fallbacks**: Ensure polling fallback for constrained platforms
4. **Test Performance**: Validate memory usage and performance improvements

### From Basic WebSocket Implementations
1. **Channel Migration**: Organize messages into typed channels
2. **Add Platform Awareness**: Implement platform-specific optimizations
3. **Enhanced Error Handling**: Add retry logic and connection recovery
4. **Monitoring Integration**: Add real-time metrics and analytics

## Best Practices

### Code Organization
```typescript
// Separate real-time logic into dedicated modules
// realtime/
//   ├── connection.ts      // Connection management
//   ├── channels.ts        // Channel definitions
//   ├── events.ts          // Event handlers
//   ├── platform.ts        // Platform-specific logic
//   └── utils.ts           // Helper functions
```

### Error Handling
```typescript
class RealtimeErrorHandler {
  static handleConnectionError(error: Error, platform: string) {
    console.error(`Connection error on ${platform}:`, error);
    
    // Platform-specific recovery strategies
    if (platform === 'dreamcast') {
      return 'fallback-to-polling';
    } else if (platform === 'vita') {
      return 'retry-with-backoff';
    }
    
    return 'default-recovery';
  }
  
  static handleMessageError(message: RealtimeMessage, error: Error) {
    // Log the error with context
    console.error('Message processing error:', {
      messageId: message.id,
      messageType: message.messageType,
      error: error.message
    });
    
    // Send to dead letter queue if critical
    if (message.priority === MessagePriority.Critical) {
      sendToDeadLetterQueue(message);
    }
  }
}
```

### Testing Strategies
```typescript
// Mock real-time connections for unit tests
const mockConnection = {
  id: 'test-connection',
  state: ConnectionState.Connected,
  send: jest.fn(),
  close: jest.fn(),
};

// Integration tests with platform simulation
describe('Platform-specific behavior', () => {
  test('Dreamcast uses polling mode', () => {
    const config = getPlatformConfig('dreamcast');
    expect(config.connectionMode).toBe('polling');
    expect(config.updateInterval).toBe(5000);
  });
});
```

## Support and Documentation

### Additional Resources
- [Flight-Core Platform Guide](./flight-core-integration-validation-guide.md)
- [V6R Integration Documentation](./v6r-integration-guide.md)
- [Memory Types Integration](./memory-types-integration-guide.md)
- [Authentication Types Integration](./authentication-types-integration-guide.md)

### Community and Support
- **GitHub Issues**: Report bugs and feature requests
- **Discord**: Join the Flight-Core community for real-time support
- **Documentation**: Comprehensive guides and API references

---

**Version**: 1.0.0  
**Last Updated**: June 2025  
**Compatibility**: Flight-Core 2.0+, V6R Cloud 3.0+, Node.js 18+
