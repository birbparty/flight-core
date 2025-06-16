// Package authentication provides Flight-Core authentication types for Go
// Generated from Flight-Core authentication.wit
package authentication

import (
	"time"
)

// SessionID represents a session identifier
type SessionID string

// TokenType represents different types of authentication tokens
type TokenType string

const (
	TokenTypeBearer           TokenType = "bearer"
	TokenTypeApiKey           TokenType = "api-key"
	TokenTypeSession          TokenType = "session"
	TokenTypeJWT              TokenType = "jwt"
	TokenTypePlatformSpecific TokenType = "platform-specific"
	TokenTypeV6RDevelopment   TokenType = "v6r-development"
	TokenTypeV6RProduction    TokenType = "v6r-production"
)

// AuthProvider represents different authentication providers
type AuthProvider string

const (
	AuthProviderFlightLocal    AuthProvider = "flight-local"
	AuthProviderV6RCloud       AuthProvider = "v6r-cloud"
	AuthProviderOAuth2         AuthProvider = "oauth2"
	AuthProviderLDAP           AuthProvider = "ldap"
	AuthProviderApiKey         AuthProvider = "api-key"
	AuthProviderGuest          AuthProvider = "guest"
	AuthProviderPlatformNative AuthProvider = "platform-native"
)

// UserRole represents user roles in the system
type UserRole string

const (
	UserRoleAdmin            UserRole = "admin"
	UserRoleUser             UserRole = "user"
	UserRoleDeveloper        UserRole = "developer"
	UserRoleServiceAccount   UserRole = "service-account"
	UserRoleGuest            UserRole = "guest"
	UserRolePlatformOperator UserRole = "platform-operator"
	UserRoleVMManager        UserRole = "vm-manager"
	UserRoleObserver         UserRole = "observer"
)

// Permission represents permission definitions
type Permission string

const (
	PermissionRead              Permission = "read"
	PermissionWrite             Permission = "write"
	PermissionExecute           Permission = "execute"
	PermissionAdmin             Permission = "admin"
	PermissionCreate            Permission = "create"
	PermissionDelete            Permission = "delete"
	PermissionManageSessions    Permission = "manage-sessions"
	PermissionManageVMs         Permission = "manage-vms"
	PermissionAccessHAL         Permission = "access-hal"
	PermissionManageComponents  Permission = "manage-components"
	PermissionViewMetrics       Permission = "view-metrics"
	PermissionConfigurePlatform Permission = "configure-platform"
)

// AuthState represents authentication state
type AuthState string

const (
	AuthStateUnauthenticated AuthState = "unauthenticated"
	AuthStateAuthenticating  AuthState = "authenticating"
	AuthStateAuthenticated   AuthState = "authenticated"
	AuthStateExpired         AuthState = "expired"
	AuthStateRevoked         AuthState = "revoked"
	AuthStateError           AuthState = "error"
)

// V6RSubscriptionTier represents V6R subscription tiers
type V6RSubscriptionTier string

const (
	V6RSubscriptionTierFree       V6RSubscriptionTier = "free"
	V6RSubscriptionTierIndividual V6RSubscriptionTier = "individual"
	V6RSubscriptionTierTeam       V6RSubscriptionTier = "team"
	V6RSubscriptionTierEnterprise V6RSubscriptionTier = "enterprise"
	V6RSubscriptionTierCustom     V6RSubscriptionTier = "custom"
)

// AuthToken represents an authentication token
type AuthToken struct {
	Token        string            `json:"token"`
	TokenType    TokenType         `json:"token_type"`
	ExpiresAt    time.Time         `json:"expires_at"`
	RefreshToken *string           `json:"refresh_token,omitempty"`
	Scopes       []string          `json:"scopes"`
	Issuer       string            `json:"issuer"`
	Platform     *string           `json:"platform,omitempty"`
	Metadata     map[string]string `json:"metadata"`
}

// UserAuth represents user authentication information
type UserAuth struct {
	UserID          string            `json:"user_id"`
	Username        string            `json:"username"`
	Provider        AuthProvider      `json:"provider"`
	Roles           []UserRole        `json:"roles"`
	Permissions     []Permission      `json:"permissions"`
	AuthenticatedAt time.Time         `json:"authenticated_at"`
	LastActivity    time.Time         `json:"last_activity"`
	Platform        string            `json:"platform"`
	Metadata        map[string]string `json:"metadata"`
}

// AuthContext represents authentication context
type AuthContext struct {
	State     AuthState         `json:"state"`
	UserAuth  *UserAuth         `json:"user_auth,omitempty"`
	Token     *AuthToken        `json:"token,omitempty"`
	SessionID *SessionID        `json:"session_id,omitempty"`
	Platform  string            `json:"platform"`
	Provider  AuthProvider      `json:"provider"`
	Metadata  map[string]string `json:"metadata"`
}

// PermissionCheck represents a permission check request
type PermissionCheck struct {
	UserID   string  `json:"user_id"`
	Resource string  `json:"resource"`
	Action   string  `json:"action"`
	Context  *string `json:"context,omitempty"`
	Platform string  `json:"platform"`
}

// PermissionResult represents permission check result
type PermissionResult struct {
	Granted              bool         `json:"granted"`
	Reason               string       `json:"reason"`
	RequiredRoles        []UserRole   `json:"required_roles"`
	RequiredPermissions  []Permission `json:"required_permissions"`
	EffectivePermissions []Permission `json:"effective_permissions"`
}

// V6RResourceQuotas represents V6R resource quotas
type V6RResourceQuotas struct {
	MaxVMs      uint32 `json:"max_vms"`
	MaxVMMemory uint64 `json:"max_vm_memory"`
	MaxStorage  uint64 `json:"max_storage"`
	MaxAPICalls uint32 `json:"max_api_calls"`
	MaxSessions uint32 `json:"max_sessions"`
}

// V6RAuthExtensions represents V6R-specific authentication extensions
type V6RAuthExtensions struct {
	OrganizationID   *string             `json:"organization_id,omitempty"`
	TeamMemberships  []string            `json:"team_memberships"`
	SubscriptionTier V6RSubscriptionTier `json:"subscription_tier"`
	ResourceQuotas   V6RResourceQuotas   `json:"resource_quotas"`
	BillingEnabled   bool                `json:"billing_enabled"`
}

// FlightAuthExtensions represents Flight-Core platform authentication
type FlightAuthExtensions struct {
	RequiredCapabilities []string          `json:"required_capabilities"`
	HALPermissions       []string          `json:"hal_permissions"`
	ComponentPermissions []string          `json:"component_permissions"`
	MemoryPermissions    []string          `json:"memory_permissions"`
	PlatformSettings     map[string]string `json:"platform_settings"`
}

// AuthError represents authentication errors
type AuthError struct {
	Code    string      `json:"code"`
	Message string      `json:"message"`
	Details interface{} `json:"details,omitempty"`
}

// Error implements the error interface
func (e AuthError) Error() string {
	return e.Message
}

// AuthResult represents result of authentication operations
type AuthResult[T any] struct {
	Success bool       `json:"success"`
	Data    *T         `json:"data,omitempty"`
	Error   *AuthError `json:"error,omitempty"`
}

// NewSuccessResult creates a successful AuthResult
func NewSuccessResult[T any](data T) AuthResult[T] {
	return AuthResult[T]{
		Success: true,
		Data:    &data,
	}
}

// NewErrorResult creates a failed AuthResult
func NewErrorResult[T any](err AuthError) AuthResult[T] {
	return AuthResult[T]{
		Success: false,
		Error:   &err,
	}
}

// IsValid checks if the token is still valid
func (t *AuthToken) IsValid() bool {
	return time.Now().Before(t.ExpiresAt)
}

// ExpiresIn returns duration until token expiration
func (t *AuthToken) ExpiresIn() time.Duration {
	return time.Until(t.ExpiresAt)
}

// HasScope checks if token has specific scope
func (t *AuthToken) HasScope(scope string) bool {
	for _, s := range t.Scopes {
		if s == scope {
			return true
		}
	}
	return false
}

// HasPermission checks if user has specific permission
func (u *UserAuth) HasPermission(permission Permission) bool {
	for _, p := range u.Permissions {
		if p == permission {
			return true
		}
	}
	return false
}

// HasRole checks if user has specific role
func (u *UserAuth) HasRole(role UserRole) bool {
	for _, r := range u.Roles {
		if r == role {
			return true
		}
	}
	return false
}

// HasAnyPermission checks if user has any of the specified permissions
func (u *UserAuth) HasAnyPermission(permissions ...Permission) bool {
	for _, permission := range permissions {
		if u.HasPermission(permission) {
			return true
		}
	}
	return false
}

// HasAllPermissions checks if user has all specified permissions
func (u *UserAuth) HasAllPermissions(permissions ...Permission) bool {
	for _, permission := range permissions {
		if !u.HasPermission(permission) {
			return false
		}
	}
	return true
}

// HasAnyRole checks if user has any of the specified roles
func (u *UserAuth) HasAnyRole(roles ...UserRole) bool {
	for _, role := range roles {
		if u.HasRole(role) {
			return true
		}
	}
	return false
}

// HasAllRoles checks if user has all specified roles
func (u *UserAuth) HasAllRoles(roles ...UserRole) bool {
	for _, role := range roles {
		if !u.HasRole(role) {
			return false
		}
	}
	return true
}

// IsAuthenticated checks if the context represents an authenticated state
func (c *AuthContext) IsAuthenticated() bool {
	return c.State == AuthStateAuthenticated && c.Token != nil && c.Token.IsValid()
}

// GetSubscriptionTier extracts subscription tier from user metadata
func (c *AuthContext) GetSubscriptionTier() *V6RSubscriptionTier {
	if c.UserAuth == nil {
		return nil
	}

	if tier, exists := c.UserAuth.Metadata["subscription_tier"]; exists {
		subscriptionTier := V6RSubscriptionTier(tier)
		return &subscriptionTier
	}

	return nil
}

// GetOrganizationID extracts organization ID from user metadata
func (c *AuthContext) GetOrganizationID() *string {
	if c.UserAuth == nil {
		return nil
	}

	if orgID, exists := c.UserAuth.Metadata["organization_id"]; exists && orgID != "" {
		return &orgID
	}

	return nil
}

// JWT Claims structure for token parsing
type JWTClaims struct {
	UserID      string       `json:"sub"`
	Issuer      string       `json:"iss"`
	Audience    string       `json:"aud"`
	ExpiresAt   int64        `json:"exp"`
	IssuedAt    int64        `json:"iat"`
	NotBefore   int64        `json:"nbf,omitempty"`
	JWTID       string       `json:"jti,omitempty"`
	Scope       string       `json:"scope,omitempty"`
	Platform    string       `json:"platform,omitempty"`
	Roles       []UserRole   `json:"roles,omitempty"`
	Permissions []Permission `json:"permissions,omitempty"`
}

// AuthConfig represents authentication configuration
type AuthConfig struct {
	V6RAPIUrl        string   `json:"v6r_api_url"`
	ClientID         string   `json:"client_id"`
	ClientSecret     string   `json:"client_secret,omitempty"`
	RedirectURI      string   `json:"redirect_uri,omitempty"`
	Scopes           []string `json:"scopes"`
	AutoRefresh      bool     `json:"auto_refresh"`
	RefreshThreshold int      `json:"refresh_threshold"` // seconds before expiry to refresh
}

// OAuth2Response represents OAuth2 authentication response
type OAuth2Response struct {
	AccessToken      string `json:"access_token"`
	RefreshToken     string `json:"refresh_token,omitempty"`
	ExpiresIn        int    `json:"expires_in"`
	TokenType        string `json:"token_type"`
	Scope            string `json:"scope,omitempty"`
	UserID           string `json:"user_id,omitempty"`
	Username         string `json:"username,omitempty"`
	SubscriptionTier string `json:"subscription_tier,omitempty"`
	OrganizationID   string `json:"organization_id,omitempty"`
}

// AuthEventType represents authentication event types
type AuthEventType string

const (
	AuthEventTypeLogin            AuthEventType = "login"
	AuthEventTypeLogout           AuthEventType = "logout"
	AuthEventTypeTokenRefresh     AuthEventType = "token-refresh"
	AuthEventTypeTokenExpired     AuthEventType = "token-expired"
	AuthEventTypePermissionDenied AuthEventType = "permission-denied"
	AuthEventTypeSessionExpired   AuthEventType = "session-expired"
)

// AuthEvent represents an authentication event
type AuthEvent struct {
	Type      AuthEventType          `json:"type"`
	Timestamp time.Time              `json:"timestamp"`
	UserID    *string                `json:"user_id,omitempty"`
	Platform  string                 `json:"platform"`
	Metadata  map[string]interface{} `json:"metadata,omitempty"`
}

// PlatformAuthRequirements represents platform authentication requirements
type PlatformAuthRequirements struct {
	Platform            string               `json:"platform"`
	RequiredRoles       []UserRole           `json:"required_roles"`
	RequiredPermissions []Permission         `json:"required_permissions"`
	MinSubscriptionTier *V6RSubscriptionTier `json:"min_subscription_tier,omitempty"`
	AdditionalChecks    []string             `json:"additional_checks,omitempty"`
}

// AuthMiddlewareConfig represents authentication middleware configuration
type AuthMiddlewareConfig struct {
	Required        bool                    `json:"required"`
	Roles           []UserRole              `json:"roles,omitempty"`
	Permissions     []Permission            `json:"permissions,omitempty"`
	Platforms       []string                `json:"platforms,omitempty"`
	CustomValidator func(*AuthContext) bool `json:"-"` // Not serialized
}

// AuthSession represents session extension for authentication
type AuthSession struct {
	AuthContext  AuthContext `json:"auth_context"`
	ExpiresAt    time.Time   `json:"expires_at"`
	LastActivity time.Time   `json:"last_activity"`
	IPAddress    *string     `json:"ip_address,omitempty"`
	UserAgent    *string     `json:"user_agent,omitempty"`
}
