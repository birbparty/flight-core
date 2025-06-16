/**
 * Flight-Core Authentication Types - TypeScript Bindings
 * Universal authentication for Flight-Core and V6R cloud integration
 */

// Export all types
export * from './types';

// Export V6R authentication manager
export { V6RAuthManager, createV6RAuthManager } from './v6r-auth-manager';

// Export React hooks (if React is available)
export * from './react/auth-hooks';

// Convenience re-exports for common types
export type {
  AuthToken,
  UserAuth,
  AuthContext,
  AuthProvider,
  AuthState,
  TokenType,
  UserRole,
  Permission,
  V6RSubscriptionTier,
  AuthConfig,
  AuthResult,
  AuthError,
} from './types';
