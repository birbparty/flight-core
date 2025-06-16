/**
 * Flight Real-time Communication Types
 *
 * Main entry point for TypeScript bindings of Flight-Core and V6R
 * real-time communication types and utilities.
 */
export * from './types';
export type { ConnectionInfo, RealtimeEvent, RealtimeMessage, ChannelInfo, Subscription, ClientInfo, RealtimeMetrics, PlatformPerformance, ConnectionId, ChannelId, MessageId, } from './types';
export declare const VERSION = "1.0.0";
export declare const PACKAGE_NAME = "@flight/realtime-types";
export declare function detectPlatform(): string;
export declare function isConnected(state: ConnectionState): boolean;
export declare function isDisconnected(state: ConnectionState): boolean;
export declare function isPriorityMessage(priority: MessagePriority): boolean;
export declare function getPlatformCapabilities(platform: string): string[];
export declare function getMemoryConstraint(platform: string): string;
export declare function getConnectionMode(platform: string): 'polling' | 'wifi' | 'websocket';
export declare function createEventTypeFilter(types: string[]): (event: RealtimeEvent) => boolean;
export declare function createMessageRouting(platform: string, priority: MessagePriority, requiresAck?: boolean): MessageRouting;
export declare function createEqualsFilter(field: string, value: string): SubscriptionFilter;
export declare function createContainsFilter(field: string, value: string): SubscriptionFilter;
export declare function createPriorityFilter(minPriority: MessagePriority): SubscriptionFilter;
export declare function createFlightCoreClientInfo(platform: string, version?: string): ClientInfo;
export declare function createV6RClientInfo(clientType: ClientType, version?: string, subscriptionTier?: string): ClientInfo;
export declare function createWebBrowserClientInfo(version?: string): ClientInfo;
export declare function createMemoryUpdatesChannel(platform: string): ChannelInfo;
export declare function createComponentEventsChannel(platform: string): ChannelInfo;
export declare function createV6RVmChannel(organization?: string): ChannelInfo;
export declare const FLIGHT_PLATFORMS: readonly ["dreamcast", "psp", "vita"];
export declare const V6R_SUBSCRIPTION_TIERS: readonly ["free", "individual", "team", "enterprise"];
export declare const CONSTRAINED_PLATFORMS: readonly ["dreamcast", "psp"];
export declare const DEFAULT_MESSAGE_PRIORITIES: {
    readonly SYSTEM: MessagePriority.Critical;
    readonly USER_ACTION: MessagePriority.High;
    readonly UPDATE: MessagePriority.Normal;
    readonly BACKGROUND: MessagePriority.Low;
};
export declare const DEFAULT_RETRY_CONFIGS: {
    readonly DREAMCAST: {
        readonly maxRetries: 1;
        readonly retryIntervalMs: 5000;
        readonly backoffMultiplier: 1;
        readonly maxRetryIntervalMs: 5000;
    };
    readonly PSP: {
        readonly maxRetries: 2;
        readonly retryIntervalMs: 3000;
        readonly backoffMultiplier: 1.5;
        readonly maxRetryIntervalMs: 6000;
    };
    readonly STANDARD: {
        readonly maxRetries: 3;
        readonly retryIntervalMs: 1000;
        readonly backoffMultiplier: 2;
        readonly maxRetryIntervalMs: 10000;
    };
};
import { ConnectionState, MessagePriority, ClientType, SubscriptionFilter, MessageRouting, ClientInfo, ChannelInfo, RealtimeEvent } from './types';
//# sourceMappingURL=index.d.ts.map