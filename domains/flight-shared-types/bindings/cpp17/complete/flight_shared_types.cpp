/**
 * Complete C++17 Bindings Implementation for Flight Shared Types
 */

#include "flight_shared_types.hpp"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <random>

namespace flight::shared_types {

// Utility functions
std::string generate_uuid() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static std::uniform_int_distribution<> dis2(8, 11);
    
    std::stringstream ss;
    ss << std::hex;
    for (int i = 0; i < 8; i++) {
        ss << dis(gen);
    }
    ss << "-";
    for (int i = 0; i < 4; i++) {
        ss << dis(gen);
    }
    ss << "-4";
    for (int i = 0; i < 3; i++) {
        ss << dis(gen);
    }
    ss << "-";
    ss << dis2(gen);
    for (int i = 0; i < 3; i++) {
        ss << dis(gen);
    }
    ss << "-";
    for (int i = 0; i < 12; i++) {
        ss << dis(gen);
    }
    return ss.str();
}

uint64_t current_timestamp() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

std::string format_bytes(uint64_t bytes) {
    const uint64_t KB = 1024;
    const uint64_t MB = KB * 1024;
    const uint64_t GB = MB * 1024;
    
    std::stringstream ss;
    if (bytes >= GB) {
        ss << std::fixed << std::setprecision(1) << (double)bytes / GB << "GB";
    } else if (bytes >= MB) {
        ss << std::fixed << std::setprecision(1) << (double)bytes / MB << "MB";
    } else if (bytes >= KB) {
        ss << std::fixed << std::setprecision(1) << (double)bytes / KB << "KB";
    } else {
        ss << bytes << "B";
    }
    return ss.str();
}

// Enum to string conversions
const char* to_string(ErrorSeverity severity) noexcept {
    switch (severity) {
        case ErrorSeverity::Info: return "info";
        case ErrorSeverity::Warning: return "warning";
        case ErrorSeverity::Error: return "error";
        case ErrorSeverity::Critical: return "critical";
        case ErrorSeverity::Fatal: return "fatal";
        default: return "unknown";
    }
}

const char* to_string(ErrorCategory category) noexcept {
    switch (category) {
        case ErrorCategory::Memory: return "memory";
        case ErrorCategory::Platform: return "platform";
        case ErrorCategory::Network: return "network";
        case ErrorCategory::Validation: return "validation";
        case ErrorCategory::Security: return "security";
        case ErrorCategory::Component: return "component";
        case ErrorCategory::ServiceIntegration: return "service-integration";
        case ErrorCategory::FlightSystem: return "flight-system";
        case ErrorCategory::Application: return "application";
        case ErrorCategory::Unknown: return "unknown";
        default: return "unknown";
    }
}

const char* to_string(ComponentState state) noexcept {
    switch (state) {
        case ComponentState::Loaded: return "loaded";
        case ComponentState::Instantiating: return "instantiating";
        case ComponentState::Instantiated: return "instantiated";
        case ComponentState::Running: return "running";
        case ComponentState::Suspended: return "suspended";
        case ComponentState::Terminating: return "terminating";
        case ComponentState::Terminated: return "terminated";
        case ComponentState::Error: return "error";
        default: return "unknown";
    }
}

const char* to_string(MemoryPurpose purpose) noexcept {
    switch (purpose) {
        case MemoryPurpose::VmHeap: return "vm-heap";
        case MemoryPurpose::ComponentStack: return "component-stack";
        case MemoryPurpose::AssetCache: return "asset-cache";
        case MemoryPurpose::JitCodeCache: return "jit-code-cache";
        case MemoryPurpose::SystemReserved: return "system-reserved";
        case MemoryPurpose::WasmLinear: return "wasm-linear";
        case MemoryPurpose::NetworkBuffers: return "network-buffers";
        case MemoryPurpose::Temporary: return "temporary";
        default: return "unknown";
    }
}

// MemorySize implementation
std::string MemorySize::format_bytes(uint64_t bytes) {
    return flight::shared_types::format_bytes(bytes);
}

// MemoryAllocation implementation
std::chrono::seconds MemoryAllocation::duration() const {
    uint64_t end_time = freed_at.value_or(current_timestamp());
    return std::chrono::seconds(end_time - allocated_at);
}

// FlightError implementation
FlightError::FlightError(ErrorSeverity sev, ErrorCategory cat, std::string msg, 
                        std::string source, std::string operation)
    : id(generate_uuid())
    , severity(sev)
    , category(cat)
    , message(std::move(msg))
    , context{std::move(source), std::move(operation), {}, {}, {}, {}, {}}
    , timestamp(current_timestamp())
{}

FlightError FlightError::platform_error(const std::string& message, 
                                       const std::optional<std::string>& details) {
    FlightError error(ErrorSeverity::Error, ErrorCategory::Platform, message, "platform", "platform_operation");
    if (details) {
        error.details = *details;
    }
    return error;
}

FlightError FlightError::memory_error(const std::string& message, 
                                     const std::optional<std::string>& details) {
    FlightError error(ErrorSeverity::Error, ErrorCategory::Memory, message, "memory", "memory_operation");
    if (details) {
        error.details = *details;
    }
    return error;
}

FlightError FlightError::component_error(const std::string& message, 
                                        const std::optional<std::string>& details) {
    FlightError error(ErrorSeverity::Error, ErrorCategory::Component, message, "component", "component_operation");
    if (details) {
        error.details = *details;
    }
    return error;
}

FlightError& FlightError::with_details(const std::string& details) {
    this->details = details;
    return *this;
}

FlightError& FlightError::with_metadata(const std::string& key, const std::string& value) {
    context.metadata.push_back({key, value});
    return *this;
}

std::string FlightError::to_string() const {
    std::stringstream ss;
    ss << "[" << flight::shared_types::to_string(severity) << "/" 
       << flight::shared_types::to_string(category) << "] " << message;
    if (details) {
        ss << ": " << *details;
    }
    return ss.str();
}

// MemoryManager implementation
FlightResult<MemoryAllocation> MemoryManager::create_allocation(
    const std::string& session_id,
    const MemorySize& size,
    MemoryPurpose purpose
) {
    std::string allocation_id = generate_uuid();
    auto allocation = std::make_unique<MemoryAllocation>();
    allocation->id = allocation_id;
    allocation->session_id = session_id;
    allocation->size = size;
    allocation->purpose = purpose;
    allocation->allocated_at = current_timestamp();
    
    MemoryAllocation result = *allocation;
    allocations_[allocation_id] = std::move(allocation);
    
    return result;
}

FlightResult<bool> MemoryManager::free_allocation(const std::string& allocation_id) {
    auto it = allocations_.find(allocation_id);
    if (it == allocations_.end()) {
        return FlightError::memory_error("Allocation not found", allocation_id);
    }
    
    it->second->freed_at = current_timestamp();
    return true;
}

FlightResult<MemoryUsageSnapshot> MemoryManager::get_memory_snapshot(
    const std::string& session_id
) const {
    uint64_t total_allocated = 0;
    uint32_t active_allocations = 0;
    
    for (const auto& [id, allocation] : allocations_) {
        if (allocation->session_id == session_id && allocation->is_active()) {
            total_allocated += allocation->size.bytes;
            active_allocations++;
        }
    }
    
    MemoryUsageSnapshot snapshot;
    snapshot.timestamp = current_timestamp();
    snapshot.session_id = session_id;
    snapshot.platform = "cpp17-platform";
    snapshot.total = MemorySize(1024 * 1024 * 1024); // 1GB default
    snapshot.used = MemorySize(total_allocated);
    snapshot.available = MemorySize(snapshot.total.bytes - total_allocated);
    snapshot.fragmentation_ratio = active_allocations > 100 ? 
        std::min(static_cast<float>(active_allocations) / 1000.0f, 0.3f) : 0.05f;
    
    return snapshot;
}

FlightResult<std::vector<MemoryAllocation>> MemoryManager::list_allocations(
    const std::string& session_id
) const {
    std::vector<MemoryAllocation> result;
    
    for (const auto& [id, allocation] : allocations_) {
        if (allocation->session_id == session_id) {
            result.push_back(*allocation);
        }
    }
    
    return result;
}

// ComponentManager implementation
ComponentManager::ComponentManager() 
    : memory_manager_(std::make_unique<MemoryManager>())
{}

FlightResult<ComponentId> ComponentManager::create_component(
    const std::string& name,
    const std::string& world,
    const std::string& platform,
    const std::optional<std::string>& session_id
) {
    ComponentId component_id = generate_uuid();
    auto component = std::make_unique<ComponentInfo>();
    
    component->id = component_id;
    component->name = name;
    component->version = "1.0.0";
    component->state = ComponentState::Loaded;
    component->world = world;
    component->platform = platform;
    component->session_id = session_id;
    component->created_at = current_timestamp();
    component->last_activity = current_timestamp();
    component->metadata.push_back({"created_by", "cpp17-integration"});
    
    components_[component_id] = std::move(component);
    return component_id;
}

FlightResult<ComponentInfo> ComponentManager::get_component(const ComponentId& id) const {
    auto it = components_.find(id);
    if (it == components_.end()) {
        return FlightError::component_error("Component not found", id);
    }
    return *it->second;
}

FlightResult<bool> ComponentManager::start_component(const ComponentId& id) {
    auto it = components_.find(id);
    if (it == components_.end()) {
        return FlightError::component_error("Component not found", id);
    }
    
    it->second->state = ComponentState::Running;
    it->second->last_activity = current_timestamp();
    return true;
}

FlightResult<bool> ComponentManager::stop_component(const ComponentId& id) {
    auto it = components_.find(id);
    if (it == components_.end()) {
        return FlightError::component_error("Component not found", id);
    }
    
    it->second->state = ComponentState::Suspended;
    it->second->last_activity = current_timestamp();
    return true;
}

FlightResult<bool> ComponentManager::destroy_component(const ComponentId& id) {
    auto it = components_.find(id);
    if (it == components_.end()) {
        return FlightError::component_error("Component not found", id);
    }
    
    it->second->state = ComponentState::Terminated;
    components_.erase(it);
    return true;
}

FlightResult<std::vector<ComponentInfo>> ComponentManager::list_components(
    const std::optional<std::string>& session_id,
    const std::optional<ComponentState>& state_filter
) const {
    std::vector<ComponentInfo> result;
    
    for (const auto& [id, component] : components_) {
        bool include = true;
        
        if (session_id && component->session_id != *session_id) {
            include = false;
        }
        
        if (state_filter && component->state != *state_filter) {
            include = false;
        }
        
        if (include) {
            result.push_back(*component);
        }
    }
    
    return result;
}

// FlightCoreIntegration implementation
FlightCoreIntegration::FlightCoreIntegration() 
    : component_manager_(std::make_unique<ComponentManager>())
{}

FlightResult<bool> FlightCoreIntegration::initialize(const std::string& platform_id) {
    if (platform_id == "dreamcast") {
        platform_info_ = PlatformInfo{
            "dreamcast",
            "Sega Dreamcast",
            "SH-4",
            MemorySize(16 * 1024 * 1024), // 16MB
            {"basic-graphics", "audio"},
            PlatformConstraints{
                MemorySize(16 * 1024 * 1024),
                1, // max_cpu_threads
                32, // max_open_files
                false // network_enabled
            },
            "1.0",
            "Sega",
            {"component-model"}
        };
    } else if (platform_id == "vita") {
        platform_info_ = PlatformInfo{
            "vita",
            "PlayStation Vita",
            "ARM Cortex-A9",
            MemorySize(512 * 1024 * 1024), // 512MB
            {"graphics", "audio", "network"},
            PlatformConstraints{
                MemorySize(512 * 1024 * 1024),
                4, // max_cpu_threads
                1024, // max_open_files
                true // network_enabled
            },
            "3.60",
            "Sony",
            {"component-model", "real-time"}
        };
    } else {
        return FlightError::platform_error("Unknown platform: " + platform_id);
    }
    
    return true;
}

FlightResult<ComponentId> FlightCoreIntegration::create_hal_component(const std::string& platform_id) {
    if (!platform_info_) {
        return FlightError::platform_error("Platform not initialized");
    }
    
    return component_manager_->create_component(
        "hal-component",
        "flight-hal",
        platform_id,
        generate_uuid()
    );
}

FlightResult<MemoryUsageSnapshot> FlightCoreIntegration::create_memory_snapshot(
    const std::string& platform_id,
    uint64_t used_bytes
) {
    if (!platform_info_) {
        return FlightError::platform_error("Platform not initialized");
    }
    
    MemoryUsageSnapshot snapshot;
    snapshot.timestamp = current_timestamp();
    snapshot.session_id = generate_uuid();
    snapshot.platform = platform_id;
    snapshot.total = platform_info_->memory_total;
    snapshot.used = MemorySize(used_bytes);
    snapshot.available = MemorySize(snapshot.total.bytes - used_bytes);
    snapshot.fragmentation_ratio = 0.05f;
    
    return snapshot;
}

FlightResult<PlatformInfo> FlightCoreIntegration::get_platform_info() const {
    if (!platform_info_) {
        return FlightError::platform_error("Platform not initialized");
    }
    return *platform_info_;
}

FlightResult<bool> FlightCoreIntegration::authenticate(const std::string& user_id) {
    auth_context_ = AuthContext{
        true,
        user_id,
        generate_uuid()
    };
    return true;
}

FlightResult<std::vector<uint8_t>> FlightCoreIntegration::export_component() const {
    // Simplified serialization for demonstration
    std::string data = "flight-component-export";
    return std::vector<uint8_t>(data.begin(), data.end());
}

FlightResult<bool> FlightCoreIntegration::import_component(const std::vector<uint8_t>& data) {
    // Simplified deserialization for demonstration
    if (data.empty()) {
        return FlightError::component_error("Empty component data");
    }
    return true;
}

} // namespace flight::shared_types
