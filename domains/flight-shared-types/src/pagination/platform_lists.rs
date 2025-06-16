//! Platform-Specific List and Pagination Patterns

use crate::types::*;

/// Platform list pattern implementations
pub struct PlatformListPatterns;

impl PlatformListPatterns {
    /// Create Flight-Core platform constraints
    pub fn create_flight_constraints(platform: &str) -> PlatformConstraints {
        match platform {
            "dreamcast" => PlatformConstraints {
                max_per_page: 10,  // Very limited
                max_pages: Some(100),
                memory_limit: MemorySize {
                    bytes: 1024 * 1024, // 1MB for lists
                    human_readable: "1MB".to_string(),
                },
                supported_search_modes: vec![SearchMode::Simple],
                supported_filters: vec![
                    FilterOperation::Equals,
                    FilterOperation::Contains,
                    FilterOperation::MemoryBelow,
                ],
                cache_enabled: false, // No cache on Dreamcast
                realtime_updates: false,
            },
            "psp" => PlatformConstraints {
                max_per_page: 25,
                max_pages: Some(500),
                memory_limit: MemorySize {
                    bytes: 2 * 1024 * 1024, // 2MB for lists
                    human_readable: "2MB".to_string(),
                },
                supported_search_modes: vec![SearchMode::Simple, SearchMode::FullText],
                supported_filters: vec![
                    FilterOperation::Equals,
                    FilterOperation::NotEquals,
                    FilterOperation::Contains,
                    FilterOperation::StartsWith,
                    FilterOperation::MemoryBelow,
                    FilterOperation::MemoryAbove,
                ],
                cache_enabled: true,
                realtime_updates: false,
            },
            "vita" => PlatformConstraints {
                max_per_page: 50,
                max_pages: Some(1000),
                memory_limit: MemorySize {
                    bytes: 10 * 1024 * 1024, // 10MB for lists
                    human_readable: "10MB".to_string(),
                },
                supported_search_modes: vec![
                    SearchMode::Simple,
                    SearchMode::FullText,
                    SearchMode::Fuzzy,
                ],
                supported_filters: vec![
                    FilterOperation::Equals,
                    FilterOperation::NotEquals,
                    FilterOperation::Contains,
                    FilterOperation::NotContains,
                    FilterOperation::StartsWith,
                    FilterOperation::EndsWith,
                    FilterOperation::InList,
                    FilterOperation::MemoryBelow,
                    FilterOperation::MemoryAbove,
                    FilterOperation::DateRange,
                ],
                cache_enabled: true,
                realtime_updates: true,
            },
            _ => PlatformConstraints {
                max_per_page: 100,
                max_pages: None, // Unlimited
                memory_limit: MemorySize {
                    bytes: 50 * 1024 * 1024, // 50MB for lists
                    human_readable: "50MB".to_string(),
                },
                supported_search_modes: vec![
                    SearchMode::Simple,
                    SearchMode::FullText,
                    SearchMode::Fuzzy,
                    SearchMode::Regex,
                    SearchMode::Boolean,
                    SearchMode::Phrase,
                ],
                supported_filters: vec![
                    FilterOperation::Equals,
                    FilterOperation::NotEquals,
                    FilterOperation::Contains,
                    FilterOperation::NotContains,
                    FilterOperation::StartsWith,
                    FilterOperation::EndsWith,
                    FilterOperation::InList,
                    FilterOperation::NotInList,
                    FilterOperation::GreaterThan,
                    FilterOperation::GreaterThanOrEqual,
                    FilterOperation::LessThan,
                    FilterOperation::LessThanOrEqual,
                    FilterOperation::Between,
                    FilterOperation::Regex,
                    FilterOperation::IsNull,
                    FilterOperation::IsNotNull,
                    FilterOperation::MemoryAbove,
                    FilterOperation::MemoryBelow,
                    FilterOperation::DateRange,
                ],
                cache_enabled: true,
                realtime_updates: true,
            },
        }
    }

    /// Create V6R list constraints based on subscription
    pub fn create_v6r_constraints(subscription_tier: &str) -> PlatformConstraints {
        match subscription_tier {
            "free" => PlatformConstraints {
                max_per_page: 10,
                max_pages: Some(100),
                memory_limit: MemorySize {
                    bytes: 5 * 1024 * 1024, // 5MB
                    human_readable: "5MB".to_string(),
                },
                supported_search_modes: vec![SearchMode::Simple],
                supported_filters: vec![
                    FilterOperation::Equals,
                    FilterOperation::Contains,
                ],
                cache_enabled: true,
                realtime_updates: false,
            },
            "individual" => PlatformConstraints {
                max_per_page: 50,
                max_pages: Some(1000),
                memory_limit: MemorySize {
                    bytes: 20 * 1024 * 1024, // 20MB
                    human_readable: "20MB".to_string(),
                },
                supported_search_modes: vec![
                    SearchMode::Simple,
                    SearchMode::FullText,
                ],
                supported_filters: vec![
                    FilterOperation::Equals,
                    FilterOperation::NotEquals,
                    FilterOperation::Contains,
                    FilterOperation::StartsWith,
                    FilterOperation::InList,
                    FilterOperation::DateRange,
                ],
                cache_enabled: true,
                realtime_updates: true,
            },
            "team" => PlatformConstraints {
                max_per_page: 100,
                max_pages: Some(5000),
                memory_limit: MemorySize {
                    bytes: 100 * 1024 * 1024, // 100MB
                    human_readable: "100MB".to_string(),
                },
                supported_search_modes: vec![
                    SearchMode::Simple,
                    SearchMode::FullText,
                    SearchMode::Fuzzy,
                    SearchMode::Boolean,
                ],
                supported_filters: vec![
                    FilterOperation::Equals,
                    FilterOperation::NotEquals,
                    FilterOperation::Contains,
                    FilterOperation::NotContains,
                    FilterOperation::StartsWith,
                    FilterOperation::EndsWith,
                    FilterOperation::InList,
                    FilterOperation::NotInList,
                    FilterOperation::GreaterThan,
                    FilterOperation::LessThan,
                    FilterOperation::Between,
                    FilterOperation::DateRange,
                ],
                cache_enabled: true,
                realtime_updates: true,
            },
            "enterprise" => PlatformConstraints {
                max_per_page: 500,
                max_pages: None, // Unlimited
                memory_limit: MemorySize {
                    bytes: 500 * 1024 * 1024, // 500MB
                    human_readable: "500MB".to_string(),
                },
                supported_search_modes: vec![
                    SearchMode::Simple,
                    SearchMode::FullText,
                    SearchMode::Fuzzy,
                    SearchMode::Regex,
                    SearchMode::Boolean,
                    SearchMode::Phrase,
                ],
                supported_filters: vec![
                    FilterOperation::Equals,
                    FilterOperation::NotEquals,
                    FilterOperation::Contains,
                    FilterOperation::NotContains,
                    FilterOperation::StartsWith,
                    FilterOperation::EndsWith,
                    FilterOperation::InList,
                    FilterOperation::NotInList,
                    FilterOperation::GreaterThan,
                    FilterOperation::GreaterThanOrEqual,
                    FilterOperation::LessThan,
                    FilterOperation::LessThanOrEqual,
                    FilterOperation::Between,
                    FilterOperation::Regex,
                    FilterOperation::IsNull,
                    FilterOperation::IsNotNull,
                    FilterOperation::DateRange,
                ],
                cache_enabled: true,
                realtime_updates: true,
            },
            _ => Self::create_v6r_constraints("free"), // Default to free tier
        }
    }

    /// Create Flight-Core component list request
    pub fn create_component_list_request(
        platform: &str,
        memory_filter: Option<u64>,
    ) -> ListRequest {
        let constraints = Self::create_flight_constraints(platform);
        let mut filters = vec![];

        // Add memory filter if specified
        if let Some(memory_limit) = memory_filter {
            filters.push(ListFilter {
                field: "memory_usage".to_string(),
                operation: FilterOperation::MemoryBelow,
                values: vec![memory_limit.to_string()],
                case_sensitive: Some(false),
            });
        }

        // Add platform filter
        filters.push(ListFilter {
            field: "platform".to_string(),
            operation: FilterOperation::Equals,
            values: vec![platform.to_string()],
            case_sensitive: Some(false),
        });

        ListRequest {
            page: Some(1),
            per_page: Some(constraints.max_per_page),
            sort_by: Some("last_activity".to_string()),
            sort_order: Some(SortOrder::Desc),
            search: None,
            filters,
            cursor: None,
            include_total: Some(true),
            memory_limit: Some(constraints.memory_limit),
        }
    }

    /// Create V6R VM list request
    pub fn create_vm_list_request(
        user_id: &str,
        subscription_tier: &str,
        vm_filter: Option<&str>,
    ) -> ListRequest {
        let constraints = Self::create_v6r_constraints(subscription_tier);
        let mut filters = vec![];

        // Add user filter
        filters.push(ListFilter {
            field: "user_id".to_string(),
            operation: FilterOperation::Equals,
            values: vec![user_id.to_string()],
            case_sensitive: Some(false),
        });

        // Add VM size filter if specified
        if let Some(vm_size) = vm_filter {
            filters.push(ListFilter {
                field: "vm_size".to_string(),
                operation: FilterOperation::Equals,
                values: vec![vm_size.to_string()],
                case_sensitive: Some(false),
            });
        }

        ListRequest {
            page: Some(1),
            per_page: Some(constraints.max_per_page),
            sort_by: Some("created_at".to_string()),
            sort_order: Some(SortOrder::Desc),
            search: None,
            filters,
            cursor: None,
            include_total: Some(true),
            memory_limit: Some(constraints.memory_limit),
        }
    }

    /// Create memory usage aggregation request
    pub fn create_memory_aggregation(platform: &str) -> AggregationRequest {
        AggregationRequest {
            field: "memory_usage".to_string(),
            aggregation_type: AggregationType::MemoryStats,
            limit: Some(10),
            filters: vec![
                ListFilter {
                    field: "platform".to_string(),
                    operation: FilterOperation::Equals,
                    values: vec![platform.to_string()],
                    case_sensitive: Some(false),
                },
            ],
        }
    }

    /// Create search parameters for platform
    pub fn create_search_params(
        query: &str,
        platform: &str,
    ) -> SearchParams {
        let constraints = Self::create_flight_constraints(platform);
        
        // Use most advanced search mode available for platform
        let mode = if constraints.supported_search_modes.contains(&SearchMode::FullText) {
            SearchMode::FullText
        } else {
            SearchMode::Simple
        };

        SearchParams {
            query: query.to_string(),
            fields: Some(vec![
                "name".to_string(),
                "description".to_string(),
                "metadata".to_string(),
            ]),
            mode,
            fuzzy_tolerance: if mode == SearchMode::FullText { Some(0.8) } else { None },
            min_word_length: Some(2),
            language: Some("en".to_string()),
            highlight: Some(true),
        }
    }

    /// Create bulk operation for platform
    pub fn create_bulk_component_operation(
        operation_type: BulkOperationType,
        component_ids: Vec<String>,
        platform: &str,
    ) -> BulkOperationRequest<String> {
        let constraints = Self::create_flight_constraints(platform);
        
        // Adjust batch size based on platform memory constraints
        let batch_size = match platform {
            "dreamcast" => 5,
            "psp" => 10,
            "vita" => 25,
            _ => 50,
        };

        BulkOperationRequest {
            operation: operation_type,
            items: component_ids,
            filters: vec![
                ListFilter {
                    field: "platform".to_string(),
                    operation: FilterOperation::Equals,
                    values: vec![platform.to_string()],
                    case_sensitive: Some(false),
                },
            ],
            parameters: vec![
                ("platform".to_string(), platform.to_string()),
                ("memory_limit".to_string(), constraints.memory_limit.bytes.to_string()),
            ],
            batch_size: Some(batch_size),
            parallel: Some(platform != "dreamcast"), // No parallel processing on Dreamcast
        }
    }

    /// Create Flight-Core list context
    pub fn create_flight_list_context(
        platform: &str,
        session_id: Option<String>,
        component_id: Option<String>,
        hal_subsystem: Option<String>,
        memory_pool: Option<String>,
    ) -> FlightListContext {
        FlightListContext {
            platform: platform.to_string(),
            session_id,
            component_id,
            hal_subsystem,
            memory_pool,
        }
    }

    /// Create V6R list context
    pub fn create_v6r_list_context(
        user_id: String,
        subscription_tier: String,
        organization_id: Option<String>,
        team_id: Option<String>,
        vm_id: Option<String>,
        project_id: Option<String>,
    ) -> V6rListContext {
        V6rListContext {
            user_id,
            organization_id,
            team_id,
            subscription_tier,
            vm_id,
            project_id,
        }
    }

    /// Get optimal page size for platform and operation
    pub fn get_optimal_page_size(platform: &str, operation_type: &str) -> u32 {
        let base_constraints = Self::create_flight_constraints(platform);
        
        match operation_type {
            "search" => {
                // Search operations are more expensive, reduce page size
                match platform {
                    "dreamcast" => 5,
                    "psp" => 10,
                    "vita" => 25,
                    _ => 50,
                }
            },
            "aggregation" => {
                // Aggregation can handle more items efficiently
                match platform {
                    "dreamcast" => base_constraints.max_per_page,
                    "psp" => base_constraints.max_per_page * 2,
                    "vita" => base_constraints.max_per_page * 2,
                    _ => base_constraints.max_per_page * 3,
                }
            },
            _ => base_constraints.max_per_page,
        }
    }

    /// Calculate memory overhead for list operation
    pub fn calculate_memory_overhead(
        platform: &str,
        items_count: u32,
        has_search: bool,
        has_aggregation: bool,
    ) -> MemorySize {
        let base_per_item = match platform {
            "dreamcast" => 512,  // 512 bytes per item
            "psp" => 1024,       // 1KB per item
            "vita" => 2048,      // 2KB per item
            _ => 4096,           // 4KB per item
        };

        let mut total_bytes = base_per_item * items_count as u64;

        // Add search overhead
        if has_search {
            total_bytes += match platform {
                "dreamcast" => 4096,      // 4KB search overhead
                "psp" => 8192,            // 8KB search overhead
                "vita" => 16384,          // 16KB search overhead
                _ => 32768,               // 32KB search overhead
            };
        }

        // Add aggregation overhead
        if has_aggregation {
            total_bytes += match platform {
                "dreamcast" => 2048,      // 2KB aggregation overhead
                "psp" => 4096,            // 4KB aggregation overhead
                "vita" => 8192,           // 8KB aggregation overhead
                _ => 16384,               // 16KB aggregation overhead
            };
        }

        MemorySize {
            bytes: total_bytes,
            human_readable: Self::format_bytes(total_bytes),
        }
    }

    /// Format bytes into human-readable string
    fn format_bytes(bytes: u64) -> String {
        const KB: u64 = 1024;
        const MB: u64 = KB * 1024;
        const GB: u64 = MB * 1024;

        if bytes >= GB {
            format!("{:.1}GB", bytes as f64 / GB as f64)
        } else if bytes >= MB {
            format!("{:.1}MB", bytes as f64 / MB as f64)
        } else if bytes >= KB {
            format!("{:.1}KB", bytes as f64 / KB as f64)
        } else {
            format!("{}B", bytes)
        }
    }
}

// Type definitions matching the WIT file
#[derive(Debug, Clone, PartialEq)]
pub struct MemorySize {
    pub bytes: u64,
    pub human_readable: String,
}

#[derive(Debug, Clone, PartialEq)]
pub struct PlatformConstraints {
    pub max_per_page: u32,
    pub max_pages: Option<u32>,
    pub memory_limit: MemorySize,
    pub supported_search_modes: Vec<SearchMode>,
    pub supported_filters: Vec<FilterOperation>,
    pub cache_enabled: bool,
    pub realtime_updates: bool,
}

#[derive(Debug, Clone, PartialEq)]
pub enum SearchMode {
    Simple,
    FullText,
    Fuzzy,
    Regex,
    Phrase,
    Boolean,
}

#[derive(Debug, Clone, PartialEq)]
pub enum FilterOperation {
    Equals,
    NotEquals,
    Contains,
    NotContains,
    StartsWith,
    EndsWith,
    InList,
    NotInList,
    GreaterThan,
    GreaterThanOrEqual,
    LessThan,
    LessThanOrEqual,
    Between,
    Regex,
    IsNull,
    IsNotNull,
    MemoryAbove,
    MemoryBelow,
    DateRange,
}

#[derive(Debug, Clone, PartialEq)]
pub enum SortOrder {
    Asc,
    Desc,
}

#[derive(Debug, Clone, PartialEq)]
pub struct ListFilter {
    pub field: String,
    pub operation: FilterOperation,
    pub values: Vec<String>,
    pub case_sensitive: Option<bool>,
}

#[derive(Debug, Clone, PartialEq)]
pub struct ListRequest {
    pub page: Option<u32>,
    pub per_page: Option<u32>,
    pub sort_by: Option<String>,
    pub sort_order: Option<SortOrder>,
    pub search: Option<String>,
    pub filters: Vec<ListFilter>,
    pub cursor: Option<String>,
    pub include_total: Option<bool>,
    pub memory_limit: Option<MemorySize>,
}

#[derive(Debug, Clone, PartialEq)]
pub struct SearchParams {
    pub query: String,
    pub fields: Option<Vec<String>>,
    pub mode: SearchMode,
    pub fuzzy_tolerance: Option<f32>,
    pub min_word_length: Option<u32>,
    pub language: Option<String>,
    pub highlight: Option<bool>,
}

#[derive(Debug, Clone, PartialEq)]
pub struct AggregationRequest {
    pub field: String,
    pub aggregation_type: AggregationType,
    pub limit: Option<u32>,
    pub filters: Vec<ListFilter>,
}

#[derive(Debug, Clone, PartialEq)]
pub enum AggregationType {
    Count,
    Sum,
    Avg,
    Min,
    Max,
    DistinctCount,
    MemoryStats,
    DateHistogram,
}

#[derive(Debug, Clone, PartialEq)]
pub enum BulkOperationType {
    Create,
    Update,
    Delete,
    Move,
    Copy,
    Export,
    Archive,
}

#[derive(Debug, Clone, PartialEq)]
pub struct BulkOperationRequest<T> {
    pub operation: BulkOperationType,
    pub items: Vec<T>,
    pub filters: Vec<ListFilter>,
    pub parameters: Vec<(String, String)>,
    pub batch_size: Option<u32>,
    pub parallel: Option<bool>,
}

#[derive(Debug, Clone, PartialEq)]
pub struct FlightListContext {
    pub platform: String,
    pub session_id: Option<String>,
    pub component_id: Option<String>,
    pub hal_subsystem: Option<String>,
    pub memory_pool: Option<String>,
}

#[derive(Debug, Clone, PartialEq)]
pub struct V6rListContext {
    pub user_id: String,
    pub organization_id: Option<String>,
    pub team_id: Option<String>,
    pub subscription_tier: String,
    pub vm_id: Option<String>,
    pub project_id: Option<String>,
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_dreamcast_list_constraints() {
        let constraints = PlatformListPatterns::create_flight_constraints("dreamcast");
        
        assert_eq!(constraints.max_per_page, 10);
        assert_eq!(constraints.max_pages, Some(100));
        assert_eq!(constraints.memory_limit.bytes, 1024 * 1024); // 1MB
        assert_eq!(constraints.supported_search_modes, vec![SearchMode::Simple]);
        assert!(!constraints.cache_enabled);
        assert!(!constraints.realtime_updates);
    }

    #[test]
    fn test_v6r_enterprise_constraints() {
        let constraints = PlatformListPatterns::create_v6r_constraints("enterprise");
        
        assert_eq!(constraints.max_per_page, 500);
        assert_eq!(constraints.max_pages, None); // Unlimited
        assert!(constraints.supported_search_modes.contains(&SearchMode::Regex));
        assert!(constraints.cache_enabled);
        assert!(constraints.realtime_updates);
    }

    #[test]
    fn test_component_list_request() {
        let request = PlatformListPatterns::create_component_list_request(
            "dreamcast",
            Some(8 * 1024 * 1024), // 8MB limit
        );
        
        assert_eq!(request.per_page, Some(10));
        assert_eq!(request.filters.len(), 2); // Memory + platform filters
        
        // Check memory filter
        let memory_filter = request.filters.iter()
            .find(|f| f.field == "memory_usage")
            .unwrap();
        assert_eq!(memory_filter.operation, FilterOperation::MemoryBelow);
    }

    #[test]
    fn test_bulk_operation_platform_adaptation() {
        let dreamcast_bulk = PlatformListPatterns::create_bulk_component_operation(
            BulkOperationType::Update,
            vec!["comp1".to_string(), "comp2".to_string()],
            "dreamcast",
        );

        let modern_bulk = PlatformListPatterns::create_bulk_component_operation(
            BulkOperationType::Update,
            vec!["comp1".to_string(), "comp2".to_string()],
            "linux-native",
        );

        // Dreamcast should have smaller batch size and no parallel processing
        assert_eq!(dreamcast_bulk.batch_size, Some(5));
        assert_eq!(dreamcast_bulk.parallel, Some(false));

        // Modern platforms should have larger batch size and parallel processing
        assert_eq!(modern_bulk.batch_size, Some(50));
        assert_eq!(modern_bulk.parallel, Some(true));
    }

    #[test]
    fn test_memory_overhead_calculation() {
        let overhead = PlatformListPatterns::calculate_memory_overhead(
            "dreamcast",
            10, // items
            true, // has search
            false, // no aggregation
        );

        // 10 items * 512 bytes + 4KB search overhead = 5120 + 4096 = 9216 bytes
        assert_eq!(overhead.bytes, 9216);
    }

    #[test]
    fn test_optimal_page_size() {
        assert_eq!(PlatformListPatterns::get_optimal_page_size("dreamcast", "search"), 5);
        assert_eq!(PlatformListPatterns::get_optimal_page_size("dreamcast", "list"), 10);
        assert_eq!(PlatformListPatterns::get_optimal_page_size("psp", "aggregation"), 50);
    }

    #[test]
    fn test_format_bytes() {
        assert_eq!(PlatformListPatterns::format_bytes(512), "512B");
        assert_eq!(PlatformListPatterns::format_bytes(1024), "1.0KB");
        assert_eq!(PlatformListPatterns::format_bytes(1024 * 1024), "1.0MB");
        assert_eq!(PlatformListPatterns::format_bytes(1024 * 1024 * 1024), "1.0GB");
    }
}
