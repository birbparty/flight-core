import { FlightError, ErrorContext, ErrorSeverity, ErrorCategory, FlightResult, ErrorCollection, ServiceErrorCode, PlatformErrorCode, NetworkErrorCode, ValidationErrorDetails, ErrorRecoverySuggestion, ErrorApiResponse, ApiResponse } from '../error-types';
export declare class FlightErrorUtils {
    /**
     * Create a new error with full context
     */
    static createError(severity: ErrorSeverity, category: ErrorCategory, message: string, context: ErrorContext): FlightError;
    /**
     * Create error with simple context
     */
    static createSimpleError(severity: ErrorSeverity, category: ErrorCategory, message: string, source: string, operation: string, additionalMetadata?: Array<[string, string]>): FlightError;
    /**
     * Create service integration error
     */
    static createServiceError(serviceCode: ServiceErrorCode, message: string, serviceId: string, sessionId?: string, userId?: string): FlightError;
    /**
     * Create platform error
     */
    static createPlatformError(platformCode: PlatformErrorCode, message: string, platform: string, operation: string): FlightError;
    /**
     * Create network error
     */
    static createNetworkError(networkCode: NetworkErrorCode, message: string, endpoint?: string): FlightError;
    /**
     * Create validation error
     */
    static createValidationError(message: string, validationDetails: ValidationErrorDetails[], source?: string): FlightError;
    /**
     * Enrich error with additional context
     */
    static enrichError(error: FlightError, additionalContext: Array<[string, string]>): FlightError;
    /**
     * Chain errors (add causative error)
     */
    static chainError(error: FlightError, cause: FlightError): FlightError;
    /**
     * Check if error is recoverable
     */
    static isRecoverable(error: FlightError): boolean;
    /**
     * Get recovery suggestions for an error
     */
    static getRecoverySuggestions(error: FlightError): ErrorRecoverySuggestion[];
    /**
     * Get error summary for logging/UI
     */
    static getErrorSummary(error: FlightError): string;
    /**
     * Collect multiple errors
     */
    static collectErrors(errors: FlightError[]): ErrorCollection;
    /**
     * Filter errors by severity
     */
    static filterBySeverity(collection: ErrorCollection, minSeverity: ErrorSeverity): FlightError[];
    /**
     * Filter errors by category
     */
    static filterByCategory(collection: ErrorCollection, categories: ErrorCategory[]): FlightError[];
    /**
     * Convert error to JSON
     */
    static errorToJson(error: FlightError): string;
    /**
     * Parse error from JSON
     */
    static errorFromJson(json: string): FlightResult<FlightError>;
    /**
     * Create API error response
     */
    static createApiErrorResponse(error: FlightError): ErrorApiResponse;
    /**
     * Wrap result in success API response
     */
    static createSuccessApiResponse<T>(data: T, requestId?: string): ApiResponse<T>;
    private static generateErrorId;
    private static getServiceErrorSeverity;
    private static getPlatformErrorSeverity;
    private static getNetworkErrorSeverity;
    private static isServiceErrorRecoverable;
    private static isPlatformErrorRecoverable;
    private static isNetworkErrorRecoverable;
    private static getServiceErrorRecoverySuggestions;
    private static getPlatformErrorRecoverySuggestions;
    private static isValidErrorStructure;
}
//# sourceMappingURL=error-utils.d.ts.map