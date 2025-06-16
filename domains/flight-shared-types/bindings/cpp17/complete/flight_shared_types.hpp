/**
 * Complete C++17 Bindings for Flight Shared Types
 * Optimized for Flight-Core integration with modern C++ features
 */

#pragma once

#include <optional>
#include <variant>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <functional>

namespace flight::shared_types {

// Forward declarations
class FlightError;

// Modern C++17 Result type using std::variant
template<typename T>
using FlightResult = std::variant<T, FlightError>;

// Helper functions for FlightResult
template<typename T>
constexpr bool is_ok(const FlightResult<T>& result) {
    return std::holds_alternative<T>(result);
}

template<typename T>
constexpr bool is_err(const FlightResult<T>& result) {
    return std::holds_alternative<FlightError>(result);
}

template<typename T>
const T& unwrap(const FlightResult<T>& result) {
    return std::get<T>(result);
}

template<typename T>
const FlightError& unwrap_err(const FlightResult<T>& result) {
    return std::get<FlightError>(result);
}

// Memory Types
struct MemorySize {
    uint64_t bytes;
    std::string human_readable;
    
    MemorySize(uint64_t b = 0) : bytes(b), human_readable(format_bytes(b)) {}
    
private:
    static std::string format_bytes(uint64_t bytes);
};

struct MemoryUsageSnapshot {
    uint64_t timestamp;
    std::string session_id;
    std::string platform;
    MemorySize total;
    MemorySize used;
    MemorySize available;
    float fragmentation_ratio;
    
    double usage_percentage() const {
        return total.bytes > 0 ? (static_cast<double>(used.bytes) / total.bytes) * 100.0 : 0.0;
    }
};

enum class MemoryPurpose {
    VmHeap,
    ComponentStack,
    AssetCache,
    JitCodeCache,
    SystemReserved,
    WasmLinear,
    NetworkBuffers,
    Temporary
};

struct MemoryAllocation {
    std::string id;
    std::string session_id;
    MemorySize size;
    MemoryPurpose purpose;
    uint64_t allocated_at;
    std::optional<uint64_t> freed_at;
    
    bool is_active() const { return !freed_at.has_value(); }
    std::chrono::seconds duration() const;
};

// Error Types
enum class ErrorSeverity {
    Info,
    Warning,
    Error,
    Critical,
    Fatal
};

enum class ErrorCategory {
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

struct MetadataPair {
    std::string key;
    std::string value;
};

struct ErrorContext {
    std::string source;
    std::string operation;
    std::optional<std::string> session_id;
    std::optional<std::string> user_id;
    std::optional<std::string> platform;
    std::optional<std::string> service_id;
    std::vector<MetadataPair> metadata;
};

class FlightError {
public:
    std::string id;
    ErrorSeverity severity;
    ErrorCategory category;
    std::string message;
    std::optional<std::string> details;
    ErrorContext context;
    uint64_t timestamp;
    std::optional<std::string> cause;
    
    FlightError(ErrorSeverity sev, ErrorCategory cat, std::string msg, 
                std::string source, std::string operation);
    
    // Static factory methods
    static FlightError platform_error(const std::string& message, 
                                     const std::optional<std::string>& details = {});
    static FlightError memory_error(const std::string& message, 
                                   const std::optional<std::string>& details = {});
    static FlightError component_error(const std::string& message, 
                                      const std::optional<std::string>& details = {});
    
    // Builder pattern methods
    FlightError& with_details(const std::string& details);
    FlightError& with_metadata(const std::string& key, const std::string& value);
    
    // Convert to string
    std::string to_string() const;
};

// Platform Types
struct PlatformConstraints {
    MemorySize max_memory;
    uint32_t max_cpu_threads;
    uint32_t max_open_files;
    bool network_enabled;
};

struct PlatformInfo {
    std::string id;
    std::string name;
    std::string architecture;
    MemorySize memory_total;
    std::vector<std::string> capabilities;
    PlatformConstraints constraints;
    std::string version;
    std::string vendor;
    std::vector<std::string> features;
};

// Component Types
using ComponentId = std::string;
using WorldName = std::string;
using InterfaceName = std::string;

enum class ComponentState {
    Loaded,
    Instantiating,
    Instantiated,
    Running,
    Suspended,
    Terminating,
    Terminated,
    Error
};

struct ComponentInfo {
    ComponentId id;
    std::string name;
    std::string version;
    ComponentState state;
    WorldName world;
    std::string platform;
    std::optional<std::string> session_id;
    uint64_t created_at;
    uint64_t last_activity;
    MemoryUsageSnapshot memory_usage;
    std::vector<MetadataPair> metadata;
};

// Authentication Types
struct AuthContext {
    bool authenticated;
    std::optional<std::string> user_id;
    std::optional<std::string> session_id;
};

// Session Types
struct SessionInfo {
    std::string id;
    std::string state;
    uint64_t created_at;
    std::optional<uint64_t> expires_at;
};

// Real-time Types
struct RealtimeEvent {
    std::string event_type;
    std::string data;
    uint64_t timestamp;
};

// Pagination Types
struct ListRequest {
    uint32_t page;
    uint32_t per_page;
    std::vector<std::string> filters;
};

struct ListResponse {
    std::vector<std::string> items;
    uint32_t total_count;
    uint32_t page;
    uint32_t per_page;
    bool has_next;
    bool has_previous;
};

// Memory Manager
class MemoryManager {
private:
    std::unordered_map<std::string, std::unique_ptr<MemoryAllocation>> allocations_;
    std::unordered_map<std::string, MemorySize> limits_;
    
public:
    MemoryManager() = default;
    ~MemoryManager() = default;
    
    // Non-copyable, movable
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;
    MemoryManager(MemoryManager&&) = default;
    MemoryManager& operator=(MemoryManager&&) = default;
    
    FlightResult<MemoryAllocation> create_allocation(
        const std::string& session_id,
        const MemorySize& size,
        MemoryPurpose purpose
    );
    
    FlightResult<bool> free_allocation(const std::string& allocation_id);
    
    FlightResult<MemoryUsageSnapshot> get_memory_snapshot(
        const std::string& session_id
    ) const;
    
    FlightResult<std::vector<MemoryAllocation>> list_allocations(
        const std::string& session_id
    ) const;
};

// Component Manager
class ComponentManager {
private:
    std::unordered_map<ComponentId, std::unique_ptr<ComponentInfo>> components_;
    std::unique_ptr<MemoryManager> memory_manager_;
    
public:
    ComponentManager();
    ~ComponentManager() = default;
    
    // Non-copyable, movable
    ComponentManager(const ComponentManager&) = delete;
    ComponentManager& operator=(const ComponentManager&) = delete;
    ComponentManager(ComponentManager&&) = default;
    ComponentManager& operator=(ComponentManager&&) = default;
    
    FlightResult<ComponentId> create_component(
        const std::string& name,
        const std::string& world,
        const std::string& platform,
        const std::optional<std::string>& session_id = {}
    );
    
    FlightResult<ComponentInfo> get_component(const ComponentId& id) const;
    
    FlightResult<bool> start_component(const ComponentId& id);
    FlightResult<bool> stop_component(const ComponentId& id);
    FlightResult<bool> destroy_component(const ComponentId& id);
    
    FlightResult<std::vector<ComponentInfo>> list_components(
        const std::optional<std::string>& session_id = {},
        const std::optional<ComponentState>& state_filter = {}
    ) const;
};

// Flight-Core Integration Manager
class FlightCoreIntegration {
private:
    std::optional<PlatformInfo> platform_info_;
    std::unique_ptr<ComponentManager> component_manager_;
    std::optional<AuthContext> auth_context_;
    
public:
    FlightCoreIntegration();
    ~FlightCoreIntegration() = default;
    
    // Non-copyable, movable
    FlightCoreIntegration(const FlightCoreIntegration&) = delete;
    FlightCoreIntegration& operator=(const FlightCoreIntegration&) = delete;
    FlightCoreIntegration(FlightCoreIntegration&&) = default;
    FlightCoreIntegration& operator=(FlightCoreIntegration&&) = default;
    
    FlightResult<bool> initialize(const std::string& platform_id);
    
    FlightResult<ComponentId> create_hal_component(const std::string& platform_id);
    
    FlightResult<MemoryUsageSnapshot> create_memory_snapshot(
        const std::string& platform_id,
        uint64_t used_bytes
    );
    
    FlightResult<PlatformInfo> get_platform_info() const;
    
    FlightResult<bool> authenticate(const std::string& user_id);
    
    // Component Model integration
    FlightResult<std::vector<uint8_t>> export_component() const;
    FlightResult<bool> import_component(const std::vector<uint8_t>& data);
};

// Utility functions
std::string generate_uuid();
uint64_t current_timestamp();
std::string format_bytes(uint64_t bytes);

// Utility functions for enum to string conversion
const char* to_string(ErrorSeverity severity) noexcept;
const char* to_string(ErrorCategory category) noexcept;
const char* to_string(ComponentState state) noexcept;
const char* to_string(MemoryPurpose purpose) noexcept;

} // namespace flight::shared_types
