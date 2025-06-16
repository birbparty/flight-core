// Enhanced Flight Error Utilities - Generic Implementation
// Production-ready utilities for error creation and management

import {
  FlightError,
  ErrorContext,
  ErrorSeverity,
  ErrorCategory,
  FlightResult,
  ErrorCollection,
  ServiceErrorCode,
  PlatformErrorCode,
  NetworkErrorCode,
  ValidationErrorDetails,
  ErrorRecoverySuggestion,
  RecoveryActionType,
  ErrorAnalyticsSummary,
  SystemHealthStatus,
  ErrorApiResponse,
  ApiResponse,
  ServiceIntegrationConfig,
  RetryStrategy
} from '../error-types';

export class FlightErrorUtils {
  /**
   * Create a new error with full context
   */
  static createError(
    severity: ErrorSeverity,
    category: ErrorCategory,
    message: string,
    context: ErrorContext
  ): FlightError {
    return {
      id: this.generateErrorId(),
      severity,
      category,
      message,
      context,
      timestamp: Date.now(),
    };
  }

  /**
   * Create error with simple context
   */
  static createSimpleError(
    severity: ErrorSeverity,
    category: ErrorCategory,
    message: string,
    source: string,
    operation: string,
    additionalMetadata: Array<[string, string]> = []
  ): FlightError {
    const context: ErrorContext = {
      source,
      operation,
      metadata: additionalMetadata,
    };

    return this.createError(severity, category, message, context);
  }

  /**
   * Create service integration error
   */
  static createServiceError(
    serviceCode: ServiceErrorCode,
    message: string,
    serviceId: string,
    sessionId?: string,
    userId?: string
  ): FlightError {
    const context: ErrorContext = {
      source: 'service-integration',
      operation: serviceCode,
      serviceId,
      metadata: [
        ['service_error_code', serviceCode],
        ['service_id', serviceId],
      ],
      ...(sessionId && { sessionId }),
      ...(userId && { userId }),
    };

    const severity = this.getServiceErrorSeverity(serviceCode);
    
    return this.createError(severity, ErrorCategory.ServiceIntegration, message, context);
  }

  /**
   * Create platform error
   */
  static createPlatformError(
    platformCode: PlatformErrorCode,
    message: string,
    platform: string,
    operation: string
  ): FlightError {
    const context: ErrorContext = {
      source: 'flight-platform',
      operation,
      platform,
      metadata: [
        ['platform_error_code', platformCode],
        ['platform', platform],
      ],
    };

    const severity = this.getPlatformErrorSeverity(platformCode);
    
    return this.createError(severity, ErrorCategory.Platform, message, context);
  }

  /**
   * Create network error
   */
  static createNetworkError(
    networkCode: NetworkErrorCode,
    message: string,
    endpoint?: string
  ): FlightError {
    const metadata: Array<[string, string]> = [
      ['network_error_code', networkCode],
    ];
    
    if (endpoint) {
      metadata.push(['endpoint', endpoint]);
    }

    const context: ErrorContext = {
      source: 'network-layer',
      operation: networkCode,
      metadata,
    };

    const severity = this.getNetworkErrorSeverity(networkCode);
    
    return this.createError(severity, ErrorCategory.Network, message, context);
  }

  /**
   * Create validation error
   */
  static createValidationError(
    message: string,
    validationDetails: ValidationErrorDetails[],
    source: string = 'validation-layer'
  ): FlightError {
    const metadata: Array<[string, string]> = [
      ['validation_errors_count', validationDetails.length.toString()],
      ...validationDetails.flatMap((detail, index) => [
        [`validation_${index}_field`, detail.field],
        [`validation_${index}_rule`, detail.rule],
        [`validation_${index}_expected`, detail.expected],
        [`validation_${index}_actual`, detail.actual],
      ] as Array<[string, string]>),
    ];

    const context: ErrorContext = {
      source,
      operation: 'validation',
      metadata,
    };

    return this.createError(ErrorSeverity.Error, ErrorCategory.Validation, message, context);
  }

  /**
   * Enrich error with additional context
   */
  static enrichError(
    error: FlightError,
    additionalContext: Array<[string, string]>
  ): FlightError {
    return {
      ...error,
      context: {
        ...error.context,
        metadata: [...error.context.metadata, ...additionalContext],
      },
    };
  }

  /**
   * Chain errors (add causative error)
   */
  static chainError(error: FlightError, cause: FlightError): FlightError {
    return {
      ...error,
      cause: cause.id,
    };
  }

  /**
   * Check if error is recoverable
   */
  static isRecoverable(error: FlightError): boolean {
    // Check by category first
    if (error.category === ErrorCategory.Security) {
      return false; // Security errors typically not recoverable
    }

    if (error.severity === ErrorSeverity.Fatal) {
      return false; // Fatal errors are not recoverable
    }

    // Check by specific error codes
    const errorCode = error.context.metadata.find(([key]) => 
      key.endsWith('_error_code')
    )?.[1];

    if (errorCode) {
      // Service integration errors
      if (Object.values(ServiceErrorCode).includes(errorCode as ServiceErrorCode)) {
        return this.isServiceErrorRecoverable(errorCode as ServiceErrorCode);
      }

      // Platform errors
      if (Object.values(PlatformErrorCode).includes(errorCode as PlatformErrorCode)) {
        return this.isPlatformErrorRecoverable(errorCode as PlatformErrorCode);
      }

      // Network errors
      if (Object.values(NetworkErrorCode).includes(errorCode as NetworkErrorCode)) {
        return this.isNetworkErrorRecoverable(errorCode as NetworkErrorCode);
      }
    }

    // Default: errors and warnings are typically recoverable
    return error.severity === ErrorSeverity.Error || error.severity === ErrorSeverity.Warning;
  }

  /**
   * Get recovery suggestions for an error
   */
  static getRecoverySuggestions(error: FlightError): ErrorRecoverySuggestion[] {
    const suggestions: ErrorRecoverySuggestion[] = [];

    // Category-based suggestions
    switch (error.category) {
      case ErrorCategory.Memory:
        suggestions.push({
          actionType: RecoveryActionType.ReduceResources,
          description: 'Reduce memory usage by closing unused resources or clearing caches',
          canAutomate: true,
          priority: 3,
        });
        break;

      case ErrorCategory.Network:
        suggestions.push({
          actionType: RecoveryActionType.Retry,
          description: 'Retry the network operation after a brief delay',
          canAutomate: true,
          priority: 2,
        });
        suggestions.push({
          actionType: RecoveryActionType.CheckStatus,
          description: 'Check network connectivity and service status',
          canAutomate: false,
          priority: 1,
        });
        break;

      case ErrorCategory.ServiceIntegration:
        const serviceCode = error.context.metadata.find(([key]) => 
          key === 'service_error_code'
        )?.[1] as ServiceErrorCode;
        
        if (serviceCode) {
          suggestions.push(...this.getServiceErrorRecoverySuggestions(serviceCode));
        }
        break;

      case ErrorCategory.Platform:
        const platformCode = error.context.metadata.find(([key]) => 
          key === 'platform_error_code'
        )?.[1] as PlatformErrorCode;
        
        if (platformCode) {
          suggestions.push(...this.getPlatformErrorRecoverySuggestions(platformCode));
        }
        break;

      case ErrorCategory.Validation:
        suggestions.push({
          actionType: RecoveryActionType.UpdateConfig,
          description: 'Correct the validation errors and retry',
          canAutomate: false,
          priority: 3,
        });
        break;
    }

    // Severity-based suggestions
    if (error.severity === ErrorSeverity.Critical || error.severity === ErrorSeverity.Fatal) {
      suggestions.push({
        actionType: RecoveryActionType.ContactSupport,
        description: 'Contact technical support for assistance with this critical error',
        canAutomate: false,
        priority: 1,
      });
    }

    return suggestions.sort((a, b) => b.priority - a.priority);
  }

  /**
   * Get error summary for logging/UI
   */
  static getErrorSummary(error: FlightError): string {
    const severityPrefix = error.severity.toUpperCase();
    const categoryInfo = error.category.replace('-', ' ').toUpperCase();
    
    return `[${severityPrefix}] ${categoryInfo}: ${error.message} (ID: ${error.id})`;
  }

  /**
   * Collect multiple errors
   */
  static collectErrors(errors: FlightError[]): ErrorCollection {
    const severityCounts = new Map<ErrorSeverity, number>();
    const categoryCounts = new Map<ErrorCategory, number>();
    let hasBlockingErrors = false;

    for (const error of errors) {
      // Count by severity
      severityCounts.set(error.severity, (severityCounts.get(error.severity) || 0) + 1);
      
      // Count by category
      categoryCounts.set(error.category, (categoryCounts.get(error.category) || 0) + 1);
      
      // Check for blocking errors
      if (error.severity === ErrorSeverity.Critical || error.severity === ErrorSeverity.Fatal) {
        hasBlockingErrors = true;
      }
    }

    return {
      errors,
      severityCounts: Array.from(severityCounts.entries()),
      categoryCounts: Array.from(categoryCounts.entries()),
      hasBlockingErrors,
      totalCount: errors.length,
    };
  }

  /**
   * Filter errors by severity
   */
  static filterBySeverity(
    collection: ErrorCollection,
    minSeverity: ErrorSeverity
  ): FlightError[] {
    const severityOrder = [
      ErrorSeverity.Info,
      ErrorSeverity.Warning,
      ErrorSeverity.Error,
      ErrorSeverity.Critical,
      ErrorSeverity.Fatal,
    ];
    
    const minIndex = severityOrder.indexOf(minSeverity);
    
    return collection.errors.filter(error => {
      const errorIndex = severityOrder.indexOf(error.severity);
      return errorIndex >= minIndex;
    });
  }

  /**
   * Filter errors by category
   */
  static filterByCategory(
    collection: ErrorCollection,
    categories: ErrorCategory[]
  ): FlightError[] {
    const categorySet = new Set(categories);
    return collection.errors.filter(error => categorySet.has(error.category));
  }

  /**
   * Convert error to JSON
   */
  static errorToJson(error: FlightError): string {
    return JSON.stringify(error);
  }

  /**
   * Parse error from JSON
   */
  static errorFromJson(json: string): FlightResult<FlightError> {
    try {
      const parsed = JSON.parse(json);
      
      // Basic validation
      if (!this.isValidErrorStructure(parsed)) {
        return {
          tag: 'err',
          val: this.createSimpleError(
            ErrorSeverity.Error,
            ErrorCategory.Validation,
            'Invalid error JSON structure',
            'error-parser',
            'parse-json'
          ),
        };
      }
      
      return { tag: 'ok', val: parsed as FlightError };
    } catch (parseError) {
      return {
        tag: 'err',
        val: this.createSimpleError(
          ErrorSeverity.Error,
          ErrorCategory.Validation,
          `JSON parse error: ${parseError instanceof Error ? parseError.message : 'Unknown error'}`,
          'error-parser',
          'parse-json'
        ),
      };
    }
  }

  /**
   * Create API error response
   */
  static createApiErrorResponse(error: FlightError): ErrorApiResponse {
    const errorCode = error.context.metadata.find(([key]) => 
      key.endsWith('_error_code')
    )?.[1] || 'unknown';

    return {
      success: false,
      error: {
        id: error.id,
        code: errorCode,
        message: error.message,
        severity: error.severity,
        timestamp: error.timestamp,
      },
      metadata: {
        requestId: error.id,
        ...(error.context.sessionId && { sessionId: error.context.sessionId }),
        ...(error.context.userId && { userId: error.context.userId }),
      },
    };
  }

  /**
   * Wrap result in success API response
   */
  static createSuccessApiResponse<T>(data: T, requestId?: string): ApiResponse<T> {
    if (requestId) {
      return {
        success: true,
        data,
        metadata: { requestId },
      };
    }

    return {
      success: true,
      data,
    };
  }

  // Private helper methods
  private static generateErrorId(): string {
    const timestamp = Date.now();
    const random = Math.random().toString(36).substr(2, 9);
    return `error_${timestamp}_${random}`;
  }

  private static getServiceErrorSeverity(code: ServiceErrorCode): ErrorSeverity {
    switch (code) {
      case ServiceErrorCode.AuthFailed:
      case ServiceErrorCode.PermissionDenied:
        return ErrorSeverity.Critical;
      
      case ServiceErrorCode.ResourceAllocationFailed:
      case ServiceErrorCode.SessionLimitExceeded:
      case ServiceErrorCode.ResourceLimitExceeded:
        return ErrorSeverity.Error;
      
      case ServiceErrorCode.RateLimitExceeded:
      case ServiceErrorCode.ServiceUnavailable:
      case ServiceErrorCode.RequestTimeout:
        return ErrorSeverity.Warning;
      
      default:
        return ErrorSeverity.Error;
    }
  }

  private static getPlatformErrorSeverity(code: PlatformErrorCode): ErrorSeverity {
    switch (code) {
      case PlatformErrorCode.PlatformInitFailed:
        return ErrorSeverity.Fatal;
      
      case PlatformErrorCode.InsufficientPlatformMemory:
      case PlatformErrorCode.HardwareConstraintViolation:
        return ErrorSeverity.Critical;
      
      case PlatformErrorCode.ComponentLoadFailed:
      case PlatformErrorCode.PlatformResourceExhausted:
        return ErrorSeverity.Error;
      
      default:
        return ErrorSeverity.Warning;
    }
  }

  private static getNetworkErrorSeverity(code: NetworkErrorCode): ErrorSeverity {
    switch (code) {
      case NetworkErrorCode.TlsError:
        return ErrorSeverity.Critical;
      
      case NetworkErrorCode.ConnectionRefused:
      case NetworkErrorCode.NetworkUnreachable:
        return ErrorSeverity.Error;
      
      default:
        return ErrorSeverity.Warning;
    }
  }

  private static isServiceErrorRecoverable(code: ServiceErrorCode): boolean {
    switch (code) {
      case ServiceErrorCode.AuthFailed:
      case ServiceErrorCode.PermissionDenied:
      case ServiceErrorCode.InvalidRequest:
        return false;
      
      case ServiceErrorCode.RateLimitExceeded:
      case ServiceErrorCode.ServiceUnavailable:
      case ServiceErrorCode.RequestTimeout:
      case ServiceErrorCode.ConnectionFailed:
        return true;
      
      default:
        return true;
    }
  }

  private static isPlatformErrorRecoverable(code: PlatformErrorCode): boolean {
    switch (code) {
      case PlatformErrorCode.PlatformInitFailed:
      case PlatformErrorCode.FeatureNotSupported:
      case PlatformErrorCode.CompatibilityError:
        return false;
      
      default:
        return true;
    }
  }

  private static isNetworkErrorRecoverable(code: NetworkErrorCode): boolean {
    switch (code) {
      case NetworkErrorCode.TlsError:
      case NetworkErrorCode.ProtocolError:
        return false;
      
      default:
        return true;
    }
  }

  private static getServiceErrorRecoverySuggestions(code: ServiceErrorCode): ErrorRecoverySuggestion[] {
    switch (code) {
      case ServiceErrorCode.RateLimitExceeded:
        return [{
          actionType: RecoveryActionType.WaitRetry,
          description: 'Wait for rate limit to reset and retry',
          canAutomate: true,
          priority: 3,
        }];
      
      case ServiceErrorCode.ResourceLimitExceeded:
        return [{
          actionType: RecoveryActionType.UpgradeResources,
          description: 'Consider upgrading to a higher service tier',
          canAutomate: false,
          priority: 2,
        }];
      
      case ServiceErrorCode.ServiceUnavailable:
        return [{
          actionType: RecoveryActionType.CheckStatus,
          description: 'Check service status page for known issues',
          canAutomate: false,
          priority: 2,
        }];
      
      default:
        return [];
    }
  }

  private static getPlatformErrorRecoverySuggestions(code: PlatformErrorCode): ErrorRecoverySuggestion[] {
    switch (code) {
      case PlatformErrorCode.InsufficientPlatformMemory:
        return [
          {
            actionType: RecoveryActionType.ReduceResources,
            description: 'Reduce memory usage or close other applications',
            canAutomate: true,
            priority: 3,
          },
          {
            actionType: RecoveryActionType.TryAlternative,
            description: 'Use alternative algorithms or data structures',
            canAutomate: false,
            priority: 2,
          },
        ];
      
      case PlatformErrorCode.FeatureNotSupported:
        return [{
          actionType: RecoveryActionType.TryAlternative,
          description: 'Use platform compatibility layer or alternative implementation',
          canAutomate: false,
          priority: 2,
        }];
      
      default:
        return [];
    }
  }

  private static isValidErrorStructure(obj: any): boolean {
    return (
      obj &&
      typeof obj.id === 'string' &&
      typeof obj.severity === 'string' &&
      typeof obj.category === 'string' &&
      typeof obj.message === 'string' &&
      typeof obj.timestamp === 'number' &&
      obj.context &&
      typeof obj.context.source === 'string' &&
      typeof obj.context.operation === 'string' &&
      Array.isArray(obj.context.metadata)
    );
  }
}
