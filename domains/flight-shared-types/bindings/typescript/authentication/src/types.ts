/**
 * TypeScript Authentication Types
 * Generated from Flight-Core authentication.wit
 */

export type SessionId = string;

// Authentication token representation
export interface AuthToken {
  /** Token value (encrypted) */
  token: string;
  /** Token type (bearer, api-key, session, etc.) */
  tokenType: TokenType;
  /** Token expiration timestamp */
  expiresAt: number;
  /** Refresh token for token renewal */
  refreshToken?: string;
  /** Token scope permissions */
  scopes: string[];
  /** Token issuer */
  issuer: string;
  /** Platform this token is valid for */
  platform?: string;
  /** Token metadata */
  metadata: [string, string][];
}

// Types of authentication tokens
export const TokenType = {
  Bearer: "bearer" as const,
  ApiKey: "api-key" as const,
  Session: "session" as const,
  Jwt: "jwt" as const,
  PlatformSpecific: "platform-specific" as const,
  V6RDevelopment: "v6r-development" as const,
  V6RProduction: "v6r-production" as const,
} as const;

export type TokenType = typeof TokenType[keyof typeof TokenType];

// User authentication information
export interface UserAuth {
  /** User unique identifier */
  userId: string;
  /** Username/email */
  username: string;
  /** Authentication provider */
  provider: AuthProvider;
  /** User roles */
  roles: UserRole[];
  /** User permissions */
  permissions: Permission[];
  /** Authentication timestamp */
  authenticatedAt: number;
  /** Last activity timestamp */
  lastActivity: number;
  /** Platform where user is authenticated */
  platform: string;
  /** User metadata */
  metadata: [string, string][];
}

// Authentication providers
export const AuthProvider = {
  FlightLocal: "flight-local" as const,
  V6RCloud: "v6r-cloud" as const,
  OAuth2: "oauth2" as const,
  Ldap: "ldap" as const,
  ApiKey: "api-key" as const,
  Guest: "guest" as const,
  PlatformNative: "platform-native" as const,
} as const;

export type AuthProvider = typeof AuthProvider[keyof typeof AuthProvider];

// User roles in the system
export const UserRole = {
  Admin: "admin" as const,
  User: "user" as const,
  Developer: "developer" as const,
  ServiceAccount: "service-account" as const,
  Guest: "guest" as const,
  PlatformOperator: "platform-operator" as const,
  VmManager: "vm-manager" as const,
  Observer: "observer" as const,
} as const;

export type UserRole = typeof UserRole[keyof typeof UserRole];

// Permission definitions
export const Permission = {
  Read: "read" as const,
  Write: "write" as const,
  Execute: "execute" as const,
  Admin: "admin" as const,
  Create: "create" as const,
  Delete: "delete" as const,
  ManageSessions: "manage-sessions" as const,
  ManageVms: "manage-vms" as const,
  AccessHal: "access-hal" as const,
  ManageComponents: "manage-components" as const,
  ViewMetrics: "view-metrics" as const,
  ConfigurePlatform: "configure-platform" as const,
} as const;

export type Permission = typeof Permission[keyof typeof Permission];

// Authentication state
export const AuthState = {
  Unauthenticated: "unauthenticated" as const,
  Authenticating: "authenticating" as const,
  Authenticated: "authenticated" as const,
  Expired: "expired" as const,
  Revoked: "revoked" as const,
  Error: "error" as const,
} as const;

export type AuthState = typeof AuthState[keyof typeof AuthState];

// Authentication context
export interface AuthContext {
  /** Authentication state */
  state: AuthState;
  /** Current user authentication */
  userAuth?: UserAuth;
  /** Active authentication token */
  token?: AuthToken;
  /** Session associated with authentication */
  sessionId?: SessionId;
  /** Platform context */
  platform: string;
  /** Authentication provider used */
  provider: AuthProvider;
  /** Authentication metadata */
  metadata: [string, string][];
}

// Permission check request
export interface PermissionCheck {
  /** User to check permissions for */
  userId: string;
  /** Resource being accessed */
  resource: string;
  /** Action being performed */
  action: string;
  /** Context for permission check */
  context?: string;
  /** Platform where check is performed */
  platform: string;
}

// Permission check result
export interface PermissionResult {
  /** Whether permission is granted */
  granted: boolean;
  /** Reason for grant/denial */
  reason: string;
  /** Required roles for access */
  requiredRoles: UserRole[];
  /** Required permissions for access */
  requiredPermissions: Permission[];
  /** Effective permissions granted */
  effectivePermissions: Permission[];
}

// V6R-specific authentication extensions
export interface V6RAuthExtensions {
  /** V6R organization ID */
  organizationId?: string;
  /** V6R team membership */
  teamMemberships: string[];
  /** V6R subscription tier */
  subscriptionTier: V6RSubscriptionTier;
  /** V6R resource quotas */
  resourceQuotas: V6RResourceQuotas;
  /** V6R billing information */
  billingEnabled: boolean;
}

// V6R subscription tiers
export enum V6RSubscriptionTier {
  Free = "free",
  Individual = "individual",
  Team = "team",
  Enterprise = "enterprise",
  Custom = "custom",
}

// V6R resource quotas
export interface V6RResourceQuotas {
  /** Maximum concurrent VMs */
  maxVms: number;
  /** Maximum memory per VM (bytes) */
  maxVmMemory: number;
  /** Maximum storage per user (bytes) */
  maxStorage: number;
  /** Maximum API calls per hour */
  maxApiCalls: number;
  /** Maximum concurrent sessions */
  maxSessions: number;
}

// Flight-Core platform authentication
export interface FlightAuthExtensions {
  /** Platform capabilities required */
  requiredCapabilities: string[];
  /** HAL access permissions */
  halPermissions: string[];
  /** Component access permissions */
  componentPermissions: string[];
  /** Memory access permissions */
  memoryPermissions: string[];
  /** Platform-specific settings */
  platformSettings: [string, string][];
}

// Error types for authentication
export interface AuthError {
  code: string;
  message: string;
  details?: any;
}

// Result type for operations
export type AuthResult<T> = {
  success: true;
  data: T;
} | {
  success: false;
  error: AuthError;
};

// OAuth2 configuration for V6R
export interface OAuth2Config {
  clientId: string;
  clientSecret?: string;
  redirectUri: string;
  scopes: string[];
  authUrl: string;
  tokenUrl: string;
  userInfoUrl?: string;
}

// V6R specific OAuth2 response
export interface V6ROAuth2Response {
  accessToken: string;
  refreshToken?: string;
  expiresIn: number;
  tokenType: string;
  scopes: string[];
  userId: string;
  username: string;
  subscriptionTier: string;
  organizationId?: string;
}

// JWT payload interface
export interface JWTPayload {
  sub: string; // Subject (user ID)
  iss: string; // Issuer
  aud: string; // Audience
  exp: number; // Expiration time
  iat: number; // Issued at
  nbf?: number; // Not before
  jti?: string; // JWT ID
  scope?: string; // Token scopes
  platform?: string; // Platform restriction
  roles?: UserRole[]; // User roles
  permissions?: Permission[]; // User permissions
}

// Authentication event types
export enum AuthEventType {
  Login = "login",
  Logout = "logout",
  TokenRefresh = "token-refresh",
  TokenExpired = "token-expired",
  PermissionDenied = "permission-denied",
  SessionExpired = "session-expired",
}

// Authentication event
export interface AuthEvent {
  type: AuthEventType;
  timestamp: number;
  userId?: string;
  platform: string;
  metadata?: Record<string, any>;
}

// Authentication configuration
export interface AuthConfig {
  v6rApiUrl: string;
  clientId: string;
  clientSecret?: string;
  redirectUri?: string;
  scopes: string[];
  autoRefresh?: boolean;
  refreshThreshold?: number; // Seconds before expiry to refresh
  storageKey?: string;
}

// Platform authentication requirements
export interface PlatformAuthRequirements {
  platform: string;
  requiredRoles: UserRole[];
  requiredPermissions: Permission[];
  minSubscriptionTier?: V6RSubscriptionTier;
  additionalChecks?: string[];
}

// Authentication middleware configuration
export interface AuthMiddlewareConfig {
  required: boolean;
  roles?: UserRole[];
  permissions?: Permission[];
  platforms?: string[];
  customValidator?: (context: AuthContext) => boolean;
}

// Session extension for authentication
export interface AuthSession {
  authContext: AuthContext;
  expiresAt: number;
  lastActivity: number;
  ipAddress?: string;
  userAgent?: string;
}
