/**
 * Flight Session React Hooks
 * 
 * React hooks for session management, state tracking, and real-time updates.
 */

import { useState, useEffect, useCallback, useRef } from 'react';
import {
  SessionInfo,
  SessionState,
  SessionType,
  SessionHealth,
  SessionResources,
  SessionStats,
  SessionEvent,
  SessionEventType,
  SessionOperations,
  SessionAnalytics,
  FlightResult,
  SessionFilterCriteria
} from '../types';
import { SessionManager } from '../utils/session-utils';

/**
 * Hook for managing a single session
 */
export function useSession(sessionId: string, operations: SessionOperations) {
  const [session, setSession] = useState<SessionInfo | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  const refreshSession = useCallback(async () => {
    try {
      setLoading(true);
      setError(null);
      const result = await operations.getSession(sessionId);
      
      if (result.tag === 'ok') {
        setSession(result.val);
      } else {
        setError(result.val.message);
        setSession(null);
      }
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Unknown error');
      setSession(null);
    } finally {
      setLoading(false);
    }
  }, [sessionId, operations]);

  useEffect(() => {
    refreshSession();
  }, [refreshSession]);

  const updateState = useCallback(async (newState: SessionState) => {
    try {
      const result = await operations.updateSessionState(sessionId, newState);
      if (result.tag === 'ok') {
        await refreshSession();
        return true;
      } else {
        setError(result.val.message);
        return false;
      }
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Unknown error');
      return false;
    }
  }, [sessionId, operations, refreshSession]);

  const terminate = useCallback(async () => {
    try {
      const result = await operations.terminateSession(sessionId);
      if (result.tag === 'ok') {
        await refreshSession();
        return true;
      } else {
        setError(result.val.message);
        return false;
      }
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Unknown error');
      return false;
    }
  }, [sessionId, operations, refreshSession]);

  const extend = useCallback(async (additionalSeconds: number) => {
    try {
      const result = await operations.extendSession(sessionId, additionalSeconds);
      if (result.tag === 'ok') {
        await refreshSession();
        return true;
      } else {
        setError(result.val.message);
        return false;
      }
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Unknown error');
      return false;
    }
  }, [sessionId, operations, refreshSession]);

  return {
    session,
    loading,
    error,
    refresh: refreshSession,
    updateState,
    terminate,
    extend,
    isExpired: session ? SessionManager.isSessionExpired(session) : false,
    duration: session ? SessionManager.getSessionDuration(session) : 0
  };
}

/**
 * Hook for monitoring session resources
 */
export function useSessionResources(
  sessionId: string, 
  operations: SessionOperations,
  refreshInterval: number = 5000
) {
  const [resources, setResources] = useState<SessionResources | null>(null);
  const [health, setHealth] = useState<SessionHealth>(SessionHealth.Unknown);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const intervalRef = useRef<NodeJS.Timeout>();

  const fetchResources = useCallback(async () => {
    try {
      setError(null);
      const [resourcesResult, healthResult] = await Promise.all([
        operations.getSessionResources(sessionId),
        operations.getSessionHealth(sessionId)
      ]);

      if (resourcesResult.tag === 'ok') {
        setResources(resourcesResult.val);
      } else {
        setError(resourcesResult.val.message);
        return;
      }

      if (healthResult.tag === 'ok') {
        setHealth(healthResult.val);
      } else {
        setHealth(SessionHealth.Unknown);
      }
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Unknown error');
    } finally {
      setLoading(false);
    }
  }, [sessionId, operations]);

  useEffect(() => {
    fetchResources();

    if (refreshInterval > 0) {
      intervalRef.current = setInterval(fetchResources, refreshInterval);
    }

    return () => {
      if (intervalRef.current) {
        clearInterval(intervalRef.current);
      }
    };
  }, [fetchResources, refreshInterval]);

  return {
    resources,
    health,
    loading,
    error,
    refresh: fetchResources
  };
}

/**
 * Hook for managing multiple sessions
 */
export function useSessions(
  operations: SessionOperations,
  criteria?: SessionFilterCriteria,
  refreshInterval: number = 10000
) {
  const [sessions, setSessions] = useState<SessionInfo[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const intervalRef = useRef<NodeJS.Timeout>();

  const fetchSessions = useCallback(async () => {
    try {
      setError(null);
      const result = await operations.listSessions(
        criteria?.userId,
        criteria?.sessionType,
        criteria?.platform
      );

      if (result.tag === 'ok') {
        let filteredSessions = result.val;
        
        // Apply additional filtering if criteria provided
        if (criteria) {
          filteredSessions = SessionManager.filterSessions(filteredSessions, criteria);
        }
        
        setSessions(filteredSessions);
      } else {
        setError(result.val.message);
        setSessions([]);
      }
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Unknown error');
      setSessions([]);
    } finally {
      setLoading(false);
    }
  }, [operations, criteria]);

  useEffect(() => {
    fetchSessions();

    if (refreshInterval > 0) {
      intervalRef.current = setInterval(fetchSessions, refreshInterval);
    }

    return () => {
      if (intervalRef.current) {
        clearInterval(intervalRef.current);
      }
    };
  }, [fetchSessions, refreshInterval]);

  const createSession = useCallback(async (
    sessionType: SessionType,
    platform: string,
    userId?: string,
    config?: any
  ) => {
    try {
      const result = await operations.createSession(sessionType, platform, userId, config);
      if (result.tag === 'ok') {
        await fetchSessions(); // Refresh the list
        return result.val;
      } else {
        setError(result.val.message);
        return null;
      }
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Unknown error');
      return null;
    }
  }, [operations, fetchSessions]);

  return {
    sessions,
    loading,
    error,
    refresh: fetchSessions,
    createSession,
    activeSessions: sessions.filter(s => s.state === SessionState.Active),
    expiredSessions: sessions.filter(s => SessionManager.isSessionExpired(s))
  };
}

/**
 * Hook for session analytics and statistics
 */
export function useSessionAnalytics(
  analytics: SessionAnalytics,
  refreshInterval: number = 30000
) {
  const [stats, setStats] = useState<SessionStats | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const intervalRef = useRef<NodeJS.Timeout>();

  const fetchStats = useCallback(async () => {
    try {
      setError(null);
      const result = await analytics.getSessionStats();

      if (result.tag === 'ok') {
        setStats(result.val);
      } else {
        setError(result.val.message);
        setStats(null);
      }
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Unknown error');
      setStats(null);
    } finally {
      setLoading(false);
    }
  }, [analytics]);

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
  }, [fetchStats, refreshInterval]);

  return {
    stats,
    loading,
    error,
    refresh: fetchStats
  };
}

/**
 * Hook for session events monitoring
 */
export function useSessionEvents(
  sessionId: string,
  operations: SessionOperations,
  limit: number = 50
) {
  const [events, setEvents] = useState<SessionEvent[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  const fetchEvents = useCallback(async () => {
    try {
      setError(null);
      const result = await operations.getSessionEvents(sessionId, limit);

      if (result.tag === 'ok') {
        setEvents(result.val.sort((a, b) => b.timestamp - a.timestamp));
      } else {
        setError(result.val.message);
        setEvents([]);
      }
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Unknown error');
      setEvents([]);
    } finally {
      setLoading(false);
    }
  }, [sessionId, operations, limit]);

  const recordEvent = useCallback(async (
    eventType: SessionEventType,
    message: string,
    data: Array<[string, string]> = []
  ) => {
    try {
      const result = await operations.recordSessionEvent(sessionId, eventType, message, data);
      if (result.tag === 'ok') {
        await fetchEvents(); // Refresh events
        return true;
      } else {
        setError(result.val.message);
        return false;
      }
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Unknown error');
      return false;
    }
  }, [sessionId, operations, fetchEvents]);

  useEffect(() => {
    fetchEvents();
  }, [fetchEvents]);

  return {
    events,
    loading,
    error,
    refresh: fetchEvents,
    recordEvent,
    recentEvents: events.slice(0, 10),
    errorEvents: events.filter(e => e.eventType === SessionEventType.ErrorOccurred)
  };
}

/**
 * Hook for session state management with validation
 */
export function useSessionState(initialState: SessionState = SessionState.Initializing) {
  const [currentState, setCurrentState] = useState(initialState);
  const [validTransitions, setValidTransitions] = useState<SessionState[]>([]);

  useEffect(() => {
    setValidTransitions(SessionManager.getValidNextStates(currentState));
  }, [currentState]);

  const transitionTo = useCallback((newState: SessionState) => {
    if (SessionManager.validateStateTransition(currentState, newState)) {
      setCurrentState(newState);
      return true;
    }
    return false;
  }, [currentState]);

  const canTransitionTo = useCallback((state: SessionState) => {
    return SessionManager.validateStateTransition(currentState, state);
  }, [currentState]);

  return {
    currentState,
    validTransitions,
    transitionTo,
    canTransitionTo,
    isTerminal: currentState === SessionState.Terminated,
    isActive: currentState === SessionState.Active,
    isError: currentState === SessionState.Error
  };
}

/**
 * Hook for real-time session monitoring with WebSocket-like updates
 */
export function useRealtimeSession(
  sessionId: string,
  operations: SessionOperations,
  onStateChange?: (newState: SessionState) => void,
  onHealthChange?: (newHealth: SessionHealth) => void
) {
  const [session, setSession] = useState<SessionInfo | null>(null);
  const [resources, setResources] = useState<SessionResources | null>(null);
  const [health, setHealth] = useState<SessionHealth>(SessionHealth.Unknown);
  const [lastUpdate, setLastUpdate] = useState<number>(Date.now());
  
  const previousState = useRef<SessionState>();
  const previousHealth = useRef<SessionHealth>();

  const update = useCallback(async () => {
    try {
      const [sessionResult, resourcesResult, healthResult] = await Promise.all([
        operations.getSession(sessionId),
        operations.getSessionResources(sessionId),
        operations.getSessionHealth(sessionId)
      ]);

      if (sessionResult.tag === 'ok') {
        const newSession = sessionResult.val;
        setSession(newSession);
        
        // Check for state change
        if (previousState.current !== undefined && 
            previousState.current !== newSession.state &&
            onStateChange) {
          onStateChange(newSession.state);
        }
        previousState.current = newSession.state;
      }

      if (resourcesResult.tag === 'ok') {
        setResources(resourcesResult.val);
      }

      if (healthResult.tag === 'ok') {
        const newHealth = healthResult.val;
        setHealth(newHealth);
        
        // Check for health change
        if (previousHealth.current !== undefined &&
            previousHealth.current !== newHealth &&
            onHealthChange) {
          onHealthChange(newHealth);
        }
        previousHealth.current = newHealth;
      }

      setLastUpdate(Date.now());
    } catch (err) {
      console.error('Failed to update session data:', err);
    }
  }, [sessionId, operations, onStateChange, onHealthChange]);

  useEffect(() => {
    update();
    const interval = setInterval(update, 2000); // Update every 2 seconds
    
    return () => clearInterval(interval);
  }, [update]);

  return {
    session,
    resources,
    health,
    lastUpdate,
    refresh: update
  };
}
