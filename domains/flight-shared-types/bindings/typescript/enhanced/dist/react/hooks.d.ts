import { MemoryUsageSnapshot, MemoryPressure, MemoryUpdate, SessionConfig, MemoryStats, MemoryAllocation } from '../types';
import { MemoryEventEmitter } from '../utils/memory-utils';
declare const memoryEventEmitter: MemoryEventEmitter;
export interface UseMemoryMonitorResult {
    snapshot: MemoryUsageSnapshot | null;
    loading: boolean;
    error: string | null;
    usagePercentage: number;
    pressure: MemoryPressure;
    pressureColor: string;
    refresh: () => Promise<void>;
}
export interface UseMemoryMonitorOptions {
    refreshInterval?: number;
    enableRealtime?: boolean;
    onUpdate?: (snapshot: MemoryUsageSnapshot) => void;
    onError?: (error: string) => void;
}
/**
 * Hook for monitoring memory usage in React applications
 *
 * @param sessionId - Session to monitor
 * @param options - Configuration options
 */
export declare function useMemoryMonitor(sessionId: string, options?: UseMemoryMonitorOptions): UseMemoryMonitorResult;
export interface UseSessionConfigResult {
    isValid: boolean;
    platform: string;
    config: SessionConfig;
    validationErrors: string[];
}
/**
 * Hook for session configuration management
 *
 * @param config - Session configuration to validate
 */
export declare function useSessionConfig(config: SessionConfig): UseSessionConfigResult;
export interface UseMemoryWebSocketOptions {
    onConnect?: () => void;
    onDisconnect?: () => void;
    onError?: (error: string) => void;
    reconnectInterval?: number;
    maxReconnectAttempts?: number;
}
export interface UseMemoryWebSocketResult {
    connected: boolean;
    error: string | null;
    reconnectAttempts: number;
    sendUpdate: (update: MemoryUpdate) => void;
    connect: () => void;
    disconnect: () => void;
}
/**
 * Hook for WebSocket memory streaming
 *
 * @param websocketUrl - WebSocket server URL
 * @param sessionId - Session ID for filtering messages
 * @param options - WebSocket options
 */
export declare function useMemoryWebSocket(websocketUrl: string, sessionId: string, options?: UseMemoryWebSocketOptions): UseMemoryWebSocketResult;
export interface UseMemoryStatsResult {
    stats: MemoryStats | null;
    loading: boolean;
    error: string | null;
    refresh: () => Promise<void>;
}
/**
 * Hook for memory statistics
 *
 * @param sessionId - Session to get stats for
 * @param refreshInterval - Auto-refresh interval (0 to disable)
 */
export declare function useMemoryStats(sessionId: string, refreshInterval?: number): UseMemoryStatsResult;
export interface UseMemoryAllocationsResult {
    allocations: MemoryAllocation[];
    loading: boolean;
    error: string | null;
    refresh: () => Promise<void>;
}
/**
 * Hook for listing memory allocations
 *
 * @param sessionId - Session to list allocations for
 */
export declare function useMemoryAllocations(sessionId: string): UseMemoryAllocationsResult;
export { memoryEventEmitter };
//# sourceMappingURL=hooks.d.ts.map