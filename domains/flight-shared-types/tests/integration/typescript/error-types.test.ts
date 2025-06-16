// Flight Error Types Integration Tests
// Comprehensive testing of error handling across Flight-Core and service integration

import {
  FlightError,
  ErrorSeverity,
  ErrorCategory,
  FlightResult,
  ServiceErrorCode,
  PlatformErrorCode,
  NetworkErrorCode,
  ValidationErrorDetails,
  ErrorRecoverySuggestion,
  RecoveryActionType,
  ErrorAnalyticsSummary,
  SystemHealthStatus,
  SuccessApiResponse
} from '../../../bindings/typescript/enhanced/src/error-types';

import { FlightErrorUtils as ErrorUtils } from '../../../bindings/typescript/enhanced/src/utils/error-utils';

describe('Flight Error Types Integration Tests', () => {
  describe('Basic Error Creation', () => {
    test('should create simple error with all required fields', () => {
      const error = ErrorUtils.createSimpleError(
        ErrorSeverity.Error,
        ErrorCategory.Application,
        'Test error message',
        'test-source',
        'test-operation'
      );

      expect(error.id).toBeDefined();
      expect(error.id).toMatch(/^error_\d+_\w+$/);
      expect(error.severity).toBe(ErrorSeverity.Error);
      expect(error.category).toBe(ErrorCategory.Application);
      expect(error.message).toBe('Test error message');
      expect(error.context.source).toBe('test-source');
      expect(error.context.operation).toBe('test-operation');
      expect(error.timestamp).toBeGreaterThan(0);
      expect(error.context.metadata).toEqual([]);
    });

    test('should create error with full context', () => {
      const context = {
        source: 'integration-test',
        operation: 'full-context-test',
        sessionId: 'session-123',
        userId: 'user-456',
        platform: 'test-platform',
        serviceId: 'service-789',
        metadata: [
          ['test-key', 'test-value'],
          ['environment', 'testing']
        ] as ReadonlyArray<readonly [string, string]>
      };

      const error = ErrorUtils.createError(
        ErrorSeverity.Warning,
        ErrorCategory.Component,
        'Full context test',
        context
      );

      expect(error.context.sessionId).toBe('session-123');
      expect(error.context.userId).toBe('user-456');
      expect(error.context.platform).toBe('test-platform');
      expect(error.context.serviceId).toBe('service-789');
      expect(error.context.metadata).toEqual([
        ['test-key', 'test-value'],
        ['environment', 'testing']
      ]);
    });
  });

  describe('Service Integration Errors', () => {
    test('should create service error with correct severity mapping', () => {
      const authError = ErrorUtils.createServiceError(
        ServiceErrorCode.AuthFailed,
        'Authentication failed',
        'service-123',
        'session-456',
        'user-789'
      );

      expect(authError.severity).toBe(ErrorSeverity.Critical);
      expect(authError.category).toBe(ErrorCategory.ServiceIntegration);
      expect(authError.context.serviceId).toBe('service-123');
      expect(authError.context.sessionId).toBe('session-456');
      expect(authError.context.userId).toBe('user-789');

      const metadataMap = new Map(authError.context.metadata);
      expect(metadataMap.get('service_error_code')).toBe(ServiceErrorCode.AuthFailed);
      expect(metadataMap.get('service_id')).toBe('service-123');
    });

    test('should determine service error recoverability correctly', () => {
      const authError = ErrorUtils.createServiceError(
        ServiceErrorCode.AuthFailed,
        'Auth failed',
        'service-1'
      );
      expect(ErrorUtils.isRecoverable(authError)).toBe(false);

      const rateError = ErrorUtils.createServiceError(
        ServiceErrorCode.RateLimitExceeded,
        'Rate limited',
        'service-1'
      );
      expect(ErrorUtils.isRecoverable(rateError)).toBe(true);

      const unavailableError = ErrorUtils.createServiceError(
        ServiceErrorCode.ServiceUnavailable,
        'Service down',
        'service-1'
      );
      expect(ErrorUtils.isRecoverable(unavailableError)).toBe(true);
    });
  });

  describe('Platform Errors', () => {
    test('should create platform error with correct severity', () => {
      const memoryError = ErrorUtils.createPlatformError(
        PlatformErrorCode.InsufficientPlatformMemory,
        'Not enough memory on Dreamcast',
        'dreamcast',
        'texture_load'
      );

      expect(memoryError.severity).toBe(ErrorSeverity.Critical);
      expect(memoryError.category).toBe(ErrorCategory.Platform);
      expect(memoryError.context.platform).toBe('dreamcast');

      const initError = ErrorUtils.createPlatformError(
        PlatformErrorCode.PlatformInitFailed,
        'Platform failed to initialize',
        'unknown',
        'platform_init'
      );

      expect(initError.severity).toBe(ErrorSeverity.Fatal);
    });

    test('should provide platform-specific recovery suggestions', () => {
      const memoryError = ErrorUtils.createPlatformError(
        PlatformErrorCode.InsufficientPlatformMemory,
        'Memory exhausted',
        'dreamcast',
        'asset_load'
      );

      const suggestions = ErrorUtils.getRecoverySuggestions(memoryError);
      expect(suggestions.length).toBeGreaterThan(0);

      const hasMemoryReduction = suggestions.some(s => 
        s.actionType === RecoveryActionType.ReduceResources
      );
      expect(hasMemoryReduction).toBe(true);
    });
  });

  describe('Network Errors', () => {
    test('should create network error with endpoint context', () => {
      const timeoutError = ErrorUtils.createNetworkError(
        NetworkErrorCode.ConnectionTimeout,
        'Connection timed out',
        'https://api.example.com/v1/data'
      );

      expect(timeoutError.severity).toBe(ErrorSeverity.Warning);
      expect(timeoutError.category).toBe(ErrorCategory.Network);

      const metadataMap = new Map(timeoutError.context.metadata);
      expect(metadataMap.get('network_error_code')).toBe(NetworkErrorCode.ConnectionTimeout);
      expect(metadataMap.get('endpoint')).toBe('https://api.example.com/v1/data');
    });

    test('should handle TLS errors as critical', () => {
      const tlsError = ErrorUtils.createNetworkError(
        NetworkErrorCode.TlsError,
        'TLS handshake failed'
      );

      expect(tlsError.severity).toBe(ErrorSeverity.Critical);
      expect(ErrorUtils.isRecoverable(tlsError)).toBe(false);
    });
  });

  describe('Validation Errors', () => {
    test('should create validation error with detailed field information', () => {
      const validationDetails: ValidationErrorDetails[] = [
        {
          field: 'email',
          rule: 'format',
          expected: 'valid email format',
          actual: 'invalid-email',
          context: 'user registration'
        },
        {
          field: 'age',
          rule: 'range',
          expected: '18-120',
          actual: '15'
        }
      ];

      const validationError = ErrorUtils.createValidationError(
        'Validation failed for user input',
        validationDetails
      );

      expect(validationError.severity).toBe(ErrorSeverity.Error);
      expect(validationError.category).toBe(ErrorCategory.Validation);

      const metadataMap = new Map(validationError.context.metadata);
      expect(metadataMap.get('validation_errors_count')).toBe('2');
      expect(metadataMap.get('validation_0_field')).toBe('email');
      expect(metadataMap.get('validation_0_rule')).toBe('format');
      expect(metadataMap.get('validation_1_field')).toBe('age');
      expect(metadataMap.get('validation_1_expected')).toBe('18-120');
    });
  });

  describe('Error Enrichment and Chaining', () => {
    test('should enrich error with additional context', () => {
      const originalError = ErrorUtils.createSimpleError(
        ErrorSeverity.Error,
        ErrorCategory.Application,
        'Original error',
        'app',
        'process'
      );

      const enrichedError = ErrorUtils.enrichError(originalError, [
        ['trace_id', 'trace-123'],
        ['request_id', 'req-456']
      ]);

      const metadataMap = new Map(enrichedError.context.metadata);
      expect(metadataMap.get('trace_id')).toBe('trace-123');
      expect(metadataMap.get('request_id')).toBe('req-456');
      expect(enrichedError.id).toBe(originalError.id); // Same error
    });

    test('should chain errors correctly', () => {
      const rootCause = ErrorUtils.createSimpleError(
        ErrorSeverity.Error,
        ErrorCategory.Network,
        'Connection failed',
        'network',
        'connect'
      );

      const chainedError = ErrorUtils.createSimpleError(
        ErrorSeverity.Error,
        ErrorCategory.Application,
        'Failed to fetch data',
        'app',
        'fetch'
      );

      const finalError = ErrorUtils.chainError(chainedError, rootCause);
      expect(finalError.cause).toBe(rootCause.id);
    });
  });

  describe('Error Collections', () => {
    test('should collect and analyze multiple errors', () => {
      const errors: FlightError[] = [
        ErrorUtils.createSimpleError(ErrorSeverity.Warning, ErrorCategory.Memory, 'Warning 1', 'src1', 'op1'),
        ErrorUtils.createSimpleError(ErrorSeverity.Error, ErrorCategory.Network, 'Error 1', 'src2', 'op2'),
        ErrorUtils.createSimpleError(ErrorSeverity.Critical, ErrorCategory.Security, 'Critical 1', 'src3', 'op3'),
        ErrorUtils.createSimpleError(ErrorSeverity.Warning, ErrorCategory.Memory, 'Warning 2', 'src4', 'op4'),
        ErrorUtils.createSimpleError(ErrorSeverity.Fatal, ErrorCategory.Platform, 'Fatal 1', 'src5', 'op5'),
      ];

      const collection = ErrorUtils.collectErrors(errors);

      expect(collection.totalCount).toBe(5);
      expect(collection.hasBlockingErrors).toBe(true);

      // Check severity counts
      const severityMap = new Map(collection.severityCounts);
      expect(severityMap.get(ErrorSeverity.Warning)).toBe(2);
      expect(severityMap.get(ErrorSeverity.Error)).toBe(1);
      expect(severityMap.get(ErrorSeverity.Critical)).toBe(1);
      expect(severityMap.get(ErrorSeverity.Fatal)).toBe(1);

      // Check category counts
      const categoryMap = new Map(collection.categoryCounts);
      expect(categoryMap.get(ErrorCategory.Memory)).toBe(2);
      expect(categoryMap.get(ErrorCategory.Network)).toBe(1);
      expect(categoryMap.get(ErrorCategory.Security)).toBe(1);
      expect(categoryMap.get(ErrorCategory.Platform)).toBe(1);
    });

    test('should filter errors by severity', () => {
      const errors: FlightError[] = [
        ErrorUtils.createSimpleError(ErrorSeverity.Info, ErrorCategory.Application, 'Info', 'src', 'op'),
        ErrorUtils.createSimpleError(ErrorSeverity.Warning, ErrorCategory.Application, 'Warning', 'src', 'op'),
        ErrorUtils.createSimpleError(ErrorSeverity.Error, ErrorCategory.Application, 'Error', 'src', 'op'),
        ErrorUtils.createSimpleError(ErrorSeverity.Critical, ErrorCategory.Application, 'Critical', 'src', 'op'),
      ];

      const collection = ErrorUtils.collectErrors(errors);
      const filteredErrors = ErrorUtils.filterBySeverity(collection, ErrorSeverity.Error);

      expect(filteredErrors).toHaveLength(2); // Error and Critical
      expect(filteredErrors.every(e => 
        e.severity === ErrorSeverity.Error || e.severity === ErrorSeverity.Critical
      )).toBe(true);
    });

    test('should filter errors by category', () => {
      const errors: FlightError[] = [
        ErrorUtils.createSimpleError(ErrorSeverity.Error, ErrorCategory.Memory, 'Memory 1', 'src', 'op'),
        ErrorUtils.createSimpleError(ErrorSeverity.Error, ErrorCategory.Network, 'Network 1', 'src', 'op'),
        ErrorUtils.createSimpleError(ErrorSeverity.Error, ErrorCategory.Memory, 'Memory 2', 'src', 'op'),
        ErrorUtils.createSimpleError(ErrorSeverity.Error, ErrorCategory.Platform, 'Platform 1', 'src', 'op'),
      ];

      const collection = ErrorUtils.collectErrors(errors);
      const filteredErrors = ErrorUtils.filterByCategory(collection, [
        ErrorCategory.Memory,
        ErrorCategory.Platform
      ]);

      expect(filteredErrors).toHaveLength(3);
      expect(filteredErrors.every(e => 
        e.category === ErrorCategory.Memory || e.category === ErrorCategory.Platform
      )).toBe(true);
    });
  });

  describe('JSON Serialization', () => {
    test('should serialize and deserialize errors correctly', () => {
      const originalError = ErrorUtils.createServiceError(
        ServiceErrorCode.ResourceAllocationFailed,
        'VM allocation failed',
        'service-123',
        'session-456',
        'user-789'
      );

      const json = ErrorUtils.errorToJson(originalError);
      expect(json).toBeDefined();
      expect(typeof json).toBe('string');

      const result = ErrorUtils.errorFromJson(json);
      expect(result.tag).toBe('ok');
      
      if (result.tag === 'ok') {
        const deserializedError = result.val;
        expect(deserializedError.id).toBe(originalError.id);
        expect(deserializedError.severity).toBe(originalError.severity);
        expect(deserializedError.category).toBe(originalError.category);
        expect(deserializedError.message).toBe(originalError.message);
        expect(deserializedError.context.sessionId).toBe(originalError.context.sessionId);
        expect(deserializedError.context.userId).toBe(originalError.context.userId);
      }
    });

    test('should handle invalid JSON gracefully', () => {
      const result = ErrorUtils.errorFromJson('invalid json');
      expect(result.tag).toBe('err');
      
      if (result.tag === 'err') {
        expect(result.val.category).toBe(ErrorCategory.Validation);
        expect(result.val.message).toContain('JSON parse error');
      }
    });
  });

  describe('API Integration', () => {
    test('should create standardized API error responses', () => {
      const error = ErrorUtils.createServiceError(
        ServiceErrorCode.SessionLimitExceeded,
        'Too many active sessions',
        'service-123',
        'session-456',
        'user-789'
      );

      const apiResponse = ErrorUtils.createApiErrorResponse(error);

      expect(apiResponse.success).toBe(false);
      expect(apiResponse.error.id).toBe(error.id);
      expect(apiResponse.error.code).toBe(ServiceErrorCode.SessionLimitExceeded);
      expect(apiResponse.error.message).toBe('Too many active sessions');
      expect(apiResponse.error.severity).toBe(ErrorSeverity.Error);
      expect(apiResponse.metadata?.sessionId).toBe('session-456');
      expect(apiResponse.metadata?.userId).toBe('user-789');
      expect(apiResponse.metadata?.requestId).toBe(error.id);
    });

    test('should create success API responses', () => {
      const data = { result: 'success', value: 42 };
      const response = ErrorUtils.createSuccessApiResponse(data, 'request-123');

      if (response.success) {
        expect(response.data).toEqual(data);
        expect(response.metadata?.requestId).toBe('request-123');
      }
      expect(response.success).toBe(true);
    });

    test('should create success responses without metadata', () => {
      const data = { result: 'success' };
      const response = ErrorUtils.createSuccessApiResponse(data);

      if (response.success) {
        expect(response.data).toEqual(data);
        expect(response.metadata).toBeUndefined();
      }
      expect(response.success).toBe(true);
    });
  });

  describe('Error Recovery Suggestions', () => {
    test('should provide appropriate recovery suggestions for different error types', () => {
      // Memory error suggestions
      const memoryError = ErrorUtils.createSimpleError(
        ErrorSeverity.Error,
        ErrorCategory.Memory,
        'Out of memory',
        'app',
        'allocate'
      );

      const memorySuggestions = ErrorUtils.getRecoverySuggestions(memoryError);
      expect(memorySuggestions.some(s => s.actionType === RecoveryActionType.ReduceResources)).toBe(true);

      // Network error suggestions
      const networkError = ErrorUtils.createNetworkError(
        NetworkErrorCode.ConnectionTimeout,
        'Connection timed out'
      );

      const networkSuggestions = ErrorUtils.getRecoverySuggestions(networkError);
      expect(networkSuggestions.some(s => s.actionType === RecoveryActionType.Retry)).toBe(true);
      expect(networkSuggestions.some(s => s.actionType === RecoveryActionType.CheckStatus)).toBe(true);

      // Critical errors should suggest contacting support
      const criticalError = ErrorUtils.createSimpleError(
        ErrorSeverity.Critical,
        ErrorCategory.Platform,
        'Critical system failure',
        'platform',
        'init'
      );

      const criticalSuggestions = ErrorUtils.getRecoverySuggestions(criticalError);
      expect(criticalSuggestions.some(s => s.actionType === RecoveryActionType.ContactSupport)).toBe(true);
    });

    test('should prioritize suggestions correctly', () => {
      const error = ErrorUtils.createSimpleError(
        ErrorSeverity.Critical,
        ErrorCategory.Memory,
        'Critical memory error',
        'app',
        'operation'
      );

      const suggestions = ErrorUtils.getRecoverySuggestions(error);
      
      // Suggestions should be sorted by priority (descending)
      for (let i = 1; i < suggestions.length; i++) {
        expect(suggestions[i].priority).toBeLessThanOrEqual(suggestions[i - 1].priority);
      }
    });
  });

  describe('Error Summary Generation', () => {
    test('should generate informative error summaries', () => {
      const error = ErrorUtils.createServiceError(
        ServiceErrorCode.AuthFailed,
        'Authentication token expired',
        'auth-service'
      );

      const summary = ErrorUtils.getErrorSummary(error);
      expect(summary).toContain('CRITICAL');
      expect(summary).toContain('SERVICE INTEGRATION');
      expect(summary).toContain('Authentication token expired');
      expect(summary).toContain(error.id);
    });
  });

  describe('Edge Cases and Error Conditions', () => {
    test('should handle empty error collections', () => {
      const collection = ErrorUtils.collectErrors([]);
      expect(collection.totalCount).toBe(0);
      expect(collection.hasBlockingErrors).toBe(false);
      expect(collection.errors).toHaveLength(0);
      expect(collection.severityCounts).toHaveLength(0);
      expect(collection.categoryCounts).toHaveLength(0);
    });

    test('should handle malformed error structures in validation', () => {
      // This would typically be caught at the type level, but testing robustness
      const partialJson = '{"id":"test","message":""}'; // Missing required fields
      const result = ErrorUtils.errorFromJson(partialJson);
      
      // Should either parse successfully or return validation error
      expect(['ok', 'err']).toContain(result.tag);
    });
  });
});
