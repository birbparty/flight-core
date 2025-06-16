#pragma once
/**
 * Flight-Core Shared Types - C++17 Integration
 * 
 * Modern C++17 bindings for Flight Shared Types with zero-cost abstractions
 * Supports platforms from Dreamcast (16MB) to V6R cloud (2GB+)
 */

#include <cstdint>
#include <optional>
#include <variant>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <memory>
#include <functional>

// Include generated C bindings
#include "../memory-types/flight_memory.h"

namespace flight::shared_types {

// C++17 type aliases for better Flight-Core integration
using Timestamp = std::chrono::system_clock::time_point;
using OptionalString = std::optional<std::string>;
using Metadata = std::unordered_map<std::string, std::string>;

/**
 * Memory Management Types
 * Optimized for Flight-Core memory subsystem integration
 */
namespace memory {

struct MemorySize {
    std::uint64_t bytes;
    std::string human_readable;
    
    // C++17 comparison operators
    bool operator==(const MemorySize& other) const noexcept {
        return bytes == other.bytes;
    }
    
    bool operator!=(const MemorySize& other) const noexcept {
        return !(*this == other);
    }
    
    bool operator<(const MemorySize& other) const noexcept {
        return bytes < other.bytes;
    }
    
    bool operator<=(const MemorySize& other) const noexcept {
        return bytes <= other.bytes;
    }
    
    bool operator>(const MemorySize& other) const noexcept {
        return bytes > other.bytes;
    }
    
    bool operator>=(const MemorySize& other) const noexcept {
        return bytes >= other.bytes;
    }
    
    // Conversion utilities
    static MemorySize from_bytes(std::uint64_t bytes) noexcept;
    static MemorySize from_kb(std::uint64_t kb) noexcept;
    static MemorySize from_mb(std::uint64_t mb) noexcept;
    static MemorySize from_gb(std::uint64_t gb) noexcept;
    
    // Platform-specific memory sizes
    static MemorySize dreamcast_total() noexcept { return from_mb(16); }
    static MemorySize psp_total() noexcept { return from_mb(32); }
    static MemorySize vita_total() noexcept { return from_mb(512); }
    static MemorySize v6r_small_total() noexcept { return from_mb(512); }
    static MemorySize v6r_medium_total() noexcept { return from_gb(1); }
    static MemorySize v6r_large_total() noexcept { return from_gb(2); }
    
    // Human-readable output
    std::string to_string() const { return human_readable; }
};

struct MemoryUsageSnapshot {
    Timestamp timestamp;
    std::string session_id;
    std::string platform;
    MemorySize total;
    MemorySize used;
    MemorySize available;
    float fragmentation_ratio;
    
    // C++17 methods for Flight-Core integration
    [[nodiscard]] double usage_percentage() const noexcept;
    [[nodiscard]] bool is_low_memory() const noexcept;
    [[nodiscard]] bool exceeds_threshold(double threshold) const noexcept;
    [[nodiscard]] bool is_fragmented() const noexcept;
};

enum class PlatformProfile : std::uint8_t {
    Dreamcast,
    Psp,
    Vita,
    V6RSmall,
    V6RMedium,
    V6RLarge,
    Custom
};

enum class MemoryPurpose : std::uint8_t {
    VmHeap,
    ComponentStack,
    AssetCache,
    JitCodeCache,
    SystemReserved,
    WasmLinear,
    NetworkBuffers,
    Temporary
};

enum class MemoryPressure : std::uint8_t {
    Low,
    Medium,
    High,
    Critical
};

struct MemoryAllocation {
    std::string id;
    std::string session_id;
    MemorySize size;
    MemoryPurpose purpose;
    Timestamp allocated_at;
    std::optional<Timestamp> freed_at;
    
    [[nodiscard]] bool is_active() const noexcept;
    [[nodiscard]] std::chrono::duration<double> age() const noexcept;
};

struct MemoryLimits {
    MemorySize heap_max;
    MemorySize stack_max;
    MemorySize cache_max;
    MemorySize soft_limit;
    MemorySize hard_limit;
    
    [[nodiscard]] bool validate_allocation(const MemorySize& size) const noexcept;
};

// Forward declarations for error handling
class MemoryError;

// Result type using std::variant (C++17)
template<typename T>
using MemoryResult = std::variant<T, MemoryError>;

class MemoryError {
public:
    enum class Code : std::uint8_t {
        InsufficientMemory,
        LimitExceeded,
        InvalidSize,
        AllocationFailed,
        AlreadyFreed,
        InvalidAllocation,
        UnsupportedPlatform,
        FragmentationError
    };
    
    MemoryError(Code code, std::string message, OptionalString details = std::nullopt)
        : code_(code), message_(std::move(message)), details_(std::move(details)), 
          timestamp_(std::chrono::system_clock::now()) {}
    
    [[nodiscard]] Code code() const noexcept { return code_; }
    [[nodiscard]] const std::string& message() const noexcept { return message_; }
    [[nodiscard]] const OptionalString& details() const noexcept { return details_; }
    [[nodiscard]] Timestamp timestamp() const noexcept { return timestamp_; }
    
    [[nodiscard]] std::string to_string() const;

private:
    Code code_;
    std::string message_;
    OptionalString details_;
    Timestamp timestamp_;
};

// V6R-specific memory utilities
namespace v6r {
    struct V6RMemoryConfig {
        std::string vm_size;
        std::string session_id;
        std::string user_id;
    };
    
    class V6RMemoryUtils {
    public:
        static MemorySize get_vm_memory_limit(std::string_view vm_size) noexcept;
        static MemoryUsageSnapshot create_snapshot(std::string_view session_id,
                                                 std::string_view platform,
                                                 const MemorySize& used) noexcept;
        static bool validate_config(const V6RMemoryConfig& config) noexcept;
    };
}

// Result type helper functions
template<typename T>
[[nodiscard]] constexpr bool is_ok(const MemoryResult<T>& result) noexcept {
    return std::holds_alternative<T>(result);
}

template<typename T>
[[nodiscard]] constexpr bool is_err(const MemoryResult<T>& result) noexcept {
    return std::holds_alternative<MemoryError>(result);
}

template<typename T>
[[nodiscard]] const T& unwrap(const MemoryResult<T>& result) {
    return std::get<T>(result);
}

template<typename T>
[[nodiscard]] const MemoryError& unwrap_err(const MemoryResult<T>& result) {
    return std::get<MemoryError>(result);
}

// Memory operations interface
class MemoryOperations {
public:
    static MemoryResult<MemoryAllocation> create_allocation(
        std::string_view session_id,
        const MemorySize& size,
        MemoryPurpose purpose) noexcept;
    
    static MemoryResult<bool> free_allocation(std::string_view allocation_id) noexcept;
    
    static MemoryResult<MemoryUsageSnapshot> get_memory_snapshot(
        std::string_view session_id) noexcept;
    
    static MemoryResult<PlatformProfile> get_platform_profile(
        std::string_view platform) noexcept;
    
    static MemoryResult<bool> set_memory_limits(
        std::string_view session_id,
        const MemoryLimits& limits) noexcept;
    
    static MemoryResult<MemoryLimits> get_memory_limits(
        std::string_view session_id) noexcept;
    
    static MemoryResult<MemoryPressure> get_memory_pressure(
        std::string_view session_id) noexcept;
    
    static MemoryResult<bool> validate_allocation_request(
        std::string_view session_id,
        const MemorySize& size,
        MemoryPurpose purpose) noexcept;
    
    static MemoryResult<std::vector<MemoryAllocation>> list_allocations(
        std::string_view session_id) noexcept;
};

} // namespace memory

/**
 * Error Handling Types
 * Exception-free error handling using std::variant
 */
namespace error {

enum class ErrorSeverity : std::uint8_t {
    Info,
    Warning,
    Error,
    Critical,
    Fatal
};

enum class ErrorCategory : std::uint8_t {
    Memory,
    Platform,
    Network,
    Validation,
    Security,
    Component,
    ServiceIntegration,
    FlightSystem,
    Application,
    Unknown
};

struct ErrorContext {
    std::string source;
    std::string operation;
    OptionalString session_id;
    OptionalString user_id;
    OptionalString platform;
    OptionalString service_id;
    Metadata metadata;
};

class FlightError {
public:
    FlightError(std::string id, ErrorSeverity severity, ErrorCategory category,
                std::string message, ErrorContext context,
                OptionalString details = std::nullopt, OptionalString cause = std::nullopt)
        : id_(std::move(id)), severity_(severity), category_(category),
          message_(std::move(message)), details_(std::move(details)),
          context_(std::move(context)), timestamp_(std::chrono::system_clock::now()),
          cause_(std::move(cause)) {}
    
    // Accessors
    [[nodiscard]] const std::string& id() const noexcept { return id_; }
    [[nodiscard]] ErrorSeverity severity() const noexcept { return severity_; }
    [[nodiscard]] ErrorCategory category() const noexcept { return category_; }
    [[nodiscard]] const std::string& message() const noexcept { return message_; }
    [[nodiscard]] const OptionalString& details() const noexcept { return details_; }
    [[nodiscard]] const ErrorContext& context() const noexcept { return context_; }
    [[nodiscard]] Timestamp timestamp() const noexcept { return timestamp_; }
    [[nodiscard]] const OptionalString& cause() const noexcept { return cause_; }
    
    // C++17 utilities
    [[nodiscard]] bool is_recoverable() const noexcept;
    [[nodiscard]] std::string to_string() const;

    // Allow ErrorOperations to access private members
    friend class ErrorOperations;

private:
    std::string id_;
    ErrorSeverity severity_;
    ErrorCategory category_;
    std::string message_;
    OptionalString details_;
    ErrorContext context_;
    Timestamp timestamp_;
    OptionalString cause_;
};

// Result type for Flight-Core integration
template<typename T>
using FlightResult = std::variant<T, FlightError>;

// Helper functions for result handling
template<typename T>
[[nodiscard]] constexpr bool is_ok(const FlightResult<T>& result) noexcept {
    return std::holds_alternative<T>(result);
}

template<typename T>
[[nodiscard]] constexpr bool is_err(const FlightResult<T>& result) noexcept {
    return std::holds_alternative<FlightError>(result);
}

template<typename T>
[[nodiscard]] const T& unwrap(const FlightResult<T>& result) {
    return std::get<T>(result);
}

template<typename T>
[[nodiscard]] const FlightError& unwrap_err(const FlightResult<T>& result) {
    return std::get<FlightError>(result);
}

enum class ServiceErrorCode : std::uint8_t {
    ResourceAllocationFailed,
    SessionLimitExceeded,
    AuthFailed,
    RateLimitExceeded,
    ServiceUnavailable,
    InvalidConfig,
    ResourceLimitExceeded,
    ConnectionFailed,
    RequestTimeout,
    InvalidRequest,
    PermissionDenied,
    ResourceNotFound
};

enum class PlatformErrorCode : std::uint8_t {
    InsufficientPlatformMemory,
    FeatureNotSupported,
    HardwareConstraintViolation,
    PlatformInitFailed,
    ComponentLoadFailed,
    CompatibilityError,
    PlatformResourceExhausted
};

enum class NetworkErrorCode : std::uint8_t {
    ConnectionTimeout,
    ConnectionRefused,
    DnsResolutionFailed,
    TlsError,
    HttpError,
    WebsocketError,
    RequestTimeout,
    NetworkUnreachable,
    ProtocolError,
    BandwidthLimitExceeded
};

struct ValidationErrorDetails {
    std::string field;
    std::string rule;
    std::string expected;
    std::string actual;
    OptionalString context;
};

enum class RecoveryActionType : std::uint8_t {
    Retry,
    ReduceResources,
    UpdateConfig,
    ContactSupport,
    TryAlternative,
    WaitRetry,
    UpgradeResources,
    CheckStatus
};

struct ErrorRecoverySuggestion {
    RecoveryActionType action_type;
    std::string description;
    bool can_automate;
    std::uint32_t priority;
};

// Error operations
class ErrorOperations {
public:
    static FlightError create_error(ErrorSeverity severity, ErrorCategory category,
                                  std::string message, ErrorContext context);
    
    static FlightError create_simple_error(ErrorSeverity severity, ErrorCategory category,
                                         std::string message, std::string source,
                                         std::string operation);
    
    static FlightError create_service_error(ServiceErrorCode code, std::string message,
                                          std::string service_id, OptionalString session_id);
    
    static FlightError create_platform_error(PlatformErrorCode code, std::string message,
                                           std::string platform, std::string operation);
    
    static FlightError create_network_error(NetworkErrorCode code, std::string message,
                                          OptionalString endpoint);
    
    static FlightError create_validation_error(std::string message,
                                             std::vector<ValidationErrorDetails> details);
    
    static FlightError enrich_error(FlightError error, Metadata additional_context);
    
    static FlightError chain_error(FlightError error, FlightError cause);
    
    static bool is_recoverable(const FlightError& error);
    
    static std::vector<ErrorRecoverySuggestion> get_recovery_suggestions(const FlightError& error);
    
    static std::string get_error_summary(const FlightError& error);
};

} // namespace error

/**
 * Platform Detection Types
 * Platform-specific optimizations and capability detection
 */
namespace platform {

enum class PlatformType : std::uint8_t {
    // Flight-Core retro targets
    Dreamcast,
    Psp,
    Vita,
    
    // Flight-Core modern targets
    MacosNative,
    WindowsNative,
    LinuxNative,
    WebBrowser,
    
    // V6R cloud targets
    V6RVmSmall,
    V6RVmMedium,
    V6RVmLarge,
    V6RContainer
};

enum class PlatformCapability : std::uint8_t {
    Minimal,    // Dreamcast-class (16MB)
    Basic,      // PSP-class (32-64MB)
    Standard,   // Vita-class (512MB)
    Enhanced,   // V6R small (512MB-1GB)
    Full,       // V6R medium (1-2GB)
    Unlimited   // V6R large (2GB+)
};

enum class PlatformCategory : std::uint8_t {
    RetroGaming,
    ModernGaming,
    Mobile,
    Desktop,
    Cloud,
    Development,
    Testing
};

struct PlatformMemoryInfo {
    memory::MemorySize total_memory;
    memory::MemorySize available_memory;
    memory::MemorySize system_reserved;
    memory::MemorySize stack_limit;
    memory::MemorySize heap_limit;
    bool has_virtual_memory;
    bool has_mmu;
    bool has_dma;
};

enum class CpuArchitecture : std::uint8_t {
    Sh4,        // SuperH (Dreamcast)
    Mips,       // MIPS (PSP)
    Arm,        // ARM (modern mobile/embedded)
    Arm64,      // ARM64 (modern mobile/cloud)
    X86,        // x86 (legacy desktop)
    X86_64,     // x86-64 (modern desktop/cloud)
    Wasm        // WebAssembly (universal)
};

struct PlatformCpuInfo {
    CpuArchitecture architecture;
    std::uint32_t core_count;
    std::uint32_t clock_speed_mhz;
    bool has_fpu;
    bool has_simd;
};

enum class GraphicsApi : std::uint8_t {
    Software,
    OpenglEs,
    Opengl,
    Vulkan,
    DirectX,
    Metal,
    WebGL,
    Custom
};

struct PlatformGraphicsInfo {
    std::vector<GraphicsApi> apis;
    std::uint32_t max_texture_size;
    memory::MemorySize video_memory;
    bool has_hardware_accel;
    std::uint32_t color_depth;
};

struct PlatformInfo {
    std::string id;
    std::string name;
    PlatformType type;
    PlatformCategory category;
    PlatformCapability capability;
    PlatformMemoryInfo memory;
    PlatformCpuInfo cpu;
    PlatformGraphicsInfo graphics;
    Metadata metadata;
    
    // C++17 convenience methods
    [[nodiscard]] bool is_constrained() const noexcept;
    [[nodiscard]] bool supports_threading() const noexcept;
    [[nodiscard]] bool supports_networking() const noexcept;
    [[nodiscard]] bool is_cloud_platform() const noexcept;
    [[nodiscard]] bool is_retro_platform() const noexcept;
};

// Platform detection utilities
class PlatformDetector {
public:
    static error::FlightResult<PlatformInfo> detect_current_platform() noexcept;
    static PlatformInfo get_dreamcast_info() noexcept;
    static PlatformInfo get_psp_info() noexcept;
    static PlatformInfo get_vita_info() noexcept;
    static PlatformInfo get_v6r_info(std::string_view vm_size) noexcept;
    
    static bool has_feature(const PlatformInfo& platform, std::string_view feature) noexcept;
    static std::vector<PlatformInfo> get_supported_platforms() noexcept;
    static std::vector<PlatformInfo> filter_by_capability(PlatformCapability min_capability) noexcept;
};

} // namespace platform

/**
 * Session Management Types
 * Universal session handling across Flight-Core and V6R
 */
namespace session {

enum class SessionState : std::uint8_t {
    Initializing,
    Active,
    Suspended,
    Terminating,
    Terminated,
    Error
};

enum class SessionType : std::uint8_t {
    Component,
    User,
    Development,
    System,
    Testing,
    Custom
};

enum class SessionHealth : std::uint8_t {
    Healthy,
    Warning,
    Degraded,
    Critical,
    Unknown
};

struct ResourceLimits {
    std::optional<memory::MemorySize> max_memory;
    std::optional<float> max_cpu_percent;
    std::optional<std::uint64_t> max_network_bps;
    std::optional<memory::MemorySize> max_storage;
    std::optional<std::uint32_t> max_connections;
    std::optional<std::uint64_t> timeout_seconds;
    Metadata custom_limits;
};

struct SessionResources {
    memory::MemoryUsageSnapshot memory;
    float cpu_usage;
    std::uint64_t network_usage;
    memory::MemorySize storage_usage;
    std::uint32_t connection_count;
    Metadata custom_metrics;
};

struct SessionInfo {
    std::string id;
    SessionType type;
    SessionState state;
    std::string platform;
    OptionalString user_id;
    OptionalString parent_session_id;
    Timestamp created_at;
    Timestamp last_activity;
    std::optional<Timestamp> expires_at;
    Metadata metadata;
    
    // C++17 utilities
    [[nodiscard]] bool is_active() const noexcept;
    [[nodiscard]] bool is_expired() const noexcept;
    [[nodiscard]] std::chrono::duration<double> age() const noexcept;
    [[nodiscard]] bool is_healthy() const noexcept;
};

// Session operations
class SessionOperations {
public:
    static error::FlightResult<SessionInfo> create_session(
        SessionType type, std::string platform, OptionalString user_id) noexcept;
    
    static error::FlightResult<SessionInfo> get_session(std::string_view session_id) noexcept;
    
    static error::FlightResult<bool> update_session_state(
        std::string_view session_id, SessionState new_state) noexcept;
    
    static error::FlightResult<bool> terminate_session(std::string_view session_id) noexcept;
    
    static error::FlightResult<SessionResources> get_session_resources(
        std::string_view session_id) noexcept;
    
    static error::FlightResult<std::vector<SessionInfo>> list_sessions(
        OptionalString user_id, std::optional<SessionType> type, OptionalString platform) noexcept;
    
    static error::FlightResult<bool> extend_session(
        std::string_view session_id, std::uint64_t additional_seconds) noexcept;
    
    static error::FlightResult<SessionHealth> get_session_health(
        std::string_view session_id) noexcept;
};

} // namespace session

/**
 * Component Model Types
 * Advanced Component Model integration for Flight-Core
 */
namespace component {

using ComponentId = std::string;
using InstanceId = std::string;
using WorldName = std::string;
using InterfaceName = std::string;
using ResourceHandle = std::uint32_t;

enum class ComponentState : std::uint8_t {
    Loaded,
    Instantiating,
    Instantiated,
    Running,
    Suspended,
    Terminating,
    Terminated,
    Error
};

enum class ExecutionPriority : std::uint8_t {
    Low,
    Normal,
    High,
    Critical
};

enum class ExecutionMode : std::uint8_t {
    SingleThreaded,
    MultiThreaded,
    AsyncExecution,
    RealTime
};

struct ComponentInfo {
    ComponentId id;
    std::string name;
    std::string version;
    ComponentState state;
    WorldName world;
    std::string platform;
    OptionalString session_id;
    Timestamp created_at;
    Timestamp last_activity;
    memory::MemoryUsageSnapshot memory_usage;
    Metadata metadata;
    
    // C++17 utilities
    [[nodiscard]] bool is_running() const noexcept;
    [[nodiscard]] bool is_healthy() const noexcept;
    [[nodiscard]] std::chrono::duration<double> uptime() const noexcept;
};

struct ExecutionContext {
    ComponentId component;
    std::uint32_t stack_depth;
    memory::MemorySize available_memory;
    std::uint64_t cpu_time_ms;
    ExecutionPriority priority;
    ExecutionMode execution_mode;
};

// Component operations
class ComponentOperations {
public:
    static error::FlightResult<ComponentInfo> create_component(
        std::string_view name, std::string_view world, std::string_view platform) noexcept;
    
    static error::FlightResult<ComponentInfo> get_component(
        const ComponentId& component_id) noexcept;
    
    static error::FlightResult<bool> update_component_state(
        const ComponentId& component_id, ComponentState new_state) noexcept;
    
    static error::FlightResult<std::vector<ComponentInfo>> list_components(
        OptionalString platform, std::optional<ComponentState> state) noexcept;
    
    static error::FlightResult<ExecutionContext> get_execution_context(
        const ComponentId& component_id) noexcept;
};

} // namespace component

/**
 * Flight-Core Integration Utilities
 * High-level utilities for Flight-Core integration
 */
namespace integration {

class FlightCoreIntegration {
public:
    // Platform-specific initialization
    static error::FlightResult<platform::PlatformInfo> initialize_platform() noexcept;
    
    // Memory subsystem integration
    static error::FlightResult<memory::MemoryUsageSnapshot> get_system_memory() noexcept;
    
    // Component lifecycle integration
    static error::FlightResult<component::ComponentInfo> create_hal_component(
        std::string_view platform_id) noexcept;
    
    static error::FlightResult<component::ComponentInfo> create_runtime_component(
        std::string_view platform_id) noexcept;
    
    // Session management
    static error::FlightResult<session::SessionInfo> create_system_session(
        std::string_view platform_id) noexcept;
    
    // V6R integration utilities
    static error::FlightResult<session::SessionInfo> create_v6r_session(
        std::string_view vm_size, std::string_view user_id) noexcept;
    
    static error::FlightResult<memory::MemoryUsageSnapshot> get_v6r_memory_usage(
        std::string_view session_id) noexcept;
};

// C++17 RAII helpers for resource management
template<typename T>
class ScopedResource {
    T resource_;
    std::function<void(T&)> cleanup_;
    
public:
    ScopedResource(T resource, std::function<void(T&)> cleanup)
        : resource_(std::move(resource)), cleanup_(std::move(cleanup)) {}
    
    ~ScopedResource() { if (cleanup_) cleanup_(resource_); }
    
    // Move-only semantics
    ScopedResource(const ScopedResource&) = delete;
    ScopedResource& operator=(const ScopedResource&) = delete;
    
    ScopedResource(ScopedResource&&) = default;
    ScopedResource& operator=(ScopedResource&&) = default;
    
    [[nodiscard]] const T& get() const noexcept { return resource_; }
    [[nodiscard]] T& get() noexcept { return resource_; }
    [[nodiscard]] T& operator*() noexcept { return resource_; }
    [[nodiscard]] const T& operator*() const noexcept { return resource_; }
    [[nodiscard]] T* operator->() noexcept { return &resource_; }
    [[nodiscard]] const T* operator->() const noexcept { return &resource_; }
};

// Factory function for creating scoped resources
template<typename T>
auto make_scoped_resource(T resource, std::function<void(T&)> cleanup) {
    return ScopedResource<T>(std::move(resource), std::move(cleanup));
}

} // namespace integration

} // namespace flight::shared_types

// C++17 structured binding support for common types
namespace std {
    template<>
    struct tuple_size<flight::shared_types::memory::MemorySize> : integral_constant<size_t, 2> {};
    
    template<>
    struct tuple_element<0, flight::shared_types::memory::MemorySize> {
        using type = uint64_t;
    };
    
    template<>
    struct tuple_element<1, flight::shared_types::memory::MemorySize> {
        using type = string;
    };
}

// Required get functions for structured binding
namespace flight::shared_types::memory {
    template<size_t I>
    auto get(const MemorySize& size) -> decltype(auto) {
        if constexpr (I == 0) {
            return size.bytes;
        } else if constexpr (I == 1) {
            return size.human_readable;
        }
    }
}
