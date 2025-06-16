//! Flight Core Error Validation Component
//! Validates error handling for Flight-Core and V6R integration

use wit_bindgen::generate;

generate!({
    world: "flight-error",
    path: "../../wit/error.wit"
});

use exports::flight::error::error_types::{
    FlightError, ErrorSeverity, ErrorCategory, ErrorContext, FlightResult,
    ServiceErrorCode, PlatformErrorCode, NetworkErrorCode, ValidationErrorDetails
};

use exports::flight::error::error_operations::{
    Guest as ErrorOperationsGuest
};

use exports::flight::error::error_analytics::{
    Guest as ErrorAnalyticsGuest, ErrorAnalyticsSummary, SystemHealthStatus,
    ErrorTrendAnalysis, TrendDirection
};

// Error operations implementation
struct ErrorOperations;

impl ErrorOperationsGuest for ErrorOperations {
    fn create_error(
        severity: ErrorSeverity,
        category: ErrorCategory,
        message: String,
        context: ErrorContext,
    ) -> FlightError {
        FlightError {
            id: generate_error_id(),
            severity,
            category,
            message,
            details: None,
            context,
            timestamp: get_current_timestamp(),
            cause: None,
        }
    }

    fn create_simple_error(
        severity: ErrorSeverity,
        category: ErrorCategory,
        message: String,
        source: String,
        operation: String,
    ) -> FlightError {
        let context = ErrorContext {
            source,
            operation,
            session_id: None,
            user_id: None,
            platform: None,
            service_id: None,
            metadata: vec![],
        };

        Self::create_error(severity, category, message, context)
    }

    fn create_service_error(
        service_code: ServiceErrorCode,
        message: String,
        service_id: String,
        session_id: Option<String>,
    ) -> FlightError {
        let metadata = vec![
            ("service_error_code".to_string(), format!("{:?}", service_code)),
            ("service_id".to_string(), service_id.clone()),
        ];

        let context = ErrorContext {
            source: "service-integration".to_string(),
            operation: format!("{:?}", service_code),
            session_id,
            user_id: None,
            platform: None,
            service_id: Some(service_id),
            metadata,
        };

        let severity = get_service_error_severity(&service_code);
        Self::create_error(severity, ErrorCategory::ServiceIntegration, message, context)
    }

    fn create_platform_error(
        platform_code: PlatformErrorCode,
        message: String,
        platform: String,
        operation: String,
    ) -> FlightError {
        let metadata = vec![
            ("platform_error_code".to_string(), format!("{:?}", platform_code)),
            ("platform".to_string(), platform.clone()),
        ];

        let context = ErrorContext {
            source: "flight-platform".to_string(),
            operation,
            session_id: None,
            user_id: None,
            platform: Some(platform),
            service_id: None,
            metadata,
        };

        let severity = get_platform_error_severity(&platform_code);
        Self::create_error(severity, ErrorCategory::Platform, message, context)
    }

    fn create_network_error(
        network_code: NetworkErrorCode,
        message: String,
        endpoint: Option<String>,
    ) -> FlightError {
        let mut metadata = vec![
            ("network_error_code".to_string(), format!("{:?}", network_code)),
        ];

        if let Some(endpoint) = endpoint.as_ref() {
            metadata.push(("endpoint".to_string(), endpoint.clone()));
        }

        let context = ErrorContext {
            source: "network-layer".to_string(),
            operation: format!("{:?}", network_code),
            session_id: None,
            user_id: None,
            platform: None,
            service_id: None,
            metadata,
        };

        let severity = get_network_error_severity(&network_code);
        Self::create_error(severity, ErrorCategory::Network, message, context)
    }

    fn create_validation_error(
        message: String,
        validation_details: Vec<ValidationErrorDetails>,
    ) -> FlightError {
        let mut metadata = vec![
            ("validation_errors_count".to_string(), validation_details.len().to_string()),
        ];

        for (index, detail) in validation_details.iter().enumerate() {
            metadata.extend([
                (format!("validation_{}_field", index), detail.field.clone()),
                (format!("validation_{}_rule", index), detail.rule.clone()),
                (format!("validation_{}_expected", index), detail.expected.clone()),
                (format!("validation_{}_actual", index), detail.actual.clone()),
            ]);
        }

        let context = ErrorContext {
            source: "validation-layer".to_string(),
            operation: "validation".to_string(),
            session_id: None,
            user_id: None,
            platform: None,
            service_id: None,
            metadata,
        };

        Self::create_error(ErrorSeverity::Error, ErrorCategory::Validation, message, context)
    }

    fn enrich_error(
        mut error: FlightError,
        additional_context: Vec<(String, String)>,
    ) -> FlightError {
        error.context.metadata.extend(additional_context);
        error
    }

    fn chain_error(mut error: FlightError, cause: FlightError) -> FlightError {
        error.cause = Some(cause.id);
        error
    }

    fn is_recoverable(error: FlightError) -> bool {
        // Security errors typically not recoverable
        if matches!(error.category, ErrorCategory::Security) {
            return false;
        }

        // Fatal errors are not recoverable
        if matches!(error.severity, ErrorSeverity::Fatal) {
            return false;
        }

        // Check by specific error codes
        let error_code = error.context.metadata
            .iter()
            .find(|(key, _)| key.ends_with("_error_code"))
            .map(|(_, value)| value.as_str());

        if let Some(code) = error_code {
            // Service integration errors
            if let Ok(service_code) = serde_json::from_str::<ServiceErrorCode>(&format!("\"{}\"", code)) {
                return is_service_error_recoverable(&service_code);
            }

            // Platform errors
            if let Ok(platform_code) = serde_json::from_str::<PlatformErrorCode>(&format!("\"{}\"", code)) {
                return is_platform_error_recoverable(&platform_code);
            }

            // Network errors
            if let Ok(network_code) = serde_json::from_str::<NetworkErrorCode>(&format!("\"{}\"", code)) {
                return is_network_error_recoverable(&network_code);
            }
        }

        // Default: errors and warnings are typically recoverable
        matches!(error.severity, ErrorSeverity::Error | ErrorSeverity::Warning)
    }

    fn get_recovery_suggestions(
        _error: FlightError,
    ) -> Vec<exports::flight::error::error_types::ErrorRecoverySuggestion> {
        // Simplified implementation for validation
        vec![]
    }

    fn get_error_summary(error: FlightError) -> String {
        format!("[{:?}] {:?}: {} (ID: {})", 
            error.severity, error.category, error.message, error.id)
    }

    fn collect_errors(
        errors: Vec<FlightError>,
    ) -> exports::flight::error::error_types::ErrorCollection {
        use std::collections::HashMap;
        
        let mut severity_counts: HashMap<ErrorSeverity, u32> = HashMap::new();
        let mut category_counts: HashMap<ErrorCategory, u32> = HashMap::new();
        let mut has_blocking_errors = false;

        for error in &errors {
            *severity_counts.entry(error.severity.clone()).or_insert(0) += 1;
            *category_counts.entry(error.category.clone()).or_insert(0) += 1;
            
            if matches!(error.severity, ErrorSeverity::Critical | ErrorSeverity::Fatal) {
                has_blocking_errors = true;
            }
        }

        exports::flight::error::error_types::ErrorCollection {
            errors,
            severity_counts: severity_counts.into_iter().collect(),
            category_counts: category_counts.into_iter().collect(),
            has_blocking_errors,
            total_count: errors.len() as u32,
        }
    }

    fn filter_by_severity(
        collection: exports::flight::error::error_types::ErrorCollection,
        min_severity: ErrorSeverity,
    ) -> Vec<FlightError> {
        let min_level = severity_to_level(&min_severity);
        
        collection.errors.into_iter()
            .filter(|error| severity_to_level(&error.severity) >= min_level)
            .collect()
    }

    fn filter_by_category(
        collection: exports::flight::error::error_types::ErrorCollection,
        categories: Vec<ErrorCategory>,
    ) -> Vec<FlightError> {
        let category_set: std::collections::HashSet<_> = categories.into_iter().collect();
        
        collection.errors.into_iter()
            .filter(|error| category_set.contains(&error.category))
            .collect()
    }

    fn error_to_json(error: FlightError) -> String {
        serde_json::to_string(&error).unwrap_or_else(|_| "{}".to_string())
    }

    fn error_from_json(json: String) -> FlightResult<FlightError> {
        match serde_json::from_str::<FlightError>(&json) {
            Ok(error) => FlightResult::Ok(error),
            Err(e) => FlightResult::Err(Self::create_simple_error(
                ErrorSeverity::Error,
                ErrorCategory::Validation,
                format!("JSON parse error: {}", e),
                "error-parser".to_string(),
                "parse-json".to_string(),
            )),
        }
    }

    fn validate_error(error: FlightError) -> FlightResult<bool> {
        // Basic validation - check required fields
        if error.id.is_empty() || error.message.is_empty() || error.context.source.is_empty() {
            FlightResult::Err(Self::create_simple_error(
                ErrorSeverity::Error,
                ErrorCategory::Validation,
                "Invalid error structure: missing required fields".to_string(),
                "error-validator".to_string(),
                "validate-structure".to_string(),
            ))
        } else {
            FlightResult::Ok(true)
        }
    }
}

// Error analytics implementation (simplified)
struct ErrorAnalytics;

impl ErrorAnalyticsGuest for ErrorAnalytics {
    fn analyze_errors(
        errors: Vec<FlightError>,
        time_window: u64,
    ) -> ErrorAnalyticsSummary {
        use std::collections::HashMap;
        
        let mut severity_counts: HashMap<ErrorSeverity, u32> = HashMap::new();
        let mut category_counts: HashMap<ErrorCategory, u32> = HashMap::new();
        let mut message_counts: HashMap<String, u32> = HashMap::new();

        for error in &errors {
            *severity_counts.entry(error.severity.clone()).or_insert(0) += 1;
            *category_counts.entry(error.category.clone()).or_insert(0) += 1;
            *message_counts.entry(error.message.clone()).or_insert(0) += 1;
        }

        let mut top_errors: Vec<_> = message_counts.into_iter().collect();
        top_errors.sort_by(|a, b| b.1.cmp(&a.1));
        top_errors.truncate(10);

        let error_rate = if time_window > 0 {
            errors.len() as f32 / time_window as f32
        } else {
            0.0
        };

        ErrorAnalyticsSummary {
            total_errors: errors.len() as u32,
            by_severity: severity_counts.into_iter().collect(),
            by_category: category_counts.into_iter().collect(),
            top_errors,
            error_rate,
            time_window,
            analyzed_at: get_current_timestamp(),
        }
    }

    fn assess_system_health(analytics: ErrorAnalyticsSummary) -> SystemHealthStatus {
        if analytics.total_errors == 0 {
            return SystemHealthStatus::Healthy;
        }

        let critical_errors = analytics.by_severity.iter()
            .filter(|(severity, _)| matches!(severity, ErrorSeverity::Critical | ErrorSeverity::Fatal))
            .map(|(_, count)| count)
            .sum::<u32>();

        if critical_errors > 0 {
            SystemHealthStatus::Critical
        } else if analytics.error_rate > 10.0 {
            SystemHealthStatus::Degraded
        } else {
            SystemHealthStatus::Healthy
        }
    }

    fn analyze_error_trends(
        _error_history: Vec<ErrorAnalyticsSummary>,
    ) -> ErrorTrendAnalysis {
        // Simplified implementation for validation
        ErrorTrendAnalysis {
            trend: TrendDirection::Stable,
            confidence: 0.5,
            predicted_rate: 0.0,
            recommendations: vec!["Monitor error patterns".to_string()],
        }
    }

    fn get_error_hotspots(analytics: ErrorAnalyticsSummary) -> Vec<(String, u32)> {
        analytics.top_errors
    }

    fn generate_error_report(
        analytics: ErrorAnalyticsSummary,
        _include_details: bool,
    ) -> String {
        format!(
            "Error Analytics Report\n\
             Total Errors: {}\n\
             Error Rate: {:.2}/s\n\
             Time Window: {}s\n\
             Top Error: {}",
            analytics.total_errors,
            analytics.error_rate,
            analytics.time_window,
            analytics.top_errors.first()
                .map(|(msg, count)| format!("{} ({})", msg, count))
                .unwrap_or_else(|| "None".to_string())
        )
    }
}

// Helper functions
fn generate_error_id() -> String {
    use std::time::{SystemTime, UNIX_EPOCH};
    let timestamp = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap()
        .as_nanos();
    format!("error_{}_{}", timestamp / 1_000_000, timestamp % 1_000_000)
}

fn get_current_timestamp() -> u64 {
    use std::time::{SystemTime, UNIX_EPOCH};
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap()
        .as_secs()
}

fn get_service_error_severity(code: &ServiceErrorCode) -> ErrorSeverity {
    match code {
        ServiceErrorCode::AuthFailed | ServiceErrorCode::PermissionDenied => ErrorSeverity::Critical,
        ServiceErrorCode::ResourceAllocationFailed 
        | ServiceErrorCode::SessionLimitExceeded 
        | ServiceErrorCode::ResourceLimitExceeded => ErrorSeverity::Error,
        ServiceErrorCode::RateLimitExceeded 
        | ServiceErrorCode::ServiceUnavailable 
        | ServiceErrorCode::RequestTimeout => ErrorSeverity::Warning,
        _ => ErrorSeverity::Error,
    }
}

fn get_platform_error_severity(code: &PlatformErrorCode) -> ErrorSeverity {
    match code {
        PlatformErrorCode::PlatformInitFailed => ErrorSeverity::Fatal,
        PlatformErrorCode::InsufficientPlatformMemory 
        | PlatformErrorCode::HardwareConstraintViolation => ErrorSeverity::Critical,
        PlatformErrorCode::ComponentLoadFailed 
        | PlatformErrorCode::PlatformResourceExhausted => ErrorSeverity::Error,
        _ => ErrorSeverity::Warning,
    }
}

fn get_network_error_severity(code: &NetworkErrorCode) -> ErrorSeverity {
    match code {
        NetworkErrorCode::TlsError => ErrorSeverity::Critical,
        NetworkErrorCode::ConnectionRefused | NetworkErrorCode::NetworkUnreachable => ErrorSeverity::Error,
        _ => ErrorSeverity::Warning,
    }
}

fn is_service_error_recoverable(code: &ServiceErrorCode) -> bool {
    match code {
        ServiceErrorCode::AuthFailed 
        | ServiceErrorCode::PermissionDenied 
        | ServiceErrorCode::InvalidRequest => false,
        ServiceErrorCode::RateLimitExceeded 
        | ServiceErrorCode::ServiceUnavailable 
        | ServiceErrorCode::RequestTimeout 
        | ServiceErrorCode::ConnectionFailed => true,
        _ => true,
    }
}

fn is_platform_error_recoverable(code: &PlatformErrorCode) -> bool {
    match code {
        PlatformErrorCode::PlatformInitFailed 
        | PlatformErrorCode::FeatureNotSupported 
        | PlatformErrorCode::CompatibilityError => false,
        _ => true,
    }
}

fn is_network_error_recoverable(code: &NetworkErrorCode) -> bool {
    match code {
        NetworkErrorCode::TlsError | NetworkErrorCode::ProtocolError => false,
        _ => true,
    }
}

fn severity_to_level(severity: &ErrorSeverity) -> u8 {
    match severity {
        ErrorSeverity::Info => 0,
        ErrorSeverity::Warning => 1,
        ErrorSeverity::Error => 2,
        ErrorSeverity::Critical => 3,
        ErrorSeverity::Fatal => 4,
    }
}

#[no_mangle]
pub extern "C" fn error_validation_init() -> bool {
    true
}
