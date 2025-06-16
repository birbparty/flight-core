/**
 * V6R Cloud Authentication Manager
 * OAuth2 and JWT token handling for V6R cloud services
 */

import jwt from 'jsonwebtoken';
import axios, { AxiosInstance } from 'axios';
import {
  AuthConfig,
  AuthContext,
  AuthToken,
  UserAuth,
  AuthProvider,
  AuthState,
  TokenType,
  UserRole,
  Permission,
  V6RSubscriptionTier,
  V6ROAuth2Response,
  AuthResult,
  AuthError,
  JWTPayload,
  V6RResourceQuotas,
} from './types';

export class V6RAuthManager {
  private config: AuthConfig;
  private httpClient: AxiosInstance;
  private currentAuth: AuthContext | null = null;
  private refreshTimer: NodeJS.Timeout | null = null;

  constructor(config: AuthConfig) {
    this.config = {
      autoRefresh: true,
      refreshThreshold: 300, // 5 minutes before expiry
      storageKey: 'v6r-auth-context',
      ...config,
    };

    this.httpClient = axios.create({
      baseURL: this.config.v6rApiUrl,
      timeout: 30000,
      headers: {
        'Content-Type': 'application/json',
        'User-Agent': 'Flight-Core-V6R-Client/1.0.0',
      },
    });

    // Load existing auth context from storage
    this.loadAuthContext();
  }

  /**
   * Authenticate user with V6R OAuth2
   */
  async authenticateUser(
    username: string,
    password: string,
    platform: string = 'v6r-cloud'
  ): Promise<AuthResult<AuthContext>> {
    try {
      const authResponse = await this.performOAuth2Flow(username, password);
      
      const userAuth: UserAuth = {
        userId: authResponse.userId,
        username: authResponse.username,
        provider: AuthProvider.V6RCloud,
        roles: this.mapSubscriptionToRoles(authResponse.subscriptionTier),
        permissions: this.mapSubscriptionToPermissions(authResponse.subscriptionTier),
        authenticatedAt: Date.now(),
        lastActivity: Date.now(),
        platform,
        metadata: [
          ['subscription_tier', authResponse.subscriptionTier],
          ['organization_id', authResponse.organizationId || ''],
          ['auth_method', 'oauth2']
        ]
      };

      const token: AuthToken = {
        token: authResponse.accessToken,
        tokenType: TokenType.Jwt,
        expiresAt: Date.now() + (authResponse.expiresIn * 1000),
        refreshToken: authResponse.refreshToken,
        scopes: authResponse.scopes,
        issuer: 'v6r-auth-service',
        platform: undefined, // V6R tokens work across platforms
        metadata: [
          ['token_source', 'oauth2'],
          ['subscription_tier', authResponse.subscriptionTier]
        ]
      };

      const authContext: AuthContext = {
        state: AuthState.Authenticated,
        userAuth,
        token,
        sessionId: `v6r-session-${userAuth.userId}-${Date.now()}`,
        platform,
        provider: AuthProvider.V6RCloud,
        metadata: [
          ['auth_method', 'oauth2'],
          ['client_id', this.config.clientId]
        ]
      };

      this.currentAuth = authContext;
      this.saveAuthContext();
      this.scheduleTokenRefresh();

      return { success: true, data: authContext };

    } catch (error) {
      const authError: AuthError = {
        code: 'AUTH_FAILED',
        message: `V6R authentication failed: ${error.message}`,
        details: error,
      };
      return { success: false, error: authError };
    }
  }

  /**
   * Validate authentication token
   */
  async validateToken(token: string, platform: string): Promise<AuthResult<AuthContext>> {
    try {
      const validation = await this.performTokenValidation(token);
      
      if (!validation.valid) {
        const authError: AuthError = {
          code: 'INVALID_TOKEN',
          message: 'Token validation failed',
          details: validation.reason,
        };
        return { success: false, error: authError };
      }

      // Return existing auth context if token matches
      if (this.currentAuth?.token?.token === token) {
        return { success: true, data: this.currentAuth };
      }

      // Create new auth context from token validation
      const userAuth: UserAuth = {
        userId: validation.userId,
        username: validation.username,
        provider: AuthProvider.V6RCloud,
        roles: validation.roles,
        permissions: validation.permissions,
        authenticatedAt: validation.issuedAt,
        lastActivity: Date.now(),
        platform,
        metadata: validation.metadata
      };

      const authContext: AuthContext = {
        state: AuthState.Authenticated,
        userAuth,
        token: {
          token,
          tokenType: TokenType.Jwt,
          expiresAt: validation.expiresAt,
          refreshToken: validation.refreshToken,
          scopes: validation.scopes,
          issuer: validation.issuer,
          platform: undefined,
          metadata: []
        },
        sessionId: validation.sessionId,
        platform,
        provider: AuthProvider.V6RCloud,
        metadata: [
          ['validation_source', 'token_introspection']
        ]
      };

      this.currentAuth = authContext;
      this.saveAuthContext();

      return { success: true, data: authContext };

    } catch (error) {
      const authError: AuthError = {
        code: 'TOKEN_VALIDATION_FAILED',
        message: `Token validation failed: ${error.message}`,
        details: error,
      };
      return { success: false, error: authError };
    }
  }

  /**
   * Refresh authentication token
   */
  async refreshToken(): Promise<AuthResult<AuthToken>> {
    if (!this.currentAuth?.token?.refreshToken) {
      const authError: AuthError = {
        code: 'NO_REFRESH_TOKEN',
        message: 'No refresh token available',
      };
      return { success: false, error: authError };
    }

    try {
      const response = await this.httpClient.post('/auth/refresh', {
        refresh_token: this.currentAuth.token.refreshToken,
        client_id: this.config.clientId,
        client_secret: this.config.clientSecret,
      });

      const tokenData = response.data;
      const newToken: AuthToken = {
        token: tokenData.access_token,
        tokenType: TokenType.Jwt,
        expiresAt: Date.now() + (tokenData.expires_in * 1000),
        refreshToken: tokenData.refresh_token || this.currentAuth.token.refreshToken,
        scopes: tokenData.scope ? tokenData.scope.split(' ') : this.currentAuth.token.scopes,
        issuer: 'v6r-auth-service',
        platform: undefined,
        metadata: [
          ['token_source', 'refresh'],
          ['refreshed_at', Date.now().toString()]
        ]
      };

      // Update current auth context
      if (this.currentAuth) {
        this.currentAuth.token = newToken;
        this.currentAuth.state = AuthState.Authenticated;
        if (this.currentAuth.userAuth) {
          this.currentAuth.userAuth.lastActivity = Date.now();
        }
        this.saveAuthContext();
        this.scheduleTokenRefresh();
      }

      return { success: true, data: newToken };

    } catch (error) {
      // Mark auth as expired if refresh fails
      if (this.currentAuth) {
        this.currentAuth.state = AuthState.Expired;
        this.saveAuthContext();
      }

      const authError: AuthError = {
        code: 'REFRESH_FAILED',
        message: `Token refresh failed: ${error.message}`,
        details: error,
      };
      return { success: false, error: authError };
    }
  }

  /**
   * Revoke authentication token
   */
  async revokeToken(): Promise<AuthResult<boolean>> {
    if (!this.currentAuth?.token) {
      return { success: true, data: true }; // Already revoked
    }

    try {
      await this.httpClient.post('/auth/revoke', {
        token: this.currentAuth.token.token,
        client_id: this.config.clientId,
      });

      this.logout();
      return { success: true, data: true };

    } catch (error) {
      const authError: AuthError = {
        code: 'REVOKE_FAILED',
        message: `Token revocation failed: ${error.message}`,
        details: error,
      };
      return { success: false, error: authError };
    }
  }

  /**
   * Get current authentication context
   */
  getCurrentAuth(): AuthContext | null {
    return this.currentAuth;
  }

  /**
   * Check if user is authenticated
   */
  isAuthenticated(): boolean {
    return this.currentAuth?.state === AuthState.Authenticated &&
           this.isTokenValid();
  }

  /**
   * Check if token is still valid
   */
  isTokenValid(): boolean {
    if (!this.currentAuth?.token) return false;
    return Date.now() < this.currentAuth.token.expiresAt;
  }

  /**
   * Logout user
   */
  logout(): void {
    if (this.refreshTimer) {
      clearTimeout(this.refreshTimer);
      this.refreshTimer = null;
    }

    this.currentAuth = null;
    this.clearStoredAuthContext();
  }

  /**
   * Get user permissions
   */
  getUserPermissions(): Permission[] {
    return this.currentAuth?.userAuth?.permissions || [];
  }

  /**
   * Check if user has specific permission
   */
  hasPermission(permission: Permission): boolean {
    return this.getUserPermissions().includes(permission);
  }

  /**
   * Get user roles
   */
  getUserRoles(): UserRole[] {
    return this.currentAuth?.userAuth?.roles || [];
  }

  /**
   * Check if user has specific role
   */
  hasRole(role: UserRole): boolean {
    return this.getUserRoles().includes(role);
  }

  /**
   * Get subscription tier
   */
  getSubscriptionTier(): V6RSubscriptionTier | null {
    const metadata = this.currentAuth?.userAuth?.metadata || [];
    const tierEntry = metadata.find(([key]) => key === 'subscription_tier');
    return tierEntry ? tierEntry[1] as V6RSubscriptionTier : null;
  }

  /**
   * Private methods
   */

  private async performOAuth2Flow(username: string, password: string): Promise<V6ROAuth2Response> {
    const response = await this.httpClient.post('/auth/token', {
      grant_type: 'password',
      username,
      password,
      client_id: this.config.clientId,
      client_secret: this.config.clientSecret,
      scope: this.config.scopes.join(' '),
    });

    return this.transformOAuth2Response(response.data);
  }

  private transformOAuth2Response(data: any): V6ROAuth2Response {
    // Decode JWT to extract user info
    const payload = jwt.decode(data.access_token) as JWTPayload;
    
    return {
      accessToken: data.access_token,
      refreshToken: data.refresh_token,
      expiresIn: data.expires_in,
      tokenType: data.token_type,
      scopes: data.scope ? data.scope.split(' ') : [],
      userId: payload.sub,
      username: payload.sub, // V6R uses sub as username
      subscriptionTier: payload.permissions?.includes(Permission.Admin) ? 'enterprise' : 'individual',
      organizationId: data.organization_id,
    };
  }

  private async performTokenValidation(token: string): Promise<{
    valid: boolean;
    reason?: string;
    userId: string;
    username: string;
    roles: UserRole[];
    permissions: Permission[];
    issuedAt: number;
    expiresAt: number;
    refreshToken?: string;
    scopes: string[];
    issuer: string;
    sessionId?: string;
    metadata: [string, string][];
  }> {
    try {
      // Decode JWT without verification for basic info
      const payload = jwt.decode(token) as JWTPayload;
      
      if (!payload) {
        return { valid: false, reason: 'Invalid token format', userId: '', username: '', roles: [], permissions: [], issuedAt: 0, expiresAt: 0, scopes: [], issuer: '', metadata: [] };
      }

      // Check expiration
      if (payload.exp && Date.now() / 1000 > payload.exp) {
        return { valid: false, reason: 'Token expired', userId: '', username: '', roles: [], permissions: [], issuedAt: 0, expiresAt: 0, scopes: [], issuer: '', metadata: [] };
      }

      return {
        valid: true,
        userId: payload.sub,
        username: payload.sub,
        roles: payload.roles || [UserRole.User],
        permissions: payload.permissions || [Permission.Read],
        issuedAt: (payload.iat || 0) * 1000,
        expiresAt: (payload.exp || 0) * 1000,
        scopes: payload.scope ? payload.scope.split(' ') : [],
        issuer: payload.iss || 'v6r-auth-service',
        metadata: [
          ['token_type', 'jwt'],
          ['platform', payload.platform || 'v6r-cloud']
        ]
      };

    } catch (error) {
      return { valid: false, reason: `Token validation error: ${error.message}`, userId: '', username: '', roles: [], permissions: [], issuedAt: 0, expiresAt: 0, scopes: [], issuer: '', metadata: [] };
    }
  }

  private mapSubscriptionToRoles(subscriptionTier: string): UserRole[] {
    switch (subscriptionTier) {
      case 'free': return [UserRole.User];
      case 'individual': return [UserRole.User, UserRole.Developer];
      case 'team': return [UserRole.User, UserRole.Developer, UserRole.VmManager];
      case 'enterprise': return [UserRole.User, UserRole.Developer, UserRole.VmManager, UserRole.Admin];
      default: return [UserRole.Guest];
    }
  }

  private mapSubscriptionToPermissions(subscriptionTier: string): Permission[] {
    switch (subscriptionTier) {
      case 'free': return [Permission.Read];
      case 'individual': return [Permission.Read, Permission.Write, Permission.Create];
      case 'team': return [
        Permission.Read, Permission.Write, Permission.Create,
        Permission.ManageVms, Permission.ViewMetrics,
      ];
      case 'enterprise': return [
        Permission.Read, Permission.Write, Permission.Create, Permission.Delete,
        Permission.ManageVms, Permission.ManageSessions, Permission.ViewMetrics,
        Permission.Admin,
      ];
      default: return [Permission.Read];
    }
  }

  private scheduleTokenRefresh(): void {
    if (!this.config.autoRefresh || !this.currentAuth?.token) return;

    if (this.refreshTimer) {
      clearTimeout(this.refreshTimer);
    }

    const timeToRefresh = this.currentAuth.token.expiresAt - Date.now() - (this.config.refreshThreshold! * 1000);
    
    if (timeToRefresh > 0) {
      this.refreshTimer = setTimeout(() => {
        this.refreshToken().catch(error => {
          console.error('Automatic token refresh failed:', error);
        });
      }, timeToRefresh);
    }
  }

  private saveAuthContext(): void {
    if (typeof window !== 'undefined' && this.currentAuth) {
      try {
        localStorage.setItem(this.config.storageKey!, JSON.stringify(this.currentAuth));
      } catch (error) {
        console.warn('Failed to save auth context to storage:', error);
      }
    }
  }

  private loadAuthContext(): void {
    if (typeof window !== 'undefined') {
      try {
        const stored = localStorage.getItem(this.config.storageKey!);
        if (stored) {
          const authContext = JSON.parse(stored) as AuthContext;
          
          // Check if stored context is still valid
          if (authContext.token && Date.now() < authContext.token.expiresAt) {
            this.currentAuth = authContext;
            this.scheduleTokenRefresh();
          } else {
            this.clearStoredAuthContext();
          }
        }
      } catch (error) {
        console.warn('Failed to load auth context from storage:', error);
        this.clearStoredAuthContext();
      }
    }
  }

  private clearStoredAuthContext(): void {
    if (typeof window !== 'undefined') {
      try {
        localStorage.removeItem(this.config.storageKey!);
      } catch (error) {
        console.warn('Failed to clear auth context from storage:', error);
      }
    }
  }
}

// Export singleton instance factory
export function createV6RAuthManager(config: AuthConfig): V6RAuthManager {
  return new V6RAuthManager(config);
}
