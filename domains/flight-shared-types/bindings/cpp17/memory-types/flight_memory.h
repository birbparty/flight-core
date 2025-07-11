// Generated by `wit-bindgen` 0.42.1. DO NOT EDIT!
#ifndef __BINDINGS_FLIGHT_MEMORY_H
#define __BINDINGS_FLIGHT_MEMORY_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct flight_memory_string_t {
  uint8_t*ptr;
  size_t len;
} flight_memory_string_t;

// Memory size representation with human-readable formatting
// 
// Used throughout both ecosystems for consistent memory measurement
typedef struct exports_flight_memory_memory_types_memory_size_t {
  // Raw byte count - allows precise calculations
  uint64_t   bytes;
  // Human-readable format: "16MB", "512KB", "2GB", etc.
  // Critical for V6R UI components and logging
  flight_memory_string_t   human_readable;
} exports_flight_memory_memory_types_memory_size_t;

// Memory usage snapshot for real-time monitoring
// 
// V6R uses this for WebSocket memory updates and UI display
// Flight-Core uses this for platform memory adaptation
typedef struct exports_flight_memory_memory_types_memory_usage_snapshot_t {
  // Timestamp when snapshot was taken (Unix timestamp)
  uint64_t   timestamp;
  // Session identifier (V6R session or Flight-Core component instance)
  flight_memory_string_t   session_id;
  // Platform identifier for memory constraints
  flight_memory_string_t   platform;
  // Total available memory for this context
  exports_flight_memory_memory_types_memory_size_t   total;
  // Currently used memory
  exports_flight_memory_memory_types_memory_size_t   used;
  // Available memory (calculated: total - used)
  exports_flight_memory_memory_types_memory_size_t   available;
  // Memory fragmentation ratio (0.0-1.0)
  // Critical for Flight-Core constrained platforms
  float   fragmentation_ratio;
} exports_flight_memory_memory_types_memory_usage_snapshot_t;

// Platform-specific memory profiles
// 
// Enables both Flight-Core and V6R to adapt behavior based on memory constraints
typedef struct exports_flight_memory_memory_types_platform_profile_t {
  uint8_t tag;
  union {
    exports_flight_memory_memory_types_memory_size_t     dreamcast;
    exports_flight_memory_memory_types_memory_size_t     psp;
    exports_flight_memory_memory_types_memory_size_t     vita;
    exports_flight_memory_memory_types_memory_size_t     v6r_small;
    exports_flight_memory_memory_types_memory_size_t     v6r_medium;
    exports_flight_memory_memory_types_memory_size_t     v6r_large;
    exports_flight_memory_memory_types_memory_size_t     custom;
  } val;
} exports_flight_memory_memory_types_platform_profile_t;

// Dreamcast: 16MB baseline (PlayStation 1 dropped for C++17)
#define EXPORTS_FLIGHT_MEMORY_MEMORY_TYPES_PLATFORM_PROFILE_DREAMCAST 0
// PlayStation Portable: 32-64MB depending on model
#define EXPORTS_FLIGHT_MEMORY_MEMORY_TYPES_PLATFORM_PROFILE_PSP 1
// Sony PlayStation Vita: 512MB
#define EXPORTS_FLIGHT_MEMORY_MEMORY_TYPES_PLATFORM_PROFILE_VITA 2
// V6R Small VM: 512MB for lightweight development
#define EXPORTS_FLIGHT_MEMORY_MEMORY_TYPES_PLATFORM_PROFILE_V6R_SMALL 3
// V6R Medium VM: 1GB for standard development
#define EXPORTS_FLIGHT_MEMORY_MEMORY_TYPES_PLATFORM_PROFILE_V6R_MEDIUM 4
// V6R Large VM: 2GB+ for intensive development
#define EXPORTS_FLIGHT_MEMORY_MEMORY_TYPES_PLATFORM_PROFILE_V6R_LARGE 5
// Custom platform with specified memory
#define EXPORTS_FLIGHT_MEMORY_MEMORY_TYPES_PLATFORM_PROFILE_CUSTOM 6

// Memory allocation purposes for categorization
// 
// Enables both systems to track memory usage by category
typedef uint8_t exports_flight_memory_memory_types_memory_purpose_t;

// VM heap memory (V6R primary use case)
#define EXPORTS_FLIGHT_MEMORY_MEMORY_TYPES_MEMORY_PURPOSE_VM_HEAP 0
// Component stack memory (Flight-Core components)
#define EXPORTS_FLIGHT_MEMORY_MEMORY_TYPES_MEMORY_PURPOSE_COMPONENT_STACK 1
// Asset cache (textures, audio, etc.)
#define EXPORTS_FLIGHT_MEMORY_MEMORY_TYPES_MEMORY_PURPOSE_ASSET_CACHE 2
// JIT compiled code cache
#define EXPORTS_FLIGHT_MEMORY_MEMORY_TYPES_MEMORY_PURPOSE_JIT_CODE_CACHE 3
// System reserved memory
#define EXPORTS_FLIGHT_MEMORY_MEMORY_TYPES_MEMORY_PURPOSE_SYSTEM_RESERVED 4
// WebAssembly linear memory
#define EXPORTS_FLIGHT_MEMORY_MEMORY_TYPES_MEMORY_PURPOSE_WASM_LINEAR 5
// Network buffers
#define EXPORTS_FLIGHT_MEMORY_MEMORY_TYPES_MEMORY_PURPOSE_NETWORK_BUFFERS 6
// Temporary/scratch memory
#define EXPORTS_FLIGHT_MEMORY_MEMORY_TYPES_MEMORY_PURPOSE_TEMPORARY 7

typedef struct {
  bool is_some;
  uint64_t val;
} flight_memory_option_u64_t;

// Memory allocation tracking record
// 
// V6R uses this for VM resource accounting
// Flight-Core uses this for component memory management
typedef struct exports_flight_memory_memory_types_memory_allocation_t {
  // Unique allocation identifier
  flight_memory_string_t   id;
  // Session this allocation belongs to
  flight_memory_string_t   session_id;
  // Size of the allocation
  exports_flight_memory_memory_types_memory_size_t   size;
  // Purpose/category of this allocation
  exports_flight_memory_memory_types_memory_purpose_t   purpose;
  // When allocation was created (Unix timestamp)
  uint64_t   allocated_at;
  // When allocation was freed (None if still active)
  flight_memory_option_u64_t   freed_at;
} exports_flight_memory_memory_types_memory_allocation_t;

// Memory pressure levels for adaptive behavior
// 
// Flight-Core uses for platform adaptation
// V6R uses for VM scaling decisions
typedef uint8_t exports_flight_memory_memory_types_memory_pressure_t;

// Plenty of memory available
#define EXPORTS_FLIGHT_MEMORY_MEMORY_TYPES_MEMORY_PRESSURE_LOW 0
// Memory usage getting high but manageable
#define EXPORTS_FLIGHT_MEMORY_MEMORY_TYPES_MEMORY_PRESSURE_MEDIUM 1
// Memory critically low - aggressive cleanup needed
#define EXPORTS_FLIGHT_MEMORY_MEMORY_TYPES_MEMORY_PRESSURE_HIGH 2
// Out of memory - emergency measures required
#define EXPORTS_FLIGHT_MEMORY_MEMORY_TYPES_MEMORY_PRESSURE_CRITICAL 3

// Memory limits configuration
// 
// V6R uses for VM resource enforcement
// Flight-Core uses for platform constraint adaptation
typedef struct exports_flight_memory_memory_types_memory_limits_t {
  // Maximum memory for VM/component heap
  exports_flight_memory_memory_types_memory_size_t   heap_max;
  // Maximum memory for stack
  exports_flight_memory_memory_types_memory_size_t   stack_max;
  // Maximum memory for caching
  exports_flight_memory_memory_types_memory_size_t   cache_max;
  // Soft limit before warnings
  exports_flight_memory_memory_types_memory_size_t   soft_limit;
  // Hard limit before allocation failures
  exports_flight_memory_memory_types_memory_size_t   hard_limit;
} exports_flight_memory_memory_types_memory_limits_t;

// Memory error codes
typedef uint8_t exports_flight_memory_memory_types_memory_error_code_t;

// Insufficient memory available
#define EXPORTS_FLIGHT_MEMORY_MEMORY_TYPES_MEMORY_ERROR_CODE_INSUFFICIENT_MEMORY 0
// Memory limit exceeded
#define EXPORTS_FLIGHT_MEMORY_MEMORY_TYPES_MEMORY_ERROR_CODE_LIMIT_EXCEEDED 1
// Invalid memory size requested
#define EXPORTS_FLIGHT_MEMORY_MEMORY_TYPES_MEMORY_ERROR_CODE_INVALID_SIZE 2
// Memory allocation failed
#define EXPORTS_FLIGHT_MEMORY_MEMORY_TYPES_MEMORY_ERROR_CODE_ALLOCATION_FAILED 3
// Memory already freed
#define EXPORTS_FLIGHT_MEMORY_MEMORY_TYPES_MEMORY_ERROR_CODE_ALREADY_FREED 4
// Invalid allocation ID
#define EXPORTS_FLIGHT_MEMORY_MEMORY_TYPES_MEMORY_ERROR_CODE_INVALID_ALLOCATION 5
// Platform not supported
#define EXPORTS_FLIGHT_MEMORY_MEMORY_TYPES_MEMORY_ERROR_CODE_UNSUPPORTED_PLATFORM 6
// Memory fragmentation too high
#define EXPORTS_FLIGHT_MEMORY_MEMORY_TYPES_MEMORY_ERROR_CODE_FRAGMENTATION_ERROR 7

typedef struct {
  bool is_some;
  flight_memory_string_t val;
} flight_memory_option_string_t;

// Memory-specific error types
// 
// Comprehensive error handling for memory operations
typedef struct exports_flight_memory_memory_types_memory_error_t {
  // Error code for programmatic handling
  exports_flight_memory_memory_types_memory_error_code_t   code;
  // Human-readable error message
  flight_memory_string_t   message;
  // Additional context/details
  flight_memory_option_string_t   details;
  // When error occurred
  uint64_t   timestamp;
} exports_flight_memory_memory_types_memory_error_t;

typedef exports_flight_memory_memory_types_memory_size_t exports_flight_memory_memory_operations_memory_size_t;

typedef exports_flight_memory_memory_types_memory_allocation_t exports_flight_memory_memory_operations_memory_allocation_t;

typedef exports_flight_memory_memory_types_memory_purpose_t exports_flight_memory_memory_operations_memory_purpose_t;

typedef exports_flight_memory_memory_types_memory_usage_snapshot_t exports_flight_memory_memory_operations_memory_usage_snapshot_t;

typedef exports_flight_memory_memory_types_memory_error_t exports_flight_memory_memory_operations_memory_error_t;

typedef exports_flight_memory_memory_types_memory_limits_t exports_flight_memory_memory_operations_memory_limits_t;

typedef exports_flight_memory_memory_types_memory_pressure_t exports_flight_memory_memory_operations_memory_pressure_t;

typedef exports_flight_memory_memory_types_platform_profile_t exports_flight_memory_memory_operations_platform_profile_t;

typedef struct {
  bool is_err;
  union {
    exports_flight_memory_memory_operations_memory_allocation_t ok;
    exports_flight_memory_memory_operations_memory_error_t err;
  } val;
} exports_flight_memory_memory_operations_result_memory_allocation_memory_error_t;

typedef struct {
  bool is_err;
  union {
    bool ok;
    exports_flight_memory_memory_operations_memory_error_t err;
  } val;
} exports_flight_memory_memory_operations_result_bool_memory_error_t;

typedef struct {
  bool is_err;
  union {
    exports_flight_memory_memory_operations_memory_usage_snapshot_t ok;
    exports_flight_memory_memory_operations_memory_error_t err;
  } val;
} exports_flight_memory_memory_operations_result_memory_usage_snapshot_memory_error_t;

typedef struct {
  bool is_err;
  union {
    exports_flight_memory_memory_operations_platform_profile_t ok;
    exports_flight_memory_memory_operations_memory_error_t err;
  } val;
} exports_flight_memory_memory_operations_result_platform_profile_memory_error_t;

typedef struct {
  bool is_err;
  union {
    exports_flight_memory_memory_operations_memory_limits_t ok;
    exports_flight_memory_memory_operations_memory_error_t err;
  } val;
} exports_flight_memory_memory_operations_result_memory_limits_memory_error_t;

typedef struct {
  bool is_err;
  union {
    exports_flight_memory_memory_operations_memory_pressure_t ok;
    exports_flight_memory_memory_operations_memory_error_t err;
  } val;
} exports_flight_memory_memory_operations_result_memory_pressure_memory_error_t;

typedef struct {
  exports_flight_memory_memory_operations_memory_allocation_t *ptr;
  size_t len;
} exports_flight_memory_memory_operations_list_memory_allocation_t;

typedef struct {
  bool is_err;
  union {
    exports_flight_memory_memory_operations_list_memory_allocation_t ok;
    exports_flight_memory_memory_operations_memory_error_t err;
  } val;
} exports_flight_memory_memory_operations_result_list_memory_allocation_memory_error_t;

typedef exports_flight_memory_memory_types_memory_size_t exports_flight_memory_memory_analytics_memory_size_t;

typedef exports_flight_memory_memory_types_memory_purpose_t exports_flight_memory_memory_analytics_memory_purpose_t;

typedef exports_flight_memory_memory_types_memory_usage_snapshot_t exports_flight_memory_memory_analytics_memory_usage_snapshot_t;

typedef exports_flight_memory_memory_types_memory_error_t exports_flight_memory_memory_analytics_memory_error_t;

typedef struct {
  exports_flight_memory_memory_analytics_memory_purpose_t f0;
  exports_flight_memory_memory_analytics_memory_size_t f1;
} exports_flight_memory_memory_analytics_tuple2_memory_purpose_memory_size_t;

typedef struct {
  exports_flight_memory_memory_analytics_tuple2_memory_purpose_memory_size_t *ptr;
  size_t len;
} exports_flight_memory_memory_analytics_list_tuple2_memory_purpose_memory_size_t;

// Comprehensive memory statistics
typedef struct exports_flight_memory_memory_analytics_memory_stats_t {
  // Total allocations made
  uint64_t   total_allocations;
  // Active allocations count
  uint64_t   active_allocations;
  // Peak memory usage recorded
  exports_flight_memory_memory_analytics_memory_size_t   peak_memory;
  // Current memory usage
  exports_flight_memory_memory_analytics_memory_size_t   current_memory;
  // Average allocation size
  exports_flight_memory_memory_analytics_memory_size_t   average_allocation_size;
  // Memory usage by purpose
  exports_flight_memory_memory_analytics_list_tuple2_memory_purpose_memory_size_t   usage_by_purpose;
  // Memory efficiency ratio (0.0-1.0)
  float   efficiency_ratio;
} exports_flight_memory_memory_analytics_memory_stats_t;

// Trend direction enumeration
typedef uint8_t exports_flight_memory_memory_analytics_trend_direction_t;

#define EXPORTS_FLIGHT_MEMORY_MEMORY_ANALYTICS_TREND_DIRECTION_INCREASING 0
#define EXPORTS_FLIGHT_MEMORY_MEMORY_ANALYTICS_TREND_DIRECTION_DECREASING 1
#define EXPORTS_FLIGHT_MEMORY_MEMORY_ANALYTICS_TREND_DIRECTION_STABLE 2
#define EXPORTS_FLIGHT_MEMORY_MEMORY_ANALYTICS_TREND_DIRECTION_VOLATILE 3

typedef struct {
  exports_flight_memory_memory_analytics_memory_usage_snapshot_t *ptr;
  size_t len;
} exports_flight_memory_memory_analytics_list_memory_usage_snapshot_t;

typedef struct {
  bool is_some;
  exports_flight_memory_memory_analytics_memory_size_t val;
} exports_flight_memory_memory_analytics_option_memory_size_t;

// Memory usage trend data
typedef struct exports_flight_memory_memory_analytics_memory_trend_t {
  // Data points over time
  exports_flight_memory_memory_analytics_list_memory_usage_snapshot_t   snapshots;
  // Trend direction (increasing/decreasing/stable)
  exports_flight_memory_memory_analytics_trend_direction_t   trend_direction;
  // Predicted future usage
  exports_flight_memory_memory_analytics_option_memory_size_t   predicted_peak;
} exports_flight_memory_memory_analytics_memory_trend_t;

typedef struct {
  bool is_err;
  union {
    exports_flight_memory_memory_analytics_memory_stats_t ok;
    exports_flight_memory_memory_analytics_memory_error_t err;
  } val;
} exports_flight_memory_memory_analytics_result_memory_stats_memory_error_t;

typedef struct {
  bool is_err;
  union {
    exports_flight_memory_memory_analytics_memory_trend_t ok;
    exports_flight_memory_memory_analytics_memory_error_t err;
  } val;
} exports_flight_memory_memory_analytics_result_memory_trend_memory_error_t;

typedef struct {
  bool is_err;
  union {
    flight_memory_string_t ok;
    exports_flight_memory_memory_analytics_memory_error_t err;
  } val;
} exports_flight_memory_memory_analytics_result_string_memory_error_t;

// Exported Functions from `flight:memory/memory-operations@1.0.0`
bool exports_flight_memory_memory_operations_create_allocation(flight_memory_string_t *session_id, exports_flight_memory_memory_operations_memory_size_t *size, exports_flight_memory_memory_operations_memory_purpose_t purpose, exports_flight_memory_memory_operations_memory_allocation_t *ret, exports_flight_memory_memory_operations_memory_error_t *err);
bool exports_flight_memory_memory_operations_free_allocation(flight_memory_string_t *allocation_id, bool *ret, exports_flight_memory_memory_operations_memory_error_t *err);
bool exports_flight_memory_memory_operations_get_memory_snapshot(flight_memory_string_t *session_id, exports_flight_memory_memory_operations_memory_usage_snapshot_t *ret, exports_flight_memory_memory_operations_memory_error_t *err);
bool exports_flight_memory_memory_operations_get_platform_profile(flight_memory_string_t *platform, exports_flight_memory_memory_operations_platform_profile_t *ret, exports_flight_memory_memory_operations_memory_error_t *err);
bool exports_flight_memory_memory_operations_set_memory_limits(flight_memory_string_t *session_id, exports_flight_memory_memory_operations_memory_limits_t *limits, bool *ret, exports_flight_memory_memory_operations_memory_error_t *err);
bool exports_flight_memory_memory_operations_get_memory_limits(flight_memory_string_t *session_id, exports_flight_memory_memory_operations_memory_limits_t *ret, exports_flight_memory_memory_operations_memory_error_t *err);
bool exports_flight_memory_memory_operations_get_memory_pressure(flight_memory_string_t *session_id, exports_flight_memory_memory_operations_memory_pressure_t *ret, exports_flight_memory_memory_operations_memory_error_t *err);
bool exports_flight_memory_memory_operations_validate_allocation_request(flight_memory_string_t *session_id, exports_flight_memory_memory_operations_memory_size_t *size, exports_flight_memory_memory_operations_memory_purpose_t purpose, bool *ret, exports_flight_memory_memory_operations_memory_error_t *err);
bool exports_flight_memory_memory_operations_list_allocations(flight_memory_string_t *session_id, exports_flight_memory_memory_operations_list_memory_allocation_t *ret, exports_flight_memory_memory_operations_memory_error_t *err);

// Exported Functions from `flight:memory/memory-analytics@1.0.0`
bool exports_flight_memory_memory_analytics_calculate_memory_stats(flight_memory_string_t *session_id, exports_flight_memory_memory_analytics_memory_stats_t *ret, exports_flight_memory_memory_analytics_memory_error_t *err);
bool exports_flight_memory_memory_analytics_get_memory_trends(flight_memory_string_t *session_id, uint64_t time_window, exports_flight_memory_memory_analytics_memory_trend_t *ret, exports_flight_memory_memory_analytics_memory_error_t *err);
bool exports_flight_memory_memory_analytics_generate_memory_report(flight_memory_string_t *session_id, flight_memory_string_t *ret, exports_flight_memory_memory_analytics_memory_error_t *err);

// Helper Functions

void exports_flight_memory_memory_types_memory_size_free(exports_flight_memory_memory_types_memory_size_t *ptr);

void exports_flight_memory_memory_types_memory_usage_snapshot_free(exports_flight_memory_memory_types_memory_usage_snapshot_t *ptr);

void exports_flight_memory_memory_types_platform_profile_free(exports_flight_memory_memory_types_platform_profile_t *ptr);

void flight_memory_option_u64_free(flight_memory_option_u64_t *ptr);

void exports_flight_memory_memory_types_memory_allocation_free(exports_flight_memory_memory_types_memory_allocation_t *ptr);

void exports_flight_memory_memory_types_memory_limits_free(exports_flight_memory_memory_types_memory_limits_t *ptr);

void flight_memory_option_string_free(flight_memory_option_string_t *ptr);

void exports_flight_memory_memory_types_memory_error_free(exports_flight_memory_memory_types_memory_error_t *ptr);

void exports_flight_memory_memory_operations_memory_size_free(exports_flight_memory_memory_operations_memory_size_t *ptr);

void exports_flight_memory_memory_operations_memory_allocation_free(exports_flight_memory_memory_operations_memory_allocation_t *ptr);

void exports_flight_memory_memory_operations_memory_usage_snapshot_free(exports_flight_memory_memory_operations_memory_usage_snapshot_t *ptr);

void exports_flight_memory_memory_operations_memory_error_free(exports_flight_memory_memory_operations_memory_error_t *ptr);

void exports_flight_memory_memory_operations_memory_limits_free(exports_flight_memory_memory_operations_memory_limits_t *ptr);

void exports_flight_memory_memory_operations_platform_profile_free(exports_flight_memory_memory_operations_platform_profile_t *ptr);

void exports_flight_memory_memory_operations_result_memory_allocation_memory_error_free(exports_flight_memory_memory_operations_result_memory_allocation_memory_error_t *ptr);

void exports_flight_memory_memory_operations_result_bool_memory_error_free(exports_flight_memory_memory_operations_result_bool_memory_error_t *ptr);

void exports_flight_memory_memory_operations_result_memory_usage_snapshot_memory_error_free(exports_flight_memory_memory_operations_result_memory_usage_snapshot_memory_error_t *ptr);

void exports_flight_memory_memory_operations_result_platform_profile_memory_error_free(exports_flight_memory_memory_operations_result_platform_profile_memory_error_t *ptr);

void exports_flight_memory_memory_operations_result_memory_limits_memory_error_free(exports_flight_memory_memory_operations_result_memory_limits_memory_error_t *ptr);

void exports_flight_memory_memory_operations_result_memory_pressure_memory_error_free(exports_flight_memory_memory_operations_result_memory_pressure_memory_error_t *ptr);

void exports_flight_memory_memory_operations_list_memory_allocation_free(exports_flight_memory_memory_operations_list_memory_allocation_t *ptr);

void exports_flight_memory_memory_operations_result_list_memory_allocation_memory_error_free(exports_flight_memory_memory_operations_result_list_memory_allocation_memory_error_t *ptr);

void exports_flight_memory_memory_analytics_memory_size_free(exports_flight_memory_memory_analytics_memory_size_t *ptr);

void exports_flight_memory_memory_analytics_memory_usage_snapshot_free(exports_flight_memory_memory_analytics_memory_usage_snapshot_t *ptr);

void exports_flight_memory_memory_analytics_memory_error_free(exports_flight_memory_memory_analytics_memory_error_t *ptr);

void exports_flight_memory_memory_analytics_tuple2_memory_purpose_memory_size_free(exports_flight_memory_memory_analytics_tuple2_memory_purpose_memory_size_t *ptr);

void exports_flight_memory_memory_analytics_list_tuple2_memory_purpose_memory_size_free(exports_flight_memory_memory_analytics_list_tuple2_memory_purpose_memory_size_t *ptr);

void exports_flight_memory_memory_analytics_memory_stats_free(exports_flight_memory_memory_analytics_memory_stats_t *ptr);

void exports_flight_memory_memory_analytics_list_memory_usage_snapshot_free(exports_flight_memory_memory_analytics_list_memory_usage_snapshot_t *ptr);

void exports_flight_memory_memory_analytics_option_memory_size_free(exports_flight_memory_memory_analytics_option_memory_size_t *ptr);

void exports_flight_memory_memory_analytics_memory_trend_free(exports_flight_memory_memory_analytics_memory_trend_t *ptr);

void exports_flight_memory_memory_analytics_result_memory_stats_memory_error_free(exports_flight_memory_memory_analytics_result_memory_stats_memory_error_t *ptr);

void exports_flight_memory_memory_analytics_result_memory_trend_memory_error_free(exports_flight_memory_memory_analytics_result_memory_trend_memory_error_t *ptr);

void exports_flight_memory_memory_analytics_result_string_memory_error_free(exports_flight_memory_memory_analytics_result_string_memory_error_t *ptr);

// Transfers ownership of `s` into the string `ret`
void flight_memory_string_set(flight_memory_string_t *ret, const char*s);

// Creates a copy of the input nul-terminate string `s` and
// stores it into the component model string `ret`.
void flight_memory_string_dup(flight_memory_string_t *ret, const char*s);

// Deallocates the string pointed to by `ret`, deallocating
// the memory behind the string.
void flight_memory_string_free(flight_memory_string_t *ret);

#ifdef __cplusplus
}
#endif
#endif
