/**
 * React Hooks for Real-time Communication
 * 
 * Provides React hooks for managing real-time connections, subscriptions,
 * and event handling in Flight-Core and V6R applications.
 */

// Conditional React import - only works when React is available
let useState: any, useEffect: any, useCallback: any, useRef: any, useMemo: any;

try {
  const react = require('react');
  useState = react.useState;
  useEffect = react.useEffect;
  useCallback = react.useCallback;
  useRef = react.useRef;
  useMemo = react.useMemo;
} catch (e) {
  // React not available - hooks will throw runtime errors if used
  const notAvailable = () => {
    throw new Error('React is not available. Install React to use realtime hooks.');
  };
  useState = useEffect = useCallback = useRef = useMemo = notAvailable;
}
import { EventEmitter } from 'events';
import {
  ConnectionInfo,
  ConnectionState,
  RealtimeEvent,
  RealtimeMessage,
  ChannelInfo,
  Subscription,
  ClientInfo,
  ChannelType,
  MessagePriority,
  RealtimeMetrics,
  PlatformPerformance,
  RealtimeConnectionAPI,
  RealtimeMessagingAPI,
  RealtimeSubscriptionAPI,
  RealtimeAnalyticsAPI,
  SubscriptionFilter,
  ConnectionId,
  ChannelId,
  FlightCorePlatformConfig,
  V6RPlatformConfig
} from '../types';

// Hook for managing real-time connection
export function useRealtimeConnection(
  clientInfo: ClientInfo,
  authToken?: string,
  platform?: string
) {
  const [connection, setConnection] = useState<ConnectionInfo | null>(null);
  const [connectionState, setConnectionState] = useState<ConnectionState>(ConnectionState.Disconnected);
  const [error, setError] = useState<Error | null>(null);
  const [isConnecting, setIsConnecting] = useState(false);
  
  const connectionAPI = useRef<RealtimeConnectionAPI | null>(null);

  const connect = useCallback(async () => {
    if (isConnecting || connectionState === ConnectionState.Connected) {
      return;
    }

    try {
      setIsConnecting(true);
      setError(null);
      
      // Initialize connection API (would be injected in real implementation)
      if (!connectionAPI.current) {
        throw new Error('Connection API not available');
      }

      const result = await connectionAPI.current.establishConnection(
        clientInfo,
        authToken,
        platform || 'web'
      );

      if (result.success) {
        setConnection(result.data);
        setConnectionState(result.data.state);
      } else {
        throw new Error(result.error?.message || 'Connection failed');
      }
    } catch (err) {
      setError(err instanceof Error ? err : new Error('Unknown connection error'));
      setConnectionState(ConnectionState.Error);
    } finally {
      setIsConnecting(false);
    }
  }, [clientInfo, authToken, platform, isConnecting, connectionState]);

  const disconnect = useCallback(async (reason = 'User requested') => {
    if (!connection || !connectionAPI.current) {
      return;
    }

    try {
      await connectionAPI.current.closeConnection(connection.id, reason);
      setConnection(null);
      setConnectionState(ConnectionState.Closed);
    } catch (err) {
      setError(err instanceof Error ? err : new Error('Disconnect failed'));
    }
  }, [connection]);

  const reconnect = useCallback(async () => {
    if (connection) {
      await disconnect('Reconnecting');
    }
    await connect();
  }, [connect, disconnect, connection]);

  // Monitor connection state changes
  useEffect(() => {
    if (connection) {
      setConnectionState(connection.state);
    }
  }, [connection]);

  return {
    connection,
    connectionState,
    error,
    isConnecting,
    connect,
    disconnect,
    reconnect,
    isConnected: connectionState === ConnectionState.Connected || connectionState === ConnectionState.Authenticated,
  };
}

// Hook for subscribing to real-time channels
export function useRealtimeChannel(
  connectionId: ConnectionId | null,
  channelId: ChannelId,
  filters?: SubscriptionFilter[]
) {
  const [subscription, setSubscription] = useState<Subscription | null>(null);
  const [isSubscribed, setIsSubscribed] = useState(false);
  const [error, setError] = useState<Error | null>(null);
  
  const subscriptionAPI = useRef<RealtimeSubscriptionAPI | null>(null);

  const subscribe = useCallback(async () => {
    if (!connectionId || !subscriptionAPI.current || isSubscribed) {
      return;
    }

    try {
      setError(null);
      const result = await subscriptionAPI.current.subscribeToChannel(
        connectionId,
        channelId,
        filters
      );

      if (result.success) {
        setSubscription(result.data);
        setIsSubscribed(true);
      } else {
        throw new Error(result.error?.message || 'Subscription failed');
      }
    } catch (err) {
      setError(err instanceof Error ? err : new Error('Subscription error'));
    }
  }, [connectionId, channelId, filters, isSubscribed]);

  const unsubscribe = useCallback(async () => {
    if (!connectionId || !subscriptionAPI.current || !isSubscribed) {
      return;
    }

    try {
      await subscriptionAPI.current.unsubscribeFromChannel(connectionId, channelId);
      setSubscription(null);
      setIsSubscribed(false);
    } catch (err) {
      setError(err instanceof Error ? err : new Error('Unsubscribe error'));
    }
  }, [connectionId, channelId, isSubscribed]);

  // Auto-subscribe when connection is available
  useEffect(() => {
    if (connectionId && !isSubscribed) {
      subscribe();
    }
  }, [connectionId, subscribe, isSubscribed]);

  // Auto-unsubscribe on unmount
  useEffect(() => {
    return () => {
      if (isSubscribed) {
        unsubscribe();
      }
    };
  }, [isSubscribed, unsubscribe]);

  return {
    subscription,
    isSubscribed,
    error,
    subscribe,
    unsubscribe,
  };
}

// Hook for handling real-time events
export function useRealtimeEvents(
  connectionId: ConnectionId | null,
  eventTypes?: string[]
) {
  const [events, setEvents] = useState<RealtimeEvent[]>([]);
  const [latestEvent, setLatestEvent] = useState<RealtimeEvent | null>(null);
  const eventEmitter = useRef<EventEmitter | null>(null);

  const addEvent = useCallback((event: RealtimeEvent) => {
    setLatestEvent(event);
    setEvents(prev => [...prev.slice(-99), event]); // Keep last 100 events
  }, []);

  const clearEvents = useCallback(() => {
    setEvents([]);
    setLatestEvent(null);
  }, []);

  // Filter events by type if specified
  const filteredEvents = useMemo(() => {
    if (!eventTypes || eventTypes.length === 0) {
      return events;
    }
    return events.filter(event => eventTypes.includes(event.type));
  }, [events, eventTypes]);

  // Set up event listener
  useEffect(() => {
    if (!connectionId) {
      return;
    }

    // Initialize event emitter (would be connected to real WebSocket in implementation)
    if (!eventEmitter.current) {
      eventEmitter.current = new EventEmitter();
    }

    const handleEvent = (event: RealtimeEvent) => {
      if (!eventTypes || eventTypes.includes(event.type)) {
        addEvent(event);
      }
    };

    eventEmitter.current.on('event', handleEvent);

    return () => {
      eventEmitter.current?.off('event', handleEvent);
    };
  }, [connectionId, eventTypes, addEvent]);

  return {
    events: filteredEvents,
    latestEvent,
    clearEvents,
    eventCount: filteredEvents.length,
  };
}

// Hook for sending real-time messages
export function useRealtimeMessaging(connectionId: ConnectionId | null) {
  const [isLoading, setIsLoading] = useState(false);
  const [error, setError] = useState<Error | null>(null);
  
  const messagingAPI = useRef<RealtimeMessagingAPI | null>(null);

  const sendMessage = useCallback(async (
    channelId: ChannelId,
    messageType: string,
    payload: any,
    priority: MessagePriority = MessagePriority.Normal
  ) => {
    if (!connectionId || !messagingAPI.current) {
      throw new Error('No active connection');
    }

    setIsLoading(true);
    setError(null);

    try {
      const message: RealtimeMessage = {
        id: `msg-${Date.now()}-${Math.random().toString(36).substr(2, 9)}`,
        messageType,
        channelId,
        payload: JSON.stringify(payload),
        timestamp: Date.now(),
        priority,
        routing: {
          source: connectionId,
          route: [],
          requiresAck: priority === MessagePriority.Critical || priority === MessagePriority.Realtime,
        },
        metadata: {},
      };

      const result = await messagingAPI.current.sendMessage(connectionId, message);
      
      if (!result.success) {
        throw new Error(result.error?.message || 'Message send failed');
      }

      return result.data;
    } catch (err) {
      const error = err instanceof Error ? err : new Error('Send message error');
      setError(error);
      throw error;
    } finally {
      setIsLoading(false);
    }
  }, [connectionId]);

  const broadcastEvent = useCallback(async (
    channelId: ChannelId,
    event: RealtimeEvent
  ) => {
    if (!connectionId || !messagingAPI.current) {
      throw new Error('No active connection');
    }

    setIsLoading(true);
    setError(null);

    try {
      const result = await messagingAPI.current.broadcastEvent(channelId, event);
      
      if (!result.success) {
        throw new Error(result.error?.message || 'Event broadcast failed');
      }

      return result.data;
    } catch (err) {
      const error = err instanceof Error ? err : new Error('Broadcast event error');
      setError(error);
      throw error;
    } finally {
      setIsLoading(false);
    }
  }, [connectionId]);

  return {
    sendMessage,
    broadcastEvent,
    isLoading,
    error,
  };
}

// Hook for real-time metrics monitoring
export function useRealtimeMetrics(refreshInterval = 5000) {
  const [metrics, setMetrics] = useState<RealtimeMetrics | null>(null);
  const [platformPerformance, setPlatformPerformance] = useState<Record<string, PlatformPerformance>>({});
  const [isLoading, setIsLoading] = useState(false);
  const [error, setError] = useState<Error | null>(null);
  
  const analyticsAPI = useRef<RealtimeAnalyticsAPI | null>(null);

  const fetchMetrics = useCallback(async () => {
    if (!analyticsAPI.current) {
      return;
    }

    setIsLoading(true);
    setError(null);

    try {
      const result = await analyticsAPI.current.getRealtimeMetrics();
      
      if (result.success) {
        setMetrics(result.data);
      } else {
        throw new Error(result.error?.message || 'Failed to fetch metrics');
      }
    } catch (err) {
      setError(err instanceof Error ? err : new Error('Metrics fetch error'));
    } finally {
      setIsLoading(false);
    }
  }, []);

  const fetchPlatformPerformance = useCallback(async (platform: string) => {
    if (!analyticsAPI.current) {
      return;
    }

    try {
      const result = await analyticsAPI.current.getPlatformPerformance(platform);
      
      if (result.success) {
        setPlatformPerformance(prev => ({
          ...prev,
          [platform]: result.data,
        }));
      }
    } catch (err) {
      console.error(`Failed to fetch performance for platform ${platform}:`, err);
    }
  }, []);

  // Auto-refresh metrics
  useEffect(() => {
    fetchMetrics();
    
    const interval = setInterval(fetchMetrics, refreshInterval);
    
    return () => clearInterval(interval);
  }, [fetchMetrics, refreshInterval]);

  return {
    metrics,
    platformPerformance,
    isLoading,
    error,
    refreshMetrics: fetchMetrics,
    fetchPlatformPerformance,
  };
}

// Hook for platform-specific configuration
export function usePlatformConfig(platform: string) {
  const flightCoreConfig: FlightCorePlatformConfig | null = useMemo(() => {
    if (['dreamcast', 'psp', 'vita'].includes(platform)) {
      return {
        platform: platform as 'dreamcast' | 'psp' | 'vita',
        memoryConstraint: platform === 'dreamcast' ? '16MB' : platform === 'psp' ? '32MB' : '512MB',
        connectionMode: platform === 'vita' ? 'wifi' : 'polling',
        capabilities: platform === 'dreamcast' 
          ? ['basic-messaging', 'memory-updates']
          : ['basic-messaging', 'memory-updates', 'component-events'],
      };
    }
    return null;
  }, [platform]);

  const v6rConfig: V6RPlatformConfig | null = useMemo(() => {
    if (platform === 'v6r-cloud') {
      return {
        subscriptionTier: 'individual', // Would be determined from user context
        clientType: platform.includes('mobile') ? 'v6r-mobile' as any : 'web-browser' as any,
        capabilities: ['vm-monitoring', 'vm-management', 'performance-metrics'],
        billingEnabled: true,
      };
    }
    return null;
  }, [platform]);

  const isConstrainedPlatform = ['dreamcast', 'psp'].includes(platform);
  const supportsPushNotifications = platform.includes('mobile');
  const supportsWebSocket = !['dreamcast', 'psp'].includes(platform);

  return {
    flightCoreConfig,
    v6rConfig,
    isConstrainedPlatform,
    supportsPushNotifications,
    supportsWebSocket,
    platform,
  };
}

// Hook for managing channel list
export function useChannelList(connectionId: ConnectionId | null, userId?: string) {
  const [channels, setChannels] = useState<ChannelInfo[]>([]);
  const [isLoading, setIsLoading] = useState(false);
  const [error, setError] = useState<Error | null>(null);
  
  const subscriptionAPI = useRef<RealtimeSubscriptionAPI | null>(null);

  const fetchChannels = useCallback(async () => {
    if (!connectionId || !subscriptionAPI.current) {
      return;
    }

    setIsLoading(true);
    setError(null);

    try {
      const result = await subscriptionAPI.current.listChannels(userId);
      
      if (result.success) {
        setChannels(result.data);
      } else {
        throw new Error(result.error?.message || 'Failed to fetch channels');
      }
    } catch (err) {
      setError(err instanceof Error ? err : new Error('Channel list error'));
    } finally {
      setIsLoading(false);
    }
  }, [connectionId, userId]);

  const createChannel = useCallback(async (
    name: string,
    channelType: ChannelType,
    requiredPermissions: string[] = []
  ) => {
    if (!connectionId || !subscriptionAPI.current) {
      throw new Error('No active connection');
    }

    const channelInfo: ChannelInfo = {
      id: '', // Will be assigned by the server
      name,
      channelType,
      requiredPermissions,
      createdAt: Date.now(),
      metadata: {},
    };

    const result = await subscriptionAPI.current.createChannel(channelInfo);
    
    if (result.success) {
      await fetchChannels(); // Refresh the list
      return result.data;
    } else {
      throw new Error(result.error?.message || 'Channel creation failed');
    }
  }, [connectionId, fetchChannels]);

  // Fetch channels when connection is available
  useEffect(() => {
    if (connectionId) {
      fetchChannels();
    }
  }, [connectionId, fetchChannels]);

  return {
    channels,
    isLoading,
    error,
    refreshChannels: fetchChannels,
    createChannel,
  };
}

// Composite hook for complete real-time integration
export function useRealtimeIntegration(
  clientInfo: ClientInfo,
  authToken?: string,
  platform = 'web'
) {
  const connection = useRealtimeConnection(clientInfo, authToken, platform);
  const messaging = useRealtimeMessaging(connection.connection?.id || null);
  const events = useRealtimeEvents(connection.connection?.id || null);
  const metrics = useRealtimeMetrics();
  const platformConfig = usePlatformConfig(platform);
  const channels = useChannelList(connection.connection?.id || null, clientInfo.platform);

  const subscribeToChannel = useCallback((
    channelId: ChannelId,
    filters?: SubscriptionFilter[]
  ) => {
    return useRealtimeChannel(connection.connection?.id || null, channelId, filters);
  }, [connection.connection?.id]);

  return {
    connection,
    messaging,
    events,
    metrics,
    platformConfig,
    channels,
    subscribeToChannel,
    
    // Convenience methods
    isReady: connection.isConnected,
    connectionId: connection.connection?.id || null,
  };
}
