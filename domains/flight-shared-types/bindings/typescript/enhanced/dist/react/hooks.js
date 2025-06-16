// React hooks for Flight Memory integration
// Generic implementation suitable for any React application
import { useState, useEffect, useCallback, useRef } from 'react';
import { MemoryUtils, MemoryEventEmitter } from '../utils/memory-utils';
// Global event emitter instance
const memoryEventEmitter = new MemoryEventEmitter();
/**
 * Hook for monitoring memory usage in React applications
 *
 * @param sessionId - Session to monitor
 * @param options - Configuration options
 */
export function useMemoryMonitor(sessionId, options = {}) {
    const { refreshInterval = 5000, enableRealtime = true, onUpdate, onError } = options;
    const [snapshot, setSnapshot] = useState(null);
    const [loading, setLoading] = useState(true);
    const [error, setError] = useState(null);
    const intervalRef = useRef(null);
    const fetchSnapshotRef = useRef(null);
    const fetchMemorySnapshot = useCallback(async () => {
        try {
            setError(null);
            // This would call the actual memory operations API
            // For now, we'll create a placeholder implementation
            // In a real implementation, this would use the WASM component
            // const result = await MemoryOperations.getMemorySnapshot(sessionId);
            // if (result.tag === "ok") {
            //   setSnapshot(result.val);
            //   onUpdate?.(result.val);
            // } else {
            //   throw new Error(result.val.message);
            // }
            setLoading(false);
        }
        catch (err) {
            const errorMessage = err instanceof Error ? err.message : 'Unknown error';
            setError(errorMessage);
            onError?.(errorMessage);
            setLoading(false);
        }
    }, [sessionId, onUpdate, onError]);
    const refresh = useCallback(async () => {
        setLoading(true);
        await fetchMemorySnapshot();
    }, [fetchMemorySnapshot]);
    useEffect(() => {
        fetchSnapshotRef.current = fetchMemorySnapshot;
    }, [fetchMemorySnapshot]);
    useEffect(() => {
        // Initial fetch
        fetchMemorySnapshot();
        // Set up periodic refresh
        if (refreshInterval > 0) {
            intervalRef.current = setInterval(() => {
                fetchSnapshotRef.current?.();
            }, refreshInterval);
        }
        // Subscribe to real-time updates if enabled
        let unsubscribe = null;
        if (enableRealtime) {
            unsubscribe = memoryEventEmitter.subscribe(sessionId, (update) => {
                setSnapshot(update.snapshot);
                setError(null);
                setLoading(false);
                onUpdate?.(update.snapshot);
            });
        }
        return () => {
            if (intervalRef.current) {
                clearInterval(intervalRef.current);
            }
            unsubscribe?.();
        };
    }, [sessionId, refreshInterval, enableRealtime, fetchMemorySnapshot]);
    const usagePercentage = snapshot ? MemoryUtils.calculateUsagePercentage(snapshot) : 0;
    const pressure = MemoryUtils.getMemoryPressureFromPercentage(usagePercentage);
    const pressureColor = MemoryUtils.getMemoryPressureColor(pressure);
    return {
        snapshot,
        loading,
        error,
        usagePercentage,
        pressure,
        pressureColor,
        refresh
    };
}
/**
 * Hook for session configuration management
 *
 * @param config - Session configuration to validate
 */
export function useSessionConfig(config) {
    const [isValid, setIsValid] = useState(false);
    const [platform, setPlatform] = useState('');
    const [validationErrors, setValidationErrors] = useState([]);
    useEffect(() => {
        const errors = [];
        if (!config.sessionId) {
            errors.push('Session ID is required');
        }
        if (!config.userId) {
            errors.push('User ID is required');
        }
        if (!config.platform) {
            errors.push('Platform is required');
        }
        const valid = errors.length === 0 && MemoryUtils.validateSessionConfig(config);
        setIsValid(valid);
        setValidationErrors(errors);
        if (valid) {
            setPlatform(config.platform);
        }
    }, [config]);
    return {
        isValid,
        platform,
        config,
        validationErrors
    };
}
/**
 * Hook for WebSocket memory streaming
 *
 * @param websocketUrl - WebSocket server URL
 * @param sessionId - Session ID for filtering messages
 * @param options - WebSocket options
 */
export function useMemoryWebSocket(websocketUrl, sessionId, options = {}) {
    const { onConnect, onDisconnect, onError, reconnectInterval = 5000, maxReconnectAttempts = 5 } = options;
    const [connected, setConnected] = useState(false);
    const [error, setError] = useState(null);
    const [reconnectAttempts, setReconnectAttempts] = useState(0);
    const wsRef = useRef(null);
    const reconnectTimeoutRef = useRef(null);
    const shouldReconnectRef = useRef(true);
    const connect = useCallback(() => {
        if (wsRef.current?.readyState === WebSocket.OPEN) {
            return; // Already connected
        }
        try {
            const ws = new WebSocket(websocketUrl);
            wsRef.current = ws;
            ws.onopen = () => {
                setConnected(true);
                setError(null);
                setReconnectAttempts(0);
                onConnect?.();
            };
            ws.onclose = () => {
                setConnected(false);
                onDisconnect?.();
                // Auto-reconnect if enabled and under attempt limit
                if (shouldReconnectRef.current && reconnectAttempts < maxReconnectAttempts) {
                    reconnectTimeoutRef.current = setTimeout(() => {
                        setReconnectAttempts((prev) => prev + 1);
                        connect();
                    }, reconnectInterval);
                }
            };
            ws.onerror = () => {
                const errorMessage = 'WebSocket connection error';
                setError(errorMessage);
                setConnected(false);
                onError?.(errorMessage);
            };
            ws.onmessage = (event) => {
                try {
                    const update = JSON.parse(event.data);
                    if (update.sessionId === sessionId && update.type === 'memory_update') {
                        memoryEventEmitter.emit(update);
                    }
                }
                catch (err) {
                    console.error('Failed to parse WebSocket message:', err);
                }
            };
        }
        catch (err) {
            const errorMessage = err instanceof Error ? err.message : 'Failed to create WebSocket';
            setError(errorMessage);
            onError?.(errorMessage);
        }
    }, [websocketUrl, sessionId, onConnect, onDisconnect, onError, reconnectInterval, maxReconnectAttempts, reconnectAttempts]);
    const disconnect = useCallback(() => {
        shouldReconnectRef.current = false;
        if (reconnectTimeoutRef.current) {
            clearTimeout(reconnectTimeoutRef.current);
        }
        if (wsRef.current) {
            wsRef.current.close();
            wsRef.current = null;
        }
        setConnected(false);
        setReconnectAttempts(0);
    }, []);
    const sendUpdate = useCallback((update) => {
        if (wsRef.current?.readyState === WebSocket.OPEN) {
            wsRef.current.send(JSON.stringify(update));
        }
    }, []);
    useEffect(() => {
        shouldReconnectRef.current = true;
        connect();
        return () => {
            disconnect();
        };
    }, [websocketUrl, sessionId]);
    return {
        connected,
        error,
        reconnectAttempts,
        sendUpdate,
        connect,
        disconnect
    };
}
/**
 * Hook for memory statistics
 *
 * @param sessionId - Session to get stats for
 * @param refreshInterval - Auto-refresh interval (0 to disable)
 */
export function useMemoryStats(sessionId, refreshInterval = 0) {
    const [stats, setStats] = useState(null);
    const [loading, setLoading] = useState(true);
    const [error, setError] = useState(null);
    const intervalRef = useRef(null);
    const fetchStats = useCallback(async () => {
        try {
            setError(null);
            // This would call the actual memory analytics API
            // const result = await MemoryAnalytics.calculateMemoryStats(sessionId);
            // if (result.tag === "ok") {
            //   setStats(result.val);
            // } else {
            //   throw new Error(result.val.message);
            // }
            setLoading(false);
        }
        catch (err) {
            const errorMessage = err instanceof Error ? err.message : 'Unknown error';
            setError(errorMessage);
            setLoading(false);
        }
    }, [sessionId]);
    const refresh = useCallback(async () => {
        setLoading(true);
        await fetchStats();
    }, [fetchStats]);
    useEffect(() => {
        fetchStats();
        if (refreshInterval > 0) {
            intervalRef.current = setInterval(fetchStats, refreshInterval);
        }
        return () => {
            if (intervalRef.current) {
                clearInterval(intervalRef.current);
            }
        };
    }, [sessionId, refreshInterval, fetchStats]);
    return {
        stats,
        loading,
        error,
        refresh
    };
}
/**
 * Hook for listing memory allocations
 *
 * @param sessionId - Session to list allocations for
 */
export function useMemoryAllocations(sessionId) {
    const [allocations, setAllocations] = useState([]);
    const [loading, setLoading] = useState(true);
    const [error, setError] = useState(null);
    const fetchAllocations = useCallback(async () => {
        try {
            setError(null);
            // This would call the actual memory operations API
            // const result = await MemoryOperations.listAllocations(sessionId);
            // if (result.tag === "ok") {
            //   setAllocations(result.val);
            // } else {
            //   throw new Error(result.val.message);
            // }
            setLoading(false);
        }
        catch (err) {
            const errorMessage = err instanceof Error ? err.message : 'Unknown error';
            setError(errorMessage);
            setLoading(false);
        }
    }, [sessionId]);
    const refresh = useCallback(async () => {
        setLoading(true);
        await fetchAllocations();
    }, [fetchAllocations]);
    useEffect(() => {
        fetchAllocations();
    }, [sessionId, fetchAllocations]);
    return {
        allocations,
        loading,
        error,
        refresh
    };
}
// Export the global event emitter for direct use
export { memoryEventEmitter };
//# sourceMappingURL=hooks.js.map