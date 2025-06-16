"use strict";
/**
 * Real-time Communication Types for Flight-Core and V6R
 *
 * TypeScript bindings for real-time communication WIT types
 * Supports WebSocket messaging, event streaming, and live updates
 */
Object.defineProperty(exports, "__esModule", { value: true });
exports.isCustomEvent = exports.isFlightEvent = exports.isV6REvent = exports.isSystemEvent = exports.isAuthEvent = exports.isSessionEvent = exports.isComponentUpdateEvent = exports.isMemoryUpdateEvent = exports.FlightEventType = exports.V6REventType = exports.EventSeverity = exports.SystemEventType = exports.AuthEventType = exports.SessionEventType = exports.FilterOperation = exports.ChannelType = exports.MessagePriority = exports.ClientType = exports.ConnectionState = void 0;
var ConnectionState;
(function (ConnectionState) {
    ConnectionState["Connecting"] = "connecting";
    ConnectionState["Connected"] = "connected";
    ConnectionState["Disconnected"] = "disconnected";
    ConnectionState["Closing"] = "closing";
    ConnectionState["Closed"] = "closed";
    ConnectionState["Error"] = "error";
    ConnectionState["Authenticated"] = "authenticated";
})(ConnectionState || (exports.ConnectionState = ConnectionState = {}));
var ClientType;
(function (ClientType) {
    ClientType["WebBrowser"] = "web-browser";
    ClientType["FlightCoreNative"] = "flight-native";
    ClientType["V6RMobile"] = "v6r-mobile";
    ClientType["V6RCli"] = "v6r-cli";
    ClientType["V6RBridge"] = "v6r-bridge";
    ClientType["DevelopmentTool"] = "development-tool";
    ClientType["MonitoringService"] = "monitoring-service";
    ClientType["ApiClient"] = "api-client";
    ClientType["Unknown"] = "unknown";
})(ClientType || (exports.ClientType = ClientType = {}));
// Messaging Types
var MessagePriority;
(function (MessagePriority) {
    MessagePriority["Low"] = "low";
    MessagePriority["Normal"] = "normal";
    MessagePriority["High"] = "high";
    MessagePriority["Critical"] = "critical";
    MessagePriority["Realtime"] = "realtime";
})(MessagePriority || (exports.MessagePriority = MessagePriority = {}));
// Channel and Subscription Types
var ChannelType;
(function (ChannelType) {
    ChannelType["PublicBroadcast"] = "public-broadcast";
    ChannelType["PrivateUser"] = "private-user";
    ChannelType["SessionPrivate"] = "session-private";
    ChannelType["SystemMonitoring"] = "system-monitoring";
    ChannelType["MemoryUpdates"] = "memory-updates";
    ChannelType["ComponentEvents"] = "component-events";
    ChannelType["V6RVmManagement"] = "v6r-vm-management";
    ChannelType["FlightHalEvents"] = "flight-hal-events";
    ChannelType["Development"] = "development";
})(ChannelType || (exports.ChannelType = ChannelType = {}));
var FilterOperation;
(function (FilterOperation) {
    FilterOperation["Equals"] = "equals";
    FilterOperation["NotEquals"] = "not-equals";
    FilterOperation["Contains"] = "contains";
    FilterOperation["StartsWith"] = "starts-with";
    FilterOperation["EndsWith"] = "ends-with";
    FilterOperation["Regex"] = "regex";
    FilterOperation["GreaterThan"] = "greater-than";
    FilterOperation["LessThan"] = "less-than";
})(FilterOperation || (exports.FilterOperation = FilterOperation = {}));
// Event System Types
var SessionEventType;
(function (SessionEventType) {
    SessionEventType["SessionCreated"] = "session-created";
    SessionEventType["SessionActivated"] = "session-activated";
    SessionEventType["SessionSuspended"] = "session-suspended";
    SessionEventType["SessionTerminated"] = "session-terminated";
    SessionEventType["SessionExpired"] = "session-expired";
    SessionEventType["SessionError"] = "session-error";
})(SessionEventType || (exports.SessionEventType = SessionEventType = {}));
var AuthEventType;
(function (AuthEventType) {
    AuthEventType["UserAuthenticated"] = "user-authenticated";
    AuthEventType["UserLoggedOut"] = "user-logged-out";
    AuthEventType["TokenRefreshed"] = "token-refreshed";
    AuthEventType["TokenExpired"] = "token-expired";
    AuthEventType["AuthFailed"] = "auth-failed";
    AuthEventType["PermissionGranted"] = "permission-granted";
    AuthEventType["PermissionRevoked"] = "permission-revoked";
})(AuthEventType || (exports.AuthEventType = AuthEventType = {}));
var SystemEventType;
(function (SystemEventType) {
    SystemEventType["SystemStartup"] = "system-startup";
    SystemEventType["SystemShutdown"] = "system-shutdown";
    SystemEventType["ComponentLoaded"] = "component-loaded";
    SystemEventType["ComponentUnloaded"] = "component-unloaded";
    SystemEventType["ErrorOccurred"] = "error-occurred";
    SystemEventType["PerformanceWarning"] = "performance-warning";
    SystemEventType["MemoryPressure"] = "memory-pressure";
    SystemEventType["ResourceExhausted"] = "resource-exhausted";
})(SystemEventType || (exports.SystemEventType = SystemEventType = {}));
var EventSeverity;
(function (EventSeverity) {
    EventSeverity["Info"] = "info";
    EventSeverity["Warning"] = "warning";
    EventSeverity["Error"] = "error";
    EventSeverity["Critical"] = "critical";
    EventSeverity["Fatal"] = "fatal";
})(EventSeverity || (exports.EventSeverity = EventSeverity = {}));
var V6REventType;
(function (V6REventType) {
    V6REventType["VmCreated"] = "vm-created";
    V6REventType["VmStarted"] = "vm-started";
    V6REventType["VmStopped"] = "vm-stopped";
    V6REventType["VmDeleted"] = "vm-deleted";
    V6REventType["VmScaling"] = "vm-scaling";
    V6REventType["QuotaExceeded"] = "quota-exceeded";
    V6REventType["BillingEvent"] = "billing-event";
    V6REventType["TeamMemberAdded"] = "team-member-added";
    V6REventType["TeamMemberRemoved"] = "team-member-removed";
})(V6REventType || (exports.V6REventType = V6REventType = {}));
var FlightEventType;
(function (FlightEventType) {
    FlightEventType["PlatformDetected"] = "platform-detected";
    FlightEventType["HalInitialized"] = "hal-initialized";
    FlightEventType["ComponentLoaded"] = "component-loaded";
    FlightEventType["MemoryPoolCreated"] = "memory-pool-created";
    FlightEventType["RuntimeStarted"] = "runtime-started";
    FlightEventType["PerformanceMilestone"] = "performance-milestone";
})(FlightEventType || (exports.FlightEventType = FlightEventType = {}));
// Type guards for real-time events
const isMemoryUpdateEvent = (event) => event.type === 'memory-update';
exports.isMemoryUpdateEvent = isMemoryUpdateEvent;
const isComponentUpdateEvent = (event) => event.type === 'component-update';
exports.isComponentUpdateEvent = isComponentUpdateEvent;
const isSessionEvent = (event) => event.type === 'session-event';
exports.isSessionEvent = isSessionEvent;
const isAuthEvent = (event) => event.type === 'auth-event';
exports.isAuthEvent = isAuthEvent;
const isSystemEvent = (event) => event.type === 'system-event';
exports.isSystemEvent = isSystemEvent;
const isV6REvent = (event) => event.type === 'v6r-event';
exports.isV6REvent = isV6REvent;
const isFlightEvent = (event) => event.type === 'flight-event';
exports.isFlightEvent = isFlightEvent;
const isCustomEvent = (event) => event.type === 'custom-event';
exports.isCustomEvent = isCustomEvent;
//# sourceMappingURL=types.js.map