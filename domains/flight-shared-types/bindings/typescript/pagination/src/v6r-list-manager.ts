/**
 * V6R List Manager - TypeScript implementation for V6R cloud integration
 */

import {
  ListManagement,
  BulkOperations,
  ListRequest,
  ListResponse,
  SearchParams,
  SearchResult,
  AggregationRequest,
  AggregationResult,
  PlatformConstraints,
  ListMetrics,
  BulkOperationRequest,
  BulkOperationResult,
  V6RListManagerConfig,
  V6RListContext,
  V6RSubscriptionTier,
  V6RVM,
  FlightResult,
  FlightError,
  ErrorSeverity,
  ErrorCategory,
  ErrorContext,
  MemorySize
} from './types';

export class V6RListManager implements ListManagement, BulkOperations {
  private config: V6RListManagerConfig;
  private cache: Map<string, { data: any; expiresAt: number }> = new Map();

  constructor(config: V6RListManagerConfig) {
    this.config = config;
  }

  // Platform constraints based on subscription tier
  private getV6RConstraints(tier: V6RSubscriptionTier): PlatformConstraints {
    switch (tier) {
      case 'free':
        return {
          maxPerPage: 10,
          maxPages: 100,
          memoryLimit: { bytes: BigInt(5 * 1024 * 1024), humanReadable: '5MB' },
          supportedSearchModes: ['simple'],
          supportedFilters: ['equals', 'contains'],
          cacheEnabled: true,
          realtimeUpdates: false,
        };
      case 'individual':
        return {
          maxPerPage: 50,
          maxPages: 1000,
          memoryLimit: { bytes: BigInt(20 * 1024 * 1024), humanReadable: '20MB' },
          supportedSearchModes: ['simple', 'full-text'],
          supportedFilters: ['equals', 'not-equals', 'contains', 'starts-with', 'in-list', 'date-range'],
          cacheEnabled: true,
          realtimeUpdates: true,
        };
      case 'team':
        return {
          maxPerPage: 100,
          maxPages: 5000,
          memoryLimit: { bytes: BigInt(100 * 1024 * 1024), humanReadable: '100MB' },
          supportedSearchModes: ['simple', 'full-text', 'fuzzy', 'boolean'],
          supportedFilters: [
            'equals', 'not-equals', 'contains', 'not-contains', 'starts-with', 'ends-with',
            'in-list', 'not-in-list', 'greater-than', 'less-than', 'between', 'date-range'
          ],
          cacheEnabled: true,
          realtimeUpdates: true,
        };
      case 'enterprise':
        return {
          maxPerPage: 500,
          memoryLimit: { bytes: BigInt(500 * 1024 * 1024), humanReadable: '500MB' },
          supportedSearchModes: ['simple', 'full-text', 'fuzzy', 'regex', 'boolean', 'phrase'],
          supportedFilters: [
            'equals', 'not-equals', 'contains', 'not-contains', 'starts-with', 'ends-with',
            'in-list', 'not-in-list', 'greater-than', 'greater-than-or-equal', 'less-than',
            'less-than-or-equal', 'between', 'regex', 'is-null', 'is-not-null', 'date-range'
          ],
          cacheEnabled: true,
          realtimeUpdates: true,
        };
    }
  }

  // Create error helper
  private createError(message: string, category: ErrorCategory = ErrorCategory.ServiceIntegration): FlightError {
    return {
      id: `v6r-${Date.now()}-${Math.random().toString(36).substr(2, 9)}`,
      severity: ErrorSeverity.Error,
      category,
      message,
      context: {
        source: 'V6RListManager',
        operation: 'listItems',
        platform: 'v6r',
        serviceId: 'v6r-api',
        metadata: [['subscription_tier', this.config.subscriptionTier]]
      },
      timestamp: Date.now(),
    };
  }

  // HTTP client with retry logic
  private async makeRequest<T>(
    endpoint: string,
    options: {
      method?: string;
      headers?: Record<string, string>;
      body?: string;
      signal?: AbortSignal;
    } = {}
  ): Promise<FlightResult<T>> {
    const url = `${this.config.apiUrl}${endpoint}`;
    const headers = {
      'Authorization': `Bearer ${this.config.authToken}`,
      'Content-Type': 'application/json',
      ...options.headers,
    };

    for (let attempt = 0; attempt <= this.config.retryAttempts; attempt++) {
      try {
        const controller = new AbortController();
        const timeoutId = setTimeout(() => controller.abort(), this.config.timeoutMs);

        const response = await fetch(url, {
          ...options,
          headers,
          signal: controller.signal,
        });

        clearTimeout(timeoutId);

        if (!response.ok) {
          if (attempt === this.config.retryAttempts) {
            return {
              tag: 'err',
              val: this.createError(`HTTP ${response.status}: ${response.statusText}`, 'network')
            };
          }
          continue;
        }

        const data = await response.json();
        return { tag: 'ok', val: data };

      } catch (error) {
        if (attempt === this.config.retryAttempts) {
          return {
            tag: 'err',
            val: this.createError(
              error instanceof Error ? error.message : 'Unknown network error',
              'network'
            )
          };
        }
        // Exponential backoff
        await new Promise(resolve => setTimeout(resolve, Math.pow(2, attempt) * 1000));
      }
    }

    return {
      tag: 'err',
      val: this.createError('Max retry attempts exceeded', 'network')
    };
  }

  // Cache management
  private getCacheKey(endpoint: string, params: any): string {
    return `${endpoint}:${JSON.stringify(params)}`;
  }

  private getCachedResult<T>(cacheKey: string): T | null {
    if (!this.config.cacheEnabled) return null;
    
    const cached = this.cache.get(cacheKey);
    if (!cached) return null;
    
    if (Date.now() > cached.expiresAt) {
      this.cache.delete(cacheKey);
      return null;
    }
    
    return cached.data;
  }

  private setCachedResult<T>(cacheKey: string, data: T, ttlSeconds: number): void {
    if (!this.config.cacheEnabled) return;
    
    this.cache.set(cacheKey, {
      data,
      expiresAt: Date.now() + (ttlSeconds * 1000)
    });
  }

  // ListManagement implementation
  async listItems<T>(request: ListRequest, context?: string): Promise<FlightResult<ListResponse<T>>> {
    const cacheKey = this.getCacheKey('/list', { request, context });
    const cached = this.getCachedResult<ListResponse<T>>(cacheKey);
    
    if (cached) {
      return { tag: 'ok', val: cached };
    }

    const constraints = this.getV6RConstraints(this.config.subscriptionTier);
    
    // Validate request against platform constraints
    if (request.perPage && request.perPage > constraints.maxPerPage) {
      return {
        tag: 'err',
        val: this.createError(
          `Page size ${request.perPage} exceeds maximum ${constraints.maxPerPage} for ${this.config.subscriptionTier} tier`,
          'validation'
        )
      };
    }

    const result = await this.makeRequest<ListResponse<T>>('/v6r/list', {
      method: 'POST',
      body: JSON.stringify({ request, context })
    });

    if (result.tag === 'ok') {
      this.setCachedResult(cacheKey, result.val, 300); // 5 minute cache
    }

    return result;
  }

  async searchItems<T>(
    search: SearchParams,
    request: ListRequest,
    context?: string
  ): Promise<FlightResult<ListResponse<SearchResult<T>>>> {
    const constraints = this.getV6RConstraints(this.config.subscriptionTier);
    
    // Validate search mode
    if (!constraints.supportedSearchModes.includes(search.mode)) {
      return {
        tag: 'err',
        val: this.createError(
          `Search mode '${search.mode}' not supported for ${this.config.subscriptionTier} tier`,
          'validation'
        )
      };
    }

    const cacheKey = this.getCacheKey('/search', { search, request, context });
    const cached = this.getCachedResult<ListResponse<SearchResult<T>>>(cacheKey);
    
    if (cached) {
      return { tag: 'ok', val: cached };
    }

    const result = await this.makeRequest<ListResponse<SearchResult<T>>>('/v6r/search', {
      method: 'POST',
      body: JSON.stringify({ search, request, context })
    });

    if (result.tag === 'ok') {
      this.setCachedResult(cacheKey, result.val, 300);
    }

    return result;
  }

  async aggregateItems(
    aggregation: AggregationRequest,
    context?: string
  ): Promise<FlightResult<AggregationResult>> {
    return this.makeRequest<AggregationResult>('/v6r/aggregate', {
      method: 'POST',
      body: JSON.stringify({ aggregation, context })
    });
  }

  async getPlatformConstraints(platform: string): Promise<FlightResult<PlatformConstraints>> {
    if (platform === 'v6r') {
      return {
        tag: 'ok',
        val: this.getV6RConstraints(this.config.subscriptionTier)
      };
    }

    return this.makeRequest<PlatformConstraints>(`/v6r/platform/${platform}/constraints`);
  }

  async getListMetrics(operationId: string): Promise<FlightResult<ListMetrics>> {
    return this.makeRequest<ListMetrics>(`/v6r/metrics/${operationId}`);
  }

  async cacheListResult<T>(
    cacheKey: string,
    result: ListResponse<T>,
    ttlSeconds: number
  ): Promise<FlightResult<boolean>> {
    this.setCachedResult(cacheKey, result, ttlSeconds);
    return { tag: 'ok', val: true };
  }

  async getCachedResult<T>(cacheKey: string): Promise<FlightResult<ListResponse<T> | null>> {
    const cached = this.getCachedResult<ListResponse<T>>(cacheKey);
    return { tag: 'ok', val: cached };
  }

  // BulkOperations implementation
  async bulkOperation<T>(
    request: BulkOperationRequest<T>,
    context?: string
  ): Promise<FlightResult<BulkOperationResult>> {
    const constraints = this.getV6RConstraints(this.config.subscriptionTier);
    
    // Adjust batch size based on tier
    const maxBatchSize = constraints.maxPerPage;
    if (request.batchSize && request.batchSize > maxBatchSize) {
      request.batchSize = maxBatchSize;
    }

    return this.makeRequest<BulkOperationResult>('/v6r/bulk', {
      method: 'POST',
      body: JSON.stringify({ request, context })
    });
  }

  async getBulkOperationStatus(operationId: string): Promise<FlightResult<BulkOperationResult>> {
    return this.makeRequest<BulkOperationResult>(`/v6r/bulk/${operationId}/status`);
  }

  async cancelBulkOperation(operationId: string): Promise<FlightResult<boolean>> {
    const result = await this.makeRequest<{ cancelled: boolean }>(`/v6r/bulk/${operationId}/cancel`, {
      method: 'POST'
    });
    
    if (result.tag === 'ok') {
      return { tag: 'ok', val: result.val.cancelled };
    }
    return result as FlightResult<boolean>;
  }

  async listBulkOperations(
    request: ListRequest,
    context?: string
  ): Promise<FlightResult<ListResponse<BulkOperationResult>>> {
    return this.makeRequest<ListResponse<BulkOperationResult>>('/v6r/bulk/list', {
      method: 'POST',
      body: JSON.stringify({ request, context })
    });
  }

  // V6R-specific VM management methods
  async listVMs(request: ListRequest, context?: V6RListContext): Promise<FlightResult<ListResponse<V6RVM>>> {
    const contextStr = context ? JSON.stringify(context) : undefined;
    return this.listItems<V6RVM>(request, contextStr);
  }

  async searchVMs(
    search: SearchParams,
    request: ListRequest,
    context?: V6RListContext
  ): Promise<FlightResult<ListResponse<SearchResult<V6RVM>>>> {
    const contextStr = context ? JSON.stringify(context) : undefined;
    return this.searchItems<V6RVM>(search, request, contextStr);
  }

  // Helper methods for creating common V6R requests
  createVMListRequest(userId: string, vmSize?: string, status?: string): ListRequest {
    const filters = [
      { field: 'user_id', operation: 'equals' as const, values: [userId] }
    ];

    if (vmSize) {
      filters.push({ field: 'size', operation: 'equals' as const, values: [vmSize] });
    }

    if (status) {
      filters.push({ field: 'status', operation: 'equals' as const, values: [status] });
    }

    const constraints = this.getV6RConstraints(this.config.subscriptionTier);

    return {
      page: 1,
      perPage: Math.min(this.config.defaultPageSize, constraints.maxPerPage),
      sortBy: 'created_at',
      sortOrder: 'desc',
      filters,
      includeTotal: true,
    };
  }

  createV6RContext(
    userId: string,
    organizationId?: string,
    teamId?: string,
    vmId?: string,
    projectId?: string
  ): V6RListContext {
    return {
      userId,
      organizationId,
      teamId,
      subscriptionTier: this.config.subscriptionTier,
      vmId,
      projectId,
    };
  }

  // Memory usage aggregation
  async getMemoryUsageStats(userId: string): Promise<FlightResult<AggregationResult>> {
    return this.aggregateItems({
      field: 'memory_usage',
      aggregationType: 'memory-stats',
      limit: 10,
      filters: [
        { field: 'user_id', operation: 'equals', values: [userId] }
      ]
    });
  }

  // Real-time updates (if supported by tier)
  supportsRealtime(): boolean {
    const constraints = this.getV6RConstraints(this.config.subscriptionTier);
    return constraints.realtimeUpdates && this.config.realtimeEnabled;
  }

  // Clear cache
  clearCache(): void {
    this.cache.clear();
  }

  // Get cache statistics
  getCacheStats(): { size: number; hitRate: number } {
    // This is a simplified implementation
    return {
      size: this.cache.size,
      hitRate: 0.8 // Would be calculated from actual usage
    };
  }
}

// Factory function for easy instantiation
export function createV6RListManager(config: V6RListManagerConfig): V6RListManager {
  return new V6RListManager(config);
}

// Default configuration for different tiers
export const V6R_DEFAULT_CONFIGS: Record<V6RSubscriptionTier, Partial<V6RListManagerConfig>> = {
  free: {
    defaultPageSize: 10,
    cacheEnabled: true,
    realtimeEnabled: false,
    retryAttempts: 2,
    timeoutMs: 5000,
  },
  individual: {
    defaultPageSize: 25,
    cacheEnabled: true,
    realtimeEnabled: true,
    retryAttempts: 3,
    timeoutMs: 10000,
  },
  team: {
    defaultPageSize: 50,
    cacheEnabled: true,
    realtimeEnabled: true,
    retryAttempts: 3,
    timeoutMs: 15000,
  },
  enterprise: {
    defaultPageSize: 100,
    cacheEnabled: true,
    realtimeEnabled: true,
    retryAttempts: 5,
    timeoutMs: 30000,
  },
};

// Helper to merge config with defaults
export function createV6RConfig(
  apiUrl: string,
  authToken: string,
  subscriptionTier: V6RSubscriptionTier,
  overrides?: Partial<V6RListManagerConfig>
): V6RListManagerConfig {
  const defaults = V6R_DEFAULT_CONFIGS[subscriptionTier];
  
  return {
    apiUrl,
    authToken,
    subscriptionTier,
    ...defaults,
    ...overrides,
  } as V6RListManagerConfig;
}
