/**
 * Flight Pagination Types - Main Export
 */

// Export all types
export * from './types';

// Export V6R list manager (commented out due to browser API dependencies)
// export * from './v6r-list-manager';

// Export utility functions
export function createListRequest(options: {
  page?: number;
  perPage?: number;
  sortBy?: string;
  sortOrder?: 'asc' | 'desc';
  filters?: Array<{
    field: string;
    operation: import('./types').FilterOperation;
    values: string[];
  }>;
}): import('./types').ListRequest {
  const result: import('./types').ListRequest = {
    filters: options.filters || [],
    includeTotal: true,
  };
  
  if (options.page !== undefined) result.page = options.page;
  if (options.perPage !== undefined) result.perPage = options.perPage;
  if (options.sortBy !== undefined) result.sortBy = options.sortBy;
  if (options.sortOrder !== undefined) result.sortOrder = options.sortOrder;
  
  return result;
}

export function createPlatformConstraints(platform: string): import('./types').PlatformConstraints {
  const baseConstraints = {
    cacheEnabled: true,
    realtimeUpdates: true,
  };

  switch (platform) {
    case 'dreamcast':
      return {
        ...baseConstraints,
        maxPerPage: 10,
        maxPages: 100,
        memoryLimit: { bytes: BigInt(1024 * 1024), humanReadable: '1MB' },
        supportedSearchModes: ['simple'],
        supportedFilters: ['equals', 'contains'],
        cacheEnabled: false,
        realtimeUpdates: false,
      };
    case 'psp':
      return {
        ...baseConstraints,
        maxPerPage: 25,
        maxPages: 500,
        memoryLimit: { bytes: BigInt(2 * 1024 * 1024), humanReadable: '2MB' },
        supportedSearchModes: ['simple', 'full-text'],
        supportedFilters: ['equals', 'not-equals', 'contains', 'starts-with'],
        realtimeUpdates: false,
      };
    case 'v6r-free':
      return {
        ...baseConstraints,
        maxPerPage: 10,
        maxPages: 100,
        memoryLimit: { bytes: BigInt(5 * 1024 * 1024), humanReadable: '5MB' },
        supportedSearchModes: ['simple'],
        supportedFilters: ['equals', 'contains'],
        realtimeUpdates: false,
      };
    case 'v6r-individual':
      return {
        ...baseConstraints,
        maxPerPage: 50,
        maxPages: 1000,
        memoryLimit: { bytes: BigInt(20 * 1024 * 1024), humanReadable: '20MB' },
        supportedSearchModes: ['simple', 'full-text'],
        supportedFilters: ['equals', 'not-equals', 'contains', 'starts-with', 'in-list', 'date-range'],
      };
    case 'v6r-team':
      return {
        ...baseConstraints,
        maxPerPage: 100,
        maxPages: 5000,
        memoryLimit: { bytes: BigInt(100 * 1024 * 1024), humanReadable: '100MB' },
        supportedSearchModes: ['simple', 'full-text', 'fuzzy', 'boolean'],
        supportedFilters: [
          'equals', 'not-equals', 'contains', 'not-contains', 'starts-with', 'ends-with',
          'in-list', 'not-in-list', 'greater-than', 'less-than', 'between', 'date-range'
        ],
      };
    case 'v6r-enterprise':
      return {
        ...baseConstraints,
        maxPerPage: 500,
        memoryLimit: { bytes: BigInt(500 * 1024 * 1024), humanReadable: '500MB' },
        supportedSearchModes: ['simple', 'full-text', 'fuzzy', 'regex', 'boolean', 'phrase'],
        supportedFilters: [
          'equals', 'not-equals', 'contains', 'not-contains', 'starts-with', 'ends-with',
          'in-list', 'not-in-list', 'greater-than', 'greater-than-or-equal', 'less-than',
          'less-than-or-equal', 'between', 'regex', 'is-null', 'is-not-null', 'date-range'
        ],
      };
    default:
      return {
        ...baseConstraints,
        maxPerPage: 100,
        memoryLimit: { bytes: BigInt(50 * 1024 * 1024), humanReadable: '50MB' },
        supportedSearchModes: ['simple', 'full-text', 'fuzzy', 'regex', 'boolean', 'phrase'],
        supportedFilters: [
          'equals', 'not-equals', 'contains', 'not-contains', 'starts-with', 'ends-with',
          'in-list', 'not-in-list', 'greater-than', 'greater-than-or-equal', 'less-than',
          'less-than-or-equal', 'between', 'regex', 'is-null', 'is-not-null', 'date-range'
        ],
      };
  }
}

export function createSearchParams(
  query: string,
  options?: {
    mode?: import('./types').SearchMode;
    fields?: string[];
    fuzzyTolerance?: number;
    highlight?: boolean;
  }
): import('./types').SearchParams {
  const result: import('./types').SearchParams = {
    query,
    mode: options?.mode || 'simple',
    minWordLength: 2,
    language: 'en',
    highlight: options?.highlight ?? true,
  };
  
  if (options?.fields !== undefined) result.fields = options.fields;
  if (options?.fuzzyTolerance !== undefined) result.fuzzyTolerance = options.fuzzyTolerance;
  
  return result;
}

export function createMemorySize(bytes: number): import('./types').MemorySize {
  const formatBytes = (bytes: number): string => {
    const KB = 1024;
    const MB = KB * 1024;
    const GB = MB * 1024;

    if (bytes >= GB) {
      return `${(bytes / GB).toFixed(1)}GB`;
    } else if (bytes >= MB) {
      return `${(bytes / MB).toFixed(1)}MB`;
    } else if (bytes >= KB) {
      return `${(bytes / KB).toFixed(1)}KB`;
    } else {
      return `${bytes}B`;
    }
  };

  return {
    bytes: BigInt(bytes),
    humanReadable: formatBytes(bytes),
  };
}

// Validation helpers  
import { ErrorSeverity, ErrorCategory } from './types';

export function validateListRequest(
  request: import('./types').ListRequest,
  constraints: import('./types').PlatformConstraints
): import('./types').FlightResult<boolean> {
  if (request.perPage && request.perPage > constraints.maxPerPage) {
    return {
      tag: 'err',
      val: {
        id: `validation-${Date.now()}`,
        severity: ErrorSeverity.Error,
        category: ErrorCategory.Validation,
        message: `Page size ${request.perPage} exceeds maximum ${constraints.maxPerPage}`,
        context: {
          source: 'validateListRequest',
          operation: 'validation',
          metadata: []
        },
        timestamp: Date.now(),
      }
    };
  }

  return { tag: 'ok', val: true };
}

export function validateSearchParams(
  searchParams: import('./types').SearchParams,
  constraints: import('./types').PlatformConstraints
): import('./types').FlightResult<boolean> {
  if (!constraints.supportedSearchModes.includes(searchParams.mode)) {
    return {
      tag: 'err',
      val: {
        id: `validation-${Date.now()}`,
        severity: ErrorSeverity.Error,
        category: ErrorCategory.Validation,
        message: `Search mode '${searchParams.mode}' not supported`,
        context: {
          source: 'validateSearchParams',
          operation: 'validation',
          metadata: []
        },
        timestamp: Date.now(),
      }
    };
  }

  return { tag: 'ok', val: true };
}
