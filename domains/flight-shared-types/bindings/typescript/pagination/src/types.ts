/**
 * TypeScript bindings for Flight pagination and list management types
 * Generated from wit/pagination.wit
 */

// Core shared types (from Flight error and memory types)
export interface FlightError {
  readonly id: string;
  readonly severity: ErrorSeverity;
  readonly category: ErrorCategory;
  readonly message: string;
  readonly details?: string;
  readonly context: ErrorContext;
  readonly timestamp: number;
  readonly cause?: string;
}

export interface ErrorContext {
  readonly source: string;
  readonly operation: string;
  readonly sessionId?: string;
  readonly userId?: string;
  readonly platform?: string;
  readonly serviceId?: string;
  readonly metadata: ReadonlyArray<readonly [string, string]>;
}

export enum ErrorSeverity {
  Info = "info",
  Warning = "warning",
  Error = "error",
  Critical = "critical",
  Fatal = "fatal"
}

export enum ErrorCategory {
  Memory = "memory",
  Platform = "platform",
  Network = "network",
  Validation = "validation",
  Security = "security",
  Component = "component",
  ServiceIntegration = "service-integration",
  FlightSystem = "flight-system",
  Application = "application",
  Unknown = "unknown"
}

export type FlightResult<T> = 
  | { readonly tag: "ok"; readonly val: T }
  | { readonly tag: "err"; readonly val: FlightError };

export interface MemorySize {
  readonly bytes: bigint;
  readonly humanReadable: string;
}

// Core pagination types
export interface Pagination {
  page: number;
  perPage: number;
  total: number;
  totalPages: number;
  hasNext: boolean;
  hasPrevious: boolean;
  cursor?: string;
  nextCursor?: string;
  prevCursor?: string;
}

export type SortOrder = 'asc' | 'desc';

export type FilterOperation = 
  | 'equals'
  | 'not-equals'
  | 'contains'
  | 'not-contains'
  | 'starts-with'
  | 'ends-with'
  | 'in-list'
  | 'not-in-list'
  | 'greater-than'
  | 'greater-than-or-equal'
  | 'less-than'
  | 'less-than-or-equal'
  | 'between'
  | 'regex'
  | 'is-null'
  | 'is-not-null'
  | 'memory-above'
  | 'memory-below'
  | 'date-range';

export interface ListFilter {
  field: string;
  operation: FilterOperation;
  values: string[];
  caseSensitive?: boolean;
}

export interface ListRequest {
  page?: number;
  perPage?: number;
  sortBy?: string;
  sortOrder?: SortOrder;
  search?: string;
  filters: ListFilter[];
  cursor?: string;
  includeTotal?: boolean;
  memoryLimit?: MemorySize;
}

export interface CacheInfo {
  fromCache: boolean;
  expiresAt: number;
  cacheKey?: string;
  hitRate?: number;
}

export interface ListResponse<T> {
  items: T[];
  pagination: Pagination;
  metadata: Record<string, string>;
  generatedAt: number;
  cacheInfo?: CacheInfo;
}

// Search types
export type SearchMode = 
  | 'simple'
  | 'full-text'
  | 'fuzzy'
  | 'regex'
  | 'phrase'
  | 'boolean';

export interface SearchParams {
  query: string;
  fields?: string[];
  mode: SearchMode;
  fuzzyTolerance?: number;
  minWordLength?: number;
  language?: string;
  highlight?: boolean;
}

export interface FieldMatch {
  field: string;
  text: string;
  position: number;
  length: number;
}

export interface SearchResult<T> {
  item: T;
  score: number;
  highlights: string[];
  fieldMatches: FieldMatch[];
}

// Aggregation types
export type AggregationType = 
  | 'count'
  | 'sum'
  | 'avg'
  | 'min'
  | 'max'
  | 'distinct-count'
  | 'memory-stats'
  | 'date-histogram';

export interface AggregationRequest {
  field: string;
  aggregationType: AggregationType;
  limit?: number;
  filters: ListFilter[];
}

export interface AggregationBucket {
  key: string;
  value: number;
  docCount: number;
}

export interface AggregationResult {
  field: string;
  aggregationType: AggregationType;
  buckets: AggregationBucket[];
  totalItems: number;
}

// Platform constraints
export interface PlatformConstraints {
  maxPerPage: number;
  maxPages?: number;
  memoryLimit: MemorySize;
  supportedSearchModes: SearchMode[];
  supportedFilters: FilterOperation[];
  cacheEnabled: boolean;
  realtimeUpdates: boolean;
}

// Context types
export interface FlightListContext {
  platform: string;
  sessionId?: string;
  componentId?: string;
  halSubsystem?: string;
  memoryPool?: string;
}

export interface V6RListContext {
  userId: string;
  organizationId?: string;
  teamId?: string;
  subscriptionTier: string;
  vmId?: string;
  projectId?: string;
}

// Metrics
export interface ListMetrics {
  queryTimeMs: number;
  totalTimeMs: number;
  itemsProcessed: number;
  itemsReturned: number;
  memoryUsed: MemorySize;
  cacheHit: boolean;
  indexUsed?: string;
  warnings: string[];
}

// Bulk operations
export type BulkOperationType = 
  | 'create'
  | 'update'
  | 'delete'
  | 'move'
  | 'copy'
  | 'export'
  | 'archive';

export interface BulkOperationRequest<T> {
  operation: BulkOperationType;
  items: T[];
  filters: ListFilter[];
  parameters: Record<string, string>;
  batchSize?: number;
  parallel?: boolean;
}

export interface BulkItemResult {
  itemId: string;
  success: boolean;
  error?: string;
  updatedItem?: string; // JSON representation
}

export interface BulkOperationResult {
  operation: BulkOperationType;
  processed: number;
  succeeded: number;
  failed: number;
  itemResults: BulkItemResult[];
  metrics: ListMetrics;
  errors: string[];
}

// List management interface
export interface ListManagement {
  listItems<T>(request: ListRequest, context?: string): Promise<FlightResult<ListResponse<T>>>;
  searchItems<T>(search: SearchParams, request: ListRequest, context?: string): Promise<FlightResult<ListResponse<SearchResult<T>>>>;
  aggregateItems(aggregation: AggregationRequest, context?: string): Promise<FlightResult<AggregationResult>>;
  getPlatformConstraints(platform: string): Promise<FlightResult<PlatformConstraints>>;
  getListMetrics(operationId: string): Promise<FlightResult<ListMetrics>>;
  cacheListResult<T>(cacheKey: string, result: ListResponse<T>, ttlSeconds: number): Promise<FlightResult<boolean>>;
  getCachedResult<T>(cacheKey: string): Promise<FlightResult<ListResponse<T> | null>>;
}

// Bulk operations interface
export interface BulkOperations {
  bulkOperation<T>(request: BulkOperationRequest<T>, context?: string): Promise<FlightResult<BulkOperationResult>>;
  getBulkOperationStatus(operationId: string): Promise<FlightResult<BulkOperationResult>>;
  cancelBulkOperation(operationId: string): Promise<FlightResult<boolean>>;
  listBulkOperations(request: ListRequest, context?: string): Promise<FlightResult<ListResponse<BulkOperationResult>>>;
}

// V6R specific types
export type V6RSubscriptionTier = 'free' | 'individual' | 'team' | 'enterprise';

export interface V6RListManagerConfig {
  apiUrl: string;
  authToken: string;
  subscriptionTier: V6RSubscriptionTier;
  defaultPageSize: number;
  cacheEnabled: boolean;
  realtimeEnabled: boolean;
  retryAttempts: number;
  timeoutMs: number;
}

export interface V6RVM {
  id: string;
  name: string;
  size: string;
  status: 'running' | 'stopped' | 'suspended' | 'pending';
  createdAt: number;
  updatedAt: number;
  userId: string;
  organizationId?: string;
  teamId?: string;
  projectId?: string;
  memoryUsage: MemorySize;
  cpuUsage: number;
  diskUsage: MemorySize;
  networkUsage: MemorySize;
  metadata: Record<string, string>;
}

export interface FlightComponent {
  id: string;
  name: string;
  platform: string;
  status: 'active' | 'inactive' | 'loading' | 'error';
  memoryUsage: MemorySize;
  lastActivity: number;
  sessionId?: string;
  halSubsystem?: string;
  memoryPool?: string;
  metadata: Record<string, string>;
}

// Helper types for React integration
export interface UsePaginatedListOptions<T> {
  initialPage?: number;
  initialPageSize?: number;
  initialFilters?: ListFilter[];
  initialSort?: { field: string; order: SortOrder };
  autoRefresh?: boolean;
  refreshInterval?: number;
  cacheEnabled?: boolean;
  searchDebounceMs?: number;
}

export interface PaginatedListState<T> {
  data: T[];
  pagination: Pagination;
  loading: boolean;
  error: FlightError | null;
  metrics?: ListMetrics;
  cacheInfo?: CacheInfo;
}

export interface PaginatedListActions {
  setPage: (page: number) => void;
  setPageSize: (size: number) => void;
  setFilters: (filters: ListFilter[]) => void;
  addFilter: (filter: ListFilter) => void;
  removeFilter: (field: string) => void;
  setSort: (field: string, order: SortOrder) => void;
  search: (query: string) => void;
  refresh: () => void;
  reset: () => void;
}

export interface UseSearchOptions {
  mode?: SearchMode;
  fields?: string[];
  fuzzyTolerance?: number;
  minWordLength?: number;
  language?: string;
  highlight?: boolean;
  debounceMs?: number;
}

export interface SearchState<T> {
  results: SearchResult<T>[];
  pagination: Pagination;
  loading: boolean;
  error: FlightError | null;
  query: string;
  metrics?: ListMetrics;
}

export interface SearchActions {
  search: (query: string) => void;
  setPage: (page: number) => void;
  setPageSize: (size: number) => void;
  clear: () => void;
  refine: (filters: ListFilter[]) => void;
}

// Event types for real-time updates
export interface ListUpdateEvent<T> {
  type: 'item-added' | 'item-updated' | 'item-removed' | 'list-refreshed';
  item?: T;
  items?: T[];
  context: string;
  timestamp: number;
}

export interface ListEventSubscription {
  unsubscribe: () => void;
}

export type ListEventHandler<T> = (event: ListUpdateEvent<T>) => void;
