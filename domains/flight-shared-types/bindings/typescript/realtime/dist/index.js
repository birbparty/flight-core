"use strict";
/**
 * Flight Real-time Communication Types
 *
 * Main entry point for TypeScript bindings of Flight-Core and V6R
 * real-time communication types and utilities.
 */
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __exportStar = (this && this.__exportStar) || function(m, exports) {
    for (var p in m) if (p !== "default" && !Object.prototype.hasOwnProperty.call(exports, p)) __createBinding(exports, m, p);
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.DEFAULT_RETRY_CONFIGS = exports.DEFAULT_MESSAGE_PRIORITIES = exports.CONSTRAINED_PLATFORMS = exports.V6R_SUBSCRIPTION_TIERS = exports.FLIGHT_PLATFORMS = exports.PACKAGE_NAME = exports.VERSION = void 0;
exports.detectPlatform = detectPlatform;
exports.isConnected = isConnected;
exports.isDisconnected = isDisconnected;
exports.isPriorityMessage = isPriorityMessage;
exports.getPlatformCapabilities = getPlatformCapabilities;
exports.getMemoryConstraint = getMemoryConstraint;
exports.getConnectionMode = getConnectionMode;
exports.createEventTypeFilter = createEventTypeFilter;
exports.createMessageRouting = createMessageRouting;
exports.createEqualsFilter = createEqualsFilter;
exports.createContainsFilter = createContainsFilter;
exports.createPriorityFilter = createPriorityFilter;
exports.createFlightCoreClientInfo = createFlightCoreClientInfo;
exports.createV6RClientInfo = createV6RClientInfo;
exports.createWebBrowserClientInfo = createWebBrowserClientInfo;
exports.createMemoryUpdatesChannel = createMemoryUpdatesChannel;
exports.createComponentEventsChannel = createComponentEventsChannel;
exports.createV6RVmChannel = createV6RVmChannel;
// Core Types
__exportStar(require("./types"), exports);
// Version information
exports.VERSION = '1.0.0';
exports.PACKAGE_NAME = '@flight/realtime-types';
// Platform detection utilities
function detectPlatform() {
    if (typeof window !== 'undefined') {
        return 'web-browser';
    }
    else if (typeof process !== 'undefined') {
        return 'nodejs';
    }
    else {
        return 'unknown';
    }
}
// Connection state helpers
function isConnected(state) {
    return state === types_1.ConnectionState.Connected || state === types_1.ConnectionState.Authenticated;
}
function isDisconnected(state) {
    return state === types_1.ConnectionState.Disconnected ||
        state === types_1.ConnectionState.Closed ||
        state === types_1.ConnectionState.Error;
}
// Message priority helpers
function isPriorityMessage(priority) {
    return priority === types_1.MessagePriority.High ||
        priority === types_1.MessagePriority.Critical ||
        priority === types_1.MessagePriority.Realtime;
}
// Platform capability helpers
function getPlatformCapabilities(platform) {
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
function getMemoryConstraint(platform) {
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
function getConnectionMode(platform) {
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
function createEventTypeFilter(types) {
    return (event) => {
        return types.includes(event.type);
    };
}
// Message routing helpers
function createMessageRouting(platform, priority, requiresAck = false) {
    const baseRetryInterval = platform === 'dreamcast' || platform === 'psp' ? 5000 : 1000;
    const maxRetries = platform === 'dreamcast' || platform === 'psp' ? 1 : 3;
    return {
        source: `${platform}-system`,
        route: [platform],
        requiresAck: requiresAck || priority === types_1.MessagePriority.Critical || priority === types_1.MessagePriority.Realtime,
        retryConfig: {
            maxRetries,
            retryIntervalMs: baseRetryInterval,
            backoffMultiplier: 2.0,
            maxRetryIntervalMs: baseRetryInterval * 4,
        },
    };
}
// Subscription filter builders
function createEqualsFilter(field, value) {
    return {
        field,
        operation: types_1.FilterOperation.Equals,
        value,
    };
}
function createContainsFilter(field, value) {
    return {
        field,
        operation: types_1.FilterOperation.Contains,
        value,
    };
}
function createPriorityFilter(minPriority) {
    return {
        field: 'priority',
        operation: types_1.FilterOperation.GreaterThan,
        value: minPriority,
    };
}
// Client info builders
function createFlightCoreClientInfo(platform, version = '1.0.0') {
    return {
        clientType: types_1.ClientType.FlightCoreNative,
        version,
        platform,
        capabilities: getPlatformCapabilities(platform),
        userAgent: `Flight-Core/${version}/${platform}`,
    };
}
function createV6RClientInfo(clientType, version = '2.0.0', subscriptionTier = 'individual') {
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
function createWebBrowserClientInfo(version = '1.0.0') {
    return {
        clientType: types_1.ClientType.WebBrowser,
        version,
        platform: detectPlatform(),
        capabilities: ['websocket-mode', 'browser-notifications', 'sse-fallback'],
        userAgent: typeof navigator !== 'undefined' ? navigator.userAgent : 'Unknown',
    };
}
// Channel builders
function createMemoryUpdatesChannel(platform) {
    return {
        id: `${platform}-memory-updates`,
        name: `${platform} Memory Updates`,
        channelType: types_1.ChannelType.MemoryUpdates,
        requiredPermissions: ['memory-access'],
        maxConnections: platform === 'dreamcast' ? 1 : undefined,
        createdAt: Date.now(),
        metadata: {
            platform,
            updateInterval: platform === 'dreamcast' ? '5000' : '1000',
        },
    };
}
function createComponentEventsChannel(platform) {
    return {
        id: `${platform}-component-events`,
        name: `${platform} Component Events`,
        channelType: types_1.ChannelType.ComponentEvents,
        requiredPermissions: ['component-access'],
        maxConnections: platform === 'dreamcast' ? 1 : undefined,
        createdAt: Date.now(),
        metadata: {
            platform,
            eventTypes: 'component-loaded,component-unloaded,state-changed',
        },
    };
}
function createV6RVmChannel(organization) {
    return {
        id: `v6r-vm-${organization || 'default'}`,
        name: `V6R VM Management${organization ? ` (${organization})` : ''}`,
        channelType: types_1.ChannelType.V6RVmManagement,
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
exports.FLIGHT_PLATFORMS = ['dreamcast', 'psp', 'vita'];
exports.V6R_SUBSCRIPTION_TIERS = ['free', 'individual', 'team', 'enterprise'];
exports.CONSTRAINED_PLATFORMS = ['dreamcast', 'psp'];
exports.DEFAULT_MESSAGE_PRIORITIES = {
    SYSTEM: types_1.MessagePriority.Critical,
    USER_ACTION: types_1.MessagePriority.High,
    UPDATE: types_1.MessagePriority.Normal,
    BACKGROUND: types_1.MessagePriority.Low,
};
exports.DEFAULT_RETRY_CONFIGS = {
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
};
// Import the needed types to avoid compilation errors
const types_1 = require("./types");
//# sourceMappingURL=index.js.map