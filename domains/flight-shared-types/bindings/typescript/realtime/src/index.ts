/**
 * Flight Real-time Communication Types
 * 
 * Main entry point for TypeScript bindings of Flight-Core and V6R
 * real-time communication types and utilities.
 */

// Core Types
export * from './types';

// React Hooks (when React is available) - commented out for build compatibility
// export * from './react/realtime-hooks';

// Re-export commonly used types for convenience (interfaces only)
export type {
  ConnectionInfo,
  RealtimeEvent,
  RealtimeMessage,
  ChannelInfo,
  Subscription,
  ClientInfo,
  RealtimeMetrics,
  PlatformPerformance,
  ConnectionId,
  ChannelId,
  MessageId,
} from './types';

// Version information
export const VERSION = '1.0.0';
export const PACKAGE_NAME = '@flight/realtime-types';

// Platform detection utilities
export function detectPlatform(): string {
  if (typeof window !== 'undefined') {
    return 'web-browser';
  } else if (typeof process !== 'undefined') {
    return 'nodejs';
  } else {
    return 'unknown';
  }
}

// Connection state helpers
export function isConnected(state: ConnectionState): boolean {
  return state === ConnectionState.Connected || state === ConnectionState.Authenticated;
}

export function isDisconnected(state: ConnectionState): boolean {
  return state === ConnectionState.Disconnected || 
         state === ConnectionState.Closed || 
         state === ConnectionState.Error;
}

// Message priority helpers
export function isPriorityMessage(priority: MessagePriority): boolean {
  return priority === MessagePriority.High || 
         priority === MessagePriority.Critical || 
         priority === MessagePriority.Realtime;
}

// Platform capability helpers
export function getPlatformCapabilities(platform: string): string[] {
  switch (platform) {
    case 'dreamcast':
      return ['basic-messaging', 'memory-updates', 'polling-mode'];
    case 'psp':
      return ['basic-messaging', 'memory-updates', 'component-events', 'polling-mode'];
    case 'vita':
      return ['basic-messaging', 'memory-updates', 'component-events', 'system-monitoring', 'wifi-mode'];
    case 'web-browser':
      return ['full-messaging', 'websocket-mode', 'browser-notifications'];
    case 'v6r-cloud':
      return ['vm-monitoring', 'vm-management', 'performance-metrics', 'high-throughput'];
    default:
      return ['basic-messaging'];
  }
}

// Memory constraint helpers for platform adaptation
export function getMemoryConstraint(platform: string): string {
  switch (platform) {
    case 'dreamcast':
      return '16MB';
    case 'psp':
      return '32MB';
    case 'vita':
      return '512MB';
    default:
      return 'unlimited';
  }
}

// Connection mode detection
export function getConnectionMode(platform: string): 'polling' | 'wifi' | 'websocket' {
  switch (platform) {
    case 'dreamcast':
    case 'psp':
      return 'polling';
    case 'vita':
      return 'wifi';
    default:
      return 'websocket';
  }
}

// Event type filters
export function createEventTypeFilter(types: string[]) {
  return (event: RealtimeEvent): boolean => {
    return types.includes(event.type);
  };
}

// Message routing helpers
export function createMessageRouting(
  platform: string,
  priority: MessagePriority,
  requiresAck = false
): MessageRouting {
  const baseRetryInterval = platform === 'dreamcast' || platform === 'psp' ? 5000 : 1000;
  const maxRetries = platform === 'dreamcast' || platform === 'psp' ? 1 : 3;

  return {
    source: `${platform}-system`,
    route: [platform],
    requiresAck: requiresAck || priority === MessagePriority.Critical || priority === MessagePriority.Realtime,
    retryConfig: {
      maxRetries,
      retryIntervalMs: baseRetryInterval,
      backoffMultiplier: 2.0,
      maxRetryIntervalMs: baseRetryInterval * 4,
    },
  };
}

// Subscription filter builders
export function createEqualsFilter(field: string, value: string): SubscriptionFilter {
  return {
    field,
    operation: FilterOperation.Equals,
    value,
  };
}

export function createContainsFilter(field: string, value: string): SubscriptionFilter {
  return {
    field,
    operation: FilterOperation.Contains,
    value,
  };
}

export function createPriorityFilter(minPriority: MessagePriority): SubscriptionFilter {
  return {
    field: 'priority',
    operation: FilterOperation.GreaterThan,
    value: minPriority,
  };
}

// Client info builders
export function createFlightCoreClientInfo(platform: string, version = '1.0.0'): ClientInfo {
  return {
    clientType: ClientType.FlightCoreNative,
    version,
    platform,
    capabilities: getPlatformCapabilities(platform),
    userAgent: `Flight-Core/${version}/${platform}`,
  };
}

export function createV6RClientInfo(
  clientType: ClientType,
  version = '2.0.0',
  subscriptionTier = 'individual'
): ClientInfo {
  const capabilities = ['basic-messaging'];
  
  if (subscriptionTier === 'team' || subscriptionTier === 'enterprise') {
    capabilities.push('team-collaboration', 'shared-channels');
  }
  
  if (subscriptionTier === 'enterprise') {
    capabilities.push('admin-controls', 'audit-logs');
  }

  return {
    clientType,
    version,
    platform: 'v6r-cloud',
    capabilities,
    userAgent: `V6R-Client/${version}/${subscriptionTier}`,
  };
}

export function createWebBrowserClientInfo(version = '1.0.0'): ClientInfo {
  return {
    clientType: ClientType.WebBrowser,
    version,
    platform: detectPlatform(),
    capabilities: ['websocket-mode', 'browser-notifications', 'sse-fallback'],
    userAgent: typeof navigator !== 'undefined' ? navigator.userAgent : 'Unknown',
  };
}

// Channel builders
export function createMemoryUpdatesChannel(platform: string): ChannelInfo {
  return {
    id: `${platform}-memory-updates`,
    name: `${platform} Memory Updates`,
    channelType: ChannelType.MemoryUpdates,
    requiredPermissions: ['memory-access'],
    maxConnections: platform === 'dreamcast' ? 1 : undefined,
    createdAt: Date.now(),
    metadata: {
      platform,
      updateInterval: platform === 'dreamcast' ? '5000' : '1000',
    },
  };
}

export function createComponentEventsChannel(platform: string): ChannelInfo {
  return {
    id: `${platform}-component-events`,
    name: `${platform} Component Events`,
    channelType: ChannelType.ComponentEvents,
    requiredPermissions: ['component-access'],
    maxConnections: platform === 'dreamcast' ? 1 : undefined,
    createdAt: Date.now(),
    metadata: {
      platform,
      eventTypes: 'component-loaded,component-unloaded,state-changed',
    },
  };
}

export function createV6RVmChannel(organization?: string): ChannelInfo {
  return {
    id: `v6r-vm-${organization || 'default'}`,
    name: `V6R VM Management${organization ? ` (${organization})` : ''}`,
    channelType: ChannelType.V6RVmManagement,
    requiredPermissions: ['vm-management'],
    createdAt: Date.now(),
    metadata: {
      platform: 'v6r-cloud',
      organization: organization || '',
      vmEvents: 'created,started,stopped,deleted,scaling',
    },
  };
}

// Constants for common configurations
export const FLIGHT_PLATFORMS = ['dreamcast', 'psp', 'vita'] as const;
export const V6R_SUBSCRIPTION_TIERS = ['free', 'individual', 'team', 'enterprise'] as const;
export const CONSTRAINED_PLATFORMS = ['dreamcast', 'psp'] as const;

export const DEFAULT_MESSAGE_PRIORITIES = {
  SYSTEM: MessagePriority.Critical,
  USER_ACTION: MessagePriority.High,
  UPDATE: MessagePriority.Normal,
  BACKGROUND: MessagePriority.Low,
} as const;

export const DEFAULT_RETRY_CONFIGS = {
  DREAMCAST: {
    maxRetries: 1,
    retryIntervalMs: 5000,
    backoffMultiplier: 1.0,
    maxRetryIntervalMs: 5000,
  },
  PSP: {
    maxRetries: 2,
    retryIntervalMs: 3000,
    backoffMultiplier: 1.5,
    maxRetryIntervalMs: 6000,
  },
  STANDARD: {
    maxRetries: 3,
    retryIntervalMs: 1000,
    backoffMultiplier: 2.0,
    maxRetryIntervalMs: 10000,
  },
} as const;

// Import the needed types to avoid compilation errors
import {
  ConnectionState,
  MessagePriority,
  ClientType,
  ChannelType,
  FilterOperation,
  SubscriptionFilter,
  MessageRouting,
  ClientInfo,
  ChannelInfo,
  RealtimeEvent,
} from './types';
