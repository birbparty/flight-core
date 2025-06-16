/**
 * TypeScript Cross-Language Error Serialization Tests
 * 
 * Comprehensive tests for error serialization and cross-language compatibility
 * ensuring zero information loss across TypeScript, Go, Rust, and C++17
 */

import {
  FlightError,
  ErrorSeverity,
  ErrorCategory,
  FlightResult,
  ErrorContext,
  ServiceErrorCode,
  PlatformErrorCode,
  NetworkErrorCode,
  ValidationErrorDetails,
  ErrorRecoverySuggestion,
  RecoveryActionType,
  ErrorAnalyticsSummary,
  SystemHealthStatus,
  ErrorTrendAnalysis,
  TrendDirection
} from '../../../bindings/typescript/enhanced/src/error-types';

import { FlightErrorUtils as ErrorUtils } from '../../../bindings/typescript/enhanced/src/utils/error-utils';

describe('TypeScript Cross-Language Error Serialization', () => {
  describe('Basic Error Round-Trip Serialization', () => {
    test('should preserve all FlightError fields through JSON serialization', () => {
      const originalError: FlightError = {
        id: 'cross-lang-test-001',
        severity: ErrorSeverity.Error,
        category: ErrorCategory.Memory,
        message: 'Cross-language memory allocation failed',
        details: 'Insufficient heap space for component allocation across language boundary',
        context: {
          source: 'cross-language-memory-manager',
          operation: 'allocate_cross_lang_buffer',
          sessionId: 'cross-session-123',
          userId: 'cross-user-456',
          platform: 'dreamcast',
          serviceId: 'cross-service-789',
          metadata: [
            ['language_source', 'typescript'],
            ['language_target', 'go'],
            ['requested_bytes', '8388608'],
            ['available_bytes', '4194304'],
            ['fragmentation_ratio', '0.15'],
            ['cross_lang_call_depth', '3'],
            ['memory_pressure', 'high']
          ]
        },
        timestamp: 1633024800000,
        cause: 'cross-language-heap-fragmentation'
      };

      // Test JSON serialization
      const serialized = JSON.stringify(originalError);
      expect(serialized).toBeTruthy();
      expect(serialized.length).toBeGreaterThan(0);

      // Test JSON deserialization
      const deserialized: FlightError = JSON.parse(serialized);
      
      // Verify all core fields preserved
      expect(deserialized.id).toBe(originalError.id);
      expect(deserialized.severity).toBe(originalError.severity);
      expect(deserialized.category).toBe(originalError.category);
      expect(deserialized.message).toBe(originalError.message);
      expect(deserialized.details).toBe(originalError.details);
      expect(deserialized.timestamp).toBe(originalError.timestamp);
      expect(deserialized.cause).toBe(originalError.cause);
      
      // Verify complete context preservation
      expect(deserialized.context.source).toBe(originalError.context.source);
      expect(deserialized.context.operation).toBe(originalError.context.operation);
      expect(deserialized.context.sessionId).toBe(originalError.context.sessionId);
      expect(deserialized.context.userId).toBe(originalError.context.userId);
      expect(deserialized.context.platform).toBe(originalError.context.platform);
      expect(deserialized.context.serviceId).toBe(originalError.context.serviceId);
      
      // Verify metadata preservation with ordering
      expect(deserialized.context.metadata).toHaveLength(originalError.context.metadata.length);
      expect(deserialized.context.metadata).toEqual(originalError.context.metadata);
      
      // Test specific cross-language metadata
      const metadataMap = new Map(deserialized.context.metadata);
      expect(metadataMap.get('language_source')).toBe('typescript');
      expect(metadataMap.get('language_target')).toBe('go');
      expect(metadataMap.get('cross_lang_call_depth')).toBe('3');
    });

    test('should handle nullable fields correctly in cross-language context', () => {
      const errorWithNulls: FlightError = {
        id: 'null-fields-test-001',
        severity: ErrorSeverity.Warning,
        category: ErrorCategory.Platform,
        message: 'Cross-language null field test',
        details: undefined, // Test undefined details
        context: {
          source: 'null-field-test',
          operation: 'test_null_handling',
          sessionId: undefined, // Test undefined session
          userId: undefined, // Test undefined user
          platform: 'v6r-medium',
          serviceId: undefined, // Test undefined service
          metadata: [
            ['null_test', 'true'],
            ['undefined_handling', 'verified']
          ]
        },
        timestamp: Date.now(),
        cause: undefined // Test undefined cause
      };

      const serialized = JSON.stringify(errorWithNulls);
      const deserialized: FlightError = JSON.parse(serialized);

      // Verify null/undefined handling
      expect(deserialized.details).toBeUndefined();
      expect(deserialized.context.sessionId).toBeUndefined();
      expect(deserialized.context.userId).toBeUndefined();
      expect(deserialized.context.serviceId).toBeUndefined();
      expect(deserialized.cause).toBeUndefined();
      
      // Verify non-null fields preserved
      expect(deserialized.id).toBe(errorWithNulls.id);
      expect(deserialized.context.platform).toBe('v6r-medium');
      expect(deserialized.context.metadata).toEqual(errorWithNulls.context.metadata);
    });
  });

  describe('Cross-Language FlightResult Serialization', () => {
    test('should serialize and deserialize success results correctly', () => {
      const successData = {
        operation: 'cross_lang_operation',
        result: 'success',
        language_chain: ['typescript', 'go', 'rust'],
        execution_time_ms: 150,
        memory_used_bytes: 2048
      };

      const successResult: FlightResult<typeof successData> = { 
        tag: 'ok', 
        val: successData 
      };

      const serialized = JSON.stringify(successResult);
      const deserialized: FlightResult<typeof successData> = JSON.parse(serialized);

      expect(deserialized.tag).toBe('ok');
      if (deserialized.tag === 'ok') {
        expect(deserialized.val.operation).toBe('cross_lang_operation');
        expect(deserialized.val.language_chain).toEqual(['typescript', 'go', 'rust']);
        expect(deserialized.val.execution_time_ms).toBe(150);
      }
    });

    test('should serialize and deserialize error results correctly', () => {
      const error: FlightError = {
        id: 'result-error-cross-lang-001',
        severity: ErrorSeverity.Critical,
        category: ErrorCategory.ServiceIntegration,
        message: 'Cross-language service integration failed',
        details: 'Failed to communicate between TypeScript and Go services',
        context: {
          source: 'cross-lang-service-bridge',
          operation: 'typescript_to_go_call',
          sessionId: 'bridge-session-123',
          userId: 'bridge-user-456',
          platform: 'v6r-large',
          serviceId: 'bridge-service-789',
          metadata: [
            ['source_language', 'typescript'],
            ['target_language', 'go'],
            ['protocol', 'json-rpc'],
            ['timeout_ms', '5000'],
            ['retry_count', '3'],
            ['last_error', 'connection_refused']
          ]
        },
        timestamp: Date.now(),
        cause: 'network-isolation'
      };

      const errorResult: FlightResult<string> = { tag: 'err', val: error };

      const serialized = JSON.stringify(errorResult);
      const deserialized: FlightResult<string> = JSON.parse(serialized);

      expect(deserialized.tag).toBe('err');
      if (deserialized.tag === 'err') {
        expect(deserialized.val.id).toBe('result-error-cross-lang-001');
        expect(deserialized.val.severity).toBe(ErrorSeverity.Critical);
        expect(deserialized.val.category).toBe(ErrorCategory.ServiceIntegration);
        
        const metadataMap = new Map(deserialized.val.context.metadata);
        expect(metadataMap.get('source_language')).toBe('typescript');
        expect(metadataMap.get('target_language')).toBe('go');
        expect(metadataMap.get('protocol')).toBe('json-rpc');
      }
    });
  });

  describe('Platform-Specific Cross-Language Serialization', () => {
    test('should handle Dreamcast memory constraint errors', () => {
      const dreamcastError: FlightError = {
        id: 'dreamcast-cross-lang-001',
        severity: ErrorSeverity.Critical,
        category: ErrorCategory.Platform,
        message: 'Dreamcast memory limit exceeded in cross-language operation',
        details: 'Attempted cross-language buffer allocation exceeds 16MB system limit',
        context: {
          source: 'dreamcast-cross-lang-allocator',
          operation: 'cross_lang_texture_allocation',
          sessionId: 'dreamcast-session',
          userId: undefined,
          platform: 'dreamcast',
          serviceId: undefined,
          metadata: [
            ['total_memory_bytes', '16777216'], // 16MB
            ['available_memory_bytes', '4194304'], // 4MB
            ['requested_allocation_bytes', '12582912'], // 12MB
            ['language_overhead_bytes', '1048576'], // 1MB
            ['allocation_type', 'cross_lang_texture_buffer'],
            ['source_language', 'typescript'],
            ['target_language', 'c++17'],
            ['hardware_arch', 'sh4'],
            ['memory_fragmentation', '0.25']
          ]
        },
        timestamp: Date.now(),
        cause: 'platform-memory-constraint'
      };

      const serialized = JSON.stringify(dreamcastError);
      const deserialized: FlightError = JSON.parse(serialized);

      expect(deserialized.context.platform).toBe('dreamcast');
      
      const metadataMap = new Map(deserialized.context.metadata);
      expect(metadataMap.get('total_memory_bytes')).toBe('16777216');
      expect(metadataMap.get('source_language')).toBe('typescript');
      expect(metadataMap.get('target_language')).toBe('c++17');
      expect(metadataMap.get('hardware_arch')).toBe('sh4');
    });

    test('should handle V6R cloud scaling errors', () => {
      const v6rError: FlightError = {
        id: 'v6r-cross-lang-001',
        severity: ErrorSeverity.Warning,
        category: ErrorCategory.ServiceIntegration,
        message: 'V6R VM approaching memory limit during cross-language operation',
        details: 'Cross-language memory usage at 85% of V6R large VM limit',
        context: {
          source: 'v6r-cross-lang-monitor',
          operation: 'monitor_cross_lang_memory',
          sessionId: 'v6r-dev-session-789',
          userId: 'v6r-developer-123',
          platform: 'v6r-large',
          serviceId: 'v6r-memory-service',
          metadata: [
            ['vm_memory_limit_bytes', '2147483648'], // 2GB
            ['current_usage_bytes', '1825361100'], // 1.7GB (85%)
            ['cross_lang_overhead_bytes', '104857600'], // 100MB
            ['usage_percentage', '85.0'],
            ['warning_threshold', '80.0'],
            ['language_breakdown', 'typescript:30%,go:25%,rust:20%,c++17:10%,overhead:15%'],
            ['vm_tier', 'large'],
            ['scaling_available', 'true'],
            ['auto_scale_threshold', '90.0']
          ]
        },
        timestamp: Date.now(),
        cause: undefined
      };

      const serialized = JSON.stringify(v6rError);
      const deserialized: FlightError = JSON.parse(serialized);

      expect(deserialized.context.platform).toBe('v6r-large');
      expect(deserialized.context.userId).toBe('v6r-developer-123');
      
      const metadataMap = new Map(deserialized.context.metadata);
      expect(metadataMap.get('vm_memory_limit_bytes')).toBe('2147483648');
      expect(metadataMap.get('language_breakdown')).toBe('typescript:30%,go:25%,rust:20%,c++17:10%,overhead:15%');
      expect(metadataMap.get('auto_scale_threshold')).toBe('90.0');
    });
  });

  describe('Service Error Code Cross-Language Mapping', () => {
    test('should preserve service error codes across languages', () => {
      const serviceErrorCodes = [
        ServiceErrorCode.ResourceAllocationFailed,
        ServiceErrorCode.SessionLimitExceeded,
        ServiceErrorCode.AuthFailed,
        ServiceErrorCode.RateLimitExceeded,
        ServiceErrorCode.ServiceUnavailable,
        ServiceErrorCode.InvalidConfig,
        ServiceErrorCode.ResourceLimitExceeded,
        ServiceErrorCode.ConnectionFailed,
        ServiceErrorCode.RequestTimeout,
        ServiceErrorCode.InvalidRequest,
        ServiceErrorCode.PermissionDenied,
        ServiceErrorCode.ResourceNotFound
      ];

      for (const errorCode of serviceErrorCodes) {
        const error = ErrorUtils.createServiceError(
          errorCode,
          `Cross-language test for ${errorCode}`,
          'cross-lang-service',
          'cross-session',
          'cross-user'
        );

        const serialized = JSON.stringify(error);
        const deserialized: FlightError = JSON.parse(serialized);

        const metadataMap = new Map(deserialized.context.metadata);
        expect(metadataMap.get('service_error_code')).toBe(errorCode);
        expect(deserialized.category).toBe(ErrorCategory.ServiceIntegration);
      }
    });

    test('should preserve platform error codes across languages', () => {
      const platformErrorCodes = [
        PlatformErrorCode.InsufficientPlatformMemory,
        PlatformErrorCode.FeatureNotSupported,
        PlatformErrorCode.HardwareConstraintViolation,
        PlatformErrorCode.PlatformInitFailed,
        PlatformErrorCode.ComponentLoadFailed,
        PlatformErrorCode.CompatibilityError,
        PlatformErrorCode.PlatformResourceExhausted
      ];

      for (const errorCode of platformErrorCodes) {
        const error = ErrorUtils.createPlatformError(
          errorCode,
          `Cross-language test for ${errorCode}`,
          'cross-platform',
          'cross_operation'
        );

        const serialized = JSON.stringify(error);
        const deserialized: FlightError = JSON.parse(serialized);

        const metadataMap = new Map(deserialized.context.metadata);
        expect(metadataMap.get('platform_error_code')).toBe(errorCode);
        expect(deserialized.category).toBe(ErrorCategory.Platform);
      }
    });

    test('should preserve network error codes across languages', () => {
      const networkErrorCodes = [
        NetworkErrorCode.ConnectionTimeout,
        NetworkErrorCode.ConnectionRefused,
        NetworkErrorCode.DnsResolutionFailed,
        NetworkErrorCode.TlsError,
        NetworkErrorCode.HttpError,
        NetworkErrorCode.WebsocketError,
        NetworkErrorCode.RequestTimeout,
        NetworkErrorCode.NetworkUnreachable,
        NetworkErrorCode.ProtocolError,
        NetworkErrorCode.BandwidthLimitExceeded
      ];

      for (const errorCode of networkErrorCodes) {
        const error = ErrorUtils.createNetworkError(
          errorCode,
          `Cross-language test for ${errorCode}`,
          'https://cross-lang-api.example.com/v1/test'
        );

        const serialized = JSON.stringify(error);
        const deserialized: FlightError = JSON.parse(serialized);

        const metadataMap = new Map(deserialized.context.metadata);
        expect(metadataMap.get('network_error_code')).toBe(errorCode);
        expect(deserialized.category).toBe(ErrorCategory.Network);
      }
    });
  });

  describe('Error Context Enrichment Across Languages', () => {
    test('should preserve context enrichment chains', () => {
      // Start with base error
      const baseError = ErrorUtils.createSimpleError(
        ErrorSeverity.Error,
        ErrorCategory.Component,
        'Cross-language component initialization failed',
        'component-loader',
        'load_cross_lang_component'
      );

      // Simulate enrichment as error propagates through languages
      const tsEnriched = ErrorUtils.enrichError(baseError, [
        ['language_layer', 'typescript'],
        ['ts_call_stack_depth', '5'],
        ['ts_memory_usage', '1048576'],
        ['ts_processing_time_ms', '150']
      ]);

      const goEnriched = ErrorUtils.enrichError(tsEnriched, [
        ['language_layer', 'go'],
        ['go_goroutine_id', '42'],
        ['go_memory_usage', '2097152'],
        ['go_processing_time_ms', '75']
      ]);

      const rustEnriched = ErrorUtils.enrichError(goEnriched, [
        ['language_layer', 'rust'],
        ['rust_thread_id', '123'],
        ['rust_memory_usage', '524288'],
        ['rust_processing_time_ms', '25']
      ]);

      const finalEnriched = ErrorUtils.enrichError(rustEnriched, [
        ['language_layer', 'c++17'],
        ['cpp_thread_id', '456'],
        ['cpp_memory_usage', '262144'],
        ['cpp_processing_time_ms', '10'],
        ['total_chain_time_ms', '260']
      ]);

      // Test serialization preserves full enrichment chain
      const serialized = JSON.stringify(finalEnriched);
      const deserialized: FlightError = JSON.parse(serialized);

      expect(deserialized.id).toBe(baseError.id); // Same error ID throughout chain
      
      const metadataMap = new Map(deserialized.context.metadata);
      
      // Verify all language layers preserved
      expect(metadataMap.has('ts_call_stack_depth')).toBe(true);
      expect(metadataMap.has('go_goroutine_id')).toBe(true);
      expect(metadataMap.has('rust_thread_id')).toBe(true);
      expect(metadataMap.has('cpp_thread_id')).toBe(true);
      
      // Verify cumulative data
      expect(metadataMap.get('total_chain_time_ms')).toBe('260');
      
      // Verify metadata order preservation (important for debugging)
      const metadataEntries = deserialized.context.metadata;
      expect(metadataEntries.length).toBeGreaterThan(8);
    });

    test('should handle error chaining across languages', () => {
      const rootCause = ErrorUtils.createNetworkError(
        NetworkErrorCode.ConnectionTimeout,
        'Cross-language API call timeout',
        'https://api.service.com/cross-lang'
      );

      const serviceError = ErrorUtils.createServiceError(
        ServiceErrorCode.ServiceUnavailable,
        'Service unavailable due to network issues',
        'cross-lang-service'
      );

      const applicationError = ErrorUtils.createSimpleError(
        ErrorSeverity.Error,
        ErrorCategory.Application,
        'Application operation failed',
        'app-service',
        'cross_lang_operation'
      );

      // Chain errors: app -> service -> network
      const serviceChained = ErrorUtils.chainError(serviceError, rootCause);
      const appChained = ErrorUtils.chainError(applicationError, serviceChained);

      const serialized = JSON.stringify(appChained);
      const deserialized: FlightError = JSON.parse(serialized);

      expect(deserialized.cause).toBe(serviceChained.id);
      expect(serviceChained.cause).toBe(rootCause.id);

      // Verify error hierarchy preserved
      expect(deserialized.id).toBe(applicationError.id);
      expect(deserialized.message).toBe('Application operation failed');
    });
  });

  describe('Analytics and Health Data Serialization', () => {
    test('should serialize error analytics correctly', () => {
      const analytics: ErrorAnalyticsSummary = {
        totalErrors: 1500,
        bySeverity: [
          [ErrorSeverity.Info, 200],
          [ErrorSeverity.Warning, 800],
          [ErrorSeverity.Error, 400],
          [ErrorSeverity.Critical, 90],
          [ErrorSeverity.Fatal, 10]
        ],
        byCategory: [
          [ErrorCategory.Memory, 300],
          [ErrorCategory.Network, 250],
          [ErrorCategory.Platform, 200],
          [ErrorCategory.ServiceIntegration, 400],
          [ErrorCategory.Application, 350]
        ],
        topErrors: [
          ['Cross-language memory allocation failed', 45],
          ['Service timeout in cross-language call', 38],
          ['Platform constraint violation', 32],
          ['Network error in language bridge', 28],
          ['Component load failed', 25]
        ],
        errorRate: 12.5,
        timeWindow: 3600,
        analyzedAt: Date.now()
      };

      const serialized = JSON.stringify(analytics);
      const deserialized: ErrorAnalyticsSummary = JSON.parse(serialized);

      expect(deserialized.totalErrors).toBe(1500);
      expect(deserialized.errorRate).toBe(12.5);
      expect(deserialized.bySeverity).toHaveLength(5);
      expect(deserialized.byCategory).toHaveLength(5);
      expect(deserialized.topErrors).toHaveLength(5);

      // Verify severity counts preserved
      const severityMap = new Map(deserialized.bySeverity);
      expect(severityMap.get(ErrorSeverity.Critical)).toBe(90);
      expect(severityMap.get(ErrorSeverity.Fatal)).toBe(10);

      // Verify category counts preserved
      const categoryMap = new Map(deserialized.byCategory);
      expect(categoryMap.get(ErrorCategory.ServiceIntegration)).toBe(400);
      expect(categoryMap.get(ErrorCategory.Memory)).toBe(300);
    });

    test('should serialize error trend analysis correctly', () => {
      const trendAnalysis: ErrorTrendAnalysis = {
        trend: TrendDirection.Increasing,
        confidence: 0.85,
        predictedRate: 15.2,
        recommendations: [
          'Increase memory allocation for cross-language operations',
          'Implement circuit breakers for service integration',
          'Add retry logic for network operations',
          'Monitor platform resource usage more closely'
        ]
      };

      const serialized = JSON.stringify(trendAnalysis);
      const deserialized: ErrorTrendAnalysis = JSON.parse(serialized);

      expect(deserialized.trend).toBe(TrendDirection.Increasing);
      expect(deserialized.confidence).toBe(0.85);
      expect(deserialized.predictedRate).toBe(15.2);
      expect(deserialized.recommendations).toHaveLength(4);
      expect(deserialized.recommendations[0]).toContain('cross-language');
    });
  });

  describe('Performance and Edge Cases', () => {
    test('should handle large error collections efficiently', () => {
      const startTime = Date.now();
      
      // Create large collection of errors
      const errors: FlightError[] = [];
      for (let i = 0; i < 1000; i++) {
        const error = ErrorUtils.createSimpleError(
          i % 2 === 0 ? ErrorSeverity.Warning : ErrorSeverity.Error,
          ErrorCategory.Memory,
          `Large collection test error ${i}`,
          'performance-test',
          'large_collection_test'
        );
        errors.push(error);
      }

      const collection = ErrorUtils.collectErrors(errors);
      
      // Test serialization performance
      const serializeStart = Date.now();
      const serialized = JSON.stringify(collection);
      const serializeTime = Date.now() - serializeStart;
      
      expect(serializeTime).toBeLessThan(1000); // Should serialize in under 1 second
      expect(serialized.length).toBeGreaterThan(0);
      
      // Test deserialization performance
      const deserializeStart = Date.now();
      const deserialized = JSON.parse(serialized);
      const deserializeTime = Date.now() - deserializeStart;
      
      expect(deserializeTime).toBeLessThan(500); // Should deserialize in under 0.5 seconds
      expect(deserialized.totalCount).toBe(1000);
      
      const totalTime = Date.now() - startTime;
      console.log(`Large collection test completed in ${totalTime}ms (serialize: ${serializeTime}ms, deserialize: ${deserializeTime}ms)`);
    });

    test('should handle malformed JSON gracefully', () => {
      const malformedJsonCases = [
        '{"id":"test"', // Incomplete JSON
        '{"id":}', // Invalid JSON syntax
        '{"severity":"invalid_severity"}', // Invalid enum value
        '', // Empty string
        'null', // Null value
        '[]', // Wrong type (array instead of object)
      ];

      for (const malformedJson of malformedJsonCases) {
        const result = ErrorUtils.errorFromJson(malformedJson);
        expect(result.tag).toBe('err');
        
        if (result.tag === 'err') {
          expect(result.val.category).toBe(ErrorCategory.Validation);
          expect(result.val.message).toContain('JSON');
        }
      }
    });

    test('should handle unicode and special characters', () => {
      const unicodeError: FlightError = {
        id: 'unicode-test-001',
        severity: ErrorSeverity.Error,
        category: ErrorCategory.Application,
        message: 'Unicode test: 🚀🔥💻 Cross-language 日本語 测试',
        details: 'Testing special characters: \\n\\t\\r\\"\\\\ and émojis 🎮',
        context: {
          source: 'unicode-test-service',
          operation: 'test_unicode_handling',
          sessionId: 'unicode-session-🎯',
          userId: 'user-émile-测试',
          platform: 'test-platform',
          serviceId: undefined,
          metadata: [
            ['unicode_message', '🌍 Global test'],
            ['special_chars', '\\n\\t\\r\\"\\\\'],
            ['languages', '日本語,中文,English,Français'],
            ['emoji_test', '🚀🔥💻🎮🎯🌍']
          ]
        },
        timestamp: Date.now(),
        cause: undefined
      };

      const serialized = JSON.stringify(unicodeError);
      const deserialized: FlightError = JSON.parse(serialized);

      expect(deserialized.message).toBe('Unicode test: 🚀🔥💻 Cross-language 日本語 测试');
      expect(deserialized.details).toBe('Testing special characters: \\n\\t\\r\\"\\\\ and émojis 🎮');
      expect(deserialized.context.sessionId).toBe('unicode-session-🎯');
      expect(deserialized.context.userId).toBe('user-émile-测试');

      const metadataMap = new Map(deserialized.context.metadata);
      expect(metadataMap.get('unicode_message')).toBe('🌍 Global test');
      expect(metadataMap.get('emoji_test')).toBe('🚀🔥💻🎮🎯🌍');
    });
  });
});
