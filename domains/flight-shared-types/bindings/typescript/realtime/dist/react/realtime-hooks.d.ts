/**
 * React Hooks for Real-time Communication
 *
 * Provides React hooks for managing real-time connections, subscriptions,
 * and event handling in Flight-Core and V6R applications.
 */
import { ClientInfo, SubscriptionFilter, ConnectionId, ChannelId, FlightCorePlatformConfig, V6RPlatformConfig } from '../types';
export declare function useRealtimeConnection(clientInfo: ClientInfo, authToken?: string, platform?: string): {
    connection: any;
    connectionState: any;
    error: any;
    isConnecting: any;
    connect: any;
    disconnect: any;
    reconnect: any;
    isConnected: boolean;
};
export declare function useRealtimeChannel(connectionId: ConnectionId | null, channelId: ChannelId, filters?: SubscriptionFilter[]): {
    subscription: any;
    isSubscribed: any;
    error: any;
    subscribe: any;
    unsubscribe: any;
};
export declare function useRealtimeEvents(connectionId: ConnectionId | null, eventTypes?: string[]): {
    events: any;
    latestEvent: any;
    clearEvents: any;
    eventCount: any;
};
export declare function useRealtimeMessaging(connectionId: ConnectionId | null): {
    sendMessage: any;
    broadcastEvent: any;
    isLoading: any;
    error: any;
};
export declare function useRealtimeMetrics(refreshInterval?: number): {
    metrics: any;
    platformPerformance: any;
    isLoading: any;
    error: any;
    refreshMetrics: any;
    fetchPlatformPerformance: any;
};
export declare function usePlatformConfig(platform: string): {
    flightCoreConfig: FlightCorePlatformConfig | null;
    v6rConfig: V6RPlatformConfig | null;
    isConstrainedPlatform: boolean;
    supportsPushNotifications: boolean;
    supportsWebSocket: boolean;
    platform: string;
};
export declare function useChannelList(connectionId: ConnectionId | null, userId?: string): {
    channels: any;
    isLoading: any;
    error: any;
    refreshChannels: any;
    createChannel: any;
};
export declare function useRealtimeIntegration(clientInfo: ClientInfo, authToken?: string, platform?: string): {
    connection: {
        connection: any;
        connectionState: any;
        error: any;
        isConnecting: any;
        connect: any;
        disconnect: any;
        reconnect: any;
        isConnected: boolean;
    };
    messaging: {
        sendMessage: any;
        broadcastEvent: any;
        isLoading: any;
        error: any;
    };
    events: {
        events: any;
        latestEvent: any;
        clearEvents: any;
        eventCount: any;
    };
    metrics: {
        metrics: any;
        platformPerformance: any;
        isLoading: any;
        error: any;
        refreshMetrics: any;
        fetchPlatformPerformance: any;
    };
    platformConfig: {
        flightCoreConfig: FlightCorePlatformConfig | null;
        v6rConfig: V6RPlatformConfig | null;
        isConstrainedPlatform: boolean;
        supportsPushNotifications: boolean;
        supportsWebSocket: boolean;
        platform: string;
    };
    channels: {
        channels: any;
        isLoading: any;
        error: any;
        refreshChannels: any;
        createChannel: any;
    };
    subscribeToChannel: any;
    isReady: boolean;
    connectionId: any;
};
//# sourceMappingURL=realtime-hooks.d.ts.map