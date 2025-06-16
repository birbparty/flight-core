// Component Model infrastructure types for Flight-Core and V6R
// 
// TypeScript bindings for Component Model WIT types with enhanced
// type safety and modern JavaScript features

// Import types from existing bindings
export interface MemorySize {
  bytes: bigint;
  humanReadable: string;
}

export interface MemoryUsageSnapshot {
  timestamp: number;
  sessionId: string;
  platform: string;
  total: MemorySize;
  used: MemorySize;
  available: MemorySize;
  fragmentationRatio: number;
}

export interface FlightResult<T> {
  success: boolean;
  data?: T;
  error?: FlightError;
}

export interface FlightError {
  code: string;
  message: string;
  details?: string;
}

export interface PlatformInfo {
  id: string;
  name: string;
  category: string;
  capability: string;
}

// Component identification types
export type ComponentId = string;
export type InstanceId = string;
export type WorldName = string;
export type InterfaceName = string;

// Component lifecycle states
export enum ComponentState {
  /** Component binary loaded but not instantiated */
  Loaded = 'loaded',
  /** Component instantiating (calling constructors) */
  Instantiating = 'instantiating',
  /** Component fully instantiated and ready */
  Instantiated = 'instantiated',
  /** Component actively executing */
  Running = 'running',
  /** Component execution suspended */
  Suspended = 'suspended',
  /** Component shutting down (calling destructors) */
  Terminating = 'terminating',
  /** Component terminated and resources released */
  Terminated = 'terminated',
  /** Component in error state */
  Error = 'error',
}

// Component metadata and information
export interface ComponentInfo {
  /** Unique component identifier */
  id: ComponentId;
  /** Component name */
  name: string;
  /** Component version */
  version: string;
  /** Current lifecycle state */
  state: ComponentState;
  /** World interface this component implements */
  world: WorldName;
  /** Platform where component runs */
  platform: string;
  /** Session this component belongs to */
  sessionId?: string;
  /** Component creation time (Unix timestamp) */
  createdAt: number;
  /** Last activity timestamp (Unix timestamp) */
  lastActivity: number;
  /** Memory usage by this component */
  memoryUsage: MemoryUsageSnapshot;
  /** Component-specific metadata */
  metadata: [string, string][];
}

// Component resource management
export type ResourceHandle = number;
export type ResourceType = string;

// Resource ownership information
export interface ResourceInfo {
  /** Resource handle identifier */
  handle: ResourceHandle;
  /** Type of resource (memory, file, network, etc.) */
  resourceType: ResourceType;
  /** Component that owns this resource */
  owner: ComponentId;
  /** Reference count for shared resources */
  refCount: number;
  /** Resource creation time (Unix timestamp) */
  createdAt: number;
  /** Resource size/usage */
  size: MemorySize;
  /** Resource-specific metadata */
  metadata: [string, string][];
}

// Inter-component communication interface
export interface ComponentInterface {
  /** Interface name */
  name: InterfaceName;
  /** Component providing this interface */
  provider: ComponentId;
  /** Interface version */
  version: string;
  /** Interface methods available */
  methods: string[];
  /** Interface state (connected, disconnected, etc.) */
  state: InterfaceState;
}

// Interface connection state
export enum InterfaceState {
  /** Interface available for connection */
  Available = 'available',
  /** Interface connected and operational */
  Connected = 'connected',
  /** Interface temporarily disconnected */
  Disconnected = 'disconnected',
  /** Interface deprecated but still functional */
  Deprecated = 'deprecated',
  /** Interface no longer available */
  Unavailable = 'unavailable',
}

// Component dependency tracking
export interface ComponentDependency {
  /** Component that has the dependency */
  dependent: ComponentId;
  /** Component or interface being depended on */
  dependency: string;
  /** Type of dependency */
  dependencyType: DependencyType;
  /** Whether dependency is optional */
  optional: boolean;
  /** Dependency resolution status */
  resolved: boolean;
}

// Dependency types for components
export enum DependencyType {
  /** Required interface import */
  InterfaceImport = 'interface-import',
  /** Required resource access */
  ResourceAccess = 'resource-access',
  /** Required platform capability */
  PlatformCapability = 'platform-capability',
  /** Required memory allocation */
  MemoryRequirement = 'memory-requirement',
  /** Required other component */
  ComponentDependency = 'component-dependency',
}

// Component execution context
export interface ExecutionContext {
  /** Component being executed */
  component: ComponentId;
  /** Current execution stack depth */
  stackDepth: number;
  /** Available memory */
  availableMemory: MemorySize;
  /** CPU time allocated (milliseconds) */
  cpuTimeMs: number;
  /** Execution priority */
  priority: ExecutionPriority;
  /** Platform execution mode */
  executionMode: ExecutionMode;
}

// Execution priority levels
export enum ExecutionPriority {
  /** Low priority background tasks */
  Low = 'low',
  /** Normal priority execution */
  Normal = 'normal',
  /** High priority interactive tasks */
  High = 'high',
  /** Critical priority system tasks */
  Critical = 'critical',
}

// Platform execution modes
export enum ExecutionMode {
  /** Single-threaded execution (Dreamcast) */
  SingleThreaded = 'single-threaded',
  /** Multi-threaded execution */
  MultiThreaded = 'multi-threaded',
  /** Async/await execution */
  AsyncExecution = 'async-execution',
  /** Real-time execution */
  RealTime = 'real-time',
}

// V6R-specific component extensions
export interface V6RComponentExtensions {
  /** Container runtime integration */
  containerId?: string;
  /** Kubernetes pod integration */
  podName?: string;
  /** Service mesh integration */
  serviceMeshEnabled: boolean;
  /** Auto-scaling configuration */
  autoScaling: V6RAutoScalingConfig;
  /** Load balancing configuration */
  loadBalancing: boolean;
  /** Monitoring and observability */
  monitoring: V6RMonitoringConfig;
}

// V6R auto-scaling configuration
export interface V6RAutoScalingConfig {
  /** Minimum component instances */
  minInstances: number;
  /** Maximum component instances */
  maxInstances: number;
  /** CPU utilization trigger percentage */
  cpuThreshold: number;
  /** Memory utilization trigger percentage */
  memoryThreshold: number;
  /** Scaling enabled */
  enabled: boolean;
}

// V6R monitoring configuration
export interface V6RMonitoringConfig {
  /** Metrics collection enabled */
  metricsEnabled: boolean;
  /** Distributed tracing enabled */
  tracingEnabled: boolean;
  /** Log aggregation enabled */
  loggingEnabled: boolean;
  /** Health check configuration */
  healthChecks: boolean;
}

// Component lifecycle management functions
export interface ComponentLifecycleManager {
  /** Create new component instance */
  createComponent(
    name: string,
    world: WorldName,
    platform: string,
    sessionId?: string
  ): Promise<FlightResult<ComponentId>>;

  /** Get component information */
  getComponent(id: ComponentId): Promise<FlightResult<ComponentInfo>>;

  /** Start component execution */
  startComponent(id: ComponentId): Promise<FlightResult<boolean>>;

  /** Suspend component execution */
  suspendComponent(id: ComponentId): Promise<FlightResult<boolean>>;

  /** Resume component execution */
  resumeComponent(id: ComponentId): Promise<FlightResult<boolean>>;

  /** Terminate component */
  terminateComponent(id: ComponentId): Promise<FlightResult<boolean>>;

  /** Update component state */
  updateComponentState(
    id: ComponentId,
    newState: ComponentState
  ): Promise<FlightResult<boolean>>;

  /** List all components */
  listComponents(
    sessionId?: string,
    stateFilter?: ComponentState
  ): Promise<FlightResult<ComponentInfo[]>>;

  /** Get component execution context */
  getExecutionContext(id: ComponentId): Promise<FlightResult<ExecutionContext>>;
}

// Component resource management functions
export interface ComponentResourceManager {
  /** Allocate resource for component */
  allocateResource(
    component: ComponentId,
    resourceType: ResourceType,
    size: MemorySize
  ): Promise<FlightResult<ResourceHandle>>;

  /** Release component resource */
  releaseResource(handle: ResourceHandle): Promise<FlightResult<boolean>>;

  /** Get resource information */
  getResource(handle: ResourceHandle): Promise<FlightResult<ResourceInfo>>;

  /** List component resources */
  listComponentResources(
    component: ComponentId
  ): Promise<FlightResult<ResourceInfo[]>>;

  /** Share resource between components */
  shareResource(
    handle: ResourceHandle,
    targetComponent: ComponentId
  ): Promise<FlightResult<boolean>>;

  /** Add component dependency */
  addDependency(
    dependent: ComponentId,
    dependency: string,
    dependencyType: DependencyType,
    optional: boolean
  ): Promise<FlightResult<boolean>>;

  /** Resolve component dependencies */
  resolveDependencies(
    component: ComponentId
  ): Promise<FlightResult<ComponentDependency[]>>;

  /** Check dependency status */
  checkDependencies(component: ComponentId): Promise<FlightResult<boolean>>;
}

// Inter-component communication functions
export interface ComponentCommunicationManager {
  /** Register component interface */
  registerInterface(
    provider: ComponentId,
    interfaceName: InterfaceName,
    version: string,
    methods: string[]
  ): Promise<FlightResult<boolean>>;

  /** Connect to component interface */
  connectInterface(
    consumer: ComponentId,
    interfaceName: InterfaceName
  ): Promise<FlightResult<ComponentInterface>>;

  /** Disconnect from interface */
  disconnectInterface(
    consumer: ComponentId,
    interfaceName: InterfaceName
  ): Promise<FlightResult<boolean>>;

  /** List available interfaces */
  listInterfaces(): Promise<FlightResult<ComponentInterface[]>>;

  /** Get interface status */
  getInterfaceStatus(
    interfaceName: InterfaceName
  ): Promise<FlightResult<InterfaceState>>;

  /** Send message between components */
  sendMessage(
    from: ComponentId,
    to: ComponentId,
    interfaceName: InterfaceName,
    method: string,
    payload: string
  ): Promise<FlightResult<string>>;
}

// Flight-Core specific component configurations
export interface FlightCoreComponentConfig {
  /** Platform-specific HAL configuration */
  halConfig: {
    platform: string;
    cpuArch: string;
    memoryConstraint: string;
    graphicsApi: string;
    soundSystem?: string;
  };
  /** Runtime configuration */
  runtimeConfig: {
    executionMode: ExecutionMode;
    wasmEngine: string;
    cppStandard: string;
    threadingModel: string;
  };
  /** Build configuration */
  buildConfig: {
    targetTriple: string;
    toolchain: string;
    compileFlags: string[];
    linkFlags: string[];
    requiredLibs: string[];
    includePaths: string[];
  };
}

// Utility functions for working with components
export class ComponentUtils {
  /** Check if component is in active state */
  static isActive(state: ComponentState): boolean {
    return state === ComponentState.Running || state === ComponentState.Instantiated;
  }

  /** Check if component can be started */
  static canStart(state: ComponentState): boolean {
    return state === ComponentState.Instantiated || state === ComponentState.Suspended;
  }

  /** Check if component can be suspended */
  static canSuspend(state: ComponentState): boolean {
    return state === ComponentState.Running;
  }

  /** Check if component can be terminated */
  static canTerminate(state: ComponentState): boolean {
    return state !== ComponentState.Terminated && state !== ComponentState.Terminating;
  }

  /** Get memory usage percentage */
  static getMemoryUsagePercentage(info: ComponentInfo): number {
    const total = info.memoryUsage.total.bytes;
    const used = info.memoryUsage.used.bytes;
    return total > 0 ? (Number(used) / Number(total)) * 100 : 0;
  }

  /** Format component uptime */
  static getUptime(info: ComponentInfo): string {
    const uptimeMs = Date.now() - info.createdAt * 1000;
    const hours = Math.floor(uptimeMs / (1000 * 60 * 60));
    const minutes = Math.floor((uptimeMs % (1000 * 60 * 60)) / (1000 * 60));
    const seconds = Math.floor((uptimeMs % (1000 * 60)) / 1000);
    
    if (hours > 0) {
      return `${hours}h ${minutes}m ${seconds}s`;
    } else if (minutes > 0) {
      return `${minutes}m ${seconds}s`;
    } else {
      return `${seconds}s`;
    }
  }

  /** Create component metadata map */
  static getMetadataMap(metadata: [string, string][]): Map<string, string> {
    return new Map(metadata);
  }

  /** Get component type from metadata */
  static getComponentType(info: ComponentInfo): string {
    const metadataMap = this.getMetadataMap(info.metadata);
    return metadataMap.get('component_type') || 'unknown';
  }

  /** Check if component is platform-specific */
  static isPlatformSpecific(info: ComponentInfo): boolean {
    const metadataMap = this.getMetadataMap(info.metadata);
    return metadataMap.get('platform_specific') === 'true';
  }
}

// Component event types for monitoring and logging
export interface ComponentEvent {
  /** Event timestamp */
  timestamp: number;
  /** Component that generated the event */
  componentId: ComponentId;
  /** Event type */
  eventType: ComponentEventType;
  /** Event message */
  message: string;
  /** Additional event data */
  data?: Record<string, any>;
}

export enum ComponentEventType {
  Created = 'created',
  Started = 'started',
  Suspended = 'suspended',
  Resumed = 'resumed',
  Terminated = 'terminated',
  ErrorOccurred = 'error-occurred',
  ResourceAllocated = 'resource-allocated',
  ResourceReleased = 'resource-released',
  InterfaceConnected = 'interface-connected',
  InterfaceDisconnected = 'interface-disconnected',
  DependencyResolved = 'dependency-resolved',
  StateChanged = 'state-changed',
}

// Component health monitoring
export interface ComponentHealthStatus {
  /** Component identifier */
  componentId: ComponentId;
  /** Overall health status */
  healthy: boolean;
  /** Health check timestamp */
  lastCheck: number;
  /** Health issues found */
  issues: ComponentHealthIssue[];
  /** Performance metrics */
  metrics: ComponentMetrics;
}

export interface ComponentHealthIssue {
  /** Issue severity */
  severity: 'info' | 'warning' | 'error' | 'critical';
  /** Issue description */
  description: string;
  /** Suggested resolution */
  resolution?: string;
}

export interface ComponentMetrics {
  /** CPU usage percentage */
  cpuUsage: number;
  /** Memory usage percentage */
  memoryUsage: number;
  /** Response time in milliseconds */
  responseTime: number;
  /** Error rate (0.0-1.0) */
  errorRate: number;
  /** Request count */
  requestCount: number;
  /** Active connections */
  activeConnections: number;
}
