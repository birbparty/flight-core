/**
 * React Hooks for V6R Authentication
 * Provides React integration for authentication state management
 */

import { useState, useEffect, useCallback, useMemo, useContext, createContext } from 'react';
import {
  AuthContext as AuthContextType,
  AuthConfig,
  AuthState,
  UserRole,
  Permission,
  V6RSubscriptionTier,
  AuthResult,
  AuthToken,
} from '../types';
import { V6RAuthManager } from '../v6r-auth-manager';

// React Context for authentication
const AuthContext = createContext<{
  authManager: V6RAuthManager | null;
  authContext: AuthContextType | null;
}>({
  authManager: null,
  authContext: null,
});

// Auth Provider component
export interface AuthProviderProps {
  config: AuthConfig;
  children: React.ReactNode;
}

export function AuthProvider({ config, children }: AuthProviderProps) {
  const [authManager] = useState(() => new V6RAuthManager(config));
  const [authContext, setAuthContext] = useState<AuthContextType | null>(
    authManager.getCurrentAuth()
  );

  useEffect(() => {
    // Set up auth state listener
    const interval = setInterval(() => {
      const currentAuth = authManager.getCurrentAuth();
      setAuthContext(currentAuth);
    }, 1000); // Check every second

    return () => clearInterval(interval);
  }, [authManager]);

  const contextValue = useMemo(() => ({
    authManager,
    authContext,
  }), [authManager, authContext]);

  return (
    <AuthContext.Provider value={contextValue}>
      {children}
    </AuthContext.Provider>
  );
}

// Hook to use authentication context
export function useAuth() {
  const context = useContext(AuthContext);
  if (!context.authManager) {
    throw new Error('useAuth must be used within an AuthProvider');
  }
  return context;
}

// Hook for authentication state
export function useAuthState() {
  const { authContext } = useAuth();
  
  return useMemo(() => ({
    isAuthenticated: authContext?.state === AuthState.Authenticated,
    isLoading: authContext?.state === AuthState.Authenticating,
    isExpired: authContext?.state === AuthState.Expired,
    isRevoked: authContext?.state === AuthState.Revoked,
    hasError: authContext?.state === AuthState.Error,
    state: authContext?.state || AuthState.Unauthenticated,
  }), [authContext?.state]);
}

// Hook for user information
export function useUser() {
  const { authContext } = useAuth();
  
  return useMemo(() => ({
    user: authContext?.userAuth || null,
    userId: authContext?.userAuth?.userId,
    username: authContext?.userAuth?.username,
    provider: authContext?.userAuth?.provider,
    platform: authContext?.userAuth?.platform,
    authenticatedAt: authContext?.userAuth?.authenticatedAt,
    lastActivity: authContext?.userAuth?.lastActivity,
  }), [authContext?.userAuth]);
}

// Hook for user permissions
export function usePermissions() {
  const { authManager, authContext } = useAuth();
  
  const permissions = useMemo(() => 
    authContext?.userAuth?.permissions || [], 
    [authContext?.userAuth?.permissions]
  );

  const hasPermission = useCallback((permission: Permission) => {
    return authManager.hasPermission(permission);
  }, [authManager]);

  const hasAnyPermission = useCallback((permissionList: Permission[]) => {
    return permissionList.some(permission => authManager.hasPermission(permission));
  }, [authManager]);

  const hasAllPermissions = useCallback((permissionList: Permission[]) => {
    return permissionList.every(permission => authManager.hasPermission(permission));
  }, [authManager]);

  return {
    permissions,
    hasPermission,
    hasAnyPermission,
    hasAllPermissions,
  };
}

// Hook for user roles
export function useRoles() {
  const { authManager, authContext } = useAuth();
  
  const roles = useMemo(() => 
    authContext?.userAuth?.roles || [], 
    [authContext?.userAuth?.roles]
  );

  const hasRole = useCallback((role: UserRole) => {
    return authManager.hasRole(role);
  }, [authManager]);

  const hasAnyRole = useCallback((roleList: UserRole[]) => {
    return roleList.some(role => authManager.hasRole(role));
  }, [authManager]);

  const hasAllRoles = useCallback((roleList: UserRole[]) => {
    return roleList.every(role => authManager.hasRole(role));
  }, [authManager]);

  return {
    roles,
    hasRole,
    hasAnyRole,
    hasAllRoles,
  };
}

// Hook for V6R subscription information
export function useV6RSubscription() {
  const { authManager, authContext } = useAuth();
  
  const subscriptionTier = useMemo(() => 
    authManager.getSubscriptionTier(), 
    [authManager, authContext]
  );

  const isSubscriptionTier = useCallback((tier: V6RSubscriptionTier) => {
    return subscriptionTier === tier;
  }, [subscriptionTier]);

  const hasMinimumTier = useCallback((minimumTier: V6RSubscriptionTier) => {
    if (!subscriptionTier) return false;
    
    const tierOrder = [
      V6RSubscriptionTier.Free,
      V6RSubscriptionTier.Individual,
      V6RSubscriptionTier.Team,
      V6RSubscriptionTier.Enterprise,
      V6RSubscriptionTier.Custom,
    ];
    
    const currentIndex = tierOrder.indexOf(subscriptionTier);
    const minimumIndex = tierOrder.indexOf(minimumTier);
    
    return currentIndex >= minimumIndex;
  }, [subscriptionTier]);

  return {
    subscriptionTier,
    isSubscriptionTier,
    hasMinimumTier,
    isFree: subscriptionTier === V6RSubscriptionTier.Free,
    isIndividual: subscriptionTier === V6RSubscriptionTier.Individual,
    isTeam: subscriptionTier === V6RSubscriptionTier.Team,
    isEnterprise: subscriptionTier === V6RSubscriptionTier.Enterprise,
    isCustom: subscriptionTier === V6RSubscriptionTier.Custom,
  };
}

// Hook for authentication actions
export function useAuthActions() {
  const { authManager } = useAuth();
  const [isLoading, setIsLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const login = useCallback(async (username: string, password: string, platform?: string) => {
    setIsLoading(true);
    setError(null);
    
    try {
      const result = await authManager.authenticateUser(username, password, platform);
      if (!result.success) {
        setError(result.error.message);
        return result;
      }
      return result;
    } finally {
      setIsLoading(false);
    }
  }, [authManager]);

  const logout = useCallback(() => {
    setError(null);
    authManager.logout();
  }, [authManager]);

  const refreshToken = useCallback(async () => {
    setIsLoading(true);
    setError(null);
    
    try {
      const result = await authManager.refreshToken();
      if (!result.success) {
        setError(result.error.message);
      }
      return result;
    } finally {
      setIsLoading(false);
    }
  }, [authManager]);

  const revokeToken = useCallback(async () => {
    setIsLoading(true);
    setError(null);
    
    try {
      const result = await authManager.revokeToken();
      if (!result.success) {
        setError(result.error.message);
      }
      return result;
    } finally {
      setIsLoading(false);
    }
  }, [authManager]);

  return {
    login,
    logout,
    refreshToken,
    revokeToken,
    isLoading,
    error,
    clearError: () => setError(null),
  };
}

// Hook for token information
export function useToken() {
  const { authContext } = useAuth();
  
  return useMemo(() => {
    const token = authContext?.token;
    if (!token) {
      return {
        token: null,
        isValid: false,
        expiresAt: null,
        expiresIn: null,
        scopes: [],
        hasScope: () => false,
      };
    }

    const expiresIn = Math.max(0, token.expiresAt - Date.now());
    const isValid = expiresIn > 0;

    return {
      token,
      isValid,
      expiresAt: new Date(token.expiresAt),
      expiresIn,
      scopes: token.scopes,
      hasScope: (scope: string) => token.scopes.includes(scope),
    };
  }, [authContext?.token]);
}

// Higher-order component for protected routes
export interface ProtectedRouteProps {
  children: React.ReactNode;
  requiredRoles?: UserRole[];
  requiredPermissions?: Permission[];
  minimumSubscriptionTier?: V6RSubscriptionTier;
  fallback?: React.ReactNode;
  onUnauthorized?: () => void;
}

export function ProtectedRoute({
  children,
  requiredRoles = [],
  requiredPermissions = [],
  minimumSubscriptionTier,
  fallback = <div>Access denied</div>,
  onUnauthorized,
}: ProtectedRouteProps) {
  const { isAuthenticated } = useAuthState();
  const { hasAllRoles } = useRoles();
  const { hasAllPermissions } = usePermissions();
  const { hasMinimumTier } = useV6RSubscription();

  const hasAccess = useMemo(() => {
    if (!isAuthenticated) return false;
    
    if (requiredRoles.length > 0 && !hasAllRoles(requiredRoles)) {
      return false;
    }
    
    if (requiredPermissions.length > 0 && !hasAllPermissions(requiredPermissions)) {
      return false;
    }
    
    if (minimumSubscriptionTier && !hasMinimumTier(minimumSubscriptionTier)) {
      return false;
    }
    
    return true;
  }, [
    isAuthenticated,
    requiredRoles,
    requiredPermissions,
    minimumSubscriptionTier,
    hasAllRoles,
    hasAllPermissions,
    hasMinimumTier,
  ]);

  useEffect(() => {
    if (!hasAccess && onUnauthorized) {
      onUnauthorized();
    }
  }, [hasAccess, onUnauthorized]);

  if (!hasAccess) {
    return <>{fallback}</>;
  }

  return <>{children}</>;
}

// Hook for conditional rendering based on permissions
export function useConditionalRender() {
  const { hasRole, hasAnyRole, hasAllRoles } = useRoles();
  const { hasPermission, hasAnyPermission, hasAllPermissions } = usePermissions();
  const { hasMinimumTier } = useV6RSubscription();
  const { isAuthenticated } = useAuthState();

  const renderIf = useCallback((conditions: {
    authenticated?: boolean;
    roles?: UserRole[];
    anyRole?: UserRole[];
    permissions?: Permission[];
    anyPermission?: Permission[];
    minimumTier?: V6RSubscriptionTier;
  }) => {
    if (conditions.authenticated !== undefined && isAuthenticated !== conditions.authenticated) {
      return false;
    }
    
    if (conditions.roles && !hasAllRoles(conditions.roles)) {
      return false;
    }
    
    if (conditions.anyRole && !hasAnyRole(conditions.anyRole)) {
      return false;
    }
    
    if (conditions.permissions && !hasAllPermissions(conditions.permissions)) {
      return false;
    }
    
    if (conditions.anyPermission && !hasAnyPermission(conditions.anyPermission)) {
      return false;
    }
    
    if (conditions.minimumTier && !hasMinimumTier(conditions.minimumTier)) {
      return false;
    }
    
    return true;
  }, [
    isAuthenticated,
    hasRole,
    hasAnyRole,
    hasAllRoles,
    hasPermission,
    hasAnyPermission,
    hasAllPermissions,
    hasMinimumTier,
  ]);

  return { renderIf };
}
