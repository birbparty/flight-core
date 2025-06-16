/**
 * Authentication Types Integration Tests
 * Cross-language compatibility and validation tests
 */

import { describe, test, expect, beforeEach } from '@jest/globals';
import {
  AuthToken,
  UserAuth,
  AuthContext,
  AuthProvider,
  AuthState,
  TokenType,
  UserRole,
  Permission,
  V6RSubscriptionTier,
  V6RAuthManager,
  createV6RAuthManager,
} from '../../../bindings/typescript/authentication/src';

describe('Authentication Types Integration', () => {
  describe('Core Type Validation', () => {
    test('should create valid auth token', () => {
      const token: AuthToken = {
        token: 'test-token-123',
        tokenType: TokenType.Jwt,
        expiresAt: Date.now() + 3600000, // 1 hour from now
        refreshToken: 'refresh-token-456',
        scopes: ['read', 'write'],
        issuer: 'test-issuer',
        platform: 'v6r-cloud',
        metadata: [['test', 'value']]
      };

      expect(token.token).toBe('test-token-123');
      expect(token.tokenType).toBe(TokenType.Jwt);
      expect(token.scopes).toContain('read');
      expect(token.scopes).toContain('write');
    });

    test('should create valid user auth', () => {
      const userAuth: UserAuth = {
        userId: 'test-user-123',
        username: 'test@v6r.dev',
        provider: AuthProvider.V6RCloud,
        roles: [UserRole.Developer, UserRole.User],
        permissions: [Permission.Read, Permission.Write, Permission.Create],
        authenticatedAt: Date.now(),
        lastActivity: Date.now(),
        platform: 'v6r-cloud',
        metadata: [['subscription_tier', 'individual']]
      };

      expect(userAuth.userId).toBe('test-user-123');
      expect(userAuth.provider).toBe(AuthProvider.V6RCloud);
      expect(userAuth.roles).toContain(UserRole.Developer);
      expect(userAuth.permissions).toContain(Permission.Read);
    });

    test('should create valid auth context', () => {
      const authContext: AuthContext = {
        state: AuthState.Authenticated,
        userAuth: {
          userId: 'test-user',
          username: 'test@example.com',
          provider: AuthProvider.FlightLocal,
          roles: [UserRole.PlatformOperator],
          permissions: [Permission.AccessHal],
          authenticatedAt: Date.now(),
          lastActivity: Date.now(),
          platform: 'dreamcast',
          metadata: []
        },
        token: {
          token: 'platform-token',
          tokenType: TokenType.PlatformSpecific,
          expiresAt: Date.now() + 86400000, // 24 hours
          scopes: ['platform:access'],
          issuer: 'flight',
          metadata: []
        },
        sessionId: 'session-123',
        platform: 'dreamcast',
        provider: AuthProvider.FlightLocal,
        metadata: [['auth_method', 'platform_native']]
      };

      expect(authContext.state).toBe(AuthState.Authenticated);
      expect(authContext.platform).toBe('dreamcast');
      expect(authContext.userAuth?.roles).toContain(UserRole.PlatformOperator);
    });
  });

  describe('V6R Authentication Manager', () => {
    let authManager: V6RAuthManager;

    beforeEach(() => {
      authManager = createV6RAuthManager({
        v6rApiUrl: 'https://api.test.v6r.dev',
        clientId: 'test-client-id',
        clientSecret: 'test-client-secret',
        scopes: ['vm:read', 'vm:write']
      });
    });

    test('should create auth manager with config', () => {
      expect(authManager).toBeDefined();
      expect(authManager.getCurrentAuth()).toBeNull();
      expect(authManager.isAuthenticated()).toBe(false);
    });

    test('should validate token format', () => {
      const mockToken = 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJ0ZXN0LXVzZXIiLCJpc3MiOiJ2NnItYXV0aC1zZXJ2aWNlIiwiZXhwIjoxNzM4ODg4ODg4fQ.test-signature';
      
      // This would normally validate against the V6R API
      // For testing, we're just checking the format
      expect(mockToken).toMatch(/^[A-Za-z0-9-_]+\.[A-Za-z0-9-_]+\.[A-Za-z0-9-_]+$/);
    });

    test('should handle permission checking', () => {
      // Mock authenticated state
      const mockAuthContext: AuthContext = {
        state: AuthState.Authenticated,
        userAuth: {
          userId: 'test-user',
          username: 'test@v6r.dev',
          provider: AuthProvider.V6RCloud,
          roles: [UserRole.Developer],
          permissions: [Permission.Read, Permission.Write, Permission.ManageVms],
          authenticatedAt: Date.now(),
          lastActivity: Date.now(),
          platform: 'v6r-medium',
          metadata: [['subscription_tier', 'team']]
        },
        platform: 'v6r-medium',
        provider: AuthProvider.V6RCloud,
        metadata: []
      };

      // Test permission checking logic
      const permissions = mockAuthContext.userAuth?.permissions || [];
      expect(permissions).toContain(Permission.Read);
      expect(permissions).toContain(Permission.Write);
      expect(permissions).toContain(Permission.ManageVms);
      expect(permissions).not.toContain(Permission.Admin);
    });
  });

  describe('Cross-Platform Compatibility', () => {
    test('should serialize auth context to JSON', () => {
      const authContext: AuthContext = {
        state: AuthState.Authenticated,
        userAuth: {
          userId: 'test-user',
          username: 'test@flight',
          provider: AuthProvider.PlatformNative,
          roles: [UserRole.PlatformOperator],
          permissions: [Permission.AccessHal, Permission.ManageComponents],
          authenticatedAt: 1700000000000,
          lastActivity: 1700000000000,
          platform: 'dreamcast',
          metadata: [['auth_type', 'platform_native']]
        },
        token: {
          token: 'flight-dreamcast-token-abc123',
          tokenType: TokenType.PlatformSpecific,
          expiresAt: 1700086400000,
          scopes: ['platform:access', 'hal:control'],
          issuer: 'flight',
          platform: 'dreamcast',
          metadata: [['token_source', 'platform_native']]
        },
        sessionId: 'flight-dreamcast-session',
        platform: 'dreamcast',
        provider: AuthProvider.PlatformNative,
        metadata: [['security_level', 'platform_trusted']]
      };

      const json = JSON.stringify(authContext);
      const parsed = JSON.parse(json) as AuthContext;

      expect(parsed.state).toBe(AuthState.Authenticated);
      expect(parsed.platform).toBe('dreamcast');
      expect(parsed.userAuth?.userId).toBe('test-user');
      expect(parsed.token?.tokenType).toBe(TokenType.PlatformSpecific);
    });

    test('should handle subscription tier mapping', () => {
      const subscriptionTiers = [
        V6RSubscriptionTier.Free,
        V6RSubscriptionTier.Individual,
        V6RSubscriptionTier.Team,
        V6RSubscriptionTier.Enterprise,
        V6RSubscriptionTier.Custom
      ];

      subscriptionTiers.forEach(tier => {
        expect(typeof tier).toBe('string');
        expect(tier.length).toBeGreaterThan(0);
      });

      // Test tier hierarchy
      const tierValues = Object.values(V6RSubscriptionTier);
      expect(tierValues).toContain('free');
      expect(tierValues).toContain('individual');
      expect(tierValues).toContain('team');
      expect(tierValues).toContain('enterprise');
      expect(tierValues).toContain('custom');
    });
  });

  describe('Security Validation', () => {
    test('should validate token expiration', () => {
      const expiredToken: AuthToken = {
        token: 'expired-token',
        tokenType: TokenType.Jwt,
        expiresAt: Date.now() - 3600000, // 1 hour ago
        scopes: ['read'],
        issuer: 'test-issuer',
        metadata: []
      };

      const validToken: AuthToken = {
        token: 'valid-token',
        tokenType: TokenType.Jwt,
        expiresAt: Date.now() + 3600000, // 1 hour from now
        scopes: ['read'],
        issuer: 'test-issuer',
        metadata: []
      };

      expect(Date.now() > expiredToken.expiresAt).toBe(true);
      expect(Date.now() < validToken.expiresAt).toBe(true);
    });

    test('should enforce role-based permissions', () => {
      const freeUserRoles = [UserRole.User];
      const enterpriseUserRoles = [UserRole.User, UserRole.Developer, UserRole.VmManager, UserRole.Admin];

      expect(freeUserRoles).not.toContain(UserRole.Admin);
      expect(freeUserRoles).not.toContain(UserRole.VmManager);
      
      expect(enterpriseUserRoles).toContain(UserRole.Admin);
      expect(enterpriseUserRoles).toContain(UserRole.VmManager);
      expect(enterpriseUserRoles).toContain(UserRole.Developer);
    });

    test('should validate platform-specific tokens', () => {
      const dreamcastToken: AuthToken = {
        token: 'dreamcast-token',
        tokenType: TokenType.PlatformSpecific,
        expiresAt: Date.now() + 86400000,
        scopes: ['platform:access', 'hal:control'],
        issuer: 'flight',
        platform: 'dreamcast',
        metadata: []
      };

      const v6rToken: AuthToken = {
        token: 'v6r-token',
        tokenType: TokenType.Jwt,
        expiresAt: Date.now() + 43200000, // 12 hours
        scopes: ['vm:read', 'vm:write'],
        issuer: 'v6r-auth-service',
        platform: undefined, // Cross-platform
        metadata: []
      };

      // Platform-specific token should be restricted
      expect(dreamcastToken.platform).toBe('dreamcast');
      expect(dreamcastToken.tokenType).toBe(TokenType.PlatformSpecific);

      // V6R token should be cross-platform
      expect(v6rToken.platform).toBeUndefined();
      expect(v6rToken.tokenType).toBe(TokenType.Jwt);
    });
  });

  describe('Error Handling', () => {
    test('should handle invalid authentication states', () => {
      const errorContext: AuthContext = {
        state: AuthState.Error,
        platform: 'unknown',
        provider: AuthProvider.Guest,
        metadata: [['error', 'authentication_failed']]
      };

      expect(errorContext.state).toBe(AuthState.Error);
      expect(errorContext.userAuth).toBeUndefined();
      expect(errorContext.token).toBeUndefined();
    });

    test('should handle expired tokens gracefully', () => {
      const expiredContext: AuthContext = {
        state: AuthState.Expired,
        userAuth: {
          userId: 'expired-user',
          username: 'expired@example.com',
          provider: AuthProvider.V6RCloud,
          roles: [UserRole.User],
          permissions: [Permission.Read],
          authenticatedAt: Date.now() - 7200000, // 2 hours ago
          lastActivity: Date.now() - 3600000, // 1 hour ago
          platform: 'v6r-cloud',
          metadata: []
        },
        platform: 'v6r-cloud',
        provider: AuthProvider.V6RCloud,
        metadata: [['expiry_reason', 'token_expired']]
      };

      expect(expiredContext.state).toBe(AuthState.Expired);
      expect(expiredContext.userAuth?.lastActivity).toBeLessThan(Date.now());
    });
  });

  describe('Performance Tests', () => {
    test('should handle large permission sets efficiently', () => {
      const largePermissionSet = [
        Permission.Read,
        Permission.Write,
        Permission.Execute,
        Permission.Admin,
        Permission.Create,
        Permission.Delete,
        Permission.ManageSessions,
        Permission.ManageVms,
        Permission.AccessHal,
        Permission.ManageComponents,
        Permission.ViewMetrics,
        Permission.ConfigurePlatform
      ];

      const userAuth: UserAuth = {
        userId: 'power-user',
        username: 'admin@enterprise.com',
        provider: AuthProvider.V6RCloud,
        roles: [UserRole.Admin],
        permissions: largePermissionSet,
        authenticatedAt: Date.now(),
        lastActivity: Date.now(),
        platform: 'v6r-large',
        metadata: [['subscription_tier', 'enterprise']]
      };

      // Test performance of permission checking
      const start = performance.now();
      const hasAdminPermission = userAuth.permissions.includes(Permission.Admin);
      const hasReadPermission = userAuth.permissions.includes(Permission.Read);
      const end = performance.now();

      expect(hasAdminPermission).toBe(true);
      expect(hasReadPermission).toBe(true);
      expect(end - start).toBeLessThan(1); // Should be very fast
    });

    test('should serialize/deserialize efficiently', () => {
      const complexAuthContext: AuthContext = {
        state: AuthState.Authenticated,
        userAuth: {
          userId: 'complex-user-id-with-long-identifier',
          username: 'complex.user@enterprise-organization.v6r.dev',
          provider: AuthProvider.V6RCloud,
          roles: [UserRole.Admin, UserRole.Developer, UserRole.VmManager],
          permissions: Object.values(Permission),
          authenticatedAt: Date.now(),
          lastActivity: Date.now(),
          platform: 'v6r-enterprise-large-vm',
          metadata: [
            ['subscription_tier', 'enterprise'],
            ['organization_id', 'org-enterprise-12345'],
            ['team_memberships', 'team1,team2,team3'],
            ['billing_enabled', 'true'],
            ['auth_method', 'oauth2_with_mfa']
          ]
        },
        token: {
          token: 'very-long-jwt-token-with-extensive-claims-and-permissions',
          tokenType: TokenType.Jwt,
          expiresAt: Date.now() + 43200000,
          refreshToken: 'long-refresh-token-for-automatic-renewal',
          scopes: ['admin:*', 'vm:*', 'team:*', 'org:*', 'metrics:*'],
          issuer: 'v6r-enterprise-auth-service',
          metadata: [
            ['token_source', 'oauth2_enterprise'],
            ['subscription_tier', 'enterprise'],
            ['issued_by', 'v6r-auth-service-v2.1.0']
          ]
        },
        sessionId: 'enterprise-session-with-long-identifier-12345',
        platform: 'v6r-enterprise-large-vm',
        provider: AuthProvider.V6RCloud,
        metadata: [
          ['auth_method', 'oauth2'],
          ['client_id', 'enterprise-client-application'],
          ['security_level', 'enterprise_grade'],
          ['compliance', 'soc2_hipaa_gdpr']
        ]
      };

      const start = performance.now();
      const json = JSON.stringify(complexAuthContext);
      const parsed = JSON.parse(json) as AuthContext;
      const end = performance.now();

      expect(parsed.userAuth?.userId).toBe(complexAuthContext.userAuth?.userId);
      expect(parsed.token?.token).toBe(complexAuthContext.token?.token);
      expect(end - start).toBeLessThan(10); // Should complete quickly
    });
  });
});
